# list out the driver program source
# TEST_SRC = $(wildcard tests/*.c)
# TEST_SRC = tests/1-i2s-test.c
# PROGS = tests/2-i2s-dump.c
# TEST_SRC = tests/3-i2s-audiovis.c
# TEST_SRC = tests/4-fft-test.c
# TEST_SRC = tests/5-fft-time.c
# TEST_SRC = tests/6-fft-freqout.c
# TEST_SRC = tests/7-fft-freqviz.c
# TEST_SRC = tests/8-fft-buckets.c
#PROGS = send_audio.c
#PROGS = read_audio.c
PROGS = output_audio.c
# OPT_LEVEL = -O3

LIBS += ./libgcc.a

STAFF_OBJS  +=  ../../libpi/staff-objs/kmalloc.o 
STAFF_OBJS += $(CS140E_2023_PATH)/libpi/staff-objs/new-spi.o

# your source, shared by driver code.
#   if you want to use our staff-hc-sr04.o,
#   comment SRC out and uncomment STAFF_OBJS
SRC = i2s.c nrf-hw-support.c nrf-public.c nrf-driver.c pwm-audio.c

# define this if you need to give the device for your pi
TTYUSB = 

# set RUN = 1 if you want the code to automatically run after building.
RUN = 1

DEPS = ./Makefile
COMMON_OBJS := $(SRC:.c=.o)
include ../../libpi/mk/Makefile.template-v2
