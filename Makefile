.PHONY: run clean

run:
	$(MAKE) -C bootloader
	$(MAKE) -C kernel
	./run.sh

build:
	$(MAKE) -C bootloader
	$(MAKE) -C kernel

clean:
	$(MAKE) -C bootloader clean
	$(MAKE) -C kernel clean
