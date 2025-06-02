CROSS = aarch64-none-elf
CFLAGS = -ffreestanding -O0 -nostdlib -nostartfiles -Wall -mcpu=cortex-a72 -march=armv8-a -mabi=lp64

SRC = \
  boot/start.S \
  boot/exception_vector.S \
  kernel/context_switch.S \
  kernel/threads.c \
  kernel/scheduler.c \
  kernel/uart.c \
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
  user/sensor_task.c \
  user/shell.c \
  net/dma.c \
  fs/vfs.c \
  fs/dummyfs.c \
  fs/fat32.c \
  block/sd.c
  
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
