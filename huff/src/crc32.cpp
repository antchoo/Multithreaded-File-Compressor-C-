#include "crc32.hpp"
#include <cstddef>  

// Poly 0xEDB88320 (IEEE)
std::uint32_t crc32_update(std::uint32_t crc, const unsigned char* data, std::size_t len) {
    for (std::size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (int k = 0; k < 8; ++k) {
            std::uint32_t mask = -(crc & 1u);
            crc = (crc >> 1) ^ (0xEDB88320u & mask);
        }
    }
    return crc;
}

