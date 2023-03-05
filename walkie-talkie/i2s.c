#include "rpi.h"
#include "i2s.h"

void i2s_init() {
    //Using instructions found at: https://forums.raspberrypi.com/viewtopic.php?t=275436
    //Step 1: set the BCLK, DIN and LRCL lines to ALT0
    gpio_set_function(BCLK, GPIO_FUNC_ALT0);
    gpio_set_function(DOUT, GPIO_FUNC_ALT0);
    gpio_set_function(LRCL, GPIO_FUNC_ALT0);


    dev_barrier();

    //Step 2: Enable the I2S clock with frequency 4.096Mhz (= 4.0 divisor)
    PUT32(CM_I2SCTL, CM_PASSWORD | CM_SRC_OSCILLATOR);
    PUT32(CM_I2SDIV, CM_PASSWORD | (4 << 12));
    PUT32(CM_I2SCTL, CM_PASSWORD | CM_SRC_OSCILLATOR | (1 << 4));

    //Step 3: frame length to 64 clocks (63 + 1) and frame sync to 32
    PUT32(I2S_MODE, (63 << I2S_MODE_FLEN) | (32 << I2S_MODE_FSLEN));
    
    // Channel 1 enabled with width 32 and offset 0
    PUT32(I2S_RXC, (1 << I2S_RXC_CH1WEX) | (8 << I2S_RXC_CH1WID) | (1 << I2S_RXC_CH1EN));
    
    // Set up RX and disable STBY
    PUT32(I2S_CS, (1 << I2S_CS_STBY) | (1 << I2S_CS_RXCLR) | (1 << I2S_CS_RXON));
    delay_ms(1000);

    // Enable I2S
    PUT32(I2S_CS, GET32(I2S_CS) | (1 << I2S_CS_EN));

    dev_barrier();
}

// void i2s_enable_polled_mode() {
//     //BCM page 122
//     //Step 1: Set the EN bit to enable the PCM block
    //Step 2: Set all operational values to define the frame and channel settings. 
    //Step 3: Assert RXCLR and/or TXCLR wait for 2 PCM clocks to ensure the FIFOs are reset. 
    //Step 4: The SYNC bit can be used to determine when 2 clocks have passed.
    // Set RXTHR/TXTHR to determine the FIFO thresholds
    
// }

inline uint32_t read_sample() {
    // wait until the RX FIFO has data
    while ((GET32(I2S_CS) & (1 << I2S_CS_RXD)) == 0) {};
    // then return sample of FIFO
    return GET32(I2S_FIFO);
}

void notmain() {
    printk("STARTING I2S\n");

    gpio_set_output(27);
    gpio_set_on(27);
    delay_ms(1000);
    gpio_set_off(27);

    i2s_init();
    // while (1) {
    //     printk("%d\n", gpio_read(DOUT));
    // }
    // printk("%x\n", GET32(CM_I2SCTL));
    // printk("%x\n", GET32(CM_I2SDIV));
    // printk("%x\n", GET32(I2S_MODE));
    // printk("%x\n", GET32(I2S_RXC));
    // while (1) {
    //     // printk("%b\n", GET32(I2S_CS));
    //     printk("%b\n", GET32(I2S_FIFO));
    // }

    // while (1) {
    // //     printk("%b\n", GET32(I2S_CS));
        printk("sample:%b\n", read_sample());
    //     //printk("FIFO:%x\n", GET32(I2S_FIFO));
    //     PUT32(I2S_CS, GET32(I2S_CS) | (1 << I2S_CS_RXCLR));
    //     // PUT32(I2S_CS, GET32(I2S_CS) | (1 << I2S_CS_SYNC));
    //     // while((GET32(I2S_CS) & (1 << I2S_CS_SYNC)) == 0){};
    // }

}