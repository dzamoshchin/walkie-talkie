#include "nrf-test.h"
#include "pwm-audio.h"


#define SECS 5
#define SAMPLE_RATE 44100
#define N (SAMPLE_RATE * SECS)

// useful to mess around with these. 
enum { ntrial = 1000, timeout_usec = 1000 };

static int net_get32(nrf_t *nic, uint32_t *out) {
    int ret = nrf_read_exact_timeout(nic, out, 4, timeout_usec);
    if (ret != 4) return 0;
    return 1;
}

// for simplicity the code is hardwired for 4 byte packets (the size
// of the data we send/recv).
static void read_audio(nrf_t *s, nrf_t *client) {
    unsigned client_addr = client->rxaddr;
    unsigned server_addr = s->rxaddr;
    unsigned ntimeout = 0, npackets = 0;

//    uint32_t sync = 0;

//    nrf_output("waiting for data sync bit...");
//    while (sync == 0) {
//        net_get32(client, &sync);
//    }
//
//    nrf_output("Received sync bit! Starting data stream...");

    for(unsigned i = 0; i < N*2; i++) {

        // receive on client
        uint32_t x = 0;
        if(net_get32(s, &x)) {
            // we aren't doing acks, so can easily lose packets.  [i.e.,
            // it's not actually an error in the code.]

            printk("DUMP%x\n", x);
            i2s_write_sample((unsigned) x);

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

    // audio_init();

    i2s_init();
    i2s_enable_tx();

    // nrf-test.h
    nrf_t *s = server_mk_noack(server_addr, nbytes);
    nrf_t *c = client_mk_noack(client_addr, nbytes);

    // do the test
    read_audio(s, c);

}
