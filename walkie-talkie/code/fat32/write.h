#include "fat32.h"

void write_wav(fat32_fs_t* fs, pi_dirent_t* root, int16_t* buf, char* filename, int sample_rate, int num_bytes, int N);