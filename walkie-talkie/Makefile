PROGS := i2s.c

# uncomment if you want it to automatically run.
RUN=1

# define this if you need to give the device for your pi
BOOTLOADER = my-install

#for running on Ari's PI
TTYUSB = /dev/ttyS9

# GREP_STR := 'HASH:\|ERROR:\|PANIC:\|SUCCESS:\|NRF:'
EXCLUDE ?= grep -v simple_boot
include $(CS140E_2023_PATH)/libpi/mk/Makefile.template-v2