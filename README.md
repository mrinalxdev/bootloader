# Building Bootloader Simulation

## Project Overview

### Bootloader (`bootloader.c`)
- **Disk Detection**: Added `detect_disk()` to verify disk presence.
- **Write Sector**: Added `write_sector()` for potential future use.
- **Memory Map**: Added `build_memory_map()` to detect memory regions using BIOS calls.
- **Hex Printing**: Added `print_hex()` for better debugging output.
- **Verbose Output**: More status messages for each step.

### Kernel (`kernel.c`)
- **Interrupt Setup**: Added `setup_idt()` to initialize a basic Interrupt Descriptor Table.
- **Memory Initialization**: Added `init_memory()` to simulate memory management.
- **Simple Shell**: Added `simple_shell()` and `handle_keyboard()` for basic user interaction (responds to A, B, C keys).
- **Hex Printing**: Added `print_hex()` for consistency with bootloader.
- **More Functionality**: Expanded startup sequence with delays and additional steps.

Linux (Recommended)

Install dependencies:
bashCopysudo apt-get update
sudo apt-get install gcc binutils make qemu-system-i386

Create build script (build.sh):
bashCopy#!/bin/bash

# Compile bootloader and kernel
gcc -m32 -ffreestanding -c -o bootloader.o bootloader.c
gcc -m32 -ffreestanding -c -o kernel.o kernel.c

# Link bootloader and kernel
ld -m elf_i386 -Ttext 0x7C00 -o bootloader.elf bootloader.o
ld -m elf_i386 -Ttext 0x10000 -o kernel.elf kernel.o

# Extract binary files
objcopy -O binary bootloader.elf bootloader.bin
objcopy -O binary kernel.elf kernel.bin

# Create floppy disk image
dd if=/dev/zero of=floppy.img bs=512 count=2880
dd if=bootloader.bin of=floppy.img conv=notrunc
dd if=kernel.bin of=floppy.img seek=1 conv=notrunc

# Run in QEMU
qemu-system-i386 -fda floppy.img

Make build script executable:
bashCopychmod +x build.sh
