#include "bitio.hpp"

void BitWriter::write_bit(int b) {
    buf = static_cast<uint8_t>((buf << 1) | (b & 1));
    bits++;
    if (bits == 8) {
        std::fputc(buf, f);
        bits = 0; buf = 0;
    }
}

void BitWriter::write_bits(uint32_t v, int n) {
    for (int i = n - 1; i >= 0; --i) {
        write_bit((v >> i) & 1);
    }
}

void BitWriter::flush() {
    if (bits > 0) {
        buf <<= (8 - bits);
        std::fputc(buf, f);
        bits = 0; buf = 0;
    }
}

int BitReader::read_bit() {
    if (bits == 0) {
        int c = std::fgetc(f);
        if (c == EOF) return -1;
        buf = static_cast<uint8_t>(c);
        bits = 8;
    }
    int b = (buf >> 7) & 1;
    buf <<= 1;
    bits--;
    return b;
}

uint32_t BitReader::read_bits(int n) {
    uint32_t v = 0;
    for (int i = 0; i < n; ++i) {
        int b = read_bit();
        if (b < 0) break;
        v = (v << 1) | static_cast<uint32_t>(b);
    }
    return v;
}
