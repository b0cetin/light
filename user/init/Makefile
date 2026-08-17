TARGET  := x86_64-elf
CC      := $(TARGET)-gcc
LD      := $(TARGET)-ld

EXEC_NAME := $(notdir $(CURDIR))

CFLAGS  := -g -ffreestanding -nostdlib -fno-stack-protector -O0
LDFLAGS := -n -T linker.ld

ASFLAGS := \
    -ffreestanding \
    -mno-red-zone \
    -mcmodel=large

OUT := ../../dist/user
TMP := /tmp/user-build/$(EXEC_NAME)

C_SRCS := $(wildcard *.c)
OBJS   := $(C_SRCS:%.c=$(TMP)/%.o)

.PHONY: all clean $(EXEC_NAME)

all: $(EXEC_NAME)

$(EXEC_NAME): $(OUT)/$(EXEC_NAME).elf

# Rule to create the temporary build directory
$(TMP):
	$(Q)mkdir -p $(TMP)

$(TMP)/%.o: %.c | $(TMP)
	@echo "  CC    $(notdir $<)"
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/$(EXEC_NAME).elf: $(OBJS)
	@echo "  AS    crt0.s"
	$(Q)$(CC) $(ASFLAGS) -c crt0.s -o crt0.o

	@echo "  LD    $(notdir $@)"
	$(Q)mkdir -p $(OUT)
	$(Q)$(LD) $(LDFLAGS) -o $@ $(OBJS) crt0.o
	@echo "  ✓  $(OUT)/$(EXEC_NAME).elf"

clean:
	$(Q)rm -rf $(TMP) $(OUT)/$(EXEC_NAME).elf
