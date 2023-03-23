#include "i2s.h"

// ONLY ACCEPTS 8000 OR 44100 FOR NOW
void i2s_init(int bit_rate, int sample_rate) {

    if (bit_rate != 8 && bit_rate != 16 && bit_rate != 32) {
        panic("ERROR: bit rate must be 8, 16, or 32\n");
        return;
    } else if (sample_rate != 4000 && sample_rate != 8000 && sample_rate != 44100) {
        panic("ERROR: sample rate must be 8000 or 44100\n");
        return;
    }

    // we should have a dev barrier in case we changed devices
    dev_barrier();

    //Step 1: set the BCLK, DIN and LRCL lines to ALT0
    gpio_set_function(I2S_PIN_CLK, GPIO_FUNC_ALT0);
    gpio_set_function(I2S_PIN_DIN, GPIO_FUNC_ALT0);
    gpio_set_function(I2S_PIN_DOUT, GPIO_FUNC_ALT0);
    gpio_set_function(I2S_PIN_FS, GPIO_FUNC_ALT0);

    dev_barrier();

    //Step 2: Enable the I2S clock
    PUT32(CM_I2SCTL, CM_PASSWORD | CM_SRC_OSCILLATOR);
    if (sample_rate == 44100) {
        PUT32(CM_I2SDIV, CM_PASSWORD | (6 << 12) | 8027);
    } else if (sample_rate == 8000) {
        PUT32(CM_I2SDIV, CM_PASSWORD | (37 << 12) | 5);
    } else if (sample_rate == 4000) {
        PUT32(CM_I2SDIV, CM_PASSWORD | (75 << 12) | 0);
    }
    PUT32(CM_I2SCTL, CM_PASSWORD | GET32(CM_I2SCTL) | (1 << 4));

    dev_barrier();

    //Step 3: frame length to 64 clocks (63 + 1) and frame sync to 32
    PUT32(I2S_MODE, (63 << I2S_MODE_FLEN_LB) | (32 << I2S_MODE_FSLEN_LB));
    
    // Channel 1 enabled with width 32 and offset 0
    if (bit_rate == 8) {
        PUT32(I2S_RXC, (1 << I2S_RXC_CH1EN));
    } else if (bit_rate == 16) {
        PUT32(I2S_RXC, (8 << I2S_RXC_CH1WID_LB) | (1 << I2S_RXC_CH1EN));
    } else if (bit_rate == 32) {
        PUT32(I2S_RXC, (1 << I2S_RXC_CH1WEX) | (8 << I2S_RXC_CH1WID_LB) | (1 << I2S_RXC_CH1EN));
    }
    PUT32(I2S_TXC, (1 << I2S_TXC_CH1WEX) | (8 << I2S_TXC_CH1WID_LB) | (1 << I2S_TXC_CH1EN));
    
    // clear TX and RX and disable STBY
    PUT32(I2S_CS, (1 << I2S_CS_STBY) | (1 << I2S_CS_RXCLR) | (1 << I2S_CS_TXCLR));
    delay_cycles(4);  // per spec in polled mode, delay at least 2 PCM cycles to ensure FIFOs reset

    //    PUT32(I2S_CS, GET32(I2S_CS) | (0b01 << 5));  // TXTHR to less than full

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

void i2s_disable() {
    dev_barrier();
    PUT32(I2S_CS, GET32(I2S_CS) & ~(1 << I2S_CS_EN));
}

void i2s_enable_rx() {
    dev_barrier();
    PUT32(I2S_CS, GET32(I2S_CS) & ~(1 << I2S_CS_TXON));
    PUT32(I2S_CS, GET32(I2S_CS) | (1 << I2S_CS_RXON));
}

void i2s_enable_tx() {
    dev_barrier();
    PUT32(I2S_CS, GET32(I2S_CS) & ~(1 << I2S_CS_RXON));
    PUT32(I2S_CS, GET32(I2S_CS) | (1 << I2S_CS_TXON));
}

void i2s_disable_tx() {
    dev_barrier();
    PUT32(I2S_CS, GET32(I2S_CS) & ~(1 << I2S_CS_TXON));
}

int32_t i2s_read_sample(void) {
    dev_barrier();
    // wait until the RX FIFO has data
    while ((GET32(I2S_CS) & (1 << I2S_CS_RXD)) == 0) {};
    // then return sample of FIFO
    return GET32(I2S_FIFO);
}

void i2s_transmit() {
    while ((GET32(I2S_CS) & (1 << I2S_CS_TXE)) == 0) {
        i2s_enable_tx();
        printk("Transmitting tx fifo....");
    }
    printk("\nTransmit complete, disabling TX...\n");
    i2s_disable_tx();
    dev_barrier();
}

void i2s_write_sample(int32_t val) {
    dev_barrier();

    while ((GET32(I2S_CS) & (1 << I2S_CS_TXD)) == 0) {
        printk("FIFO is full, queue transmit...\n");
        i2s_transmit();
    }
    // then write sample of FIFO
    PUT32(I2S_FIFO, val);
}

void print_csreg() {
    for (int i = 0; i < 26; i++) {
        printk("| %d", i);
        if (i / 10  == 0) {
            printk(" ");
        }
    }
    printk("\n");

    uint32_t csreg = GET32(I2S_CS);
    for (int i = 0; i < 26; i++) {
        printk("| %d ", (csreg & 0b1));
        csreg = csreg >> 1;
    }
    printk("\n");
}