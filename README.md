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
