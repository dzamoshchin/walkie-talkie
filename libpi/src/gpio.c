/*
 * Implement the following routines to set GPIO pins to input or output,
 * and to read (input) and write (output) them.
 *
 * DO NOT USE loads and stores directly: only use GET32 and PUT32
 * to read and write memory.  Use the minimal number of such calls.
 *
 * See rpi.h in this directory for the definitions.
 */
#include "rpi.h"

// see broadcomm documents for magic addresses.
enum {
    GPIO_BASE = 0x20200000,
    gpio_set0  = (GPIO_BASE + 0x1C),
    gpio_clr0  = (GPIO_BASE + 0x28),
    gpio_lev0  = (GPIO_BASE + 0x34),
    gpio_fsel0 = (GPIO_BASE),
    gpio_gppud = (GPIO_BASE + 0x94),
    gpio_gppudclk0 = (GPIO_BASE + 0x98)
};

//
// Part 1 implement gpio_set_on, gpio_set_off, gpio_set_output
//

void gpio_set_function(unsigned pin, gpio_func_t func) {
    if(pin >= 32 && pin != 47)
        return;
    if(func < 0 || func >7)
        return;

    volatile unsigned loc = gpio_fsel0 + (pin / 10) * sizeof(unsigned);
    unsigned value = GET32(loc);
    int shift = (pin % 10) * 3;
    value &= ~(0b111 << shift);
    value |= (func << shift);
    PUT32(loc, value);
}

// set <pin> to be an output pin.
//
// note: fsel0, fsel1, fsel2 are contiguous in memory, so you
// can (and should) use array calculations!
void gpio_set_output(unsigned pin) {
    if(pin >= 32 && pin != 47)
        return;

    gpio_set_function(pin, GPIO_FUNC_OUTPUT);
}

// set GPIO <pin> on.
void gpio_set_on(unsigned pin) {
    if(pin >= 32 && pin != 47)
        return;

    PUT32(gpio_set0, (1 << pin));
}

// set GPIO <pin> off
void gpio_set_off(unsigned pin) {
    if(pin >= 32 && pin != 47)
        return;

    PUT32(gpio_clr0, (1 << pin));
}

// set <pin> to <v> (v \in {0,1})
void gpio_write(unsigned pin, unsigned v) {
    if(v)
        gpio_set_on(pin);
    else
        gpio_set_off(pin);
}

//
// Part 2: implement gpio_set_input and gpio_read
//

// set <pin> to input.
void gpio_set_input(unsigned pin) {
    if(pin >= 32 && pin != 47)
        return;
    gpio_set_function(pin, GPIO_FUNC_INPUT);
}

// return the value of <pin>
int gpio_read(unsigned pin) {
    if(pin >= 32 && pin != 47)
        return -1;
    unsigned v = 0;
    unsigned value = GET32(gpio_lev0);
    value &= (1 << pin);
    v = value >> pin;
    return DEV_VAL32(v);
}

void gpio_pud_off(unsigned pin) {
    PUT32(gpio_gppud, 0b00);
}

void gpio_set_pullup(unsigned pin) {
    PUT32(gpio_gppud, 0b10);
    delay_cycles(150);
    PUT32(gpio_gppudclk0, (1 << pin));
    delay_cycles(150);
    PUT32(gpio_gppudclk0, (0 << pin));
}

void gpio_set_pulldown(unsigned pin) {
    PUT32(gpio_gppud, 0b01);
    delay_cycles(150);
    PUT32(gpio_gppudclk0, (1 << pin));
    delay_cycles(150);
    PUT32(gpio_gppudclk0, (0 << pin));
}