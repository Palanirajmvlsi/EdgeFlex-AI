# EdgeFlex AI — Benchmark Comparison

All numbers below came from actually running the code (host tests or the
real cross-compiler/linker). Nothing here is estimated or projected.

## Workload

Model `tinymlp_8_16_12_4`: Dense+ReLU(8→16) → Dense+ReLU(16→12) → Dense(12→4).
Fixed weights (seed=42), fixed input. Identical model/weights/input used by
both Baseline and EdgeFlex — see `Tests/host/test_baseline.c`, which asserts
their outputs are **bit-identical**: `[-0.1626, 0.1678, 0.0549, -0.2775]`.

## HOST MEASUREMENTS (native gcc, ASan/UBSan, reproducible via `make test` in `Tests/host`)

| Metric | Baseline (static buffers) | EdgeFlex (dynamic pool) |
|---|---|---|
| Peak temporary memory | 128 bytes (16+12+4 floats, all reserved at once) | 112 bytes (worst adjacent-layer pair: layer1+layer2 outputs live simultaneously) |
| Allocation calls | 0 (no allocator used) | 3 (one per layer) |
| Release calls | 0 | 3 |
| Reuse count | 0 (concept doesn't apply) | 1 (layer 3 reuses layer 1's freed block; layer 2 can't reuse anything yet — see `EdgeFlex_Runtime/Inc/edgeflex_inference.h` for why) |
| Output | `[-0.1626, 0.1678, 0.0549, -0.2775]` | `[-0.1626, 0.1678, 0.0549, -0.2775]` (identical) |

**Peak-temporary-memory reduction (host, measured): (128 − 112) / 128 × 100 = 12.5%.**

This is a real but modest number for a genuine reason: the model only has
3 layers, and a *safe* (no-use-after-free) pool can't reuse a block until
the layer that reads it has finished — so at most 2 buffers are ever
simultaneously live either way here. The gap between "reserve everything"
and "reuse safely" widens with more/larger layers; it is not exaggerated
here to make a bigger number.

## STM32 HARDWARE MEASUREMENTS

**NOT YET MEASURED.** The EdgeFlex firmware cross-compiles cleanly for
STM32F401CCU6 (0 errors, 0 warnings — see below) and is flash-ready, but
no physical board has been flashed in this environment. Baseline has not
been integrated into a separate on-target firmware image (host-only, by
design, since the comparison that matters for the pool is memory
behavior, not a second flashable binary).

| Metric | Value |
|---|---|
| Inference time (ms) | NOT YET MEASURED |
| Runtime peak SRAM (on-device) | NOT YET MEASURED |
| Power (mW) | NOT YET MEASURED |

Once flashed, DWT-cycle-counter timing and the same allocation/reuse
counters will print over UART — see `Docs/HARDWARE_TEST.md`.

## Static memory footprint (STM32F401CCU6, real linker output, EdgeFlex firmware only)

This is the compiled firmware's fixed footprint — **not** the Dynamic
Memory Manager's runtime optimization result. Conflating the two would be
misleading, so they are reported as separate metrics:

| Section | Bytes | Notes |
|---|---|---|
| `.text` + `.rodata` + `.isr_vector` (Flash) | 14428 | Flash usage = text+data = 14528 B / 256 KB (5.68%) |
| `.data` (Flash→RAM init copy) | 100 | |
| `.bss` (as reported by `size`, includes reserved stack+heap) | 9204 | RAM usage = data+bss = 9304 B / 64 KB (14.19%) |
| — of which: EdgeFlex pool (`s_pool`, static array) | 4096 | `EDGEFLEX_POOL_SIZE`, fixed at compile time |
| — of which: reserved stack (`_Min_Stack_Size`) | 4096 | Never touched by EdgeFlex logic directly |
| — of which: reserved newlib heap (`_Min_Heap_Size`) | 512 | Unused by EdgeFlex (no malloc calls); exists only for libc internals (`vsnprintf`) |
| — of which: other `.bss` (HAL structs, globals) | 500 | `huart1`, monitor/runtime static state, etc. |

Source: `build/edgeflex_ai.map` and `arm-none-eabi-size -A build/edgeflex_ai.elf`, reproducible via `make all` from the repo root.

## What is NOT claimed here

- No inference-latency percentage improvement (hardware not measured).
- No power/energy numbers (no measurement hardware available).
- The 12.5% figure is a **host-measured, logical peak-temporary-memory**
  reduction for this specific 3-layer/112-vs-128-byte workload — it is
  not a general "EdgeFlex reduces memory by 12.5%" claim, and it is not a
  substitute for on-device runtime SRAM measurement.
