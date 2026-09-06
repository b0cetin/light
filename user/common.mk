
# Compiler Collection

TARGET  := x86_64-elf
CC      := $(TARGET)-gcc
LD      := $(TARGET)-ld

# Paths

USERSPACE_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
LIBC_DIR       := $(USERSPACE_ROOT)/libc

EXEC_NAME := $(notdir $(CURDIR))

TMP := /tmp/user-build/$(EXEC_NAME)
OUT := $(USERSPACE_ROOT)/../dist/user

C_SRCS := $(shell find . -type f -name '*.c')
OBJS   := $(C_SRCS:%.c=$(TMP)/%.o)

OBJ_DIRS := $(sort $(dir $(OBJS)))

# Flags

Q ?= @

CFLAGS  := -g -ffreestanding -nostdlib -fno-pic -fno-stack-protector \
           -mno-red-zone -mcmodel=large -Wall -O3 \
           -I$(LIBC_DIR)/include \
		   -I.

ASFLAGS := -ffreestanding -mno-red-zone -mcmodel=large

LDFLAGS   := -n -T ../linker.ld

# Building

.PHONY: all clean

$(TMP):
	mkdir -p $(TMP)

$(OBJ_DIRS):
	$(Q)mkdir -p $@

$(TMP)/%.o: %.c | $(TMP) $(OBJ_DIRS)
	@echo "  CC    $(notdir $<)"
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

all: $(OUT)/$(EXEC_NAME).elf

$(LIBC_DIR)/libc.a:
	$(MAKE) -C $(LIBC_DIR)

$(OUT)/$(EXEC_NAME).elf: $(OBJS) $(LIBC_DIR)/libc.a
	@echo "  LD    $(notdir $@)"
	$(Q)mkdir -p $(OUT)
	$(Q)$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LIBC_DIR)/libc.a

clean:
	rm -rf $(TMP) $(OUT)/$(EXEC_NAME).elf
