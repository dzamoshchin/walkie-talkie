#include "rpi.h"
#include "pwm.h"
#include "pitch.h"
#include "fat32.h"
#include "audio.h"
#include "i2s.h"
#include "nrf-test.h"


#define SECS 15
#define SAMPLE_RATE 8000
#define BIT_RATE 16
#define N (SAMPLE_RATE * SECS)

static void net_put32(nrf_t *nic, uint32_t txaddr, uint32_t x) {
    int ret = nrf_send_noack(nic, txaddr, &x, 4);
}

void notmain ()
{

    i2s_init(BIT_RATE, SAMPLE_RATE);
    i2s_enable_rx();

    int16_t *buf = (int16_t *)kmalloc(N * sizeof(int16_t));

    const int button = 27;
    gpio_set_input(button);

    kmalloc_init();
    pi_sd_init();

    fat32_fs_t fs;
    pi_dirent_t root;
    config_fs(&fs, &root);

    printk("Playing query message...\n");
    play_wav(&fs, &root, "MSG.WAV", 44100);
    printk("done playing\n");

    delay_ms(100);
    printk("playing tone...\n");
    for(int a = 0; a < 200; a++) {
        play_tone(NOTE_A6);
        delay_ms(1);
        play_tone(0);
        delay_ms(1);
    }

    set_sample_rate(SAMPLE_RATE);

    unsigned i = 0;
    printk("started recording.\n");
    while(i < N) {
        buf[i] = (int16_t) i2s_read_sample() + 0x8000;
        if (gpio_read(button))
            break;
        i++;
    }
    printk("finished recording.\n");

    nrf_t *s = server_mk_noack(server_addr, 4);
    nrf_t *c = client_mk_noack(client_addr, 4);

    // Transmitting message...
    net_put32(c, s->rxaddr, 1);
    for (unsigned sample = 0; sample < i + 1; sample++) {
        uint8_t pcm = buf[sample] >> 8;
        net_put32(c, server_addr, pcm);
    }
    net_put32(c, s->rxaddr, 0xFFF);

}
