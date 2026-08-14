/**
 ******************************************************************************
 * @file    edgeflex_model.h
 * @brief   EdgeFlex AI - Model Loader (compact embedded model representation)
 *
 * NOT a TensorFlow Lite parser and does not claim TFLite compatibility.
 * This is the smallest practical representation that still lets the
 * Inference Engine drive real layer-by-layer execution against the
 * Dynamic Memory Manager: each layer knows its operator, its input/output
 * shape, and how many workspace bytes it needs, so the loader can hand the
 * engine everything required without the engine ever touching model
 * internals directly.
 *
 * Models are plain C structs/arrays (see Models/tinymlp_model.c), i.e.
 * "compiled in" rather than parsed from a file - appropriate for a
 * Cortex-M4 MVP with no filesystem.
 ******************************************************************************
 */
#ifndef EDGEFLEX_MODEL_H
#define EDGEFLEX_MODEL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define EDGEFLEX_MODEL_MAX_LAYERS (8u)
#define EDGEFLEX_MODEL_NAME_MAXLEN (32u)

typedef enum {
    EDGEFLEX_OP_DENSE = 0,   /* y = W*x + b            (fully connected) */
    EDGEFLEX_OP_RELU,        /* y = max(0, x)           (in-place activation) */
    EDGEFLEX_OP_DENSE_RELU,  /* y = relu(W*x + b)       (fused, saves a buffer) */
} edgeflex_op_type_t;

typedef struct {
    edgeflex_op_type_t op;
    uint16_t input_size;      /* elements in                                */
    uint16_t output_size;     /* elements out                               */
    const float *weights;     /* [output_size x input_size], row-major, or
                                  NULL for ops that don't need weights      */
    const float *biases;      /* [output_size], or NULL                    */
} edgeflex_layer_t;

typedef struct {
    char name[EDGEFLEX_MODEL_NAME_MAXLEN];
    uint8_t layer_count;
    const edgeflex_layer_t *layers;  /* array of layer_count entries        */
    uint16_t input_size;
    uint16_t output_size;
} edgeflex_model_t;

typedef enum {
    EDGEFLEX_MODEL_OK = 0,
    EDGEFLEX_MODEL_ERR_NULL,
    EDGEFLEX_MODEL_ERR_NO_LAYERS,
    EDGEFLEX_MODEL_ERR_TOO_MANY_LAYERS,
    EDGEFLEX_MODEL_ERR_SHAPE_MISMATCH,   /* layer[i].out != layer[i+1].in    */
    EDGEFLEX_MODEL_ERR_MISSING_WEIGHTS,  /* DENSE op with weights == NULL   */
    EDGEFLEX_MODEL_ERR_ZERO_DIM          /* a layer has input/output size 0 */
} edgeflex_model_status_t;

/**
 * @brief Validate that a model's layers form a consistent chain the
 *        Inference Engine can safely execute (shapes line up, required
 *        weight pointers are present, layer count is in bounds).
 *
 * Call this once after "loading" (i.e. after the model struct is
 * constructed) and before running inference. Never assume a compiled-in
 * model is correct by construction - this catches copy/paste shape bugs.
 */
edgeflex_model_status_t edgeflex_model_validate(const edgeflex_model_t *model);

/** @brief Bytes of output-tensor workspace layer `idx` needs from the pool. */
size_t edgeflex_model_layer_workspace_bytes(const edgeflex_model_t *model, uint8_t idx);

/** @brief Human-readable string for a status/error code (for UART logs). */
const char *edgeflex_model_status_str(edgeflex_model_status_t status);

#ifdef __cplusplus
}
#endif

#endif /* EDGEFLEX_MODEL_H */
