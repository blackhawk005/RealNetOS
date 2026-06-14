CROSS = aarch64-none-elf
CFLAGS = -ffreestanding -O0 -nostdlib -nostartfiles -Wall -mcpu=cortex-a72 -march=armv8-a -mabi=lp64
PLATFORM ?= qemu

ifeq ($(PLATFORM),qemu)
  SD_SRC = block/sd_qemu.c
  CFLAGS += -DENABLE_GENET=0
else ifeq ($(PLATFORM),rpi)
  SD_SRC = block/sd_rpi.c
  CFLAGS += -DENABLE_GENET=1
else
  $(error Unknown PLATFORM '$(PLATFORM)'; use qemu or rpi)
endif

SRC = \
  boot/start.S \
  boot/exception_vector.S \
  kernel/context_switch.S \
  kernel/el0_test.S \
  kernel/user_ctx.S \
  kernel/threads.c \
  kernel/scheduler.c \
  kernel/uart.c \
  kernel/mailbox.c \
  kernel/fb.c \
  kernel/irq.c \
  kernel/timer.c \
  kernel/mem.c \
  kernel/page.c \
  kernel/syscall.c \
  kernel/user1.S \
  kernel/user2.S \
  kernel/main.c \
  kernel/syscall_entry.c \
  kernel/genet.c \
  src/lib.c \
  net/ip.c \
  net/udp.c \
  net/ethernet.c \
  net/tcp.c \
  net/rx_tx_threads.c \
  net/packet_queue.c \
  kernel/user3_entry.c \
  kernel/ksh.c \
  user/sensor_task.c \
  user/rt_demo.c \
  user/user_syscall.c \
  user/shell.c \
  net/dma.c \
  fs/vfs.c \
  fs/dummyfs.c \
  fs/fat32.c \
  $(SD_SRC)
  
OBJ = $(SRC:.c=.o)
OBJ := $(OBJ:.S=.o)

all: kernel8.img

%.o: %.c
	$(CROSS)-gcc $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CROSS)-gcc $(CFLAGS) -c $< -o $@

kernel8.img: $(OBJ)
	$(CROSS)-ld -T boot/kernel.ld -o kernel.elf $(OBJ)
	$(CROSS)-objcopy -O binary kernel.elf kernel8.img

clean:
	rm -f *.img *.elf */*.o kernel8.img kernel.elf
	rm -rf dist

# Serial-only QEMU run (shell on stdio). Ctrl-A X to quit.
run-qemu: kernel8.img
	qemu-system-aarch64 -M raspi4b -kernel kernel8.img -nographic

# QEMU with an emulated HDMI display window (shows the framebuffer console).
run-graphic: kernel8.img
	qemu-system-aarch64 -M raspi4b -kernel kernel8.img -serial stdio

# Stage an SD-card image folder for a real Raspberry Pi 4.
# Builds the SDHCI (rpi) kernel and copies it next to config.txt into dist/.
# You still need to drop the Pi firmware (start4.elf, fixup4.dat) into dist/.
sdcard:
	$(MAKE) clean
	$(MAKE) PLATFORM=rpi
	mkdir -p dist
	cp kernel8.img config.txt dist/
	@echo ""
	@echo "==> dist/ ready. Now add the Raspberry Pi 4 firmware:"
	@echo "    start4.elf and fixup4.dat from https://github.com/raspberrypi/firmware/tree/master/boot"
	@echo "    Then copy the full contents of dist/ to a FAT32-formatted SD card."
