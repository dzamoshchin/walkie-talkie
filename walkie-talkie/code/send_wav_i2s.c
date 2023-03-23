#include "rpi.h"
#include "i2s/i2s.h"


// #include "samples/sin8.h"
// #include "samples/sound2.h"
#include "samples/sound3.h"


const int button = 27;


void notmain() {
    printk("--------------BEFORE I2S INIT--------------\n");
    print_csreg();
    i2s_init(8, sample_freq);
    printk("--------------AFTER I2S INIT--------------\n");
    print_csreg();

    while (1) {
        if (gpio_read(button)) {
            printk("starting sending...\n");
            for (unsigned int sample = 0; sample < sizeof(wav_data) / 2; sample++) {
                i2s_write_sample((unsigned) wav_data[sample]);
//                printk("FIFO: %x\n", GET32(I2S_FIFO));
//                print_csreg();
            }
            printk("done sending...\n");
            print_csreg();

            break;
        }

    }
}
