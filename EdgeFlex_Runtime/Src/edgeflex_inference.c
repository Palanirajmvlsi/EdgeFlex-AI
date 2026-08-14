#include "edgeflex_inference.h"
#include <string.h>

/* y[out_n] = W[out_n x in_n] * x[in_n] + b[out_n] */
static void dense(const float *x, uint16_t in_n,
                   const float *w, const float *b,
                   uint16_t out_n, float *y)
{
    for (uint16_t o = 0; o < out_n; o++) {
        float acc = b[o];
        const float *wrow = &w[(size_t)o * in_n];
        for (uint16_t i = 0; i < in_n; i++) {
            acc += wrow[i] * x[i];
        }
        y[o] = acc;
    }
}

static void relu_inplace(float *buf, uint16_t n)
{
    for (uint16_t i = 0; i < n; i++) {
        if (buf[i] < 0.0f) {
            buf[i] = 0.0f;
        }
    }
}

edgeflex_infer_status_t edgeflex_infer_run(const edgeflex_model_t *model,
                                            const float *input, uint16_t input_len,
                                            float *output, uint16_t output_cap,
                                            uint16_t *written_out)
{
    if (edgeflex_model_validate(model) != EDGEFLEX_MODEL_OK) {
        return EDGEFLEX_INFER_ERR_INVALID_MODEL;
    }
    if (input == NULL || input_len != model->input_size) {
        return EDGEFLEX_INFER_ERR_INPUT_SIZE;
    }
    if (output == NULL || output_cap < model->output_size) {
        return EDGEFLEX_INFER_ERR_OUTPUT_BUF_TOO_SMALL;
    }

    /* `cur_in` starts out pointing at the caller's input array, which is
     * NOT pool-managed (nothing to free for it). From layer 1 onward,
     * cur_in points at a live pool allocation that must be released only
     * after the layer that consumes it has finished. */
    const float *cur_in = input;
    void *cur_in_pool_ptr = NULL; /* NULL => cur_in is the external input, don't free it */

    for (uint8_t i = 0; i < model->layer_count; i++) {
        const edgeflex_layer_t *layer = &model->layers[i];
        size_t out_bytes = (size_t)layer->output_size * sizeof(float);

        void *out_ptr = NULL;
        edgeflex_mem_status_t mrc = edgeflex_mem_alloc(out_bytes, &out_ptr);
        if (mrc != EDGEFLEX_MEM_OK) {
            /* Clean up the input buffer we were still holding before bailing. */
            if (cur_in_pool_ptr != NULL) {
                edgeflex_mem_free(cur_in_pool_ptr);
            }
            return EDGEFLEX_INFER_ERR_ALLOC_FAILED;
        }
        float *out = (float *)out_ptr;

        /* EXECUTE - reads cur_in (still valid/live), writes the new buffer. */
        switch (layer->op) {
            case EDGEFLEX_OP_DENSE:
                dense(cur_in, layer->input_size, layer->weights, layer->biases,
                      layer->output_size, out);
                break;
            case EDGEFLEX_OP_DENSE_RELU:
                dense(cur_in, layer->input_size, layer->weights, layer->biases,
                      layer->output_size, out);
                relu_inplace(out, layer->output_size);
                break;
            case EDGEFLEX_OP_RELU:
                /* In-place style op on a same-size tensor; we still went
                 * through the pool so the memory-manager statistics stay
                 * consistent with "every layer requests its own workspace". */
                memcpy(out, cur_in, (size_t)layer->output_size * sizeof(float));
                relu_inplace(out, layer->output_size);
                break;
            default:
                edgeflex_mem_free(out_ptr);
                if (cur_in_pool_ptr != NULL) {
                    edgeflex_mem_free(cur_in_pool_ptr);
                }
                return EDGEFLEX_INFER_ERR_INVALID_MODEL;
        }

        /* Only now - after `out` has been fully computed from cur_in - is
         * it safe to release the OLD input. Releasing any earlier would
         * be a use-after-free of the tensor the layer just read. */
        if (cur_in_pool_ptr != NULL) {
            edgeflex_mem_status_t frc = edgeflex_mem_free(cur_in_pool_ptr);
            if (frc != EDGEFLEX_MEM_OK) {
                edgeflex_mem_free(out_ptr);
                return EDGEFLEX_INFER_ERR_FREE_FAILED;
            }
        }

        cur_in = out;
        cur_in_pool_ptr = out_ptr;
    }

    /* cur_in now holds the final layer's output, still a live allocation.
     * Copy it out to the caller's buffer, THEN release the pool block -
     * copy-before-free, never the reverse. */
    memcpy(output, cur_in, (size_t)model->output_size * sizeof(float));
    edgeflex_mem_status_t frc = edgeflex_mem_free(cur_in_pool_ptr);
    if (frc != EDGEFLEX_MEM_OK) {
        return EDGEFLEX_INFER_ERR_FREE_FAILED;
    }

    if (written_out != NULL) {
        *written_out = model->output_size;
    }
    return EDGEFLEX_INFER_OK;
}
