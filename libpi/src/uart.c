// implement:
//  void uart_init(void)
//
//  int uart_can_get8(void);
//  int uart_get8(void);
//
//  int uart_can_put8(void);
//  void uart_put8(uint8_t c);
//
//  int uart_tx_is_empty(void) {
//
// see that hello world works.
//
//
#include "rpi.h"

#define BAUD_REG_VAL 270

enum {
    AUX_BASE = 0x20215000,
    AUX_ENABLES = (AUX_BASE + 0x4),
    AUX_MU_IO_REG = (AUX_BASE + 0x40),
    AUX_MU_IER_REG = (AUX_BASE + 0x44),
    AUX_MU_IIR_REG = (AUX_BASE + 0x48),
    AUX_MU_LCR_REG = (AUX_BASE + 0x4c),
    AUX_MU_MCR_REG = (AUX_BASE + 0x50),
    AUX_MU_CNTL_REG = (AUX_BASE + 0x60),
    AUX_MU_STAT_REG = (AUX_BASE + 0x64),
    AUX_MU_BAUD_REG = (AUX_BASE + 0x68),
};

// called first to setup uart to 8n1 115200  baud,
// no interrupts.
//  - you will need memory barriers, use <dev_barrier()>
//
//  later: should add an init that takes a baud rate.
void uart_init(void) {
    // setup GPIO pins
    dev_barrier();
    gpio_set_function(GPIO_TX, GPIO_FUNC_ALT5);
    gpio_set_function(GPIO_RX, GPIO_FUNC_ALT5);
    dev_barrier();

    // turn on the UART in AUX
    unsigned value = GET32(AUX_ENABLES);
    PUT32(AUX_ENABLES, value | 0b1);
    dev_barrier();
    
    // disable rx, tx
    PUT32(AUX_MU_CNTL_REG, 0b00);

    // disable interrupts
    PUT32(AUX_MU_IER_REG, 0b00);

    // set UART to 8-bit mode
    PUT32(AUX_MU_LCR_REG, 0b11);

    // set data ready
    PUT32(AUX_MU_MCR_REG, 0b0);

    // clear the receive FIFO, clear the transmit FIFO
    PUT32(AUX_MU_IIR_REG, 0b11 << 1);

    // set baud rate to 115,200 by setting BAUD_REG to 270
    PUT32(AUX_MU_BAUD_REG, BAUD_REG_VAL);

    // enable rx, tx
    PUT32(AUX_MU_CNTL_REG, 0b11);

    dev_barrier();
}

// disable the uart.
void uart_disable(void) {
    uart_flush_tx();
    unsigned value = GET32(AUX_ENABLES);
    PUT32(AUX_ENABLES, value & ~(0b1));
    dev_barrier();
}


// returns one byte from the rx queue, if needed
// blocks until there is one.
int uart_get8(void) {
    dev_barrier();
    while (!uart_has_data()) {}
    return GET32(AUX_MU_IO_REG) & 0b11111111;
}

// 1 = space to put at least one byte, 0 otherwise.
int uart_can_put8(void) {
    return ((GET32(AUX_MU_STAT_REG) >> 1) & 1);
}

// put one byte on the tx qqueue, if needed, blocks
// until TX has space.
// returns < 0 on error.
int uart_put8(uint8_t c) {
    dev_barrier(); 
    while (!uart_can_put8()) {}
    PUT32(AUX_MU_IO_REG, c);
    return 0;
}

// simple wrapper routines useful later.

// 1 = at least one byte on rx queue, 0 otherwise
int uart_has_data(void) {
    return (GET32(AUX_MU_STAT_REG) & 1);
}

// return -1 if no data, otherwise the byte.
int uart_get8_async(void) { 
    if(!uart_has_data())
        return -1;
    return uart_get8();
}

// 1 = tx queue empty, 0 = not empty.
int uart_tx_is_empty(void) {
    return ((GET32(AUX_MU_STAT_REG) >> 9) & 1);
}

// flush out all bytes in the uart --- we use this when 
// turning it off / on, etc.
void uart_flush_tx(void) {
    while(!uart_tx_is_empty())
        ;
}
