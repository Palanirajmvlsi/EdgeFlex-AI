/**
 ******************************************************************************
 * @file    edgeflex_memory.h
 * @brief   EdgeFlex AI - Dynamic Memory Manager
 *
 * Core novelty of EdgeFlex AI. Provides a deterministic, bounded memory pool
 * for layer-by-layer neural-network inference on ARM Cortex-M devices.
 *
 * Instead of reserving a separate static buffer for every layer's workspace
 * simultaneously, EdgeFlex hands out temporary regions from ONE fixed pool.
 * A layer requests memory right before it executes and releases it right
 * after, so the next layer can reuse the same physical SRAM.
 *
 * Design goals (see project spec, "STM32 SAFETY RULES"):
 *   - No calls to malloc/free/libc heap. The pool is a single static array.
 *   - Every allocation is bounds-checked against the pool extents.
 *   - Every free is validated (must be a live, in-pool allocation) before
 *     the block is touched, to guard against invalid-free / double-free.
 *   - Allocation failure is reported explicitly (EDGEFLEX_MEM_ERR_NO_SPACE),
 *     never silently ignored.
 *   - No dynamic behaviour depends on HAL/MCU headers, so this module is
 *     100% portable and can be unit-tested on a host PC as well as
 *     cross-compiled for the STM32F401 target (see Tests/host and Makefile).
 *
 * Allocator strategy: first-fit free list over a fixed static arena, with
 * immediate coalescing of adjacent free blocks on release. This is O(n) in
 * the number of live blocks, n is small (single-digit layers), and it is
 * simple enough to reason about exhaustively on a Cortex-M4 -> deterministic
 * behaviour is preferred here over throughput.
 ******************************************************************************
 */
#ifndef EDGEFLEX_MEMORY_H
#define EDGEFLEX_MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* ------------------------------------------------------------------------ */
/* Configuration                                                             */
/* ------------------------------------------------------------------------ */

/**
 * Total size in bytes of the EdgeFlex memory pool.
 *
 * STM32F401CCU6 has 64 KB of SRAM total. This pool is sized to leave ample
 * headroom for the stack, globals, and HAL/UART buffers on the real target.
 * Override at build time with -DEDGEFLEX_POOL_SIZE=<bytes> if needed.
 */
#ifndef EDGEFLEX_POOL_SIZE
#define EDGEFLEX_POOL_SIZE (4096u)
#endif

/** Alignment (bytes) guaranteed for every pointer returned by alloc(). */
#define EDGEFLEX_MEM_ALIGNMENT (4u)

/* ------------------------------------------------------------------------ */
/* Types                                                                     */
/* ------------------------------------------------------------------------ */

typedef enum {
    EDGEFLEX_MEM_OK = 0,
    EDGEFLEX_MEM_ERR_NO_SPACE,      /* Pool exhausted or too fragmented    */
    EDGEFLEX_MEM_ERR_INVALID_PTR,   /* free() on a pointer we didn't hand out */
    EDGEFLEX_MEM_ERR_DOUBLE_FREE,   /* free() on an already-free block     */
    EDGEFLEX_MEM_ERR_ZERO_SIZE,     /* alloc(0) requested                  */
    EDGEFLEX_MEM_ERR_NOT_INIT       /* manager used before init()          */
} edgeflex_mem_status_t;

typedef struct {
    uint32_t pool_size_bytes;       /* Total pool capacity                 */
    uint32_t current_usage_bytes;   /* Bytes currently allocated (live)    */
    uint32_t peak_usage_bytes;      /* High-water mark since init()        */
    uint32_t free_bytes;            /* pool_size_bytes - current_usage - overhead */
    uint32_t allocation_count;      /* Total successful alloc() calls      */
    uint32_t release_count;         /* Total successful free() calls       */
    uint32_t reuse_count;           /* Allocations satisfied by a block that
                                        had been used and released before  */
    uint32_t failure_count;         /* Total failed alloc() calls          */
    uint32_t live_block_count;      /* Blocks currently allocated          */
} edgeflex_mem_stats_t;

/* ------------------------------------------------------------------------ */
/* API                                                                       */
/* ------------------------------------------------------------------------ */

/**
 * @brief Initialize (or reset) the memory pool. Must be called once before
 *        any alloc()/free() call. Safe to call again to reset all state
 *        (e.g. between benchmark runs) - invalidates all previous pointers.
 */
void edgeflex_mem_init(void);

/**
 * @brief Request `size` bytes from the pool for the active layer.
 *
 * @param size    Number of bytes required (workspace for the active layer).
 * @param out_ptr Receives the pointer to usable memory on success.
 * @return EDGEFLEX_MEM_OK on success, or an error code. On any error,
 *         *out_ptr is set to NULL and no memory is reserved.
 */
edgeflex_mem_status_t edgeflex_mem_alloc(size_t size, void **out_ptr);

/**
 * @brief Release a previously allocated region, making it available for
 *        reuse by the next layer. Adjacent free regions are coalesced.
 *
 * @param ptr Pointer previously returned by edgeflex_mem_alloc().
 * @return EDGEFLEX_MEM_OK on success, or an error code describing why the
 *         pointer could not be safely freed (never crashes on bad input).
 */
edgeflex_mem_status_t edgeflex_mem_free(void *ptr);

/** @brief Fill `out_stats` with a snapshot of current usage/statistics. */
void edgeflex_mem_get_stats(edgeflex_mem_stats_t *out_stats);

/** @brief Convenience: bytes currently free (see edgeflex_mem_stats_t). */
uint32_t edgeflex_mem_free_bytes(void);

/** @brief Convenience: peak bytes used since the last init(). */
uint32_t edgeflex_mem_peak_bytes(void);

#ifdef __cplusplus
}
#endif

#endif /* EDGEFLEX_MEMORY_H */
