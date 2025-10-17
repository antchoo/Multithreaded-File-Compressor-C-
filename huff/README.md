🗜️ Multithreaded File Compressor (C++)

A lightweight command-line tool for compressing and decompressing files using Huffman coding, with optional parallel chunk encoding to leverage multi-core CPUs.
Built from scratch in modern C++20 with POSIX threads, bit-level I/O, and clean modular design.

🚀 Features

Huffman Compression / Decompression

Canonical Huffman codes for compact headers and deterministic output.

Bit-accurate decoding — original data restored exactly.

Multithreading Support

Files split into chunks that are compressed in parallel (threads.cpp).

Merges bitstreams into one seamless output (no per-chunk padding).

CRC32 Integrity Checking

CRC of the original data stored in the header and verified on decompression.

Modular Design

Each component is separated:

bitio.* – bit-level read/write

huff.* – canonical Huffman code logic

compress.* / decompress.* – main algorithms

threads.* – optional multithreaded encoder

crc32.* – checksum utility

Cross-platform

Builds cleanly on Windows (MSYS2 / MinGW-w64) and Linux with make.

🧩 Project Structure
huff/
├── include/
│   ├── bitio.hpp
│   ├── huff.hpp
│   ├── threads.hpp
│   └── crc32.hpp
├── src/
│   ├── bitio.cpp
│   ├── compress.cpp
│   ├── decompress.cpp
│   ├── huff.cpp
│   ├── threads.cpp
│   ├── crc32.cpp
│   └── main.cpp
├── tests/
│   └── smoke.txt
├── Makefile
└── README.md

⚙️ Build Instructions

Requirements:

g++ ≥ 11 (C++20 support)

POSIX threads (included on Linux / MinGW-w64)

make

Build:
make clean && make

This creates the executable:
huff.exe   # Windows
./huff     # Linux / macOS

🧪 Testing

Smoke test included:
make test

Example script:
echo "hello hello hello huffman!" > tests/smoke.txt
./huff -c tests/smoke.txt -o tests/smoke.huff
./huff -d tests/smoke.huff -o tests/smoke.out
cmp -l tests/smoke.txt tests/smoke.out && echo OK

🧑‍💻 Author

Anton Choo (Yuan-Yu)
Computer Science @ Oregon State University
Focus: Systems, AI/ML, and high-performance software engineering