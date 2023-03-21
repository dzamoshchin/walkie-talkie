#include <stdint.h>
#include "nrf/nrf-test.h"
#include "rpi.h"
#include "pwm/pwm.h"


#define SECS 5
#define SAMPLE_RATE 44100
#define N (SAMPLE_RATE * SECS)
#define DELAY 2
#define CLOCK_DIVISOR 5
#define CYCLES 1024
#define RIGHTSHIFT (1024/CYCLES - 1)

// useful to mess around with these. 
enum { ntrial = 1000, timeout_usec = 1000 };

static int net_get32(nrf_t *nic, uint32_t *out) {
    int ret = nrf_read_exact_timeout(nic, out, 4, timeout_usec);
    if (ret != 4) return 0;
    return 1;
}

int16_t swap_int16( int16_t val )
{
    return (val << 8) | ((val >> 8) & 0xFF);
}

uint16_t swap_uint16( uint16_t val )
{
    return (val << 8) | (val >> 8 );
}

uint32_t abs(int v) {
    int const mask = v >> (sizeof(int) * 8 - 1);
    return (v + mask) ^ mask;
}

// for simplicity the code is hardwired for 4 byte packets (the size
// of the data we send/recv).
static void read_audio(nrf_t *s, nrf_t *client) {
    unsigned client_addr = client->rxaddr;
    unsigned server_addr = s->rxaddr;
    unsigned ntimeout = 0, npackets = 0;

    uint32_t sync = 0;

//    nrf_output("waiting for data sync bit...");
//    while (sync == 0) {
//        net_get32(client, &sync);
//    }
//
//    nrf_output("Received sync bit! Starting data stream...");

    for(unsigned i = 0; i < N*2; i++) {

        // receive on client
        uint32_t x;
        if(net_get32(s, &x)) {
            // we aren't doing acks, so can easily lose packets.  [i.e.,
            // it's not actually an error in the code.]

            printk("DUMP: %u\n", x);
            int status = pwm_get_status();
            if(!(status & PWM_FULL1)) {

//                unsigned wave = swap_int16(x) - INT16_MIN;
                unsigned wave = abs((int)x - 0xFFFF);
                uint8_t pcm = wave >> 8;
                printk("pcm: %x\n", pcm);
                pwm_write(pcm); // channel 0
                pwm_write(pcm); // channel 1
            }

            if(x != i) {
//                nrf_output("lost/dup packet: received %d (expected=%d)\n", x, i);
                client->tot_lost++;
            }
            npackets++;
        } else {
//            nrf_output("receive failed for packet=%d\n", i);
            ntimeout++;
        }
    }
    trace("trial: successfully sent %d no-ack'd pkts, [lost=%d, timeouts=%d]\n",
        npackets, client->tot_lost, ntimeout, ntimeout);
}


void notmain(void) {
    unsigned nbytes = 4;

    // nrf-test.h
    nrf_t *s = server_mk_noack(server_addr, nbytes);
    nrf_t *c = client_mk_noack(client_addr, nbytes);

    audio_init(SAMPLE_RATE);

    // do the test
    read_audio(s, c);

}
