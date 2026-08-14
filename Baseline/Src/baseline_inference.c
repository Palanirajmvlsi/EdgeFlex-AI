#include "baseline_inference.h"
#include <string.h>

/* Sized specifically for the shared tinymlp workload (8 -> 16 -> 12 -> 4).
 * A "real" conventional static implementation hardcodes shapes like this -
 * that rigidity (vs EdgeFlex's model-driven pool) is itself part of what's
 * being compared. */
#define BL_L1_OUT 16u
#define BL_L2_OUT 12u
#define BL_L3_OUT 4u

/* Reserved for the ENTIRE program lifetime, not just while their layer is
 * active - this is the conventional pattern EdgeFlex's pool avoids. */
static float s_l1_buf[BL_L1_OUT];
static float s_l2_buf[BL_L2_OUT];
static float s_l3_buf[BL_L3_OUT];

static void dense(const float *x, uint16_t in_n, const float *w, const float *b,
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
        if (buf[i] < 0.0f) buf[i] = 0.0f;
    }
}

baseline_status_t baseline_infer_run(const edgeflex_model_t *model,
                                      const float *input, uint16_t input_len,
                                      float *output, uint16_t output_cap,
                                      uint16_t *written_out,
                                      baseline_stats_t *stats_out)
{
    if (edgeflex_model_validate(model) != EDGEFLEX_MODEL_OK) {
        return BASELINE_ERR_INVALID_MODEL;
    }
    if (input == NULL || input_len != model->input_size) {
        return BASELINE_ERR_INPUT_SIZE;
    }
    if (output == NULL || output_cap < model->output_size) {
        return BASELINE_ERR_OUTPUT_BUF_TOO_SMALL;
    }
    if (model->layer_count != 3 ||
        model->layers[0].output_size != BL_L1_OUT ||
        model->layers[1].output_size != BL_L2_OUT ||
        model->layers[2].output_size != BL_L3_OUT) {
        /* Honest limitation: this baseline is NOT a generic engine, it's
         * fixed-shape static buffers, exactly like conventional embedded
         * code tends to be written. A shape it wasn't sized for is a
         * real error here, not something to silently reinterpret. */
        return BASELINE_ERR_UNSUPPORTED_SHAPE;
    }

    const edgeflex_layer_t *l1 = &model->layers[0];
    const edgeflex_layer_t *l2 = &model->layers[1];
    const edgeflex_layer_t *l3 = &model->layers[2];

    dense(input, l1->input_size, l1->weights, l1->biases, l1->output_size, s_l1_buf);
    relu_inplace(s_l1_buf, l1->output_size);

    dense(s_l1_buf, l2->input_size, l2->weights, l2->biases, l2->output_size, s_l2_buf);
    relu_inplace(s_l2_buf, l2->output_size);

    dense(s_l2_buf, l3->input_size, l3->weights, l3->biases, l3->output_size, s_l3_buf);
    /* layer 3 has no activation (EDGEFLEX_OP_DENSE) */

    memcpy(output, s_l3_buf, (size_t)model->output_size * sizeof(float));
    if (written_out != NULL) {
        *written_out = model->output_size;
    }
    if (stats_out != NULL) {
        stats_out->static_buffer_bytes = (uint32_t)(sizeof(s_l1_buf) + sizeof(s_l2_buf) + sizeof(s_l3_buf));
        stats_out->layer_count = model->layer_count;
    }
    return BASELINE_OK;
}
