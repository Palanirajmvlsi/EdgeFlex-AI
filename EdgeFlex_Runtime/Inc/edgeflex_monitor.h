/**
 ******************************************************************************
 * @file    edgeflex_monitor.h
 * @brief   EdgeFlex AI - Performance Monitor
 *
 * Reports memory-manager statistics (allocations, releases, reuses, peak/
 * current usage) plus inference timing.
 *
 * Timing is deliberately hardware-independent: this module never reads a
 * clock itself. The caller plugs in a tick source via
 * edgeflex_monitor_set_time_source(). On the STM32 target that source is
 * the Cortex-M4 DWT cycle counter (see Core/Src/main.c); on host builds no
 * source is registered, so timing honestly reports NOT_MEASURED instead of
 * a fabricated number. This keeps the module testable on a PC and reusable
 * on any Cortex-M part without editing this file.
 ******************************************************************************
 */
#ifndef EDGEFLEX_MONITOR_H
#define EDGEFLEX_MONITOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "edgeflex_memory.h"

/** Returns a monotonically increasing tick count in the caller-chosen unit
 *  (e.g. CPU cycles). Registered once via edgeflex_monitor_set_time_source(). */
typedef uint32_t (*edgeflex_time_fn_t)(void);

typedef struct {
    edgeflex_mem_stats_t mem;         /* snapshot from the Dynamic Memory Manager */
    bool     timing_available;         /* false => no time source registered      */
    uint32_t elapsed_ticks;            /* raw ticks between start()/stop()        */
    uint32_t ticks_per_ms;             /* conversion factor, 0 if unknown         */
    float    elapsed_ms;               /* only valid if timing_available          */
} edgeflex_perf_report_t;

/**
 * @brief Register the platform's tick source. Call once at startup.
 * @param fn            Function returning current tick count (e.g. DWT->CYCCNT).
 * @param ticks_per_ms  How many ticks correspond to 1 millisecond
 *                       (e.g. SystemCoreClock / 1000 for a cycle counter).
 *                       Pass 0 if unknown - elapsed_ms will then be reported
 *                       as unavailable even though raw ticks are captured.
 */
void edgeflex_monitor_set_time_source(edgeflex_time_fn_t fn, uint32_t ticks_per_ms);

/** @brief Mark the start of a timed region (e.g. right before inference). */
void edgeflex_monitor_start(void);

/** @brief Mark the end of a timed region and capture elapsed ticks. */
void edgeflex_monitor_stop(void);

/**
 * @brief Build a full report: current memory-manager stats + the last
 *        start()/stop() timing window (or "not measured" if no time
 *        source was ever registered - this function never invents a value).
 */
void edgeflex_monitor_get_report(edgeflex_perf_report_t *out_report);

#ifdef __cplusplus
}
#endif

#endif /* EDGEFLEX_MONITOR_H */
