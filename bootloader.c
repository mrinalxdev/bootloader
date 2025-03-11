#include <stdint.h>

#define SECTOR_SIZE 512
#define STACK_BASE 0x9000
#define VIDEO_MEMORY 0xB8000
#define KERNEL_LOAD_ADDR 0x10000
#define DISK_READ_CMD 0x02
#define DISK_WRITE_CMD 0x03
#define MAX_ATTEMPTS 5
#define MEMORY_MAP_BASE 0x8000
#define MAX_MEMORY_ENTRIES 20

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

struct disk_params {
    uint8_t drive;
    uint8_t head;
    uint8_t sector;
    uint8_t cylinder;
    uint8_t count;
};

struct memory_entry {
    uint32_t base_low;
    uint32_t base_high;
    uint32_t length_low;
    uint32_t length_high;
    uint32_t type;
};

void outb(uint16_t port, uint8_t value) {
    asm volatile ("outb %0, %1" : : "a"(value), "d"(port));
}

uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "d"(port));
    return ret;
}

void print_char(uint8_t c, uint8_t color) {
    uint16_t* vmem = (uint16_t*)VIDEO_MEMORY;
    static uint16_t pos = 0;
    if (c == '\n') {
        pos += 80 - (pos % 80);
    } else if (pos < 80 * 25) {
        vmem[pos++] = (color << 8) | c;
    }
}

void print_string(const char* str, uint8_t color) {
    while (*str) {
        print_char(*str++, color);
    }
}

void print_hex(uint32_t value, uint8_t color) {
    char hex[] = "0123456789ABCDEF";
    print_string("0x", color);
    for (int i = 28; i >= 0; i -= 4) {
        print_char(hex[(value >> i) & 0xF], color);
    }
}

void clear_screen() {
    uint16_t* vmem = (uint16_t*)VIDEO_MEMORY;
    for (int i = 0; i < 80 * 25; i++) {
        vmem[i] = (0x07 << 8) | ' ';
    }
}

void wait_disk() {
    while (inb(0x1F7) & 0x80);
}

void reset_disk(uint8_t drive) {
    outb(0x1F6, drive);
    outb(0x1F2, 0);
    outb(0x1F3, 0);
    outb(0x1F4, 0);
    outb(0x1F5, 0);
    outb(0x1F7, 0x04);
    wait_disk();
}

int detect_disk(uint8_t drive) {
    outb(0x1F6, 0xA0 | (drive << 4));
    outb(0x1F7, 0xEC);
    wait_disk();
    if (inb(0x1F7) == 0) return 0;
    return 1;
}

int read_sector(uint8_t drive, uint32_t lba, uint8_t* buffer) {
    uint8_t sector = (lba % 18) + 1;
    uint8_t cylinder = (lba / 18) / 36;
    uint8_t head = (lba / 18) % 2;
    uint8_t attempts = 0;

    while (attempts < MAX_ATTEMPTS) {
        wait_disk();
        outb(0x1F6, 0xA0 | (drive << 4) | head);
        outb(0x1F2, 1);
        outb(0x1F3, sector);
        outb(0x1F4, cylinder & 0xFF);
        outb(0x1F5, (cylinder >> 8) & 0xFF);
        outb(0x1F7, DISK_READ_CMD);

        wait_disk();
        if (!(inb(0x1F7) & 0x01)) {
            for (int i = 0; i < SECTOR_SIZE / 2; i++) {
                uint16_t data = inb(0x1F0);
                buffer[i * 2] = data & 0xFF;
                buffer[i * 2 + 1] = (data >> 8) & 0xFF;
            }
            return 1;
        }
        reset_disk(drive);
        attempts++;
    }
    return 0;
}

int write_sector(uint8_t drive, uint32_t lba, uint8_t* buffer) {
    uint8_t sector = (lba % 18) + 1;
    uint8_t cylinder = (lba / 18) / 36;
    uint8_t head = (lba / 18) % 2;
    uint8_t attempts = 0;

    while (attempts < MAX_ATTEMPTS) {
        wait_disk();
        outb(0x1F6, 0xA0 | (drive << 4) | head);
        outb(0x1F2, 1);
        outb(0x1F3, sector);
        outb(0x1F4, cylinder & 0xFF);
        outb(0x1F5, (cylinder >> 8) & 0xFF);
        outb(0x1F7, DISK_WRITE_CMD);

        wait_disk();
        if (!(inb(0x1F7) & 0x01)) {
            for (int i = 0; i < SECTOR_SIZE / 2; i++) {
                uint16_t data = (buffer[i * 2 + 1] << 8) | buffer[i * 2];
                outb(0x1F0, data);
            }
            return 1;
        }
        reset_disk(drive);
        attempts++;
    }
    return 0;
}

void load_kernel(uint8_t drive) {
    uint8_t* kernel_buffer = (uint8_t*)KERNEL_LOAD_ADDR;
    int sectors_to_read = 6;
    int lba = 1;

    print_string("Detecting disk parameters...\n", 0x07);
    if (!detect_disk(drive)) {
        print_string("Disk detection failed!\n", 0x04);
        while (1);
    }
    print_string("Loading kernel into memory...\n", 0x07);
    for (int i = 0; i < sectors_to_read; i++) {
        if (!read_sector(drive, lba + i, kernel_buffer + (i * SECTOR_SIZE))) {
            print_string("Failed to load kernel sector ", 0x04);
            print_char('0' + i, 0x04);
            print_string("\n", 0x04);
            while (1);
        }
    }
    print_string("Kernel successfully loaded at ", 0x02);
    print_hex(KERNEL_LOAD_ADDR, 0x02);
    print_string("\n", 0x02);
}

void build_memory_map() {
    struct memory_entry* mmap = (struct memory_entry*)MEMORY_MAP_BASE;
    uint32_t count = 0;
    print_string("Building memory map...\n", 0x07);
    asm volatile (
        "mov $0xE801, %ax\n"
        "int $0x15\n"
        "mov %ax, %bx\n"
        "mov %cx, %dx\n"
    );
    mmap[count].base_low = 0;
    mmap[count].base_high = 0;
    mmap[count].length_low = 0xA0000;
    mmap[count].length_high = 0;
    mmap[count].type = 1;
    count++;
    mmap[count].base_low = 0x100000;
    mmap[count].base_high = 0;
    mmap[count].length_low = 0x1000000;
    mmap[count].length_high = 0;
    mmap[count].type = 1;
    count++;
    print_string("Memory map built with ", 0x07);
    print_char('0' + count, 0x07);
    print_string(" entries\n", 0x07);
}

void set_stack() {
    asm volatile (
        "mov $0x9000, %esp\n"
        "mov $0x9000, %ebp\n"
    );
}

void jump_to_kernel() {
    void (*kernel_entry)(void) = (void (*)())KERNEL_LOAD_ADDR;
    kernel_entry();
}

void boot_main() {
    clear_screen();
    print_string("Bootloader initializing...\n", 0x07);
    print_string("Setting up stack...\n", 0x07);
    set_stack();
    print_string("Checking hardware compatibility...\n", 0x07);
    if (!detect_disk(0x80)) {
        print_string("No compatible disk found!\n", 0x04);
        while (1);
    }
    load_kernel(0x80);
    build_memory_map();
    print_string("Preparing to jump to kernel...\n", 0x07);
    jump_to_kernel();
    print_string("Bootloader failed to jump!\n", 0x04);
    while (1);
}

__attribute__((section(".boot")))
void boot_entry() {
    asm volatile (
        "cli\n"
        "mov $0x07C0, %ax\n"
        "mov %ax, %ds\n"
        "mov %ax, %es\n"
        "mov %ax, %fs\n"
        "mov %ax, %gs\n"
        "mov $0x9000, %sp\n"
        "call boot_main\n"
    );
}

__attribute__((section(".signature")))
unsigned short boot_signature = 0xAA55;
