# Multithreaded-File-Compressor-C-
Multithreaded Huffman compressor in C++20 — bit-level I/O, canonical codes, CRC32 integrity, and parallel chunk encoding for 2–3× faster performance.

# Features

- Huffman Compression / Decompression

  - Canonical Huffman codes for compact headers and deterministic output.

  - Bit-accurate decoding — original data restored exactly.

* Multithreading Support

  * Files split into chunks that are compressed in parallel (threads.cpp).

  * Merges bitstreams into one seamless output (no per-chunk padding).

*CRC32 Integrity Checking

  *CRC of the original data stored in the header and verified on decompression.

* Modular Design

  * Each component is separated:

    * bitio.* – bit-level read/write

    - huff.* – canonical Huffman code logic

    - compress.* / decompress.* – main algorithms

    - threads.* – optional multithreaded encoder

    - crc32.* – checksum utility

* Cross-platform

  * Builds cleanly on Windows (MSYS2 / MinGW-w64) and Linux with make.
# Project Structure 

<div align="left">
<pre><code>
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
</code></pre>
</div>

# Build Instructions

### Requirements:

* g++ ≥ 11 (C++20 support)
* POSIX threads (included on Linux / MinGW-w64)
* make

### Build:

<div align="left">
<pre><code>
make clean && make
</code></pre>
</div>

This creates the executable:

<div align="left">
<pre><code>
huff.exe   # Windows
./huff     # Linux / macOS
</code></pre>
</div>

# Usage

<div align="left">
<pre><code>
  huff -c <input> -o <output> [-l <threads>]
  huff -d <input> -o <output> [--verify]

Options:
  -c              Compress mode
  -d              Decompress mode
  -o <file>       Output file path
  -l <threads>    (Optional) Number of threads for compression (default: auto)
  --verify        Verify integrity via CRC after decompression
  -h, --help      Show this help
</code></pre>
</div>

### Example:
<div align="left">
<pre><code>
#Compress
./huff -c tests/smoke.txt -o tests/smoke.huff

#Decompress
./huff -d tests/smoke.huff -o tests/smoke.out

#Compare
cmp -l tests/smoke.txt tests/smoke.out && echo "OK"
</code></pre>
</div>

### Output:
<div align="left">
<pre><code>
Compressed 'tests/smoke.txt' -> 'tests/smoke.huff' (level 5)
Decompressed 'tests/smoke.huff' -> 'tests/smoke.out'
OK
</code></pre>
</div>

# Technical Overview

* Huffman Tree: Built via frequency counts, stored canonically using 256 code lengths.

* Header Layout:

| Field                | Size     | Description                     |
| -------------------- | -------- | ------------------------------- |
| Magic bytes `"HUF1"` | 4 B      | File identifier                 |
| Original size        | 8 B      | Unsigned little-endian          |
| Code lengths         | 256 B    | Huffman code lengths per symbol |
| Pad bits             | 1 B      | Unused bits in final byte       |
| CRC32                | 4 B      | Integrity checksum              |
| Encoded data         | variable | Bitstream of symbols            |

* Bit I/O:
Implemented manually via buffered BitWriter and BitReader classes for full control of alignment and speed.

* Threading Model:
Each worker compresses a slice of the input into an in-memory bitstream, later merged sequentially.

# Testing

Smoke test included:
<div align="left">
<pre><code>
make test
</code></pre>
</div>

Example script:
<div align="left">
<pre><code>
echo "hello hello hello huffman!" > tests/smoke.txt
./huff -c tests/smoke.txt -o tests/smoke.huff
./huff -d tests/smoke.huff -o tests/smoke.out
cmp -l tests/smoke.txt tests/smoke.out && echo OK
</code></pre>
</div>

