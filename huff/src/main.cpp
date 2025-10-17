// src/main.cpp
#include <iostream>
#include <string>
#include <vector>
#include <cstring>

#include "huff.hpp"  // declares: int compress_file(const char*, const char*, int);
                    //           int decompress_file(const char*, const char*, int);

struct Options {
    enum Mode { None, Compress, Decompress } mode = None;
    std::string in, out;
    int level = 5;     // default compression level (0..9? you decide later)
    int verify = 0;    // 1 to verify after decompress
};

static void print_usage(const char* prog) {
    std::cerr <<
        "Usage:\n"
        "  " << prog << " -c <input> -o <output> [-l <level>]\n"
        "  " << prog << " -d <input> -o <output> [--verify]\n"
        "\n"
        "Options:\n"
        "  -c              Compress mode\n"
        "  -d              Decompress mode\n"
        "  -o <file>       Output file path\n"
        "  -l <level>      Compression level (integer, default 5)\n"
        "  --verify        Verify integrity after decompression\n"
        "  -h, --help      Show this help\n";
}

static bool parse_args(int argc, char** argv, Options& opt) {
    if (argc < 2) return false;

    for (int i = 1; i < argc; ++i) {
        const char* a = argv[i];

        if (!std::strcmp(a, "-h") || !std::strcmp(a, "--help")) {
            return false; // triggers usage
        } else if (!std::strcmp(a, "-c")) {
            if (opt.mode != Options::None) { std::cerr << "Choose only one of -c or -d.\n"; return false; }
            opt.mode = Options::Compress;
            if (i + 1 >= argc) { std::cerr << "-c requires an input file.\n"; return false; }
            opt.in = argv[++i];
        } else if (!std::strcmp(a, "-d")) {
            if (opt.mode != Options::None) { std::cerr << "Choose only one of -c or -d.\n"; return false; }
            opt.mode = Options::Decompress;
            if (i + 1 >= argc) { std::cerr << "-d requires an input file.\n"; return false; }
            opt.in = argv[++i];
        } else if (!std::strcmp(a, "-o")) {
            if (i + 1 >= argc) { std::cerr << "-o requires an output file.\n"; return false; }
            opt.out = argv[++i];
        } else if (!std::strcmp(a, "-l")) {
            if (i + 1 >= argc) { std::cerr << "-l requires a level integer.\n"; return false; }
            try {
                opt.level = std::stoi(argv[++i]);
            } catch (...) {
                std::cerr << "Invalid level for -l.\n"; return false;
            }
        } else if (!std::strcmp(a, "--verify")) {
            opt.verify = 1;
        } else if (a[0] == '-') {
            std::cerr << "Unknown option: " << a << "\n";
            return false;
        } else {
            // Positional fallback: if input not set yet, take it.
            if (opt.in.empty()) opt.in = a;
            else if (opt.out.empty()) opt.out = a;
            else {
                std::cerr << "Unexpected extra argument: " << a << "\n";
                return false;
            }
        }
    }

    if (opt.mode == Options::None) {
        std::cerr << "You must specify -c or -d.\n";
        return false;
    }
    if (opt.in.empty()) {
        std::cerr << "Missing input file (use -c <in> or -d <in>).\n";
        return false;
    }
    if (opt.out.empty()) {
        std::cerr << "Missing output file (-o <out>).\n";
        return false;
    }
    return true;
}

int main(int argc, char** argv) {
    Options opt;
    if (!parse_args(argc, argv, opt)) {
        print_usage(argv[0]);
        return 2; // usage error
    }

    int rc = 1;
    if (opt.mode == Options::Compress) {
        rc = compress_file(opt.in.c_str(), opt.out.c_str(), opt.level);
        if (rc != 0) {
            std::cerr << "Compression failed (code " << rc << ").\n";
            return rc;
        }
        std::cout << "Compressed '" << opt.in << "' -> '" << opt.out
                  << "' (level " << opt.level << ")\n";
    } else {
        rc = decompress_file(opt.in.c_str(), opt.out.c_str(), opt.verify);
        if (rc != 0) {
            std::cerr << "Decompression failed (code " << rc << ").\n";
            return rc;
        }
        std::cout << "Decompressed '" << opt.in << "' -> '" << opt.out
                  << "'" << (opt.verify ? " [verified]" : "") << "\n";
    }
    return 0;
}
