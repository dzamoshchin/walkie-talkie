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
#define SAMPLE_RATE 4000
#define BIT_RATE 16
#define N (SAMPLE_RATE * SECS)

void notmain(void) {

    int16_t *buf = (int16_t *)kmalloc(N * sizeof(int16_t));

    i2s_init(BIT_RATE, SAMPLE_RATE);
    i2s_enable_rx();


    printk("started recording.\n");
    for (int i = 0; i < N; i++) {
        buf[i] = i2s_read_sample();
    }
    printk("finished recording.\n");

    for (int i = 0; i < N; i++) {
        buf[i] = buf[i] + 0x8000;
    }

    nrf_t *s = server_mk_noack(server_addr, 4);
    nrf_t *c = client_mk_noack(client_addr, 4);

    int repeat = 0;
    net_put32(c, s->rxaddr, 1);
    for (unsigned int sample = 0; sample < N; sample++) {
        uint8_t pcm = buf[sample] >> 8;
        net_put32(c, server_addr, pcm);
    }
    net_put32(c, s->rxaddr, 0xFFF);

    output("done!\n");
}
