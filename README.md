# STM32H5 Basic Runtime

![Build Status](../../actions/workflows/build.yml/badge.svg)

STM32H573xx microcontroller firmware with basic peripherals.

## Features

- **MCU**: STM32H573xx (Cortex-M33)
- **Peripherals**: 
  - 2x UART (USART1, USART3)
  - RNG (Random Number Generator)
  - ICACHE (Instruction Cache)
  - GPIO

## Prerequisites

- ARM GCC toolchain (`gcc-arm-none-eabi`)
- CMake 3.22+
- Ninja build system

### Ubuntu/Debian
```bash
sudo apt-get install gcc-arm-none-eabi cmake ninja-build
```

## Build

### Debug build
```bash
cmake --preset Debug
cmake --build build/Debug
```

### Release build
```bash
cmake --preset Release
cmake --build build/Release
```

## Flash

Binary files are generated in `build/<Debug|Release>/`:
- `Cube.elf` - ELF executable with debug symbols
- `Cube.bin` - Raw binary for flashing
- `Cube.hex` - Intel HEX format

Flash using ST-LINK:
```bash
st-flash write build/Release/Cube.bin 0x8000000
```

## CI/CD

Gitea Actions automatically builds both Debug and Release configurations on every push and provides downloadable firmware artifacts.

## Hardware Configuration

- USART1: PA9 (TX), PA10 (RX)
- USART3: PB10 (TX), PB11 (RX)
- Clock: HSI (32 MHz), HSI48 (for RNG)

## License

See LICENSE.txt files in respective driver directories.
