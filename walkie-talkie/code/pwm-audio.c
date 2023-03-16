#include "pwm-audio.h"

void audio_init(void) {
    dev_barrier();
    gpio_set_function(PWM_CLK_CNTL, GPIO_FUNC_ALT0); // Ensure PWM1 is mapped to GPIO 40/41
    gpio_set_function(PWM_CLK_DIV, GPIO_FUNC_ALT0);
//    gpio_set_function(45, GPIO_FUNC_ALT0);
    dev_barrier();

    // Setup clock
//    PUT32(CM_PWMCTL, PM_PASSWORD | (1 << 5)); // Stop clock
//    dev_barrier();

    printk("CLK control REG before: %b\n", GET32(CM_PWMCTL));

    PUT32(CM_PWMCTL, PM_PASSWORD | 1);  // Osc
    PUT32(CM_PWMDIV, PM_PASSWORD | (0b110 << 12) | CM_DIV_FRAC);
    PUT32(CM_PWMCTL, PM_PASSWORD | GET32(CM_PWMCTL) | (1 << 4));  // Enable
    dev_barrier();

    printk("CLK control REG after: %b\n", GET32(CM_PWMCTL));


    // Setup PWM
    PUT32(PWM_CONTROL, 0);
    dev_barrier();

    PUT32(PWM0_RANGE, 0x7FFFFFFF); // 40x264: 4.1khz, Stereo, 8-bit (54Mhz / 44100 / 2)
    PUT32(PWM1_RANGE, 0x7FFFFFFF);

    printk("PWM control REG before: %b\n", GET32(PWM_CONTROL));

    PUT32(PWM_CONTROL, GET32(PWM_CONTROL) | PWM1_USEFIFO |
                       PWM1_ENABLE |
                       PWM0_USEFIFO |
                       PWM0_ENABLE | 1 << 6);

    printk("PWM control REG after: %b\n", GET32(PWM_CONTROL));


    PUT32(PWM_STATUS, STA_BERR);
}


int write_pwm_stereo(unsigned l, unsigned r) {
    long status;
    int ret = 0;

    status = GET32(PWM_STATUS);

    printk("PWM status: %b\n", status);

    if (!(status & STA_FULL1)) {
        printk("outputting: %d\n", l);
        PUT32(PWM_FIFO, l);  // left channel
        PUT32(PWM_FIFO, r);  // left channel
        ret = 1;
    }

    if ((status & ERRORMASK)) {
        PUT32(PWM_STATUS, ERRORMASK);
    }

    return ret;
}