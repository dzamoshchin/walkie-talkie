#include "rpi.h"
#include "i2s.h"

void notmain(void) {
    //init i2c
    i2s_init();

    i2s_enable_tx();

    // do the test
    i2s_write_sample(0xdeadbeef);
}
