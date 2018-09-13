#include "shard_hdr.h"

#include <iomanip> // std::quoted

const uint32_t shard_hdr_t::expected_magic = 0xB007C8AD;

std::ostream &operator<<(std::ostream &strm, const shard_hdr_t &that) {
  return strm
      << "{ shard_idx: " << that.shard_idx
      << ", shard_count: " << that.shard_count
      << ", original_size: " << that.original_size
      << ", original_crc: " << that.original_crc
      << ", shard_size: " << that.shard_size
      << ", shard_crc: " << that.shard_crc
      << ", original_name: " << std::quoted(that.original_name)
      << " }";
}
