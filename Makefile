.PHONY: image efi run clean

image:
	docker build -t bootloader bootloader/

efi:
	docker run --rm \
	    -v $(shell pwd)/bootloader:/src \
	    -v $(shell pwd)/dist:/out \
	    bootloader

efi-clean:
	docker run --rm \
	    -v $(shell pwd)/bootloader:/src \
	    -v $(shell pwd)/dist:/out \
	    bootloader clean

clean: efi-clean
	$(MAKE) -C kernel clean

build: efi
	$(MAKE) -C kernel

run: build
	./run.sh
