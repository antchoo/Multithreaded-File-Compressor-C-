// src/compress.cpp
#include "huff.hpp"
#include "bitio.hpp"
#include "crc32.hpp"
#include "threads.hpp"   // Codeword + MemBitWriter + encode_chunks_parallel

#include <cstdint>
#include <cstdio>
#include <array>
#include <vector>
#include <queue>
#include <algorithm>
#include <thread>        // for hardware_concurrency
#include <cstring>       // optional for perror if you add more diagnostics

namespace {

// --- constants ---
static const uint8_t MAGIC[4] = {'H','U','F','1'};

// --- helpers ---
static inline void write_u64_le(std::FILE* f, std::uint64_t x) {
    for (int i = 0; i < 8; ++i) { std::fputc(int(x & 0xFF), f); x >>= 8; }
}
static inline void write_u32_le(std::FILE* f, std::uint32_t x) {
    for (int i = 0; i < 4; ++i) { std::fputc(int(x & 0xFF), f); x >>= 8; }
}

// --- Huffman node ---
struct Node {
    std::uint64_t freq;
    int sym; // 0..255 leaf, -1 internal
    Node* left;
    Node* right;
    Node(std::uint64_t f,int s,Node*L=nullptr,Node*R=nullptr):freq(f),sym(s),left(L),right(R){}
};

struct Cmp {
    bool operator()(const Node* a, const Node* b) const {
        if (a->freq != b->freq) return a->freq > b->freq; // min-heap
        int as = (a->sym >= 0 ? a->sym : 256);
        int bs = (b->sym >= 0 ? b->sym : 256);
        return as > bs;
    }
};

static void free_tree(Node* n) {
    if (!n) return;
    free_tree(n->left);
    free_tree(n->right);
    delete n;
}

// DFS to compute code lengths
static void gather_lengths(Node* n, int depth, std::array<uint8_t,256>& lens) {
    if (!n) return;
    if (n->sym >= 0) {
        // Single-symbol file => assign length 1
        lens[size_t(n->sym)] = uint8_t(depth == 0 ? 1 : depth);
        return;
    }
    gather_lengths(n->left,  depth+1, lens);
    gather_lengths(n->right, depth+1, lens);
}

// ---- canonical codes from lengths (matches decoder) ----
struct Code { uint32_t code=0; uint8_t len=0; };

static std::array<Code,256> build_canonical(const std::array<uint8_t,256>& lens) {
    struct L { uint8_t len; int sym; };
    std::vector<L> items; items.reserve(256);
    for (int s=0; s<256; ++s) if (lens[s] > 0) items.push_back({lens[s], s});

    // Sort by length, then by symbol
    std::sort(items.begin(), items.end(), [](const L& a, const L& b){
        if (a.len != b.len) return a.len < b.len;
        return a.sym < b.sym;
    });

    std::array<Code,256> out{}; // zero-init
    if (items.empty()) return out;

    uint32_t code = 0;
    int prev_len = items.front().len;

    // First symbol gets code 0 of its length
    out[items.front().sym] = {code, (uint8_t)prev_len};

    // Progress for the rest: increment, then shift if length increased
    for (size_t i = 1; i < items.size(); ++i) {
        const auto& it = items[i];
        code += 1;
        if (it.len > prev_len) {
            code <<= (it.len - prev_len);
            prev_len = it.len;
        }
        out[it.sym] = {code, it.len};
    }
    return out;
}

} // namespace

int compress_file(const char* in_path, const char* out_path, int /*level*/) {
    // --- open input ---
    std::FILE* fi = std::fopen(in_path, "rb");
    if (!fi) {
        // std::perror("compress fopen input");
        return 1;
    }

    // --- read whole input (simple baseline; later you can stream) ---
    std::vector<uint8_t> data;
    std::array<std::uint64_t,256> freq{}; freq.fill(0);

    uint8_t buf[1<<14];
    size_t n;
    while ((n = std::fread(buf, 1, sizeof buf, fi)) > 0) {
        data.insert(data.end(), buf, buf + n);
        for (size_t i=0;i<n;++i) freq[buf[i]]++;
    }
    std::fclose(fi);

    std::uint64_t orig_size = (std::uint64_t)data.size();

    // --- open output ---
    std::FILE* fo = std::fopen(out_path, "wb");
    if (!fo) {
        // std::perror("compress fopen output");
        return 2;
    }

    // --- write magic + size ---
    std::fwrite(MAGIC, 1, 4, fo);
    write_u64_le(fo, orig_size);

    // --- empty file: write zero lengths + pad_bits=0 + crc=0 and return ---
    if (orig_size == 0) {
        std::array<uint8_t,256> zero{}; zero.fill(0);
        std::fwrite(zero.data(), 1, 256, fo);
        std::fputc(0, fo);               // pad_bits
        write_u32_le(fo, 0x00000000u);   // CRC32 of empty by our wrapper
        std::fclose(fo);
        return 0;
    }

    // --- build tree ---
    std::priority_queue<Node*, std::vector<Node*>, Cmp> pq;
    for (int s=0;s<256;++s) if (freq[s] > 0) pq.push(new Node(freq[s], s));
    if (pq.size() == 1) { // single symbol → add dummy parent
        Node* a = pq.top(); pq.pop();
        pq.push(new Node(a->freq, -1, a, nullptr));
    }
    while (pq.size() > 1) {
        Node* a = pq.top(); pq.pop();
        Node* b = pq.top(); pq.pop();
        pq.push(new Node(a->freq + b->freq, -1, a, b));
    }
    Node* root = pq.top();

    // --- lengths + canonical codes ---
    std::array<uint8_t,256> lengths{}; lengths.fill(0);
    gather_lengths(root, 0, lengths);
    auto codes = build_canonical(lengths);

    // --- write lengths[256] ---
    std::fwrite(lengths.data(), 1, 256, fo);

    // --- compute header tail: total_bits, pad_bits, CRC32(original) ---
    std::uint64_t total_bits = 0;
    for (uint8_t b : data) total_bits += lengths[b];
    uint8_t pad_bits = uint8_t((8 - (total_bits % 8)) % 8);

    std::uint32_t crc = crc32(reinterpret_cast<const unsigned char*>(data.data()), data.size());

    // --- write pad_bits + crc32 ---
    std::fputc(pad_bits, fo);
    write_u32_le(fo, crc);

    // --- parallel payload encode into chunks, then stitch into final BitWriter ---
    // Build Codeword table for threads API
    std::array<Codeword,256> table{};
    for (int s = 0; s < 256; ++s) {
        table[s].code = codes[s].code;
        table[s].len  = codes[s].len;
    }

    const std::size_t chunk_size = (std::size_t)1 << 20; // 1 MiB chunks
    int threads = (int)std::max(1u, std::thread::hardware_concurrency());
    // For demo determinism you can force: int threads = 4;

    std::vector<MemBitWriter> chunks;
    encode_chunks_parallel(data, table, chunk_size, threads, chunks);

    BitWriter bw(fo);
    for (const auto& mbw : chunks) {
        mbw.replay_into(bw);   // writes only valid bits of each buffer (no per-chunk padding)
    }
    bw.flush();

    std::fclose(fo);
    free_tree(root);
    return 0;
}
