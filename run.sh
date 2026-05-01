#!/bin/bash
set -e

qemu-system-x86_64 \
  -machine q35 \
  -m 256M \
  -drive if=pflash,format=raw,readonly=on,file=/usr/share/edk2/ovmf/OVMF_CODE.fd \
  -drive if=pflash,format=raw,file=../ovmf/OVMF_VARS.fd \
  -drive format=raw,file=fat:rw:fat \
  -net none \
  -d guest_errors \
  -serial stdio
