#!/bin/bash
set -e

mkdir -p fat/EFI/BOOT
mkdir -p fat/kernel

cp dist/BOOTX64.EFI fat/EFI/BOOT/BOOTX64.EFI
cp dist/kernel.elf fat/kernel/kernel.elf

QEMU_ARGS=(
  -machine q35
  -m 256M
  -drive if=pflash,format=raw,readonly=on,file=qemu/edk2-x86_64-code.fd
  -drive if=pflash,format=raw,file=qemu/ovmf-vars.fd
  -drive format=raw,file=fat:rw:fat
  -net none
  -d guest_errors
  -serial stdio
)

if [[ "$DEBUG" == "1" ]]; then
  QEMU_ARGS+=(-gdb tcp::1234 -S)
fi

qemu-system-x86_64 "${QEMU_ARGS[@]}"
