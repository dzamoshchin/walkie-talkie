#include <stdint.h>
#include "nrf/nrf-test.h"
#include "fat32/fat32.h"
#include "rpi.h"
#include "pwm/pwm.h"
#include "i2s/i2s.h"


#define SECS 5
#define SAMPLE_RATE 4000
#define GAIN 4.0
#define N (SAMPLE_RATE * SECS)

// useful to mess around with these. 
enum { ntrial = 1000, timeout_usec = 1000 };

const int txbutton = 27;
const int playbackbutton = 100;


static int net_get32(nrf_t *nic, uint32_t *out) {
    int ret = nrf_read_exact_timeout(nic, out, 4, timeout_usec);
    if (ret != 4) return 0;
    return 1;
}

static void net_put32(nrf_t *nic, uint32_t txaddr, uint32_t x) {
    int ret = nrf_send_noack(nic, txaddr, &x, 4);
}


void config_fs(fat32_fs_t *fs, pi_dirent_t *root) {
    printk("Reading the MBR.\n");
    mbr_t *mbr = mbr_read();

    printk("Loading the first partition.\n");
    mbr_partition_ent_t partition;
    memcpy(&partition, mbr->part_tab1, sizeof(mbr_partition_ent_t));
    assert(mbr_part_is_fat32(partition.part_type));

    printk("Loading the FAT.\n");
    *fs = fat32_mk(&partition);

    printk("Loading the root directory.\n");
    *root = fat32_get_root(fs);
}


void write_wav();

void notmain(void) {
    unsigned nbytes = 4;

    nrf_t *s = server_mk_noack(server_addr, nbytes);
    nrf_t *c = client_mk_noack(client_addr, nbytes);

    i2s_init(16, SAMPLE_RATE);
    audio_init(SAMPLE_RATE);

    gpio_set_input(txbutton);
    gpio_set_input(playbackbutton);

    kmalloc_init();
    pi_sd_init();

    fat32_fs_t fs;
    pi_dirent_t root;
    config_fs(&fs, &root);


    unsigned client_addr = client->rxaddr;
    unsigned server_addr = s->rxaddr;
    uint32_t rxsync = 0;
    while (1) {
        net_get32(s, &rxsync);
        if (rxsync != 1) {
            i2s_enable_rx();
            uint8_t mic_data[N];

            uint32_t x;
            // the other pi wants to send a message over
            while(i < N && ntimeout < (unsigned)N * 1.2) {  //TODO: fix this while loop conditional
                // receive on client
                if(net_get32(s, &x)) {
                    // we aren't doing acks, so can easily lose packets.  [i.e.,
                    // it's not actually an error in the code.]
                    if (x == 0xFFF) break;

                    mic_data[i] = (uint8_t) (x);
//            printk("%d\n", mic_data[i]);

                    if (i % 1000 == 0) printk("Received sample #%d\n", i);
                    i++;
                }
                ntimeout++;
            }
            rxsync = 0;

            // TODO: write buffered audio to local WAV
            i2s_disable_rx();
        }

        if (gpio_read(txbutton)) {
            // we want to TX an audio message to other pi
            i2s_enable_tx();
            net_get32(c, server_addr, 1); // send begin sync
            while(1) {
                int32_t sample = i2s_read_sample();
                double dsample = ((double) sample - 128) * GAIN + 128;
                if (dsample > 255.0) dsample=255.0;
                if (dsample < 0.0) dsample=0.0;

                net_put32(c, server_addr, (int32_t) dsample);
                nrf_output("server: sent %x\n", (int32_t) dsample);

                if (!gpio_read(button))
                    break;
            }
            net_get32(c, server_addr, 0xFFF); // send end sync
            i2s_disable_tx();
        }

        if (gpio_read(playbackbutton)) {
            // we want to play our voicemail
            // index through saved wavs on the pi and play them
        }

    }

}
