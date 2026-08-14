# EdgeFlex AI

Lightweight AI runtime for ARM Cortex-M, built around a **Dynamic Memory
Manager** that lets neural-network layers share one bounded SRAM pool
instead of each reserving its own buffer for the whole program's lifetime.

Target: **STM32F401CCU6 (STM32 Black Pill), ARM Cortex-M4**, built with
STM32 HAL + `arm-none-eabi-gcc`.

## Overview

```
User Application
   |
EdgeFlex AI Runtime
   +-- Runtime Core        (EdgeFlex_Runtime/Src/edgeflex_runtime.c)
   +-- Model Loader        (EdgeFlex_Runtime/Src/edgeflex_model.c)
   +-- Dynamic Memory Mgr  (EdgeFlex_Runtime/Src/edgeflex_memory.c)
   +-- Inference Engine    (EdgeFlex_Runtime/Src/edgeflex_inference.c)
   +-- Performance Monitor (EdgeFlex_Runtime/Src/edgeflex_monitor.c)
   |
STM32 HAL -> ARM Cortex-M4 -> STM32 Black Pill
```

## Problem

Cortex-M devices like the STM32F401CCU6 have 64 KB of SRAM. A common
pattern for embedded NN inference is to give every layer its own
permanently-reserved workspace buffer, sized for the worst case, for the
entire run — even though only one (or two, for a producer/consumer pair)
layer is actually active at any instant.

## Solution

EdgeFlex AI's Dynamic Memory Manager hands out temporary workspace from a
single fixed-size pool, on demand, per layer:

```
Layer 1: ALLOCATE -> EXECUTE -> (held as Layer 2's input)
Layer 2: ALLOCATE -> EXECUTE -> RELEASE Layer 1's now-consumed buffer
Layer 3: ALLOCATE -> REUSES Layer 1's freed block -> EXECUTE -> RELEASE Layer 2's buffer
...
```

Memory lifetime follows the *active computation*, not a static
worst-case reservation of every layer at once.

## Core Innovation

A **bounded, first-fit, coalescing free-list allocator over a static
arena** (`EdgeFlex_Runtime/Src/edgeflex_memory.c`) — no libc heap, no
`malloc`/`free`, every allocation and release bounds-checked, invalid
pointers and double-frees explicitly rejected rather than causing
corruption — combined with an **inference engine that never releases a
tensor before the layer consuming it has finished with it** (a safe
ping-pong lifetime, not naive "free as soon as the producing layer ends").
See `EdgeFlex_Runtime/Inc/edgeflex_inference.h` for the exact allocation
pattern this produces and why layer-2 specifically *cannot* reuse
layer-1's memory (a real, verified consequence of doing this safely).

## Architecture

| Module | File | Responsibility |
|---|---|---|
| Dynamic Memory Manager | `EdgeFlex_Runtime/Src/edgeflex_memory.c` | Bounded pool: alloc/free/reuse, bounds checking, stats |
| Model Loader | `EdgeFlex_Runtime/Src/edgeflex_model.c` | Compact layer/model structs + shape/weight validation (not TFLite) |
| Inference Engine | `EdgeFlex_Runtime/Src/edgeflex_inference.c` | Layer-by-layer dense/relu execution, safe tensor lifetime |
| Performance Monitor | `EdgeFlex_Runtime/Src/edgeflex_monitor.c` | Memory stats + hardware-independent timing interface |
| Runtime Core | `EdgeFlex_Runtime/Src/edgeflex_runtime.c` | Thin init -> load -> infer -> report orchestration |
| Baseline | `Baseline/Src/baseline_inference.c` | Conventional static-per-layer-buffer implementation, same workload, for comparison |

All modules above are plain, portable C11 with zero HAL dependency, so
they compile and unit-test on a host PC (`Tests/host/`) as well as
cross-compile for the real target. Only `Core/Src/main.c` touches HAL/UART/DWT.

## Dynamic Memory Manager

- Fixed static arena, `EDGEFLEX_POOL_SIZE` = 4096 bytes by default (override with `-DEDGEFLEX_POOL_SIZE=<bytes>`).
- First-fit search over a doubly-linked free list; splits blocks that are larger than needed, coalesces adjacent free blocks on release.
- Every `alloc()` bounds-checks the returned region against the pool's actual extents.
- Every `free()` validates the pointer is a live, in-pool, previously-allocated block before touching it — rejects `NULL`, foreign pointers, and double-frees with explicit error codes (`EDGEFLEX_MEM_ERR_INVALID_PTR`, `EDGEFLEX_MEM_ERR_DOUBLE_FREE`) instead of corrupting state.
- Tracks: `allocation_count`, `release_count`, `reuse_count` (allocations satisfied by a block that had been used-and-freed before), `peak_usage_bytes`, `current_usage_bytes`, `free_bytes`, `failure_count`.
- Verified under AddressSanitizer + UndefinedBehaviorSanitizer — see `Tests/host/test_memory.c` (43 checks: init state, basic alloc/free, layer-by-layer reuse, full-pool condition, oversized allocation, invalid pointer, double free, zero-size, and a 350-cycle stress test that writes every returned byte).

## TinyMLP Model

`Models/tinymlp_model.c` — a small, fully-connected network used to
exercise the runtime, **not a claim of predictive accuracy on any real
task** (weights are fixed, seed=42, not trained on data):

```
Input(8) -> Dense+ReLU(8->16) -> Dense+ReLU(16->12) -> Dense(12->4) -> Output(4)
```

Represented as plain C structs (`edgeflex_model_t` / `edgeflex_layer_t`,
`EdgeFlex_Runtime/Inc/edgeflex_model.h`) — this is a compact, compiled-in
representation, explicitly **not** a TensorFlow Lite parser and makes no
TFLite-compatibility claim.

## Inference Workflow

`EdgeFlex_Runtime/Src/edgeflex_inference.c` keeps at most two live pool
allocations at a time (current input, current output). For N layers this
produces N allocations, N releases, and **N-2 reuses** (reuse can only
start once a block has actually been freed, and a block can't be freed
until the layer reading it has finished — see the header comment for the
full walkthrough). Verified in `Tests/host/test_inference.c` (13 checks:
model validation, real deterministic output, determinism across runs,
exact allocation/release/reuse counts, peak-usage bound, error paths).

## STM32F401CCU6

- MCU: STM32F401CCU6, ARM Cortex-M4, 256 KB Flash, 64 KB SRAM
- Board: STM32 Black Pill (WeAct)
- Clock: HSI 16 MHz, no PLL (deliberate — Black Pill HSE crystal population varies by revision; avoiding HSE removes that hardware variable from this MVP. Switching to HSE+PLL for up to 84 MHz is future work, not required for the demo)
- UART: USART1, PA9 (TX) / PA10 (RX), 115200 8N1
- Timing: Cortex-M4 DWT cycle counter (`CYCCNT`), wired into the Performance Monitor's hardware-independent time-source interface (`edgeflex_monitor_set_time_source()`) — see `Core/Src/main.c`
- CMSIS/HAL: official, unmodified STMicroelectronics sources — see `THIRD_PARTY_LICENSES.md`

## Build Instructions

Plain Makefile + `arm-none-eabi-gcc`, chosen for CI/host reproducibility
without requiring STM32CubeIDE itself:

```bash
# Toolchain (Ubuntu/Debian):
apt-get install gcc-arm-none-eabi

# From the repo root:
make clean
make all
```

Produces `build/edgeflex_ai.{elf,hex,bin,map}`. To import into
STM32CubeIDE instead: File -> Import -> C/C++ -> Existing Code as
Makefile Project, point it at this folder, keep the existing Makefile.

Host unit tests (no MCU/toolchain needed, just native `gcc`):

```bash
cd Tests/host
make test
```

## Flash Instructions

With an ST-Link (or the Black Pill's DFU bootloader):

```bash
# ST-Link (via st-flash):
st-flash write build/edgeflex_ai.bin 0x08000000

# or via STM32CubeProgrammer CLI:
STM32_Programmer_CLI -c port=SWD -w build/edgeflex_ai.hex -v -rst
```

Full step-by-step (including STM32CubeIDE's own flash button) is in
`Docs/HARDWARE_TEST.md`.

## UART Output

USART1, **115200 8N1**. Expected output on reset (real values are read at
runtime, not hardcoded):

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
Inference time:   <real DWT-measured ms> ms (<cycles> cycles)
Final Output:     [<real>, <real>, <real>, <real>]
-----------------------------------------
[DONE] Copy everything above and send it back for the benchmark log.
```

## Benchmark Method

Baseline (`Baseline/Src/baseline_inference.c`, static per-layer buffers,
no allocator) and EdgeFlex run the **same** compiled-in model, weights,
and input, on the same host build, so the comparison isolates the memory
strategy. `Tests/host/test_baseline.c` asserts their outputs are
bit-identical before any memory numbers are trusted. Full method and
results: `Benchmarks/comparison.md`.

## Baseline vs EdgeFlex

| Metric | Baseline | EdgeFlex | Measured where |
|---|---|---|---|
| Peak temporary memory | 128 bytes | 112 bytes | Host, `Tests/host/` |
| Allocation calls | 0 | 3 | Host |
| Reuse count | 0 (n/a) | 1 | Host |
| Output | identical | identical | Host (bit-exact match verified) |
| On-device inference time | — | **NOT YET MEASURED** | Requires flashed hardware |
| On-device power | — | **NOT YET MEASURED** | No power-measurement hardware available |

**Peak-temporary-memory reduction (host, measured): (128−112)/128 × 100 = 12.5%.**
This is intentionally reported as a real, modest number for this specific
3-layer workload rather than a rounded-up headline figure — see
`Benchmarks/comparison.md` for why it isn't larger (a *safe* pool can't
reuse a block until the layer reading it is done, so layer 2 gets no
reuse at all here).

**Static linker RAM usage (9304 B, see below) is a separate metric from
the numbers above and is NOT the Dynamic Memory Manager's optimization
result** — it's the compiled firmware's fixed footprint, most of which is
the pool's own fixed 4096-byte arena plus reserved stack/heap, not a
"before vs after" comparison.

## Verified Results

- **Host tests:** 59/59 checks passing, 0 failures (`test_memory` 43, `test_inference` 13, `test_baseline` 3), under AddressSanitizer + UndefinedBehaviorSanitizer.
- **STM32F401CCU6 cross-compile:** 0 errors, 0 warnings. `text=14428 data=100 bss=9204` (Flash 14528 B / 256 KB = 5.68%, static RAM 9304 B / 64 KB = 14.19%). Source: `build/edgeflex_ai.map`.
- **Baseline == EdgeFlex output:** verified bit-identical on host.
- **Hardware:** NOT TESTED — no physical STM32 Black Pill in this environment. Firmware is flash-ready (`build/edgeflex_ai.{elf,hex,bin}`).

## Limitations

- No physical hardware has been flashed; all "hardware measurements" fields in `Benchmarks/` are explicitly marked `NOT YET MEASURED`, not estimated.
- The TinyMLP workload is small (3 layers) specifically to keep the MVP provable end-to-end before the deadline; the reuse/peak-memory gap vs. Baseline would be larger with more/bigger layers.
- Baseline only runs on host; it isn't a second flashable on-target firmware image (the comparison that matters here is memory behavior, which is fully captured on host).
- Clock is fixed at HSI 16 MHz (no PLL) — a deliberate simplification, not a claim about achievable performance at 84 MHz.
- No power-measurement hardware available; power fields are left blank/`NOT MEASURED`, never invented.

## Future Scope

- Flash to real hardware, capture DWT-measured inference time and power (if a power probe becomes available), fill in the `NOT YET MEASURED` fields in `Benchmarks/results.csv` with real numbers.
- Larger/deeper models to show the reuse-vs-static gap widen.
- HSE+PLL clock config for full 84 MHz operation.
- Optional on-target Baseline firmware image for a true device-to-device comparison (currently host-only by design).

## License

EdgeFlex AI's own code is MIT-licensed — see `LICENSE`. Vendored
STMicroelectronics CMSIS/HAL sources under `Drivers/` retain their
original Apache-2.0 / BSD-3-Clause licenses — see `THIRD_PARTY_LICENSES.md`.
