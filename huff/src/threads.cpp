#include "threads.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <algorithm>

namespace {
struct Job {
    std::size_t idx;          // chunk index
    const std::uint8_t* ptr;  // start of chunk
    std::size_t len;          // bytes in chunk
};
}

void encode_chunks_parallel(const std::vector<std::uint8_t>& data,
                            const std::array<Codeword,256>& table,
                            std::size_t chunk_size,
                            int threads,
                            std::vector<MemBitWriter>& out)
{
    if (chunk_size == 0) chunk_size = 1 << 20;
    if (threads <= 0)    threads = 4;

    const std::size_t total = data.size();
    const std::size_t nchunks = (total + chunk_size - 1) / chunk_size;

    out.clear();
    out.resize(nchunks); // preserve order

    // Prepare jobs
    std::queue<Job> q;
    for (std::size_t i = 0; i < nchunks; ++i) {
        std::size_t off = i * chunk_size;
        std::size_t len = std::min(chunk_size, total - off);
        q.push(Job{ i, data.data() + off, len });
    }

    std::mutex mu;
    std::condition_variable cv;
    bool stop = false;

    auto worker = [&]() {
        for (;;) {
            Job job;
            {
                std::unique_lock<std::mutex> lk(mu);
                cv.wait(lk, [&]{ return stop || !q.empty(); });
                if (stop && q.empty()) return;
                job = q.front(); q.pop();
            }
            // Encode this chunk into its own MemBitWriter
            MemBitWriter mbw;
            const std::uint8_t* p = job.ptr;
            for (std::size_t i = 0; i < job.len; ++i) {
                const Codeword& cw = table[p[i]];
                if (cw.len) mbw.write_bits(cw.code, cw.len);
            }
            mbw.flush();
            out[job.idx] = std::move(mbw);
        }
    };

    std::vector<std::thread> pool;
    pool.reserve((std::size_t)threads);
    for (int i = 0; i < threads; ++i) pool.emplace_back(worker);

    // Kick off
    {
        std::lock_guard<std::mutex> lk(mu);
        cv.notify_all();
    }

    // Drain
    {
        std::unique_lock<std::mutex> lk(mu);
        // When queue becomes empty, signal stop
        while (!q.empty()) cv.wait_for(lk, std::chrono::milliseconds(5));
        stop = true;
        cv.notify_all();
    }

    for (auto& t : pool) t.join();
}
