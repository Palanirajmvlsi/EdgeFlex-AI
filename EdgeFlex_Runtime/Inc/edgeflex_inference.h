/**
 ******************************************************************************
 * @file    edgeflex_inference.h
 * @brief   EdgeFlex AI - Inference Engine
 *
 * Drives real layer-by-layer execution against the Dynamic Memory Manager:
 *
 * SAFETY-CRITICAL ORDERING: a layer's input buffer must never be released
 * before that layer has finished reading from it to produce its output.
 * This engine keeps at most TWO live pool allocations at any time -
 * "current input" and "current output" - and only frees the OLD input
 * immediately AFTER the new output has been fully computed from it.
 *
 * Actual allocation pattern this produces for N layers (verified by
 * Tests/host/test_inference.c, not assumed):
 *
 *   Layer 1: ALLOCATE (pool still virgin)         -> EXECUTE -> hold
 *   Layer 2: ALLOCATE (pool still virgin - layer1's -> EXECUTE -> hold
 *            block can't be freed yet, it's still Layer 2's input!)
 *            ... NOW free Layer 1's block (already consumed)
 *   Layer 3: ALLOCATE -> REUSES Layer 1's freed block -> EXECUTE -> hold
 *            ... NOW free Layer 2's block
 *   Layer 4: ALLOCATE -> REUSES Layer 2's freed block -> ...
 *
 * So reuse only starts at layer 3: for N layers there are (N-2) true
 * reuses, not (N-1) - a naive "release every buffer the instant its layer
 * finishes" scheme WOULD get (N-1) reuses, but that scheme is exactly what
 * the safety rule above forbids, because it frees a tensor before the
 * next layer has read it. Consequently, PEAK usage is bounded by the
 * largest ADJACENT PAIR of layer outputs (input+output coexist briefly),
 * not simply the single largest layer - still far below a static
 * baseline that reserves every layer's buffer for the whole run (see
 * Benchmarks/).
 ******************************************************************************
 */
#ifndef EDGEFLEX_INFERENCE_H
#define EDGEFLEX_INFERENCE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "edgeflex_model.h"
#include "edgeflex_memory.h"

typedef enum {
    EDGEFLEX_INFER_OK = 0,
    EDGEFLEX_INFER_ERR_INVALID_MODEL,
    EDGEFLEX_INFER_ERR_INPUT_SIZE,
    EDGEFLEX_INFER_ERR_OUTPUT_BUF_TOO_SMALL,
    EDGEFLEX_INFER_ERR_ALLOC_FAILED,   /* pool couldn't satisfy a layer's request */
    EDGEFLEX_INFER_ERR_FREE_FAILED     /* internal: release of a live buffer failed */
} edgeflex_infer_status_t;

/**
 * @brief Run `model` on `input` (input_len floats), writing the final
 *        layer's output into `output` (caller-provided, output_cap floats).
 *
 * Every intermediate tensor is obtained from edgeflex_mem_alloc() and
 * released via edgeflex_mem_free() as soon as - and only as soon as - it
 * is no longer needed, so the SAME pool region is reused across layers.
 * The caller must have already called edgeflex_mem_init() once.
 *
 * @param written_out  Optional; receives the number of floats written to
 *                      `output` (equals model->output_size) on success.
 */
edgeflex_infer_status_t edgeflex_infer_run(const edgeflex_model_t *model,
                                            const float *input, uint16_t input_len,
                                            float *output, uint16_t output_cap,
                                            uint16_t *written_out);

#ifdef __cplusplus
}
#endif

#endif /* EDGEFLEX_INFERENCE_H */
