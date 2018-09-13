#pragma once

#include <cstdint>
#include <ostream>

// This structure appears at the start of each chainsawed shard.  It contains
// enough information, when combined with all the shards, to reconstitute the
// original file.
struct shard_hdr_t final {

  // A magic number by which a shard may be distinguished from any other sort
  // of file.  A file which doesn't start with this number isn't a shard.
  uint32_t magic;

  // An "x of y" designation for this shard, such as "1 of 3".
  uint16_t shard_idx, shard_count;

  // The size, in bytes, of the file that was chainsawed to form this shard.
  uint64_t original_size;

  // The CRC of the file that was chainsawed to form this shard.
  uint32_t original_crc;

  // This size of this shard, in bytes, including the header.  A file whose
  // size doesn't match this value isn't a shard.
  uint64_t shard_size;

  // The CRC of the contents of this shard, not including the header.
  uint32_t shard_crc;

  // The name of the file that was chainsawed to form this shard.  This will
  // be null-terminated and padded with nulls.
  char original_name[256];

  // The expected value for magic.
  static const uint32_t expected_magic;

};  // shard_hdr_t

std::ostream &operator<<(std::ostream &strm, const shard_hdr_t &that);