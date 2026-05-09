UNIFIED_IMAGE    := light-dev

.PHONY: image
.PHONY: all bootloader kernel run debug debug-kernel clean

# ── Unified image (prepare devcontainer) ─────────────────────────────────────

image:
	docker build -t $(UNIFIED_IMAGE) -f .devcontainer/Dockerfile .

# ── Build targets (run inside devcontainer) ──────────────────────────────────

bootloader:
	make -C bootloader -f bootloader.mk

kernel:
	make -C kernel -f kernel.mk

all: bootloader kernel

clean:
	rm -rf dist /tmp/bootloader-build /tmp/kernel-build

# ── Run targets (run outside devcontainer) ──────────────────────────────────

run:
	./run.sh

debug:
	DEBUG=1 ./run.sh

debug-kernel:
	x86_64-elf-gdb dist/kernel.elf -ex "target remote localhost:1234" -ex "continue"
