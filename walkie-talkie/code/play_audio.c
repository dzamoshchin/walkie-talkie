#include "rpi.h"
#include "i2s.h"

void notmain(void) {
    i2s_init();
    i2s_enable_tx();

    // do the test
    while (1) {
        i2s_write_sample(0xdeadbeef);
        printk("wrote");
    }
}
