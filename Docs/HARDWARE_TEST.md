# Hardware Test Instructions

**Physical hardware testing has not yet been performed in this environment.**
The firmware cross-compiles cleanly (0 errors, 0 warnings) and is
flash-ready, but every "on-device" number in this project is currently
`NOT YET MEASURED` until you complete the steps below and send back the
UART output.

## 1. Required hardware

- STM32 Black Pill (STM32F401CCU6)
- ST-Link V2 (or compatible) debug probe
- USB-to-UART adapter (e.g. FTDI/CP2102) — the Black Pill has no onboard USB-serial
- 4 jumper wires (SWD) + 3 jumper wires (UART) + USB cables

## 2. ST-Link connection (SWD)

| ST-Link pin | Black Pill pin |
|---|---|
| SWCLK | SWCLK |
| SWDIO | SWDIO |
| GND | GND |
| 3.3V | 3V3 (optional, if not USB-powering the board separately) |

## 3. UART connection (USB-to-UART adapter)

| Adapter pin | Black Pill pin |
|---|---|
| TX | PA10 (USART1 RX) |
| RX | PA9 (USART1 TX) |
| GND | GND |

**UART configuration: 115200 baud, 8 data bits, no parity, 1 stop bit (115200 8N1), no flow control.**

## 4. STM32CubeIDE setup

1. Open STM32CubeIDE.
2. File -> Import -> C/C++ -> Existing Code as Makefile Project.
3. Point it at the root of this repository (the folder containing `Makefile` and `STM32F401CCUX_FLASH.ld`).
4. Toolchain: "Cross GCC" (arm-none-eabi-gcc must be on your PATH, or set the toolchain path in project properties).

## 5. Build

Either use CubeIDE's Build button, or from a terminal in the project root:

```bash
make clean
make all
```

Confirm the output ends with `0 errors` and check `build/edgeflex_ai.map`
exists alongside the `.elf`/`.hex`/`.bin`.

## 6. Flash

Option A — command line:
```bash
st-flash write build/edgeflex_ai.bin 0x08000000
```

Option B — STM32CubeProgrammer:
```bash
STM32_Programmer_CLI -c port=SWD -w build/edgeflex_ai.hex -v -rst
```

Option C — STM32CubeIDE's Run/Debug button, using the imported Makefile project's `.elf`.

## 7. Open a serial terminal

Any terminal works (PuTTY, minicom, `screen`, CubeIDE's own terminal view).
Connect at **115200 8N1** to the COM port / `/dev/ttyUSB*` your UART
adapter enumerates as.

## 8. Reset

Press the Black Pill's NRST button (or power-cycle it) after the terminal
is open and connected, so you capture output from the very first line.

## 9. Expected output

```
=========================================
     EdgeFlex AI Runtime - STM32F401CCU6
=========================================
SystemCoreClock: 16000000 Hz
Pool size:       4096 bytes
[INIT] Dynamic Memory Manager initialized.
[LOAD] Model: tinymlp_8_16_12_4 | Layers: 3 | In: 8 | Out: 4
[RUN ] Executing layer-by-layer inference...
-----------------------------------------
EdgeFlex AI Runtime - Performance Report
-----------------------------------------
Model:            tinymlp_8_16_12_4
Layers:           3
Peak SRAM (pool): 112 bytes
Current usage:    0 bytes
Free bytes:       4072 bytes
Allocations:      3
Releases:         3
Reuses:           1
Alloc failures:   0
Inference time:   <real ms> ms (<real cycles> cycles)
Final Output:     [<real>, <real>, <real>, <real>]
-----------------------------------------
[DONE] Copy everything above and send it back for the benchmark log.
```

The onboard LED (PC13) will blink once the report has printed, indicating
the board is alive and looping.

## 10. Values to record

Copy the **entire block** between the `====` banner and `[DONE]` line,
especially:

- `Inference time:` line (the real DWT-measured ms/cycles)
- `Peak SRAM (pool):`, `Allocations:`, `Releases:`, `Reuses:`
- `Final Output:` values

## 11. How to send the result back

Paste the full copied UART output as-is (don't retype or round any
numbers) back into the conversation. It will be transcribed directly into
`Benchmarks/results.csv` and `Benchmarks/comparison.md` in place of every
`NOT YET MEASURED` field, with no numbers estimated or altered.
