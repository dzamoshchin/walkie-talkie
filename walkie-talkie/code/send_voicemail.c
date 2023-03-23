#include "rpi.h"
#include "pwm.h"
#include "WAV.h"
#include "tone.h"
#include "pitch.h"
#include "play_wav.h"
#include "i2s.h"
#include "nrf-test.h"


#define SECS 15
#define SAMPLE_RATE 4000
#define BIT_RATE 16
#define N (SAMPLE_RATE * SECS)

static void net_put32(nrf_t *nic, uint32_t txaddr, uint32_t x) {
    int ret = nrf_send_noack(nic, txaddr, &x, 4);
}

void notmain ()
{
    pwm_init();
    audio_init(SAMPLE_RATE);

    i2s_init(BIT_RATE, SAMPLE_RATE);
    i2s_enable_rx();

    int16_t *buf = (int16_t *)kmalloc(N * sizeof(int16_t));

    const int button = 27;
    gpio_set_input(button);

    printk("Playing query message...\n");
    play_wav("MSG.WAV");
    printk("done playing\n");

    // play tone
    tone(NOTE_A4);
    timer_delay_ms(2000);
    audio_init(SAMPLE_RATE);

    unsigned i = 0;
    printk("started recording.\n");
    while(1) {
        buf[i] = (int16_t) i2s_read_sample() + 0x8000;
        if (!gpio_read(button))
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
