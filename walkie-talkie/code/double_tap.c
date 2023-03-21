#include "rpi.h"

#define DOUBLE_TAP_THRESHOLD 400000

void notmain() {
    const int button = 27;

    gpio_set_input(button);

    unsigned last_tap_time = 0;

    while (1) {
        if (gpio_read(button)) {
            unsigned start = timer_get_usec();
            while(1) {
                if (!gpio_read(button))
                    break;
            }
            unsigned end = timer_get_usec();

            // this is how long the tap was
            printk("%d\n", end-start);

            if (end - last_tap_time < DOUBLE_TAP_THRESHOLD) {
                // this is if double tap
                printk("double tap!\n");
            }

            last_tap_time = timer_get_usec();
        }
    }
}