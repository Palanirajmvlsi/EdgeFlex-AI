# EdgeFlex AI

**Lightweight Edge-AI Runtime Framework for ARM Cortex-M Microcontrollers**

EdgeFlex AI is a modular, embedded C runtime for running a compact neural-network workload on resource-constrained ARM Cortex-M microcontrollers. Its main innovation is a bounded Dynamic Memory Manager that lets inference layers share a reusable SRAM pool instead of permanently reserving a separate workspace for every layer.

## Target Hardware

- **Board:** STM32F411CEU6 Black Pill
- **CPU:** ARM Cortex-M4
- **Clock:** 16 MHz HSI in the current hardware demo
- **Flash:** 512 KB
- **SRAM:** 128 KB
- **Programming/debug:** ST-LINK V2 over SWD
- **Serial output:** USB-to-TTL UART through USART1
- **UART pins:** PA9 = TX, PA10 = RX
- **UART format:** 115200 8N1
- **Status LED:** PC13

## Hardware Implementation

```text
                 USB-C POWER
                      |
                      v
             +-------------------+
             | STM32F411CEU6     |
             |   ARM Cortex-M4   |
             |                   |
ST-LINK ---->| SWDIO / SWCLK     |
             |                   |
USB-TTL ---->| PA10 RX           |
USB-TTL <----| PA9 TX            |
             |                   |
             | PC13 STATUS LED   |
             +-------------------+
```

### SWD wiring

| ST-LINK V2 | STM32F411CEU6 |
|---|---|
| SWCLK | PA14 / SWCLK |
| SWDIO | PA13 / SWDIO |
| GND | GND |
| NRST | NRST (optional) |
| 3.3V | 3V3 only if ST-LINK powers the board |

### UART wiring

| USB-TTL | STM32F411CEU6 |
|---|---|
| TXD | PA10 / USART1_RX |
| RXD | PA9 / USART1_TX |
| GND | GND |
| 3.3V | Leave disconnected when USB-C powers the board |
| 5V | **Do not connect** |

Use a USB-TTL adapter with **3.3 V logic levels**. Do not connect 5 V to STM32 GPIO pins.

Complete wiring, flashing, UART and validation steps are in [`Docs/HARDWARE_TEST.md`](Docs/HARDWARE_TEST.md).

## Runtime Architecture

```text
User Application
       |
       v
+-------------------+
| EdgeFlex Runtime  |
+-------------------+
       |
       +--> Model Loader
       |
       +--> Dynamic Memory Manager
       |
       +--> Inference Engine
       |
       +--> Performance Monitor
       |
       v
 STM32F411CEU6 / Cortex-M4
       |
       +--> UART performance report
       +--> PC13 status LED
```

## Core Innovation

EdgeFlex uses a **bounded first-fit coalescing allocator over a static SRAM arena**. It avoids the libc heap for model workspace and provides explicit allocation/free validation, bounds checking, reuse statistics and peak-usage tracking.

Inference maintains safe tensor lifetimes: a tensor is not released until the layer consuming it has finished. This allows valid memory reuse without corrupting active tensors.

## TinyMLP Demonstration Model

The repository includes a deterministic compact model used to exercise the runtime:

```text
Input(8)
   -> Dense + ReLU (8 -> 16)
   -> Dense + ReLU (16 -> 12)
   -> Dense (12 -> 4)
   -> Output(4)
```

This model is a runtime demonstration workload, not a claim of predictive accuracy on a real-world dataset.

## Repository Structure

```text
EdgeFlex-AI/
├── Baseline/              # Conventional static-buffer comparison
├── Benchmarks/            # Benchmark methodology and results
├── Core/                  # STM32 hardware entry point and HAL integration
├── Docs/                  # Hardware and project documentation
├── Drivers/               # CMSIS and STM32 HAL sources
├── EdgeFlex_Runtime/      # Runtime, loader, memory, inference, monitor
├── Models/                # TinyMLP model and sample input
├── Tests/                 # Host-side unit tests
├── Makefile               # GNU Arm build configuration
├── STM32F411CEUX_FLASH.ld # STM32F411CEU6 linker configuration
└── LICENSE
```

## Hardware Firmware Flow

`Core/Src/main.c` performs the real board integration:

1. Initialize STM32 HAL.
2. Configure the internal 16 MHz HSI clock.
3. Configure PC13 status LED.
4. Configure USART1 on PA9/PA10 at 115200 8N1.
5. Enable the Cortex-M4 DWT cycle counter.
6. Provide DWT timing to the EdgeFlex Performance Monitor.
7. Initialize the Dynamic Memory Manager.
8. Validate the TinyMLP model.
9. Execute layer-by-layer inference through EdgeFlex.
10. Print live memory, allocation, reuse, timing and output information over UART.
11. Blink PC13 after the inference report.

## Build

The project uses GNU Arm Embedded GCC and is configured for STM32F411xE:

```bash
make clean
make all
```

The build produces:

```text
build/edgeflex_ai.elf
build/edgeflex_ai.hex
build/edgeflex_ai.bin
build/edgeflex_ai.map
```

Build configuration:

```text
CPU:       Cortex-M4
FPU:       FPv4-SP-D16
ABI:       hard-float
Device:    STM32F411xE
Linker:    STM32F411CEUX_FLASH.ld
Startup:   startup_stm32f411xe.s
Compiler:  arm-none-eabi-gcc
```

## Flashing

Using STM32CubeProgrammer:

```bash
STM32_Programmer_CLI -c port=SWD -w build/edgeflex_ai.hex -v -rst
```

Using ST-LINK from a compatible command-line environment:

```bash
st-flash write build/edgeflex_ai.bin 0x08000000
```

STM32CubeIDE can also import the repository as an existing Makefile project.

## UART Output

Open a serial terminal at:

```text
115200 baud
8 data bits
No parity
1 stop bit
No flow control
```

After reset, the firmware reports the model, pool size, peak SRAM usage, allocation/release/reuse counts, allocation failures, DWT-measured inference time and final output.

**On-device timing values must be taken from the physical STM32F411CEU6 UART output. They are not hardcoded or estimated.**

## Memory Benchmark

The host-side benchmark compares the same model, weights and input using a conventional static-buffer baseline and EdgeFlex's dynamic memory strategy. The repository records the measured host result and test methodology in `Benchmarks/comparison.md`.

The current 3-layer host workload reports **12.5% peak temporary-memory reduction** (128 bytes baseline vs 112 bytes EdgeFlex). This is a workload-specific measured result and should not be presented as a universal hardware improvement.

On-device inference time and power are recorded only after physical hardware measurement.

## Tests

The runtime modules are designed to compile independently from the STM32 HAL and can be tested on a host PC.

```bash
cd Tests/host
make test
```

The memory tests cover allocation, release, reuse, pool exhaustion, oversized requests, invalid pointers, double-free protection and repeated stress cycles. Inference tests validate model execution, determinism, allocation/release/reuse counts and output correctness.

## Research Position

Existing TinyML systems commonly optimize models, kernels and deployment toolchains. EdgeFlex explores an additional runtime-level approach: **bounded memory management and safe workspace reuse during layer-by-layer inference on Cortex-M devices**.

## License

EdgeFlex AI is released under the license included in this repository. Third-party ST/CMSIS components retain their respective upstream licensing terms documented under `Docs/` and `THIRD_PARTY_LICENSES.md`.
