
# Compiler Collection

TARGET  := x86_64-elf
CC      := $(TARGET)-gcc
LD      := $(TARGET)-ld
AR      := $(TARGET)-ar

# Paths

LIBC_DIR       := ../libc

EXEC_NAME := libg

TMP := /tmp/user-build/$(EXEC_NAME)
OUT := ../../dist/user

C_SRCS := $(wildcard *.c)
OBJS   := $(C_SRCS:%.c=$(TMP)/%.o)

# Flags

Q ?= @

CFLAGS  := -g -ffreestanding -nostdlib -fno-pic -fno-stack-protector \
           -mno-red-zone -mcmodel=large -Wall -O3 \
           -I./include -I$(LIBC_DIR)/include

LDFLAGS   := -n -T ../linker.ld

.PHONY: all clean

all: libg.a

$(TMP):
	mkdir -p $(TMP)

$(TMP)/%.o: %.c | $(TMP)
	@echo "  CC    $(notdir $<)"
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

libg.a: $(OBJS)
	@echo "  AR    $(notdir $@)"
	$(Q)$(AR) rcs $@ $^

clean:
	rm -rf $(TMP) libg.a
