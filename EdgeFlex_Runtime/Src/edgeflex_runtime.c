#include "edgeflex_runtime.h"
#include <string.h>

void edgeflex_runtime_init(void)
{
    edgeflex_mem_init();
}

void edgeflex_runtime_run(const edgeflex_model_t *model,
                           const float *input, uint16_t input_len,
                           edgeflex_runtime_result_t *result)
{
    memset(result, 0, sizeof(*result));

    uint16_t out_cap = EDGEFLEX_RT_MAX_OUTPUT;
    uint16_t written = 0;

    edgeflex_monitor_start();
    result->infer_status = edgeflex_infer_run(model, input, input_len,
                                               result->output, out_cap, &written);
    edgeflex_monitor_stop();

    result->output_len = written;
    edgeflex_monitor_get_report(&result->perf);
}
