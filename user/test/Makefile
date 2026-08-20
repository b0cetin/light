include ../common.mk

EXEC_NAME := $(notdir $(CURDIR))
LDFLAGS   := -n -T ../linker.ld

OUT := ../../dist/user
TMP := /tmp/user-build/$(EXEC_NAME)

C_SRCS := $(wildcard *.c)
OBJS   := $(C_SRCS:%.c=$(TMP)/%.o)

.PHONY: all clean

all: $(OUT)/$(EXEC_NAME).elf

$(TMP):
	mkdir -p $(TMP)

$(TMP)/%.o: %.c | $(TMP)
	@echo "  CC    $(notdir $<)"
	$(CC) $(CFLAGS) -c $< -o $@

$(LIBC_DIR)/libc.a:
	$(MAKE) -C $(LIBC_DIR)

$(OUT)/$(EXEC_NAME).elf: $(OBJS) $(LIBC_DIR)/libc.a
	@echo "  LD    $(notdir $@)"
	mkdir -p $(OUT)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LIBC_DIR)/libc.a
	@echo "  done: $(OUT)/$(EXEC_NAME).elf"

clean:
	rm -rf $(TMP) $(OUT)/$(EXEC_NAME).elf
