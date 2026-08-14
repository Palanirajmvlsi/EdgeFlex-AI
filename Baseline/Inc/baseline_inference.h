/**
 ******************************************************************************
 * @file    baseline_inference.h
 * @brief   Baseline / conventional implementation for the SAME workload as
 *          EdgeFlex AI (Models/tinymlp_model.c), for a fair comparison.
 *
 * Conventional approach: reserve one static buffer per layer's output,
 * ALL of them simultaneously, for the entire lifetime of the program -
 * the pattern EdgeFlex's Dynamic Memory Manager exists to avoid. No
 * alloc()/free() calls at all; everything is a fixed global array sized
 * at compile time.
 *
 * Uses the exact same weights, same input, same dense/relu math as
 * EdgeFlex_Runtime - see edgeflex_inference.c - so any output difference
 * would indicate a real bug, not a different model. Tests/host/test_baseline.c
 * checks the two implementations produce bit-identical output.
 ******************************************************************************
 */
#ifndef BASELINE_INFERENCE_H
#define BASELINE_INFERENCE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "edgeflex_model.h"

typedef struct {
    uint32_t static_buffer_bytes; /* Sum of ALL layer output buffers, reserved
                                      for the whole run regardless of which
                                      layer is currently active. */
    uint32_t layer_count;
} baseline_stats_t;

typedef enum {
    BASELINE_OK = 0,
    BASELINE_ERR_INVALID_MODEL,
    BASELINE_ERR_INPUT_SIZE,
    BASELINE_ERR_OUTPUT_BUF_TOO_SMALL,
    BASELINE_ERR_UNSUPPORTED_SHAPE /* this baseline is sized for tinymlp only */
} baseline_status_t;

baseline_status_t baseline_infer_run(const edgeflex_model_t *model,
                                      const float *input, uint16_t input_len,
                                      float *output, uint16_t output_cap,
                                      uint16_t *written_out,
                                      baseline_stats_t *stats_out);

#ifdef __cplusplus
}
#endif

#endif /* BASELINE_INFERENCE_H */
