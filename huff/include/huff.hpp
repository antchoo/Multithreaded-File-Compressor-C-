#pragma once

int compress_file(const char* in_path, const char* out_path, int level);
int decompress_file(const char* in_path, const char* out_path, int verify);
