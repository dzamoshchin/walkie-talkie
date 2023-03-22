/*
 * record i2s samples to a buffer then print them out
 * so you can generate a wav file
 * usage: make 2>&1 | grep DUMP | tr -d DUMP > ../py/dump.txt
 */
#include "rpi.h"
#include "i2s.h"
#include "pwm.h"

#define SECS 5
#define SAMPLE_RATE 8000
#define N (SAMPLE_RATE * SECS)

void notmain(void) {

    pwm_init();
    audio_init(SAMPLE_RATE);

    int16_t *buf = (int16_t *)kmalloc(N * sizeof(int16_t));

    i2s_init(16, SAMPLE_RATE);
    i2s_enable_rx();

    for (int i = 0; i < N; i++) {
        buf[i] = i2s_read_sample();
    }

    for (int i = 0; i < N; i++) {
        buf[i] = buf[i] + 0x8000;
    }

    printk("finished recording.\n");

    int repeat = 0;
    for (unsigned int sample = 0; sample < N; sample++) {
        unsigned status = pwm_get_status();
        while (status & PWM_FULL1) {
            status = pwm_get_status();
        }

        uint8_t pcm = buf[sample] >> 8;
        // mono
        pwm_write( pcm ); // output to channel 0
        pwm_write( pcm ); // output to channel 1
    }

    output("done!\n");
}
