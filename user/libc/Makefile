include ../common.mk

C_SRCS := $(wildcard *.c)
TMP    := /tmp/user-build/libc
OBJS   := $(C_SRCS:%.c=$(TMP)/%.o) $(TMP)/crt0.o

.PHONY: all clean

all: libc.a

$(TMP):
	mkdir -p $(TMP)

$(TMP)/%.o: %.c | $(TMP)
	$(CC) $(CFLAGS) -c $< -o $@

$(TMP)/%.o: %.s | $(TMP)
	$(CC) $(ASFLAGS) -c $< -o $@

libc.a: $(OBJS)
	$(AR) rcs $@ $^

clean:
	rm -rf $(TMP) libc.a
