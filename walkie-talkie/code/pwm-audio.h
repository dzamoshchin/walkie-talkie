#include "rpi.h"
#include "libc/bit-support.h"


// pins used by I2S peripheral
typedef enum {
    PWM_CLK_CNTL = 40,
    PWM_CLK_DIV = 41,
} pwm_pin_t;

// timer divider (BCM peripherals page 108)
typedef enum {
    I2S_CLK_DIV_FRAC_LB = 0,
    I2S_CLK_DIV_FRAC_UB = 11,
    I2S_CLK_DIV_INT_LB = 12,
    I2S_CLK_DIV_INT_UB = 23
} i2s_div_bits_t;

// cs register bits (BCM peripherals page 126)
typedef enum {
    I2S_CS_EN = 0,
    I2S_CS_RXON = 1,
    I2S_CS_RXCLR = 4,
    I2S_CS_RXTHR_LB = 7,
    I2S_CS_RXTHR_UB = 8,
    I2S_CS_RXSYNC = 14,
    I2S_CS_RXERR = 16,
    I2S_CS_RXR = 18,
    I2S_CS_RXD = 20,
    I2S_CS_RXF = 22,
    I2S_CS_RXSEX = 23,
    I2S_CS_SYNC = 24,
    I2S_CS_STBY = 25,
} i2s_cs_bits_t;


// PWM control register bits (BCM peripherals page 142)
typedef enum {
    PWM1_USEFIFO  = 0x2000,  /* Data from FIFO */
    PWM1_ENABLE  = 0x0100,  /* Channel enable */
    PWM0_USEFIFO = 0x0020,  /* Data from FIFO */
    PWM0_ENABLE  = 0x0001  /* Channel enable */
} pwm_cntl_bits_t;

// rx control register bits (BCM peripherals page 132)
typedef enum {
    I2S_RXC_CH2WID_LB = 0,     // channel 2 width
    I2S_RXC_CH2WID_UB = 3,
    I2S_RXC_CH2POS_LB = 4,     // channel 2 position
    I2S_RXC_CH2POS_UB = 13,
    I2S_RXC_CH2EN = 14,         // channel 2 enable
    I2S_RXC_CH2WEX = 15,        // channel 2 width extension
    I2S_RXC_CH1WID_LB = 16,     // channel 1 width
    I2S_RXC_CH1WID_UB = 19,
    I2S_RXC_CH1POS_LB = 20,     // channel 1 position
    I2S_RXC_CH1POS_UB = 29,
    I2S_RXC_CH1EN = 30,         // channel 1 enable
    I2S_RXC_CH1WEX = 31,        // channel 1 width extension
} i2s_rxc_bits_t;



#define BASE        0x20000000
#define PWM_BASE        (BASE + 0x20C000) /* PWM1 register base address */
#define CLOCK_BASE  (BASE + 0x101000)

/////////////////////////
// CLOCK MANAGER STUFF //
/////////////////////////

#define CM_SRC_OSCILLATOR 0x01

#define CM_PWMCTL (CLOCK_BASE + PWM_CLK_CNTL)
#define CM_PWMDIV (CLOCK_BASE + PWM_CLK_DIV)

#define CM_ENABLE 0x10

#define PWM_CONTROL (PWM_BASE + 0x0)
#define PWM_STATUS  (PWM_BASE + 0x4)
#define PWM_DMAC    (PWM_BASE + 0x8)
#define PWM0_RANGE  (PWM_BASE + 0x10)
#define PWM0_DATA   (PWM_BASE + 0x14)
#define PWM_FIFO    (PWM_BASE + 0x18)
#define PWM1_RANGE  (PWM_BASE + 0x20)
#define PWM1_DATA   (PWM_BASE + 0x24)


// From errata (https://elinux.org/BCM2835_datasheet_errata#p107-108_table_6-35)
// See p107-108 table 6-35 section
// "Documentation relating to the PCM/I2S clock in missing"  ... yay :)
#define CM_REGS_BASE 0x20101000
#define PM_PASSWORD 0x5A000000  // for some reason, we always need to write MS byte of CM regs with 0x5A
#define CM_CTRL_XTAL 0x01   // want to write 0x01 to CM_PCM_CTRL to use 19.2 MHz oscillator
#define CM_CTRL_EN (1 << 4) // bit 4 is enable bit
#define CM_CTRL_MASH3 (3 << 9) // bits 9-10 are MASH control, want 3 stage for best performance

// clock divider: 19.2 MHz / 6.8027 / 64 = 44.1001 KHz
#define CM_DIV_INT 6        // integer divider for 19.2 MHz clock
#define CM_DIV_FRAC 3288    // fractional divider for 19.2 MHz clock (experimentally determined)

typedef struct {
    uint32_t other_regs[0x26];  // don't care
    uint32_t pcm_ctrl;          // pcm clock control reg
    uint32_t pcm_div;           // pcm clock divider reg
} cm_regs_t;
_Static_assert(offsetof(cm_regs_t, pcm_ctrl) == 0x98, "cm_regs_t pcm_ctrl offset");
_Static_assert(offsetof(cm_regs_t, pcm_div) == 0x9C, "cm_regs_t pcm_div offset");


#define STA_GAPO2 0x20
#define STA_GAPO1 0x10
#define STA_RERR1 0x8
#define STA_WERR1 0x4
#define STA_FULL1 0x1
#define ERRORMASK (STA_GAPO2 | STA_GAPO1 | STA_RERR1 | STA_WERR1)




void audio_init(void);

int write_pwm_stereo(unsigned l, unsigned r);


