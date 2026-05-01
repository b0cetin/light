CC = $(TARGET)-gcc
LD = $(TARGET)-ld
OBJCOPY = $(TARGET)-objcopy

CFLAGS = -ffreestanding \
		 -nostdlib \
		 -fno-stack-protector \
		 -fno-stack-check \
		 -mno-red-zone \
		 -mcmodel=large \
		 -lgcc \
		 -O2 \
		 -Wall -Wextra

# LDFLAGS for a flat ELF binary
LDFLAGS = -n -T linker.ld

ASMS = gdt_flush.s kernel_stack.s idt_asm.s

OBJS := $(wildcard *.c)
OBJS := $(OBJS:.c=.o) $(ASMS:.s=.o)

all: kernel.elf

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(CC) $(CFLAGS) -c $< -o $@

kernel.elf: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

clean:
	rm -f *.o kernel.elf
