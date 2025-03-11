
gcc -c -m16 -ffreestanding -nostdlib -o bootloader.o bootloader.c
ld -m elf_i386 -Ttext 0x7C00 --oformat binary -o bootloader.bin bootloader.o
gcc -c -m32 -ffreestanding -nostdlib -o kernel.o kernel.c
ld -m elf_i386 -Ttext 0x10000 --oformat binary -o kernel.bin kernel.o
cat bootloader.bin kernel.bin > disk.img
dd if=/dev/zero of=disk.img bs=512 count=2880 seek=7 conv=notrunc
qemu-system-i386 -fda disk.img
