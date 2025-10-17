#include "huff.hpp"
#include "bitio.hpp"
#include "crc32.hpp"   // ✅ include CRC

#include <cstdint>
#include <cstdio>
#include <array>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <cstring>

namespace {

static const uint8_t MAGIC[4] = {'H','U','F','1'};

inline bool read_exact(std::FILE* f, void* dst, size_t n) {
    return std::fread(dst, 1, n, f) == n;
}

inline std::uint64_t read_u64_le(std::FILE* f) {
    std::uint64_t x = 0;
    for (int i = 0; i < 8; ++i) {
        int c = std::fgetc(f);
        if (c == EOF) return 0; // will fail later
        x |= (static_cast<std::uint64_t>(c) << (8 * i));
    }
    return x;
}

inline std::uint32_t read_u32_le(std::FILE* f) {
    std::uint32_t x = 0;
    for (int i = 0; i < 4; ++i) {
        int c = std::fgetc(f);
        if (c == EOF) return 0;
        x |= (static_cast<std::uint32_t>(c) << (8 * i));
    }
    return x;
}

struct DecNode {
    DecNode* child[2] = {nullptr, nullptr};
    int sym = -1;
};

void free_tree(DecNode* n) {
    if (!n) return;
    free_tree(n->child[0]);
    free_tree(n->child[1]);
    delete n;
}

static void build_decode_tree(const std::array<uint8_t,256>& lens, DecNode*& root) {
    struct L { uint8_t len; int sym; };
    std::vector<L> items; items.reserve(256);
    for (int s=0; s<256; ++s) if (lens[s] > 0) items.push_back({lens[s], s});

    std::sort(items.begin(), items.end(), [](const L& a, const L& b){
        if (a.len != b.len) return a.len < b.len;
        return a.sym < b.sym;
    });

    root = new DecNode();
    if (items.empty()) return;

    uint32_t code = 0;
    int prev_len = items.front().len;

    // Insert first code (0 of length prev_len)
    {
        DecNode* cur = root;
        for (int i = prev_len - 1; i >= 0; --i) {
            int bit = (code >> i) & 1;
            if (!cur->child[bit]) cur->child[bit] = new DecNode();
            cur = cur->child[bit];
        }
        cur->sym = items.front().sym;
    }

    for (size_t i = 1; i < items.size(); ++i) {
        const auto& it = items[i];
        // next code value
        code += 1;
        if (it.len > prev_len) {
            code <<= (it.len - prev_len);
            prev_len = it.len;
        }
        DecNode* cur = root;
        for (int b = it.len - 1; b >= 0; --b) {
            int bit = (code >> b) & 1;
            if (!cur->child[bit]) cur->child[bit] = new DecNode();
            cur = cur->child[bit];
        }
        cur->sym = it.sym;
    }
}


} // namespace

int decompress_file(const char* in_path, const char* out_path, int /*verify*/) {
    std::FILE* fi = std::fopen(in_path, "rb");
    if (!fi) return 1;

    // --- Read header ---
    uint8_t magic[4];
    if (!read_exact(fi, magic, 4) || std::memcmp(magic, MAGIC, 4) != 0) {
        std::fclose(fi);
        return 2; // bad magic
    }
    std::uint64_t orig_size = read_u64_le(fi);

    std::array<uint8_t,256> lengths{};
    if (!read_exact(fi, lengths.data(), 256)) {
        std::fclose(fi);
        return 3; // header truncated
    }

    // NEW: pad_bits + CRC32
    int pad_bits = std::fgetc(fi);
    if (pad_bits == EOF || pad_bits < 0 || pad_bits > 7) {
        std::fclose(fi);
        return 9; // bad pad bits
    }
    std::uint32_t crc_expected = read_u32_le(fi);

    // --- Open output ---
    std::FILE* fo = std::fopen(out_path, "wb");
    if (!fo) { std::fclose(fi); return 4; }
    if (orig_size == 0) {
        std::fclose(fo);
        std::fclose(fi);
        return 0;
    }

    // --- Rebuild decode tree ---
    DecNode* root = nullptr;
    build_decode_tree(lengths, root);
    if (!root) { std::fclose(fo); std::fclose(fi); return 5; }

    // --- Decode ---
    BitReader br(fi);
    DecNode* cur = root;
    std::uint64_t written = 0;
    std::uint32_t crc_running = 0xFFFFFFFFu;

    while (written < orig_size) {
        while (cur && cur->sym < 0) {
            int b = br.read_bit();
            if (b < 0) { // unexpected EOF
                free_tree(root);
                std::fclose(fo); std::fclose(fi);
                return 6;
            }
            cur = cur->child[b];
            if (!cur) { // invalid stream
                free_tree(root);
                std::fclose(fo); std::fclose(fi);
                return 7;
            }
        }
        unsigned char outb = static_cast<unsigned char>(cur->sym);
        if (std::fputc(outb, fo) == EOF) {
            free_tree(root);
            std::fclose(fo); std::fclose(fi);
            return 8;
        }
        crc_running = crc32_update(crc_running, &outb, 1);
        written++;
        cur = root;
    }

    crc_running ^= 0xFFFFFFFFu;
    if (crc_running != crc_expected) {
        free_tree(root);
        std::fclose(fo); std::fclose(fi);
        return 10; // CRC mismatch
    }

    free_tree(root);
    std::fclose(fo);
    std::fclose(fi);
    (void)pad_bits; // informational in this format; stopping by orig_size is sufficient
    return 0;
}
