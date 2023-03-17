#include "rpi.h"
#include "i2s.h"

void notmain(void) {
    i2s_init();
    i2s_enable_tx();
    printk("%b\n", I2S_CS);
}
