#pragma once

#include <cstdint>
#include <string>

int split(const std::string &file_name, uint64_t max_shard_size);