#include "join.h"

#include <cstring>
#include <iomanip>
#include <map>            // std::map
#include <stdexcept>
#include <sstream>
#include <tuple>

#include "crc.h"
#include "file.h"
#include "shard_hdr.h"

// Open a file as a shard and read its header.
static file_t open_shard(const std::string &path, shard_hdr_t &shard_hdr) {
  try {
    file_t in = file_t::open_ro(path);
    uint64_t in_size;
    mode_t mode;
    std::tie(in_size, mode) = in.get_size_and_mode();
    if (in_size < sizeof(shard_hdr_t)) {
      throw std::runtime_error { "The file is too small." };
    }
    in.read_exactly(
        reinterpret_cast<char *>(&shard_hdr), sizeof(shard_hdr_t));
    if (shard_hdr.magic != shard_hdr_t::expected_magic ||
        shard_hdr.shard_size != in_size) {
      throw std::runtime_error { "The file is not a shard." };
    }
    return in;
  } catch (...) {
    std::ostringstream msg;
    msg << "Could not open " << std::quoted(path) << " as a shard.";
    std::throw_with_nested(std::runtime_error { msg.str() });
  }
}

// Join shards into a single file.
int join(const std::vector<std::string> &file_names) {
  // We must have some shards to work with.
  if (file_names.empty()) {
    throw std::runtime_error { "No shards to join." };
  }
  // Open the first shard and confirm we have the right number of file names.
  shard_hdr_t master_shard_hdr;
  file_t in = open_shard(file_names[0], master_shard_hdr);
  if (file_names.size() != master_shard_hdr.shard_count) {
    std::ostringstream msg;
    msg
        << "Got " << file_names.size() << " file name(s) but expected "
        << master_shard_hdr.shard_count << " shard(s).";
    throw std::runtime_error { msg.str() };
  }
  // Build a map from shard idx to file name, ordered by idx.
  std::map<uint16_t, std::string> shard_map;
  shard_map[master_shard_hdr.shard_idx] = file_names[0];
  shard_hdr_t shard_hdr;
  for (size_t i = 1; i < file_names.size(); ++i) {
    // Open the next file.  Make sure it matches the first one.
    in = open_shard(file_names[i], shard_hdr);
    if (shard_hdr.shard_count   != master_shard_hdr.shard_count   ||
        shard_hdr.original_size != master_shard_hdr.original_size ||
        shard_hdr.original_crc  != master_shard_hdr.original_crc  ||
        strcmp(shard_hdr.original_name, master_shard_hdr.original_name) != 0) {
      std::ostringstream msg;
      msg << "Shard " << std::quoted(file_names[i]) << " doesn't match.";
      throw std::runtime_error { msg.str() };
    }
    // Add it to the map, barfing if we find a duplicate shard idx.
    auto pair = shard_map.emplace(shard_hdr.shard_idx, file_names[i]);
    if (!pair.second) {
      std::ostringstream msg;
      msg << "Shard " << std::quoted(file_names[i]) << " is a duplicate.";
      throw std::runtime_error { msg.str() };
    }
  }  // for
  // Create the output file based on the first shard.
  file_t out = file_t::open_rw(master_shard_hdr.original_name);
  // Iterate through the map in order by shard idx, reading in each shard
  // and writing to the output file.
  uint32_t total_crc = 0;
  for (const auto &pair: shard_map) {
    // Append the contents of the shard to the output, computing the CRC
    // values as we go.
    in = open_shard(pair.second, shard_hdr);
    char buffer[0x10000];
    uint32_t shard_crc = 0;
    for (;;) {
      size_t piece_size = in.read_at_most(buffer, sizeof(buffer));
      if (!piece_size) {
        break;
      }
      update_crc(total_crc, buffer, piece_size);
      update_crc(shard_crc, buffer, piece_size);
      out.write_exactly(buffer, piece_size);
    }  // for
    // Verify the CRC we computed for the shard against the one in the shard's
    // header.
    if (shard_hdr.shard_crc != shard_crc) {
      std::ostringstream msg;
      msg << "Shard " << std::quoted(pair.second) << " is damaged.";
      throw std::runtime_error { msg.str() };
    }
  }  // for
  // Verify the size and CRC of the output.
  auto out_size = out.seek(0, SEEK_CUR);
  if (out_size != master_shard_hdr.original_size ||
      total_crc != master_shard_hdr.original_crc) {
    throw std::runtime_error { "Output did not reconstruct correctly." };
  }
  return EXIT_SUCCESS;
}