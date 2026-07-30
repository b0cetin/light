
TARGET  := x86_64-elf
CC      := $(TARGET)-gcc
LD      := $(TARGET)-ld
# OBJCOPY := $(TARGET)-objcopy
# LIBGCC  := $(shell $(CC) -print-libgcc-file-name)

OUT := ../dist
TMP := /tmp/kernel-build

C_SRCS  := $(wildcard *.c)
ASM_SRCS := $(wildcard *.s)

C_OBJS   := $(C_SRCS:%.c=$(TMP)/%.o)
ASM_OBJS := $(ASM_SRCS:%.s=$(TMP)/%.o)
OBJS     := $(C_OBJS) $(ASM_OBJS)
DEPS     := $(C_OBJS:.o=.d)

CFLAGS := \
    -ffreestanding \
    -nostdlib \
    -fno-stack-protector \
    -fno-stack-check \
    -mno-red-zone \
    -mcmodel=large \
    -O2 \
    -g \
    -MMD -MP \
    -Wall -Wextra
#     -Werror

ASFLAGS := \
    -ffreestanding \
    -mno-red-zone \
    -mcmodel=large

LDFLAGS := \
    -n \
    -T linker.ld

# ── Targets ───────────────────────────────────────────────────────────────────
.PHONY: all clean compile_commands

all: $(OUT)/kernel.elf

-include $(DEPS)

$(TMP)/%.o: %.c | $(TMP)
	@echo "  CC    $(notdir $<)"
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

# Separate rule for assembly — uses ASFLAGS, not CFLAGS
$(TMP)/%.o: %.s | $(TMP)
	@echo "  AS    $(notdir $<)"
	$(Q)$(CC) $(ASFLAGS) -c $< -o $@

$(OUT)/kernel.elf: $(OBJS)
	@echo "  LD    $(notdir $@)"
	$(Q)mkdir -p $(OUT)
	$(Q)$(LD) $(LDFLAGS) -o $@ $(OBJS)
	@echo "  ✓  $(OUT)/kernel.elf"

$(TMP):
	$(Q)mkdir -p $@

clean:
	$(Q)rm -rf $(TMP) $(OUT)/kernel.elf
