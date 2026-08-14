#include <stdio.h>
#include "edgeflex_memory.h"
#include "edgeflex_model.h"
#include "edgeflex_inference.h"
#include "baseline_inference.h"
#include "tinymlp_model.h"

static int g_checks = 0, g_failures = 0;
#define CHECK(cond, msg) do { \
    g_checks++; \
    if (!(cond)) { g_failures++; printf("  [FAIL] %s\n", msg); } \
    else { printf("  [ OK ] %s\n", msg); } \
} while (0)

int main(void)
{
    printf("EdgeFlex AI - Baseline vs EdgeFlex cross-check\n\n");

    float edgeflex_out[TINYMLP_OUTPUT_SIZE];
    edgeflex_mem_init();
    edgeflex_infer_status_t erc = edgeflex_infer_run(&g_tinymlp_model,
                                                      g_tinymlp_sample_input, TINYMLP_INPUT_SIZE,
                                                      edgeflex_out, TINYMLP_OUTPUT_SIZE, NULL);
    CHECK(erc == EDGEFLEX_INFER_OK, "EdgeFlex inference succeeds");

    float baseline_out[TINYMLP_OUTPUT_SIZE];
    baseline_stats_t bstats;
    baseline_status_t brc = baseline_infer_run(&g_tinymlp_model,
                                                g_tinymlp_sample_input, TINYMLP_INPUT_SIZE,
                                                baseline_out, TINYMLP_OUTPUT_SIZE, NULL, &bstats);
    CHECK(brc == BASELINE_OK, "Baseline inference succeeds");

    int identical = 1;
    for (unsigned i = 0; i < TINYMLP_OUTPUT_SIZE; i++) {
        if (edgeflex_out[i] != baseline_out[i]) identical = 0;
    }
    CHECK(identical, "EdgeFlex and Baseline produce BIT-IDENTICAL output "
                      "(same model/weights/input, only memory strategy differs)");

    printf("  EdgeFlex output:  [%.4f, %.4f, %.4f, %.4f]\n",
           edgeflex_out[0], edgeflex_out[1], edgeflex_out[2], edgeflex_out[3]);
    printf("  Baseline output:  [%.4f, %.4f, %.4f, %.4f]\n",
           baseline_out[0], baseline_out[1], baseline_out[2], baseline_out[3]);
    printf("  Baseline static buffer bytes: %u\n", bstats.static_buffer_bytes);

    edgeflex_mem_stats_t mstats;
    edgeflex_mem_get_stats(&mstats);
    printf("  EdgeFlex peak pool usage bytes: %u\n", mstats.peak_usage_bytes);

    printf("\n%d checks run, %d failed.\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
