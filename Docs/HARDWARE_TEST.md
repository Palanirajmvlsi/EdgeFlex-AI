# EdgeFlex AI — STM32F411CEU6 Hardware Implementation

This document describes the real hardware implementation of EdgeFlex AI on the STM32F411CEU6 Black Pill.

## 1. Hardware Used

- **MCU:** STM32F411CEU6 Black Pill
- **Core:** ARM Cortex-M4 with single-precision FPU
- **Flash:** 512 KB
- **SRAM:** 128 KB
- **Programmer/Debugger:** ST-LINK V2 / compatible SWD probe
- **Serial interface:** USB-to-TTL UART adapter
- **Board power:** USB-C 5 V input through the Black Pill board regulator
- **Application LED:** PC13, active-low on the typical Black Pill board
- **UART:** USART1 at 115200 baud, 8 data bits, no parity, 1 stop bit

## 2. Hardware Architecture

```text
                    +----------------------+
                    |   STM32F411CEU6      |
                    |    ARM Cortex-M4     |
                    |                      |
 USB-C Power ------>|  3.3 V regulated     |
                    |                      |
                    |  PA9  USART1_TX ----+------> USB-TTL RX
                    |  PA10 USART1_RX <---+------ USB-TTL TX
                    |                      |
                    |  PC13 LED ----------> Status LED
                    |                      |
                    |  SWDIO <------------+------ ST-LINK SWDIO
                    |  SWCLK <------------+------ ST-LINK SWCLK
                    |  GND  <-------------+------ ST-LINK GND
                    +----------------------+
```

## 3. ST-LINK / SWD Wiring

| ST-LINK | STM32F411CEU6 |
|---|---|
| SWCLK | SWCLK / PA14 |
| SWDIO | SWDIO / PA13 |
| GND | GND |
| 3.3 V | 3V3 (only when ST-LINK is used as the board power source) |
| NRST | NRST (optional, recommended for reset/debug recovery) |

**Do not connect ST-LINK 5 V to the STM32 board.** If the board is powered from USB-C, leave ST-LINK 3.3 V disconnected and share GND.

## 4. USB-TTL UART Wiring

USART1 is implemented on PA9/PA10 in `Core/Src/main.c`.

| USB-TTL | STM32F411CEU6 |
|---|---|
| TXD | PA10 / USART1_RX |
| RXD | PA9 / USART1_TX |
| GND | GND |
| 3.3 V | Leave disconnected when board is USB-C powered |
| 5 V | **Do not connect** |

The UART adapter must use **3.3 V logic levels**.

## 5. Firmware Hardware Configuration

The hardware entry point is `Core/Src/main.c`.

At boot the firmware:

1. Initializes STM32 HAL.
2. Selects the internal 16 MHz HSI clock.
3. Configures PC13 as the status LED output.
4. Configures USART1 TX/RX on PA9/PA10 at 115200 8N1.
5. Enables the Cortex-M4 DWT cycle counter.
6. Connects DWT timing to the EdgeFlex Performance Monitor.
7. Initializes the EdgeFlex Dynamic Memory Manager.
8. Validates and loads the compiled TinyMLP model.
9. Executes layer-by-layer inference using the EdgeFlex memory pool.
10. Prints live memory, allocation, reuse, timing, and output information over UART.
11. Blinks PC13 after inference completes.

## 6. EdgeFlex Runtime on Hardware

```text
TinyMLP Model
      |
      v
+-------------+
| Model       |
| Validation  |
+-------------+
      |
      v
+-------------+
| EdgeFlex    |
| Runtime     |
+-------------+
      |
      v
+-------------+
| Dynamic     |
| Memory Pool |
+-------------+
      |
      v
+-------------+
| Inference   |
| Engine      |
+-------------+
      |
      v
+-------------+
| DWT         |
| Performance |
| Monitor     |
+-------------+
      |
      v
STM32F411CEU6
      |
      +----> UART Performance Report
      +----> PC13 Status LED
```

## 7. Build Configuration

The repository is configured for:

- `-mcpu=cortex-m4`
- `-mthumb`
- `-mfpu=fpv4-sp-d16`
- `-mfloat-abi=hard`
- `-DSTM32F411xE`
- `STM32F411CEUX_FLASH.ld`
- `startup_stm32f411xe.s`
- GNU Arm `arm-none-eabi-gcc`

Build commands:

```bash
make clean
make all
```

Expected artifacts:

```text
build/edgeflex_ai.elf
build/edgeflex_ai.hex
build/edgeflex_ai.bin
build/edgeflex_ai.map
```

## 8. Flash and Debug

Use STM32CubeIDE with the EdgeFlex project or flash the generated image with STM32CubeProgrammer/ST-LINK.

Example:

```bash
STM32_Programmer_CLI -c port=SWD -w build/edgeflex_ai.hex -v -rst
```

## 9. UART Test

Open a serial terminal on the USB-TTL adapter at:

```text
115200 baud
8 data bits
No parity
1 stop bit
No flow control
```

Then reset the Black Pill. The firmware reports:

- MCU/runtime banner
- System clock
- EdgeFlex pool size
- model name and dimensions
- peak pool usage
- current/free memory
- allocation/release/reuse counts
- allocation failures
- real DWT cycle count and inference time
- final model output

## 10. Hardware Validation Status

The repository contains the complete target-hardware implementation and wiring documentation. **Do not invent on-device benchmark values.** Record inference timing and final UART values only after running the firmware on the physical STM32F411CEU6 and copy the complete UART report into the benchmark log.

## 11. Safety / Power Notes

- Use a regulated USB-C supply for the Black Pill.
- Do not feed 5 V into STM32 GPIO pins.
- USB-TTL TX/RX must be 3.3 V logic.
- Cross UART connections: adapter TX -> MCU RX and adapter RX -> MCU TX.
- Always connect a common GND between the STM32 and USB-TTL adapter.
- Do not power the board simultaneously from conflicting external sources.
