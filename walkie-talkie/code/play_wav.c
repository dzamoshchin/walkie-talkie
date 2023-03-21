#include "rpi.h"
#include "pwm.h"

// #include "samples/sin8.h"
// #include "samples/sound2.h"
#include "samples/sound3.h"

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


void notmain ()
{
    pwm_init();
    audio_init(sample_freq);

    while (1) {
        printk("starting play\n");
        if (bits_per_sample == 8) {
            while (1) {
                for (unsigned int sample = 0; sample < sizeof(wav_data); sample++) {
                    unsigned status = pwm_get_status();
                    while (status & PWM_FULL1) {
                        status = pwm_get_status();
                    }
                    uint8_t pcm = wav_data[sample];
                    // mono
                    pwm_write( pcm ); // output to channel 0
                    pwm_write( pcm ); // output to channel 1
                }
                if (!repeat) break;
            }
        } else {
            printk("16 bit not implemented\n");
        }
        printk("done playing\n");
        delay_ms(1000);
    }
}
