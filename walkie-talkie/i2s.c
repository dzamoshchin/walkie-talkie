// #define  PERIPHERAL_BASE  0x20000000  // Peripheral Base Address
// #define CM_BASE   0x101000	// Clock manager base address
// volatile uint32_t* CM = (uint32_t*)(PERIPHERAL_BASE + CM_BASE);

// // From errata data sheet (https://elinux.org/BCM2835_datasheet_errata)
// // 0x98 div 4 = 0x26   0x9C div 4 = 0x27
// #define CM_PASSWORD 0x5A000000  // Clock Control: Password "5A"
// #define CM_SRC_OSCILLATOR 0x01   // Clock Control: Clock Source = Oscillator
// #define CM_I2SCTL 0x26	// Clock Manager I2S Clock Control offset +0x98 which is 0x26 in a uint32_t array
// #define CM_I2SDIV 0x27	// Clock Manager 12SClock Divisor offset +0x9C which is 0x27 in a uint32_t array

// #define BCLK 18
// #define DOUT 21
// #define LRCK 19

// void i2s_init() {
//     gpio_set_function(BCLK, GPIO_FUNC_ALT0);
//     gpio_set_function(DOUT, GPIO_FUNC_ALT0);
//     gpio_set_function(LRCK, GPIO_FUNC_ALT0);


//     // PUT32(CM_I2SCTL );
// }

void notmain() {
    printk("hello");
}