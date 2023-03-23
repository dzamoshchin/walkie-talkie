#include <stdint.h>
#include "nrf/nrf-test.h"
#include "rpi.h"
#include "pwm/pwm.h"
#include "audio.h"
#include "fat32.h"

#define SECS 5
#define SAMPLE_RATE 8000
#define GAIN 4.0
#define N (SAMPLE_RATE * SECS)

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
    kmalloc_init();
    pi_sd_init();

    fat32_fs_t fs;
    pi_dirent_t root;
    config_fs(&fs, &root);

    unsigned client_addr = client->rxaddr;
    unsigned server_addr = s->rxaddr;
    unsigned ntimeout = 0, npackets = 0;

    uint32_t sync = 0;

    uint8_t mic_data[N];

    while(1) {
        nrf_output("waiting for data sync bit...\n");
        while (sync != 1) {
            net_get32(s, &sync);
        }

        nrf_output("Received sync bit! Starting data stream...\n");
        unsigned i = 0;
        uint32_t x;
        int clock_rate = 19200000 / 2; // 9600000 Hz
        int range = clock_rate / SAMPLE_RATE ;

        while(i < N && ntimeout < (unsigned)N * 1.2) {
            // receive on client
            if(net_get32(s, &x)) {
                // we aren't doing acks, so can easily lose packets.  [i.e.,
                // it's not actually an error in the code.]
                if (x == 0xFFF) break;


                double dsample = ((double) x - 128) * GAIN + 128;
                if (dsample > 255.0) {dsample=255.0;}
                if (dsample < 0.0) {dsample=0.0;}

                mic_data[i] = (uint8_t) (dsample);
    //            printk("%d\n", mic_data[i]);

                if (i % 1000 == 0) printk("Received sample #%d\n", i);
                i++;
            }
            ntimeout++;
        }
        nrf_output("Received sync bit! Ending data stream...\n");

        play_wav(&fs, &root, "MAIL.WAV", 44100);

        audio_init(SAMPLE_RATE);
        for (unsigned j = 0; j < i; j++) {
            unsigned status = pwm_get_status();
            while (status & PWM_FULL1) {
                status = pwm_get_status();
            }
            uint8_t pcm = mic_data[j];
            pwm_write(pcm); // channel 0
            pwm_write(pcm); // channel 1
        }

        trace("trial: successfully sent %d no-ack'd pkts, [lost=%d, timeouts=%d]\n",
            npackets, client->tot_lost, ntimeout, ntimeout);
    }
}


void notmain(void) {
    unsigned nbytes = 4;

    // nrf-test.h
    nrf_t *s = server_mk_noack(server_addr, nbytes);
    nrf_t *c = client_mk_noack(client_addr, nbytes);

    // do the test
    read_audio(s, c);

}
