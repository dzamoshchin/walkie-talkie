#include "rpi.h"
#include "pwm.h"
#include "fat32.h"
#include "WAV.h"

#define SAMPLE_RATE 44100

void config_fs(fat32_fs_t *fs, pi_dirent_t *root) {
    printk("Reading the MBR.\n");
    mbr_t *mbr = mbr_read();

    printk("Loading the first partition.\n");
    mbr_partition_ent_t partition;
    memcpy(&partition, mbr->part_tab1, sizeof(mbr_partition_ent_t));
    assert(mbr_part_is_fat32(partition.part_type));

    printk("Loading the FAT.\n");
    *fs = fat32_mk(&partition);

    printk("Loading the root directory.\n");
    *root = fat32_get_root(fs);
}

void play_wav(char* filename) {
    kmalloc_init();
    pi_sd_init();

    fat32_fs_t fs;
    pi_dirent_t root;
    config_fs(&fs, &root);

    pwm_init();
    audio_init(SAMPLE_RATE);

    pi_file_t *file = fat32_read(&fs, &root, filename);

    file->data = file->data + sizeof(wav_header_t);

    int16_t *data = (int16_t*)file->data;

    printk("starting play\n");
    for (unsigned int sample = 0; sample < (file->n_data - sizeof(wav_header_t)) / sizeof(int16_t); sample++) {
        unsigned status = pwm_get_status();
        while (status & PWM_FULL1) {
            status = pwm_get_status();
        }
        unsigned wave = data[sample] + 0x8000;
        uint8_t pcm = wave>>8;
        pwm_write( pcm );
        pwm_write( pcm );
    }
    printk("done playing\n");
}


// void notmain() {
//     play_wav("MAIL.WAV");
// }