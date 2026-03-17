# STM32H5 Basic Runtime

![Build Status](https://github.com/kriskwiatkowski/runtime_stm32h5/actions/workflows/release.yml/badge.svg?branch=main)

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
- CMake 3.25+ (for preset schema version 6)
- Ninja build system

### Ubuntu/Debian

```bash
sudo apt-get install gcc-arm-none-eabi cmake ninja-build
```

## Build

This project uses the presets defined in `CMakePresets.json`.

### Release (`stm32h5`)

```bash
cmake --preset stm32h5
cmake --build --preset stm32h5
```

### MinSizeRel (`stm32h5-minsizerel`)

```bash
cmake --preset stm32h5-minsizerel
cmake --build --preset stm32h5-minsizerel
```

### RelWithDebInfo (`stm32h5-reldebinfo`)

```bash
cmake --preset stm32h5-reldebinfo
cmake --build --preset stm32h5-reldebinfo
```

### Package (CPack presets)

```bash
cpack --preset stm32h5
cpack --preset stm32h5-minsizerel
cpack --preset stm32h5-reldebinfo
```

Preset output layout:

- Build tree: `out/build/<preset>/`
- Install tree: `out/<preset>/`
- CPack output: `out/package/<preset>/`

## Board Setup

- Board used for testing: `STM32H573I-DK`
- Connect USB to `CN10`. This enables both flashing and `UART1`.
- Set `BOOT0` to `Flash`.

### Disabling TrustZone

TrustZone must be disabled for this firmware configuration. This is a one-time option byte programming step done via STM32CubeProgrammer.

The relevant option byte is `TZEN`. It uses a complementary byte encoding:

| State              | TZEN value |
| ------------------ | ---------- |
| TrustZone enabled  | `0xB4`     |
| TrustZone disabled | `0xC3`     |

**GUI procedure:**

1. Connect to the board via SWD in **HotPlug** mode (required when TrustZone is active).
2. Go to the **OB (Option Bytes)** tab.
3. Set **RDP** to level 1 (`0xDC`) and click **Apply**.
4. Uncheck **TZEN** and click **Apply** — CubeProgrammer writes `0xC3`.
5. Reconnect and verify TZEN reads `0xC3`.

**CLI procedure:**

```bash
# Step 1: raise RDP to level 1
STM32_Programmer_CLI -c port=SWD mode=HotPlug -ob RDP=0xDC

# Step 2: disable TZEN and restore RDP to level 0
STM32_Programmer_CLI -c port=SWD mode=HotPlug -ob RDP=0xAA TZEN=0
```

### Clock Configuration

The clock can be set to `32 MHz` or `250 MHz` at init time or switched at runtime.

| Mode                       | Frequency | Source       | Flash latency |
| -------------------------- | --------- | ------------ | ------------- |
| `PLATFORM_CLOCK_USERSPACE` | 32 MHz    | HSI/2        | `LATENCY_1`   |
| `PLATFORM_CLOCK_MAX`       | 250 MHz   | HSI via PLL1 | `LATENCY_5`   |

**At initialization:**

```c
platform_init(PLATFORM_CLOCK_USERSPACE);  // 32 MHz — more precise for benchmarking
platform_init(PLATFORM_CLOCK_MAX);        // 250 MHz — maximum performance
```

**At runtime:**

```c
struct platform_attr_t attr = {
    .attr = { PLATFORM_CLOCK_MAX },
    .n = 1,
};
platform_set_attr(&attr);
```

## Flash

Artifacts are generated under `out/build/<preset>/`:

- `librtm_stm32h5.a` - runtime static library
- `app/hello` - ELF executable
- `app/hello.bin` - raw binary for flashing
- `app/hello.hex` - Intel HEX format

The build preset also installs `hello.bin` to `out/<preset>/bin/hello.bin`.

Flash using ST-LINK:

```bash
st-flash write out/stm32h5/bin/hello.bin 0x08000000
```

Flash using pyOCD:

```bash
pyocd pack install stm32h573iitx
pyocd flash --target stm32h573iitx out/stm32h5/bin/hello.bin
```

## Building a Custom Application

After building and installing the runtime (via `cmake --build --preset stm32h5`), the installed tree under `out/stm32h5/` can be used as a library by an external project.

**Project layout:**

```text
myapp/
├── CMakeLists.txt
└── main.c
```

**`CMakeLists.txt`:**

```cmake
cmake_minimum_required(VERSION 3.22)
project(myapp C ASM)

# Path to the installed runtime (adjust to your local path)
set(RTM_DIR "/path/to/runtime_stm32h5/out/stm32h5")

# Import the runtime library target
include(${RTM_DIR}/cmake/rtm_stm32h5_target.cmake)

add_executable(myapp main.c)

# --whole-archive is required so the startup/init code is not stripped
target_link_libraries(myapp PRIVATE
    -Wl,--whole-archive rtm_stm32h5::rtm_stm32h5 -Wl,--no-whole-archive)

target_link_options(myapp PRIVATE
    "-T${RTM_DIR}/STM32H573xx_FLASH.ld")
```

**`main.c`:**

```c
#include <platform/platform.h>
#include <platform/printf.h>

extern uint32_t SystemCoreClock;

int main(void) {
    platform_init(PLATFORM_CLOCK_MAX);  // 250 MHz
    printf("Hello, world! CPU: %u MHz\n", SystemCoreClock / 1000000);
    return 0;
}
```

**Build:**

```bash
cmake -B build \
    -DCMAKE_TOOLCHAIN_FILE=/path/to/runtime_stm32h5/toolchains/gcc-armv8_m.main-unknown-none-eabi.cmake \
    -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Flash `build/myapp` (ELF) or convert to binary first:

```bash
arm-none-eabi-objcopy -O binary build/myapp build/myapp.bin
```

## CI/CD

Gitea Actions builds and packages all three presets (`stm32h5`, `stm32h5-minsizerel`, `stm32h5-reldebinfo`) on every push and uploads build/package artifacts.

## Hardware Configuration

- USART1: PA9 (TX), PA10 (RX)
- USART3: PB10 (TX), PB11 (RX)
- Clock: HSI (32 MHz), HSI48 (for RNG)

## Note on CPU cycles measurement

The board must be flashed and hard-reset before each measurement to ensure
the cycle counter starts from a clean state. Using pyOCD:

```
pyocd flash app/hello.bin
pyocd reset -m hw
```

## License

See LICENSE.txt files in respective driver directories.
