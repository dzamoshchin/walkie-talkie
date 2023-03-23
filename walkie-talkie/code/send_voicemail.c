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

// notes in the melody:
int melody[] = {
        NOTE_C4,
        NOTE_G3,
        NOTE_G3,
        NOTE_A3,
        NOTE_G3,
        0,
        NOTE_B3,
        NOTE_C4
};

// note durations: 4 = quarter note, 8 = eighth note, etc.:
int noteDurations[] = { 4, 8, 8, 4, 4, 4, 4, 4 };


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

    printk("Waiting for initial button press...\n");
    while(!gpio_read(button)) ;

    delay_ms(50);
    printk("Playing query message...\n");
    play_wav(&fs, &root, "MSG.WAV", 44100);
    printk("done playing\n");

    delay_ms(100);
    printk("playing tone...\n");
//    int thisNote;
//    for (thisNote = 0; thisNote < 8; thisNote++) {
//        // to calculate the note duration, take one second
//        // divided by the note type.
//        //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
//        int noteDuration = 1000/noteDurations[thisNote];
//        play_tone(melody[thisNote]);
//        delay_ms(noteDuration);
//
//        // to distinguish the notes, set a minimum time between them.
//        // the note's duration + 30% seems to work well:
//        int pauseBetweenNotes = noteDuration * 0.30;
//        play_tone(0); // 0 turns off sound
//        delay_ms(pauseBetweenNotes);
//    }
//    play_tone(NOTE_G5);
    delay_ms(150);
    play_tone(NOTE_C6);
    delay_ms(150);
    play_tone(NOTE_F6);
    delay_ms(120);

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

    printk("transmitting...\n");
    net_put32(c, s->rxaddr, 1);
    for (unsigned sample = 0; sample < i + 1; sample++) {
        net_put32(c, server_addr, buf[sample]);
    }
    net_put32(c, s->rxaddr, 0xFFFFF);
    printk("finished transmit\n");


}
