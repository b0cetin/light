CC      = gcc
LD      = ld
OBJCOPY = objcopy

GNU_EFI = ../../gnu-efi

OBJS = boot.o reader.o elf_loader.o virtual_mapper.o

# Added -Wall for safety
CFLAGS = -I$(GNU_EFI)/inc \
		 -I$(GNU_EFI)/inc/x86_64 \
		 -ffreestanding \
		 -fno-stack-protector \
		 -fno-stack-check \
		 -fshort-wchar \
		 -mno-red-zone \
		 -fpic \
		 -Wall

# Note: We moved $(OBJS) and crt0 out of LDFLAGS and into the Recipe for clarity
LDFLAGS = -nostdlib \
		  -znocombreloc \
		  -shared \
		  -Bsymbolic \
		  -T $(GNU_EFI)/gnuefi/elf_x86_64_efi.lds

OBJCOPYFLAGS = \
	-j .text \
	-j .sdata \
	-j .data \
	-j .rodata \
	-j .dynamic \
	-j .dynsym \
	-j .rel \
	-j .rela \
	-j .rel.* \
	-j .rela.* \
	-j .reloc \
	--target=efi-app-x86_64 \
	--subsystem=10

all: BOOTX64.EFI

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# CRITICAL CHANGE: crt0 first, then your objects, then libraries
boot.elf: $(OBJS)
		$(LD) $(LDFLAGS) \
		$(GNU_EFI)/x86_64/gnuefi/crt0-efi-x86_64.o \
		$(OBJS) \
		-L $(GNU_EFI)/x86_64/gnuefi \
		-L $(GNU_EFI)/x86_64/lib \
		-lgnuefi \
		-lefi \
		-o boot.elf

BOOTX64.EFI: boot.elf
	$(OBJCOPY) $(OBJCOPYFLAGS) $< $@

clean:
	rm -f *.o boot.elf BOOTX64.EFI
