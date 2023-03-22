/*
 * record i2s samples to a buffer then print them out
 * so you can generate a wav file
 * usage: make 2>&1 | grep DUMP | tr -d DUMP > ../py/dump.txt
 */
#include "rpi.h"
#include "i2s.h"
#include "pwm.h"
#include "nrf-test.h"

static void net_put32(nrf_t *nic, uint32_t txaddr, uint32_t x) {
    int ret = nrf_send_noack(nic, txaddr, &x, 4);
}


#define SECS 4
#define SAMPLE_RATE 44100
#define N (SAMPLE_RATE * SECS)

void notmain(void) {

    pwm_init();
    audio_init(SAMPLE_RATE);

    int16_t *buf = (int16_t *)kmalloc(N * sizeof(int16_t));

    i2s_init_at_rate(16);
    i2s_enable_rx();

    for (int i = 0; i < N; i++) {
        buf[i] = i2s_read_sample();
    }

    for (int i = 0; i < N; i++) {
        buf[i] = buf[i] + 0x8000;
    }

    printk("finished recording.\n");

    nrf_t *s = server_mk_noack(server_addr, 4);
    nrf_t *c = client_mk_noack(client_addr, 4);

    int repeat = 0;
    net_put32(c, s->rxaddr, 1);
    for (unsigned int sample = 0; sample < N; sample++) {
        // unsigned status = pwm_get_status();
        // while (status & PWM_FULL1) {
        //     status = pwm_get_status();
        // }

        uint8_t pcm = buf[sample] >> 8;
        net_put32(c, server_addr, pcm);
        // // mono
        // pwm_write( pcm ); // output to channel 0
        // pwm_write( pcm ); // output to channel 1
    }
    net_put32(c, s->rxaddr, 0xFFF);

    output("done!\n");
}
