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

    gpio_set_input(27);

    audio_init(SAMPLE_RATE);

    pi_file_t *file = fat32_read(&fs, &root, "RICK.WAV");

    // skip header
    file->data = file->data + sizeof(wav_header_t);

    int16_t *data = (int16_t*)file->data;

    printk("Press button to play\n");

    while (1) {
        if (gpio_read(27)) {
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
            break;
        }
    }
}