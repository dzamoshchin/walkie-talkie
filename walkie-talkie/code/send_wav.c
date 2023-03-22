#include "rpi.h"
#include "pwm.h"
#include "nrf-test.h"

// #include "samples/sin8.h"
// #include "samples/sound2.h"
#include "samples/sound3.h"

// example possible wrapper to send a 32-bit value.
static void net_put32(nrf_t *nic, uint32_t txaddr, uint32_t x) {
    int ret = nrf_send_noack(nic, txaddr, &x, 4);
}

void notmain ()
{
    pwm_init();
    audio_init(sample_freq);
    nrf_t *s = server_mk_noack(server_addr, 4);
    nrf_t *c = client_mk_noack(client_addr, 4);

    int repeat = 0;
    int val = 1;
    net_put32(c, s->rxaddr, val);
    while (1) {
        printk("starting sending...\n");
        for (unsigned int sample = 0; sample < sizeof(wav_data) / 2; sample++) {
            net_put32(c, s->rxaddr, wav_data[sample]);
        }
        if (!repeat) break;
    }
    val = 0xfff;
    net_put32(c, s->rxaddr, val);
}
