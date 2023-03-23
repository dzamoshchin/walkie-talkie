#include "rpi.h"
#include "i2s/i2s.h"


const int button = 27;


void notmain(void) {
    printk("--------------BEFORE I2S INIT--------------\n");
    print_csreg();
    i2s_init(16, 44100);
    printk("--------------AFTER I2S INIT--------------\n");
    print_csreg();

    gpio_set_input(button);

    uint32_t i = 0;
    // do the test
    while (1) {
        if (gpio_read(button)) {
            delay_ms(100);
            i2s_write_sample((unsigned) 0xdeadbeef);
            printk("FIFO: %x\n", GET32(I2S_FIFO));
            print_csreg();
        }
    }
}
