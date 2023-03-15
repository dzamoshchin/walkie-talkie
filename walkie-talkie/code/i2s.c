#include "i2s.h"

#define addr(x) ((uint32_t)&(x))

void i2s_init(void) {
    // we should have a dev barrier in case we changed devices
    dev_barrier();

    //Step 1: set the BCLK, DIN and LRCL lines to ALT0
    gpio_set_function(I2S_PIN_CLK, GPIO_FUNC_ALT0);
    gpio_set_function(I2S_PIN_DIN, GPIO_FUNC_ALT0);
    gpio_set_function(I2S_PIN_FS, GPIO_FUNC_ALT0);

    dev_barrier();

    //Step 2: Enable the I2S clock
    PUT32(CM_I2SCTL, CM_PASSWORD | CM_SRC_OSCILLATOR);
    PUT32(CM_I2SDIV, CM_PASSWORD | (0b110 << 12) | CM_DIV_FRAC);
    PUT32(CM_I2SCTL, CM_PASSWORD | GET32(CM_I2SCTL) | (1 << 4));

    dev_barrier();

    //Step 3: frame length to 64 clocks (63 + 1) and frame sync to 32
    PUT32(I2S_MODE, (63 << I2S_MODE_FLEN_LB) | (32 << I2S_MODE_FSLEN_LB));
    
    // Channel 1 enabled with width 32 and offset 0
    PUT32(I2S_RXC, (1 << I2S_RXC_CH1WEX) | (8 << I2S_RXC_CH1WID_LB) | (1 << I2S_RXC_CH1EN));
    PUT32(I2S_TXC, (1 << I2S_TXC_CH1WEX) | (8 << I2S_TXC_CH1WID_LB) | (1 << I2S_TXC_CH1EN));
    
    // clear TX and RX and disable STBY
    PUT32(I2S_CS, (1 << I2S_CS_STBY) | (1 << I2S_CS_RXCLR) | (1 << I2S_CS_TXCLR));

    // Enable I2S
    PUT32(I2S_CS, GET32(I2S_CS) | (1 << I2S_CS_EN));

    dev_barrier();
}

void is2_clear() {
    PUT32(I2S_CS, GET32(I2S_CS) & ~(1 << I2S_CS_EN));
    dev_barrier();
    PUT32(I2S_CS, GET32(I2S_CS) | (1 << I2S_CS_TXCLR) | (1 << I2S_CS_RXCLR));
}

void is2_enable() {
    dev_barrier();
    PUT32(I2S_CS, GET32(I2S_CS) | (1 << I2S_CS_EN));
    dev_barrier();
}

void i2s_enable_rx() {
    dev_barrier();
    is2_clear();
    PUT32(I2S_CS, GET32(I2S_CS) & ~(1 << I2S_CS_TXON));
    PUT32(I2S_CS, GET32(I2S_CS) | (1 << I2S_CS_RXON));
    is2_enable();
}

void i2s_enable_tx() {
    dev_barrier();
    is2_clear();
    PUT32(I2S_CS, GET32(I2S_CS) & ~(1 << I2S_CS_RXON));
    PUT32(I2S_CS, GET32(I2S_CS) | (1 << I2S_CS_TXON));
    is2_enable();
}

int32_t i2s_read_sample(void) {
    dev_barrier();
    // wait until the RX FIFO has data
    while ((GET32(I2S_CS) & (1 << I2S_CS_RXD)) == 0) {};
    // then return sample of FIFO
    return GET32(I2S_FIFO);
}

void i2s_write_sample(int32_t val) {
    dev_barrier();
    // wait until the RX FIFO has data
    while ((GET32(I2S_CS) & (1 << I2S_CS_TXD)) == 0) {};
    // then return sample of FIFO
    return PUT32(I2S_FIFO, val);
}