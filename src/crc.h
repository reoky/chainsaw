#pragma once

#include <cstdint> // uint32_t
#include <cstddef> // size_t

void update_crc(uint32_t &crc, const void *buffer, size_t size);