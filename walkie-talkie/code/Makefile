# ===== PLAYING =====
# PROGS = play_tone.c
# PROGS = play_wav.c
# PROGS = record_play.c
# PROGS = test_audio_i2s.c
PROGS = send_wav_i2s.c

# ===== RECEIVE =====
# PROGS = play_audio_pwm.c

# ===== SEND =====
# PROGS = send_wav.c
# PROGS = send_mic.c


# ==== OTHER =====
# PROGS = i2s-write-wav.c
# PROGS = i2s-write-5-sec.c
# PROGS = double_tap.c
# PROGS = tests/1-i2s-test.c
# PROGS = tests/2-i2s-dump.c
# PROGS = tests/2-fat32-read.c
# PROGS = tests/3-fat32-write.c

LIBS += ./libgcc.a

STAFF_OBJS += $(WALKIE_PATH)/libpi/staff-objs/kmalloc.o 
STAFF_OBJS += $(WALKIE_PATH)/libpi/staff-objs/new-spi.o

SRC = i2s/i2s.c nrf/nrf-hw-support.c nrf/nrf-public.c nrf/nrf-driver.c pwm/pwm.c
SRC += fat32/fat32.c fat32/mbr.c fat32/pi-sd.c fat32/mbr-helpers.c fat32/fat32-helpers.c fat32/fat32-lfn-helpers.c fat32/external-code/unicode-utf8.c fat32/external-code/emmc.c fat32/external-code/mbox.c 

CFLAGS_EXTRA  = -I fat32/external-code -I i2s/ -I nrf/ -I pwm/ -I fat32/

# define this if you need to give the device for your pi
# TTYUSB = /dev/ttyS7

# set RUN = 1 if you want the code to automatically run after building.
RUN = 1

DEPS = ./Makefile
COMMON_OBJS := $(SRC:.c=.o)
include $(WALKIE_PATH)/libpi/mk/Makefile.template-v2

clean::
	rm -f tests/*.list tests/*.o tests/*.elf tests/*~ tests/*.bin tests/*.d *.list *.o *.elf *.bin *.d
