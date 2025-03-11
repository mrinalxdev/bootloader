#include <stdint.h>

#define VIDEO_MEMORY 0xB8000
#define KEYBOARD_PORT 0x60
#define IDT_BASE 0x0000
#define IDT_SIZE 256

typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_high;
} idt_entry_t;

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
    static uint16_t pos = 160;
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

void delay() {
    for (int i = 0; i < 1000000; i++) {
        asm volatile ("nop");
    }
}

void draw_pattern() {
    uint16_t* vmem = (uint16_t*)VIDEO_MEMORY;
    for (int i = 0; i < 80 * 25; i++) {
        vmem[i] = (0x0A << 8) | '*';
        delay();
    }
}

void clear_screen() {
    uint16_t* vmem = (uint16_t*)VIDEO_MEMORY;
    for (int i = 0; i < 80 * 25; i++) {
        vmem[i] = (0x07 << 8) | ' ';
    }
}

void init_memory() {
    uint32_t* mem = (uint32_t*)0x100000;
    for (int i = 0; i < 1024; i++) {
        mem[i] = 0xDEADBEEF;
    }
    print_string("Memory initialized at 0x100000\n", 0x0E);
}

void setup_idt() {
    idt_entry_t* idt = (idt_entry_t*)IDT_BASE;
    for (int i = 0; i < IDT_SIZE; i++) {
        idt[i].offset_low = 0;
        idt[i].selector = 0x08;
        idt[i].zero = 0;
        idt[i].type_attr = 0x8E;
        idt[i].offset_high = 0;
    }
    print_string("Interrupt Descriptor Table setup complete\n", 0x0C);
}

void handle_keyboard() {
    uint8_t scancode = inb(KEYBOARD_PORT);
    if (scancode == 0x1E) print_char('A', 0x0F);
    if (scancode == 0x30) print_char('B', 0x0F);
    if (scancode == 0x2E) print_char('C', 0x0F);
}

void simple_shell() {
    print_string("Simple Shell> ", 0x0B);
    while (1) {
        uint8_t key = inb(KEYBOARD_PORT);
        if (key == 0x1C) {
            print_char('\n', 0x0B);
            print_string("Simple Shell> ", 0x0B);
        } else if (key < 0x80) {
            handle_keyboard();
        }
    }
}

void kernel_main() {
    clear_screen();
    print_string("Kernel booted successfully!\n", 0x0F);
    print_string("Initializing system components...\n", 0x0E);
    delay();
    print_string("Setting up memory...\n", 0x0C);
    init_memory();
    delay();
    print_string("Configuring interrupts...\n", 0x0A);
    setup_idt();
    delay();
    print_string("Drawing startup pattern...\n", 0x09);
    draw_pattern();
    delay();
    clear_screen();
    print_string("System ready!\n", 0x0A);
    print_string("Starting shell...\n", 0x0B);
    simple_shell();
    print_string("Kernel halted unexpectedly!\n", 0x04);
    while (1) {
        asm volatile ("hlt");
    }
}
