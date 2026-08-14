#include <stdio.h>
#include <math.h>
#include "edgeflex_memory.h"
#include "edgeflex_model.h"
#include "edgeflex_inference.h"
#include "../../Models/tinymlp_model.h"

static int g_checks = 0, g_failures = 0;
#define CHECK(cond, msg) do { \
    g_checks++; \
    if (!(cond)) { g_failures++; printf("  [FAIL] %s (line %d)\n", msg, __LINE__); } \
    else { printf("  [ OK ] %s\n", msg); } \
} while (0)

int main(void)
{
    printf("EdgeFlex AI - Inference Engine - host unit tests\n\n");

    printf("-- model validation --\n");
    CHECK(edgeflex_model_validate(&g_tinymlp_model) == EDGEFLEX_MODEL_OK,
          "compiled-in tinymlp model passes shape/weight validation");

    printf("-- real inference run --\n");
    edgeflex_mem_init();

    float output[TINYMLP_OUTPUT_SIZE];
    uint16_t written = 0;
    edgeflex_infer_status_t rc = edgeflex_infer_run(&g_tinymlp_model,
                                                     g_tinymlp_sample_input, TINYMLP_INPUT_SIZE,
                                                     output, TINYMLP_OUTPUT_SIZE, &written);
    CHECK(rc == EDGEFLEX_INFER_OK, "inference completes successfully");
    CHECK(written == TINYMLP_OUTPUT_SIZE, "reports correct output element count");

    int all_finite = 1;
    for (uint16_t i = 0; i < written; i++) {
        if (isnan(output[i]) || isinf(output[i])) all_finite = 0;
    }
    CHECK(all_finite, "output values are all finite real numbers (not fabricated/NaN)");
    printf("  computed output: [%.4f, %.4f, %.4f, %.4f]\n",
           output[0], output[1], output[2], output[3]);

    printf("-- determinism --\n");
    edgeflex_mem_init();
    float output2[TINYMLP_OUTPUT_SIZE];
    edgeflex_infer_run(&g_tinymlp_model, g_tinymlp_sample_input, TINYMLP_INPUT_SIZE,
                        output2, TINYMLP_OUTPUT_SIZE, NULL);
    int same = 1;
    for (uint16_t i = 0; i < TINYMLP_OUTPUT_SIZE; i++) {
        if (output[i] != output2[i]) same = 0;
    }
    CHECK(same, "re-running on a fresh pool produces bit-identical output (deterministic engine)");

    printf("-- memory-manager statistics reflect real reuse across layers --\n");
    edgeflex_mem_stats_t st;
    edgeflex_mem_get_stats(&st);
    CHECK(st.allocation_count == g_tinymlp_model.layer_count,
          "one allocation per layer (3 layers -> 3 allocations)");
    CHECK(st.release_count == g_tinymlp_model.layer_count,
          "every allocation was released (no leaks after inference)");
    /* Safe ping-pong lifetime: layer i's buffer can't be freed until layer
     * i+1 has read it, so reuse only starts at layer 3 (see
     * edgeflex_inference.h for the full walkthrough). N layers -> N-2 reuses. */
    CHECK(st.reuse_count == (uint32_t)(g_tinymlp_model.layer_count - 2),
          "reuse starts at layer 3 (1 reuse for 3 layers) - layer 2 can't reuse "
          "layer 1's block because layer 1's output is still layer 2's input");
    CHECK(st.current_usage_bytes == 0, "pool fully drained after inference completes");

    /* Peak usage is bounded by the largest ADJACENT PAIR of layer outputs
     * coexisting (current input + current output), never the sum of ALL
     * layers - that gap vs. a static all-buffers-reserved baseline is the
     * actual optimization this project demonstrates (see Benchmarks/). */
    uint32_t worst_pair_bytes = 0;
    uint32_t prev_bytes = (uint32_t)g_tinymlp_model.input_size * sizeof(float);
    for (uint8_t i = 0; i < g_tinymlp_model.layer_count; i++) {
        uint32_t b = (uint32_t)edgeflex_model_layer_workspace_bytes(&g_tinymlp_model, i);
        uint32_t pair = (i == 0) ? b : (prev_bytes + b); /* layer0's input isn't pool-managed */
        if (pair > worst_pair_bytes) worst_pair_bytes = pair;
        prev_bytes = b;
    }
    printf("  peak_usage_bytes=%u, worst adjacent-pair bound=%u bytes\n",
           st.peak_usage_bytes, worst_pair_bytes);
    CHECK(st.peak_usage_bytes == worst_pair_bytes,
          "peak SRAM usage equals the worst adjacent-layer pair, not the sum of all layers");
    CHECK(st.peak_usage_bytes <
              ((uint32_t)(TINYMLP_INPUT_SIZE + 16 + 12 + TINYMLP_OUTPUT_SIZE) * sizeof(float)),
          "peak SRAM usage is still well below reserving every layer's buffer simultaneously");

    printf("-- error paths --\n");
    float bad_input[3] = {0};
    rc = edgeflex_infer_run(&g_tinymlp_model, bad_input, 3, output, TINYMLP_OUTPUT_SIZE, NULL);
    CHECK(rc == EDGEFLEX_INFER_ERR_INPUT_SIZE, "wrong input length is rejected");

    float tiny_out[1];
    rc = edgeflex_infer_run(&g_tinymlp_model, g_tinymlp_sample_input, TINYMLP_INPUT_SIZE,
                             tiny_out, 1, NULL);
    CHECK(rc == EDGEFLEX_INFER_ERR_OUTPUT_BUF_TOO_SMALL, "undersized output buffer is rejected");

    printf("\n%d checks run, %d failed.\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
