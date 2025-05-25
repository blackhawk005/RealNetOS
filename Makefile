CROSS = aarch64-elf
CFLAGS = -ffreestanding -O0 -nostdlib -nostartfiles -Wall -mcpu=cortex-a72 -march=armv8-a -mabi=lp64

SRC = \
  boot/start.S \
  boot/exception_vector.S \
  kernel/context_switch.S \
  kernel/threads.c \
  kernel/scheduler.c \
  kernel/uart.c \
  kernel/irq.c
  
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
