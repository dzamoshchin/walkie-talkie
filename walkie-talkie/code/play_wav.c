#include "rpi.h"
#include "pwm.h"
#include "fat32.h"
#include "WAV.h"
#include "audio.h"

#define SAMPLE_RATE 44100

void notmain() {
    kmalloc_init();
    pi_sd_init();

    fat32_fs_t fs;
    pi_dirent_t root;
    config_fs(&fs, &root);

    play_wav(&fs, &root, "RICK.WAV", 44100);
}