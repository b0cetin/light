GNU_EFI := /gnu-efi
ARCH    := x86_64
OUT     := ../dist
TMP     := /tmp/bootloader-build

Q ?= @

CC      = gcc
LD      = ld
OBJCOPY = objcopy

SRCS := $(wildcard *.c)
OBJS := $(SRCS:%.c=$(TMP)/%.o)
DEPS := $(OBJS:.o=.d)

CFLAGS = \
    -I$(GNU_EFI)/inc \
    -I$(GNU_EFI)/inc/$(ARCH) \
    -I$(GNU_EFI)/inc/protocol \
    -ffreestanding \
    -fno-stack-protector \
    -fno-stack-check \
    -fshort-wchar \
    -mno-red-zone \
    -fpic \
    -MMD -MP \
    -Wall -Wextra \
    -Wno-unused-parameter \
    -Werror

LDFLAGS := \
    -nostdlib \
    -znocombreloc \
    -shared \
    -Bsymbolic \
    -T $(GNU_EFI)/gnuefi/elf_x86_64_efi.lds

OBJCOPYFLAGS := \
    -j .text    \
    -j .sdata   \
    -j .data    \
    -j .rodata  \
    -j .dynamic \
    -j .dynsym  \
    -j .rel     \
    -j .rela    \
    -j .rel.*   \
    -j .rela.*  \
    -j .reloc   \
    -I elf64-x86-64 \
    -O efi-app-x86_64 \
    --subsystem=10

# ── Targets ───────────────────────────────────────────────────────────────────

.PHONY: all clean compile_commands

all: $(OUT)/BOOTX64.EFI

-include $(DEPS)

$(TMP)/%.o: %.c | $(TMP)
	@echo "  CC    $(notdir $<)"
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

$(TMP)/boot.elf: $(OBJS)
	@echo "  LD    $(notdir $@)"
	$(Q)$(LD) $(LDFLAGS) \
	    $(GNU_EFI)/$(ARCH)/gnuefi/crt0-efi-$(ARCH).o \
	    $(OBJS) \
	    -L $(GNU_EFI)/$(ARCH)/gnuefi \
	    -L $(GNU_EFI)/$(ARCH)/lib \
	    -lgnuefi -lefi \
	    -o $@

$(OUT)/BOOTX64.EFI: $(TMP)/boot.elf
	@echo "  EFI   $@"
	$(Q)mkdir -p $(OUT)
	$(Q)$(OBJCOPY) $(OBJCOPYFLAGS) $< $@
	@echo "  ✓  $(OUT)/BOOTX64.EFI"

$(TMP):
	$(Q)mkdir -p $@

clean:
	$(Q)rm -rf $(TMP) $(OUT)/BOOTX64.EFI
