
TARGET  := x86_64-elf
CC      := $(TARGET)-gcc
LD      := $(TARGET)-ld
AR      := $(TARGET)-ar

USERSPACE_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
LIBC_DIR       := $(USERSPACE_ROOT)/libc

CFLAGS  := -g -ffreestanding -nostdlib -fno-pic -fno-stack-protector \
           -mno-red-zone -mcmodel=large -Wall -O3 \
           -I$(LIBC_DIR)/include

ASFLAGS := -ffreestanding -mno-red-zone -mcmodel=large
