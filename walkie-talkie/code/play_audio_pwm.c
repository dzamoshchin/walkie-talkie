#include <stdint.h>
#include "nrf/nrf-test.h"
#include "rpi.h"
#include "pwm/pwm.h"
#include "audio.h"
#include "fat32.h"
#include "fat32-helpers.h"
#include "pi-sd.h"
#include "write.h"
#include "WAV.h"

#define SECS 15
#define SAMPLE_RATE 8000
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

    int16_t mic_data[N];

    gpio_set_output(27);
    gpio_set_on(27);

    nrf_output("waiting for data sync bit...\n");
    while (sync != 1) {
        net_get32(s, &sync);
    }

    gpio_set_off(27);

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
            if (x == 0xFFFFF) break;

            mic_data[i] = (int16_t) (x);
//            printk("%d\n", mic_data[i]);

            if (i % 1000 == 0) printk("Received sample #%d\n", i);
            i++;
        }
        ntimeout++;
    }
    nrf_output("Received sync bit! Ending data stream...\n");

    play_wav(&fs, &root, "MAIL.WAV", 44100);
    
    printk("finished playing mail...\n");
    // set_sample_rate(SAMPLE_RATE);
    printk("starting to play message...\n");
    for (unsigned j = 0; j < i * 5.5125; j++) {
        unsigned status = pwm_get_status();
        while (status & PWM_FULL1) {
            status = pwm_get_status();
        }
        uint8_t pcm = (mic_data[(int)(j / 5.5125)] + 0x8000) >> 8;
        pwm_write(pcm); // channel 0
        pwm_write(pcm); // channel 1
    }

    uint32_t dir_n;
    fat32_dirent_t *cur_dirents = get_dirents(&fs, root.cluster_id, &dir_n);

    for (int j = 0; j<10; j++) {
        char *test_name = "REC .WAV";
        test_name[3] = 48 + j;
        //only works if there are less than 10 recordings in the directory
        if (find_dirent_with_name(cur_dirents, dir_n, test_name) == -1) {
            write_wav(&fs, &root, mic_data, test_name, 8000, sizeof(wav_header_t) + i * sizeof(int16_t), i);
            break;
        }
    }

    cur_dirents = get_dirents(&fs, root.cluster_id, &dir_n);

    play_wav(&fs, &root, "PLAY.WAV", 44100);

    int button = 17;
    gpio_set_output(button);

    unsigned start = timer_get_usec();
    while (timer_get_usec() - start < 5000000) {
        if (gpio_read(button)) {
            for (int j = 0; j<10; j++) {
                char *test_name = "REC .WAV";
                test_name[3] = 48 + j;
                //only works if there are less than 10 recordings in the directory
                if (find_dirent_with_name(cur_dirents, dir_n, test_name) == -1) {
                    play_wav(&fs, &root, "RICK.WAV", 44100);
                    break;
                } else {
                    play_wav(&fs, &root, test_name, 8000);
                }
            }
            break;
        }
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
