
# Compiler Collection

TARGET  := x86_64-elf
CC      := $(TARGET)-gcc
LD      := $(TARGET)-ld
AR      := $(TARGET)-ar

# Paths

EXEC_NAME := libc

TMP := /tmp/user-build/$(EXEC_NAME)
OUT := ../../dist/user

C_SRCS := $(wildcard *.c)
OBJS   := $(C_SRCS:%.c=$(TMP)/%.o) $(TMP)/crt0.o $(TMP)/syscall.o

# Flags

Q ?= @

CFLAGS  := -g -ffreestanding -nostdlib -fno-pic -fno-stack-protector \
           -mno-red-zone -mcmodel=large -Wall -O3 \
           -I./include

ASFLAGS := -ffreestanding -mno-red-zone -mcmodel=large

LDFLAGS   := -n -T ../linker.ld

.PHONY: all clean

all: libc.a

$(TMP):
	mkdir -p $(TMP)

$(TMP)/%.o: %.c | $(TMP)
	@echo "  CC    $(notdir $<)"
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

$(TMP)/%.o: %.s | $(TMP)
	@echo "  AS    $(notdir $<)"
	$(Q)$(CC) $(ASFLAGS) -c $< -o $@

libc.a: $(OBJS)
	@echo "  AR    $(notdir $@)"
	$(Q)$(AR) rcs $@ $^

clean:
	rm -rf $(TMP) libc.a
