/**
 * @file edgeflex_runtime.h
 * @brief EdgeFlex AI - Runtime Core. Thin orchestration: init -> load ->
 *        infer -> report. Contains no HAL calls itself (portable/testable);
 *        the platform (Core/Src/main.c) owns UART printing and the DWT
 *        time-source wiring.
 */
#ifndef EDGEFLEX_RUNTIME_H
#define EDGEFLEX_RUNTIME_H

#ifdef __cplusplus
extern "C" {
#endif

#include "edgeflex_memory.h"
#include "edgeflex_model.h"
#include "edgeflex_inference.h"
#include "edgeflex_monitor.h"

typedef struct {
    edgeflex_infer_status_t infer_status;
    edgeflex_perf_report_t  perf;
    float    output[16];      /* generous fixed cap; see EDGEFLEX_RT_MAX_OUTPUT */
    uint16_t output_len;
} edgeflex_runtime_result_t;

#define EDGEFLEX_RT_MAX_OUTPUT (16u)

/** @brief Initialize the pool. Call once at boot before run(). */
void edgeflex_runtime_init(void);

/**
 * @brief Run one full inference pass through `model` on `input`, timed via
 *        whatever time source was registered with edgeflex_monitor_set_time_source()
 *        (or "not measured" if none was registered).
 */
void edgeflex_runtime_run(const edgeflex_model_t *model,
                           const float *input, uint16_t input_len,
                           edgeflex_runtime_result_t *result);

#ifdef __cplusplus
}
#endif

#endif /* EDGEFLEX_RUNTIME_H */
