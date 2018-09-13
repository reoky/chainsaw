#include "split.h"

#include <cstring>
#include <stdexcept>
#include <sstream>
#include <tuple>
#include <utility>

#include "crc.h"
#include "file.h"
#include "shard_hdr.h"

// Given a path, a shard index, and a shard count, return a new path that is
// the name of the shard.  For example, for path="foo", idx=1, count=3, return
// "foo@2.3".
static std::string make_shard_name(
    const std::string &path, size_t idx, size_t count) {
  std::ostringstream strm;
  strm << path << '@' << idx << '.' << count;
  return strm.str();
}

int split(const std::string &file_name, uint64_t max_shard_size) {
  // Open the input file for read-only.
  file_t in = file_t::open_ro(file_name);
  // Get the size of the input file in bytes and the mode bits indicating
  // the file's permissions.
  uint64_t in_size;
  mode_t mode;
  std::tie(in_size, mode) = in.get_size_and_mode();
  // Make a pass over the file to compute its CRC, then seek back to the
  // start.
  char buffer[0x10000];
  uint32_t crc = 0;
  for (;;) {
    size_t size = in.read_at_most(buffer, sizeof(buffer));
    if (!size) {
      break;
    }
    update_crc(crc, buffer, size);
  }  // for
  in.seek(0, SEEK_SET);
  // The number of shards we'll make is based on the size of the input and
  // the maximum size of each shard.
  size_t big_shard_count = (in_size + max_shard_size - 1) / max_shard_size;
  if (big_shard_count > 65535) {
    throw std::runtime_error { "Jesus, that's a big file you have there." };
  }
  uint16_t shard_count = static_cast<uint16_t>(big_shard_count);
  // Fill in a shard header with the information shared by all the shards.
  shard_hdr_t shard_hdr;
  memset(&shard_hdr, 0, sizeof(shard_hdr_t));
  shard_hdr.magic = shard_hdr_t::expected_magic;
  shard_hdr.shard_count = shard_count;
  shard_hdr.original_size = in_size;
  shard_hdr.original_crc = crc;
  // Copy the file name into the header.
  const char *name = file_name.c_str();
  const char *slash = strrchr(name, '/');
  if (slash) {
    name = slash + 1;
  }
  if (strlen(name) >= sizeof(shard_hdr.original_name)) {
    throw std::runtime_error { "The file name was too long." };
  }
  strcpy(shard_hdr.original_name, name);
  // Loop, starting at shard 1, until we created all the shards.
  for (uint16_t shard_idx = 1; shard_idx <= shard_count; ++shard_idx) {
    // Open the hard file for read-write, creating the shard if necessary,
    // using the same mode bits as the input file.
    file_t out = file_t::open_rw(
        make_shard_name(file_name, shard_idx, shard_count),
        mode);
    // The size of this shard will be the maximum size of any shard, or
    // the number of bytes left in the input whichever is smaller.  This
    // means each shard but the last one will be of max size, and the last
    // one will just have whatever is left over.
    size_t shard_size = std::min(
        max_shard_size - sizeof(shard_hdr_t), in_size);
    // Fill in the shard-specific information in the header (except for the
    // CRC, which comes later), and write it out as-is.  Writing it now,
    // before we write anything else, means it will appear at the start of
    // the shard file.
    shard_hdr.shard_idx = shard_idx;
    shard_hdr.shard_size = shard_size + sizeof(shard_hdr_t);
    out.write_exactly(
        reinterpret_cast<const char *>(&shard_hdr), sizeof(shard_hdr));
    // Decrement the number of bytes left in the input.
    in_size -= shard_size;
    // Copy the input to the output one buffer at a time.  A buffer is any
    // convenient size, here set to 64K.
    crc = 0;
    while (shard_size) {
      // Read at most a buffer's worth of bytes.
      size_t piece_size = in.read_at_most(
          buffer, std::min(sizeof(buffer), shard_size));
      // Compute the CRC of the shard so far.
      update_crc(crc, buffer, piece_size);
      // Write out exactly the number of bytes we read in.
      out.write_exactly(buffer, piece_size);
      // Decrement the number of bytes left to copy for this shard.
      shard_size -= piece_size;
    }  // while
    // Fill in the shard CRC, rewind to the start of the shard, and write
    // the complete header.
    shard_hdr.shard_crc = crc;
    out.seek(0, SEEK_SET);
    out.write_exactly(
        reinterpret_cast<const char *>(&shard_hdr), sizeof(shard_hdr));
  }  // for
  return EXIT_SUCCESS;
}