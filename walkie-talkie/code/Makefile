# list out the driver program source
# TEST_SRC = $(wildcard tests/*.c)
# PROGS = tests/1-i2s-test.c
# PROGS = tests/2-i2s-dump.c
# TEST_SRC = tests/3-i2s-audiovis.c
# TEST_SRC = tests/4-fft-test.c
# TEST_SRC = tests/5-fft-time.c
# TEST_SRC = tests/6-fft-freqout.c
# TEST_SRC = tests/7-fft-freqviz.c
# TEST_SRC = tests/8-fft-buckets.c
# PROGS = send_audio.c
# PROGS = read_audio.c
# PROGS = output_audio.c
# PROGS = play_audio.c
# PROGS := tests/3-fat32-write.c
# PROGS += tests/2-fat32-read.c
# PROGS = i2s-write-wav.c
# PROGS = double_tap.c
# PROGS = play.c
PROGS = play_test.c
#PROGS = play.c
PROGS = play_audio_pwm.c

LIBS += ./libgcc.a

STAFF_OBJS  +=  ../../libpi/staff-objs/kmalloc.o 
STAFF_OBJS += $(WALKIE_PATH)/libpi/staff-objs/new-spi.o

# your source, shared by driver code.
#   if you want to use our staff-hc-sr04.o,
#   comment SRC out and uncomment STAFF_OBJS
SRC = i2s/i2s.c nrf/nrf-hw-support.c nrf/nrf-public.c nrf/nrf-driver.c pwm/pwm.c

SRC += fat32/fat32.c fat32/mbr.c fat32/pi-sd.c fat32/mbr-helpers.c fat32/fat32-helpers.c fat32/fat32-lfn-helpers.c fat32/external-code/unicode-utf8.c fat32/external-code/emmc.c fat32/external-code/mbox.c 
CFLAGS_EXTRA  = -I fat32/external-code

# define this if you need to give the device for your pi
# TTYUSB = /dev/ttyS7

# set RUN = 1 if you want the code to automatically run after building.
RUN = 1

DEPS = ./Makefile
COMMON_OBJS := $(SRC:.c=.o)
include $(WALKIE_PATH)/libpi/mk/Makefile.template-v2

clean::
	rm -f tests/*.list tests/*.o tests/*.elf tests/*~ tests/*.bin tests/*.d *.list *.o *.elf *.bin *.d
