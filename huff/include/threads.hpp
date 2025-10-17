#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include <cstddef>

struct Codeword {
    std::uint32_t code = 0;
    std::uint8_t  len  = 0;
};

struct MemBitWriter {
    std::vector<std::uint8_t> bytes;
    std::uint8_t buf = 0;
    int bits = 0;                 // bits currently in buf (0..7)
    int last_valid_bits = 0;      // valid bits in the final stored byte (0 if none, 8 if full)

    void write_bit(int b) {
        buf = (std::uint8_t)((buf << 1) | (b & 1));
        bits++;
        if (bits == 8) {
            bytes.push_back(buf);
            buf = 0; bits = 0;
        }
    }
    void write_bits(std::uint32_t v, int n) {
        for (int i = n - 1; i >= 0; --i) write_bit((v >> i) & 1);
    }
    void write_code(const Codeword& cw) {
        if (cw.len) write_bits(cw.code, cw.len);
    }
    void flush() {
        if (bits > 0) {
            // push partial byte left-justified and remember how many bits are valid
            std::uint8_t out = (std::uint8_t)(buf << (8 - bits));
            bytes.push_back(out);
            last_valid_bits = bits;
            buf = 0; bits = 0;
        } else if (!bytes.empty()) {
            last_valid_bits = 8; // last byte is full
        } else {
            last_valid_bits = 0; // no bytes written
        }
    }

    // Replay into a sink that has write_bit(int)
    template <class BitSink>
    void replay_into(BitSink& sink) const {
        if (bytes.empty()) return;
        const std::size_t N = bytes.size();
        for (std::size_t i = 0; i < N; ++i) {
            std::uint8_t b = bytes[i];
            int emit = (i + 1 == N ? (last_valid_bits ? last_valid_bits : 8) : 8);
            // bytes are left-justified: emit the MSB-most 'emit' bits
            for (int k = 7; k >= 8 - emit; --k) sink.write_bit((b >> k) & 1);
        }
    }
};

void encode_chunks_parallel(const std::vector<std::uint8_t>& data,
                            const std::array<Codeword,256>& table,
                            std::size_t chunk_size,
                            int threads,
                            std::vector<MemBitWriter>& out);
