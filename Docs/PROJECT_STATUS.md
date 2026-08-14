# EdgeFlex AI — Project Status

## Implemented

- Dynamic Memory Manager (`EdgeFlex_Runtime/Src/edgeflex_memory.c`) — bounded static-arena, first-fit, coalescing free-list allocator. Alloc/free/reuse, bounds checking, double-free/invalid-pointer rejection, usage statistics.
- Model Loader (`EdgeFlex_Runtime/Src/edgeflex_model.c`) — compact layer/model structs, shape and weight-pointer validation. Not TFLite, no such claim made.
- Inference Engine (`EdgeFlex_Runtime/Src/edgeflex_inference.c`) — real layer-by-layer dense/relu execution with a safety-checked ping-pong tensor lifetime (no use-after-free of a released buffer).
- Performance Monitor (`EdgeFlex_Runtime/Src/edgeflex_monitor.c`) — memory stats + hardware-independent timing interface (pluggable tick source; honestly reports "not measured" when none is registered).
- Runtime Core (`EdgeFlex_Runtime/Src/edgeflex_runtime.c`) — init/load/infer/report orchestration.
- TinyMLP workload (`Models/tinymlp_model.c`) — 8→16→12→4 dense/relu network, fixed seed-42 weights, shared identically by EdgeFlex and Baseline.
- Baseline (`Baseline/Src/baseline_inference.c`) — conventional static-per-layer-buffer implementation of the same workload, for comparison.
- STM32F401CCU6 integration (`Core/Src/main.c`) — HSI clock, USART1 UART, DWT cycle-counter timing, GPIO LED, full boot-to-report demo sequence.
- Vendored, unmodified STMicroelectronics CMSIS/HAL sources (`Drivers/`) — see `THIRD_PARTY_LICENSES.md`.

## Verified

- Dynamic Memory Manager: init/alloc/free/reuse, full-pool condition, oversized allocation, invalid pointer, double free, zero-size, 350-cycle stress test with full-region writes — all under AddressSanitizer + UndefinedBehaviorSanitizer.
- Inference Engine: real deterministic output, determinism across repeated runs, exact allocation/release/reuse counts, peak-usage bound (worst adjacent-layer pair, not sum of all layers), input/output-size error paths.
- Baseline vs EdgeFlex: bit-identical output confirmed on host.
- STM32F401CCU6 cross-compilation: 0 errors, 0 warnings, ELF/HEX/BIN/MAP all generated.

## Host Tests

`Tests/host/` — native `gcc`, ASan+UBSan, run via `make test`:

- `test_memory`: 43 checks, 0 failed
- `test_inference`: 13 checks, 0 failed
- `test_baseline`: 3 checks, 0 failed
- **Total: 59/59 checks passing, 0 failures.**

## STM32 Build

`arm-none-eabi-gcc 13.2.1`, target `-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard`:

- **0 errors, 0 warnings.**
- `text=14428  data=100  bss=9204` (from `arm-none-eabi-size`)
- Flash usage: 14528 B / 256 KB = 5.68%
- Static RAM usage: 9304 B / 64 KB = 14.19%
- Artifacts generated: `build/edgeflex_ai.elf`, `.hex`, `.bin`, `.map`

## Benchmark

Host-measured (native execution, same model/weights/input for both):

| Metric | Baseline | EdgeFlex |
|---|---|---|
| Peak temporary memory | 128 bytes | 112 bytes |
| Allocations | 0 | 3 |
| Releases | 0 | 3 |
| Reuses | 0 | 1 |
| Output | `[-0.1626, 0.1678, 0.0549, -0.2775]` | identical |

Peak-temporary-memory reduction (host, measured): **12.5%**. See
`Benchmarks/comparison.md` for the full breakdown and why this number is
modest for a 3-layer model with a safety-first (no-use-after-free)
reuse policy.

## Hardware Status

**NOT TESTED — physical STM32 Black Pill required.** No board has been
connected to this environment at any point. Firmware is flash-ready.

## Not Yet Measured

- On-device inference time (ms) — DWT timing code is written and wired in, but never executed on silicon.
- On-device runtime peak SRAM (as opposed to the host-measured logical peak above).
- Power/energy (mW) — no measurement hardware available.
- SRAM-reduction and latency-improvement *percentages on hardware* — only the host-measured 12.5% figure exists right now.

## Limitations

- Small model (3 layers) chosen deliberately to keep the MVP provably correct end-to-end before the deadline; understates how much bigger the reuse advantage gets with more/larger layers.
- Baseline is host-only, not a second flashable firmware image.
- Clock fixed at HSI 16 MHz (no PLL) to avoid Black Pill HSE crystal-revision variability.
- All hardware fields are placeholders (`NOT YET MEASURED`) until the steps in `Docs/HARDWARE_TEST.md` are performed and the UART output is provided back.

## Future Scope

- Flash to real hardware, capture DWT timing + fill in `Benchmarks/results.csv`.
- Larger/deeper model to show the reuse-vs-static gap widen.
- HSE+PLL clock config for full 84 MHz.
- On-target Baseline firmware image for a true device-to-device comparison.
- Power measurement if/when hardware becomes available.
