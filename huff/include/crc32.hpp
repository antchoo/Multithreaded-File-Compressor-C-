#pragma once
#include <cstdint>
#include <cstddef>   

std::uint32_t crc32_update(std::uint32_t crc, const unsigned char* data, std::size_t len);
inline std::uint32_t crc32(const unsigned char* data, std::size_t len) { return crc32_update(0xFFFFFFFFu, data, len) ^ 0xFFFFFFFFu; }
