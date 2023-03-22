#include "rpi.h"
#include "pwm.h"
#include "i2s.h"
#include "nrf-test.h"

// example possible wrapper to send a 32-bit value.
static void net_put32(nrf_t *nic, uint32_t txaddr, uint32_t x) {
    int ret = nrf_send_noack(nic, txaddr, &x, 4);
}

void notmain () {

    i2s_init_at_rate(8);
    i2s_enable_rx();

    const int button = 27;
    gpio_set_input(button);

    nrf_t *s = server_mk_noack(server_addr, 4);
    nrf_t *c = client_mk_noack(client_addr, 4);

    int repeat = 0;
    while (1) {
        if(gpio_read(button)) {
            net_put32(c, s->rxaddr, 1);
            
            while(1) {
                int32_t sample = i2s_read_sample();
                net_put32(c, server_addr, sample);
                nrf_output("server: sent %x\n", sample);
                
                if (!gpio_read(button))
                    break;
            }
            net_put32(c, s->rxaddr, 0xfff);
        }
    }

}
