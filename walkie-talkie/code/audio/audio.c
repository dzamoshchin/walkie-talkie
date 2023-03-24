#include "rpi.h"
#include "pwm.h"
#include "fat32.h"
#include "wav.h"

#define GAIN 8.0

void play_wav(fat32_fs_t* fs, pi_dirent_t* root, char* filename, int sample_rate) {
    audio_init(sample_rate);

    pi_file_t *file = fat32_read(fs, root, filename);

    // skip header
    file->data = file->data + sizeof(wav_header_t);

    int16_t *data = (int16_t*)file->data;

    for (unsigned int sample = 0; sample < (file->n_data - sizeof(wav_header_t)) / sizeof(int16_t); sample++) {
        unsigned wave = data[sample] + 0x8000;
        uint8_t pcm = wave>>8;
        if (sample_rate == 8000) {
            double dsample = ((double) pcm - 128) * GAIN + 128;
            if (dsample > 255.0) dsample = 255.0;
            if (dsample < 0.0) dsample = 0.0;
            pcm = (uint8_t) dsample;
        }
        unsigned status = pwm_get_status();
        while (status & PWM_FULL1) {
            status = pwm_get_status();
        }
        pwm_write( pcm );
        pwm_write( pcm );
    }
}