#include "rpi.h"
#include "pwm.h"
#include "fat32.h"
#include "WAV.h"

struct wav_format {
   uint32_t description; // should be 'RIFF'
   uint32_t filesize; // little endian
   uint32_t wav_header; // should be 'WAVE'
   uint32_t wav_format; // should be 'fmt '
   uint32_t type_format_size; // should be 16, as in the above four fields are 16 bytes 
   uint16_t wav_type; // 0x01 is PCM
   uint16_t num_channels; // 0x01 is mono, 0x02 is stereo, etc.
   uint32_t sample_rate; // e.g., 44100, little-endian
   uint32_t rate_plus; // sample rate * bits per sample * channels / 8
   uint16_t bits_per_sample; // e.g., 16
   uint32_t data_mark; // should be 'data'
   uint32_t data_size; // little endian
   uint8_t *data; // may be 16-bit
};


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

void notmain ()
{
    kmalloc_init();
    pi_sd_init();

    fat32_fs_t fs;
    pi_dirent_t root;
    config_fs(&fs, &root);

    pwm_init();
    audio_init(SAMPLE_RATE);

    pi_file_t *file = fat32_read(&fs, &root, "HELLO.WAV");

    int32_t *data = (int32_t*)file->data;

    for (int i = 0; i < file->n_data; i++) {
        printk("%x\n", data[i]);
    }

    // file->data = file->data + sizeof(wav_header_t);
    
    // printk("starting play\n");
    // while (1) {
    //     for (unsigned int sample = 0; sample < file->n_data; sample++) {
    //         unsigned status = pwm_get_status();
    //         while (status & PWM_FULL1) {
    //             status = pwm_get_status();
    //         }
    //         uint8_t pcm = file->data[sample];
    //         // mono
    //         pwm_write( pcm ); // output to channel 0
    //         pwm_write( pcm ); // output to channel 1
    //     }
    // }
    // printk("done playing\n");
    // delay_ms(1000);
}
