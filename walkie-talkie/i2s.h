//gpio pins for I2S connections
#define BCLK 18
#define DOUT 21
#define LRCL 19

#define BASE        0x20000000
#define I2S_BASE    (BASE + 0x203000)
#define CLOCK_BASE  (BASE + 0x101000)

#define CM_PASSWORD 0x5A000000
#define CM_SRC_OSCILLATOR 0x01

#define CM_I2SCTL (CLOCK_BASE + 0x98) //BCM manual page 107, CM_GP0CTL
#define CM_I2SDIV (CLOCK_BASE + 0x9c) //BCM manual page 108, CM_GP0DIV

#define CM_ENABLE 0x10

#define I2S_MODE (I2S_BASE + 0x8) //BCM manual page 125
#define I2S_RXC (I2S_BASE + 0xc)  //BCM manual page 125
#define I2S_CS (I2S_BASE + 0x0)   //BCM manual page 125
#define I2S_FIFO (I2S_BASE + 0x4) //BCM manual page 125

#define I2S_CS_STBY 25
#define I2S_CS_SYNC 24
#define I2S_CS_RXSEX 23
#define I2S_CS_RXF 22
#define I2S_CS_RXD 20
#define I2S_CS_RXR 18
#define I2S_CS_RXERR 16
#define I2S_CS_RXSYNC 14
#define I2S_CS_RXTHR 7 // (- 8)
#define I2S_CS_RXCLR 4
#define I2S_CS_RXON 1
#define I2S_CS_EN 0

#define I2S_MODE_FLEN 10 // (- 19)
#define I2S_MODE_FSLEN 0 // (- 9)

#define I2S_RXC_CH1WEX 31
#define I2S_RXC_CH1EN 30
#define I2S_RXC_CH1POS 20 // (- 29)
#define I2S_RXC_CH1WID 16 // (- 19)