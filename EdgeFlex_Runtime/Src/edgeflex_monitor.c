#include "edgeflex_monitor.h"
#include <string.h>

static edgeflex_time_fn_t s_time_fn = NULL;
static uint32_t s_ticks_per_ms = 0;
static uint32_t s_start_ticks = 0;
static uint32_t s_elapsed_ticks = 0;
static bool s_have_window = false;

void edgeflex_monitor_set_time_source(edgeflex_time_fn_t fn, uint32_t ticks_per_ms)
{
    s_time_fn = fn;
    s_ticks_per_ms = ticks_per_ms;
}

void edgeflex_monitor_start(void)
{
    s_have_window = false;
    if (s_time_fn != NULL) {
        s_start_ticks = s_time_fn();
    }
}

void edgeflex_monitor_stop(void)
{
    if (s_time_fn != NULL) {
        uint32_t end_ticks = s_time_fn();
        /* Handles a single wraparound of a free-running counter; good
         * enough for short inference windows well under a full period. */
        s_elapsed_ticks = end_ticks - s_start_ticks;
        s_have_window = true;
    }
}

void edgeflex_monitor_get_report(edgeflex_perf_report_t *out_report)
{
    if (out_report == NULL) {
        return;
    }
    memset(out_report, 0, sizeof(*out_report));

    edgeflex_mem_get_stats(&out_report->mem);

    if (s_time_fn != NULL && s_have_window) {
        out_report->timing_available = true;
        out_report->elapsed_ticks = s_elapsed_ticks;
        out_report->ticks_per_ms = s_ticks_per_ms;
        if (s_ticks_per_ms > 0) {
            out_report->elapsed_ms = (float)s_elapsed_ticks / (float)s_ticks_per_ms;
        }
    } else {
        out_report->timing_available = false; /* honestly "not measured" */
    }
}
