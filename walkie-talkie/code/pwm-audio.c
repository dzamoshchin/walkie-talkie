#include "pwm-audio.h"

void audio_init(void) {
    dev_barrier();
    gpio_set_function(PWM_CLK_CNTL, GPIO_FUNC_ALT0); // Ensure PWM1 is mapped to GPIO 40/41
    gpio_set_function(PWM_CLK_DIV, GPIO_FUNC_ALT0);
    dev_barrier();

    // Setup clock
    PUT32(CM_PWMCTL, PM_PASSWORD | (1 << 5)); // Stop clock
    dev_barrier();

    int idiv = 2;
    PUT32(CM_PWMDIV, PM_PASSWORD | (idiv << 12));
    PUT32(CM_PWMCTL, PM_PASSWORD | (1 << 4) | 1);  // Osc + Enable
    dev_barrier();

    // Setup PWM
    PUT32(PWM_CONTROL, 0);
    dev_barrier();

    PUT32(PWM0_RANGE, 0x264); // 44.1khz, Stereo, 8-bit (54Mhz / 44100 / 2)
    PUT32(PWM1_RANGE, 0x264);

    PUT32(PWM_CONTROL, PWM1_USEFIFO |
                       PWM1_ENABLE |
                       PWM0_USEFIFO |
                       PWM0_ENABLE | 1 << 6);
}


int write_pwm_stereo(unsigned l, unsigned r) {
    long status;
    int ret = 0;

    status = GET32(PWM_STATUS);

//    if (!(status & STA_FULL1)) {
        printk("outputting: %d\n", l);
        PUT32(PWM_FIFO, l);  // left channel
        PUT32(PWM_FIFO, r);  // left channel
        ret = 1;
//    }

    if ((status & ERRORMASK)) {
        PUT32(PWM_STATUS, ERRORMASK);
    }

    return ret;
}