UNIFIED_IMAGE    := light-dev

.PHONY: image
.PHONY: all bootloader kernel run debug debug-kernel clean

# ── Unified image (prepare devcontainer) ─────────────────────────────────────

image:
	docker build --platform linux/amd64 -t $(UNIFIED_IMAGE) -f .devcontainer/Dockerfile .

# ── Build targets (run inside devcontainer) ──────────────────────────────────

bootloader:
	make -C bootloader -f bootloader.mk

kernel:
	make -C kernel -f kernel.mk

all: bootloader kernel

clean:
	rm -rf dist /tmp/bootloader-build /tmp/kernel-build

# ── Run targets ──────────────────────────────────────────────────────────────

run: all
	@echo "run" > .qemu-trigger

debug:
	@echo "debug" > .qemu-trigger
	@echo "Waiting for QEMU GDB server..."
	@while ! (echo > /dev/tcp/host.docker.internal/1234) 2>/dev/null; \
	    do sleep 0.1; done
	@echo "GDB server ready"

debug-kernel:
	x86_64-elf-gdb dist/kernel.elf \
	    -ex "target remote host.docker.internal:1234" \
	    -ex "continue"
