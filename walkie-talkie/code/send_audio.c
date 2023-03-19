#include "rpi.h"
#include "i2s/i2s.h"
#include "nrf/nrf-test.h"

#define SECS 5
#define SAMPLE_RATE 44100
#define N (SAMPLE_RATE * SECS)

// useful to mess around with these. 
enum { ntrial = 1000, timeout_usec = 1000, nbytes = 4 };

// example possible wrapper to recv a 32-bit value.
static int net_get32(nrf_t *nic, uint32_t *out) {
    int ret = nrf_read_exact_timeout(nic, out, 4, timeout_usec);
    if(ret != 4) return 0;
    return 1;
}

// example possible wrapper to send a 32-bit value.
static void net_put32(nrf_t *nic, uint32_t txaddr, uint32_t x) {
    int ret = nrf_send_noack(nic, txaddr, &x, 4);
}

void send_audio(nrf_t *s, nrf_t *c) {
    unsigned client_addr = c->rxaddr;
    unsigned server_addr = s->rxaddr;
    unsigned npackets = 0, ntimeout = 0;
    uint32_t exp = 0, got = 0;

    // send sync bit
    // net_put32(c, server_addr, 1);

    const int button = 27;

    gpio_set_input(button);
    while(1) { 
        if(gpio_read(button)) {
            int32_t sample = i2s_read_sample();
            net_put32(c, server_addr, sample);
            nrf_output("server: sent %x\n", sample);
        }
    }
}

void notmain(void) {
    // configure server
    nrf_t *s = server_mk_noack(server_addr, nbytes);
    nrf_t *c = client_mk_noack(client_addr, nbytes);

    //init i2c
    i2s_init();

    i2s_enable_rx();

    // do the test
    send_audio(s, c);
}
