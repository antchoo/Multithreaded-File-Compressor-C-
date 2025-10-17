#pragma once
#include <cstdint>
#include <cstdio>

// Simple bit-level writer/reader using std::FILE*.
// Writes MSB-first within a byte.

struct BitWriter {
    std::FILE* f = nullptr;
    uint8_t buf = 0;
    int bits = 0;
    explicit BitWriter(std::FILE* fp) : f(fp) {}
    void write_bit(int b);
    void write_bits(uint32_t v, int n);
    void flush();
};

struct BitReader {
    std::FILE* f = nullptr;
    uint8_t buf = 0;
    int bits = 0;
    explicit BitReader(std::FILE* fp) : f(fp) {}
    int read_bit();            // 0/1, or -1 on EOF
    uint32_t read_bits(int n); // reads up to n bits; short-reads at EOF
};
