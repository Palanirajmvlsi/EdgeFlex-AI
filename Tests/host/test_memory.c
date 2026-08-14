/**
 * Host-side unit tests for edgeflex_memory (Dynamic Memory Manager).
 * Pure C, no HAL/MCU dependency -> compiled and run on the dev machine
 * with the native gcc, not arm-none-eabi-gcc. Real pass/fail assertions,
 * no simulated/fabricated results.
 *
 * Build/run: see Tests/host/Makefile ("make test").
 */
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "edgeflex_memory.h"

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond, msg) do { \
    g_checks++; \
    if (!(cond)) { \
        g_failures++; \
        printf("  [FAIL] %s (line %d)\n", msg, __LINE__); \
    } else { \
        printf("  [ OK ] %s\n", msg); \
    } \
} while (0)

static void test_init_state(void)
{
    printf("-- test_init_state --\n");
    edgeflex_mem_init();
    edgeflex_mem_stats_t st;
    edgeflex_mem_get_stats(&st);
    CHECK(st.pool_size_bytes == EDGEFLEX_POOL_SIZE, "pool_size_bytes == configured pool size");
    CHECK(st.current_usage_bytes == 0, "current_usage_bytes starts at 0");
    CHECK(st.peak_usage_bytes == 0, "peak_usage_bytes starts at 0");
    CHECK(st.allocation_count == 0, "allocation_count starts at 0");
    CHECK(st.release_count == 0, "release_count starts at 0");
    CHECK(st.reuse_count == 0, "reuse_count starts at 0");
    CHECK(st.free_bytes > 0 && st.free_bytes <= EDGEFLEX_POOL_SIZE, "free_bytes sane after init");
}

static void test_basic_alloc_free(void)
{
    printf("-- test_basic_alloc_free --\n");
    edgeflex_mem_init();
    void *p = NULL;
    edgeflex_mem_status_t rc = edgeflex_mem_alloc(128, &p);
    CHECK(rc == EDGEFLEX_MEM_OK, "alloc(128) succeeds");
    CHECK(p != NULL, "alloc(128) returns non-NULL");

    memset(p, 0xAB, 128); /* write across the whole region: catches OOB/corruption via ASan */

    edgeflex_mem_stats_t st;
    edgeflex_mem_get_stats(&st);
    CHECK(st.current_usage_bytes == 128, "current_usage_bytes == 128 after alloc");
    CHECK(st.peak_usage_bytes == 128, "peak_usage_bytes == 128 after alloc");
    CHECK(st.allocation_count == 1, "allocation_count == 1");

    rc = edgeflex_mem_free(p);
    CHECK(rc == EDGEFLEX_MEM_OK, "free() succeeds");

    edgeflex_mem_get_stats(&st);
    CHECK(st.current_usage_bytes == 0, "current_usage_bytes == 0 after free");
    CHECK(st.peak_usage_bytes == 128, "peak_usage_bytes retains high-water mark after free");
    CHECK(st.release_count == 1, "release_count == 1");
}

/* Mirrors the exact 3-layer demo pattern from the spec:
 * Layer1 ALLOCATE->EXECUTE->RELEASE, Layer2/3 REUSE->EXECUTE->RELEASE. */
static void test_layer_by_layer_reuse(void)
{
    printf("-- test_layer_by_layer_reuse --\n");
    edgeflex_mem_init();

    void *l1 = NULL, *l2 = NULL, *l3 = NULL;
    CHECK(edgeflex_mem_alloc(256, &l1) == EDGEFLEX_MEM_OK, "layer1 alloc");
    memset(l1, 0x11, 256);
    CHECK(edgeflex_mem_free(l1) == EDGEFLEX_MEM_OK, "layer1 release");

    CHECK(edgeflex_mem_alloc(256, &l2) == EDGEFLEX_MEM_OK, "layer2 alloc (should reuse layer1's block)");
    CHECK(l2 == l1, "layer2 got the exact same address as layer1 -> true reuse, not new memory");
    memset(l2, 0x22, 256);
    CHECK(edgeflex_mem_free(l2) == EDGEFLEX_MEM_OK, "layer2 release");

    CHECK(edgeflex_mem_alloc(256, &l3) == EDGEFLEX_MEM_OK, "layer3 alloc (should reuse again)");
    CHECK(l3 == l1, "layer3 got the same address again");
    memset(l3, 0x33, 256);
    CHECK(edgeflex_mem_free(l3) == EDGEFLEX_MEM_OK, "layer3 release");

    edgeflex_mem_stats_t st;
    edgeflex_mem_get_stats(&st);
    CHECK(st.allocation_count == 3, "allocation_count == 3");
    CHECK(st.release_count == 3, "release_count == 3");
    CHECK(st.reuse_count == 2, "reuse_count == 2 (layer2 and layer3 reused layer1's freed block)");
    CHECK(st.peak_usage_bytes == 256, "peak stays at single-layer size, never sum of all 3");
}

static void test_full_pool_condition(void)
{
    printf("-- test_full_pool_condition --\n");
    edgeflex_mem_init();
    void *p = NULL;
    /* Ask for the entire pool minus a little room for the block header. */
    size_t big = EDGEFLEX_POOL_SIZE - 64;
    edgeflex_mem_status_t rc = edgeflex_mem_alloc(big, &p);
    CHECK(rc == EDGEFLEX_MEM_OK, "near-full allocation succeeds");
    CHECK(p != NULL, "near-full allocation returns pointer");

    edgeflex_mem_stats_t st;
    edgeflex_mem_get_stats(&st);
    CHECK(st.free_bytes < 64, "free_bytes small after near-full allocation");

    edgeflex_mem_free(p);
}

static void test_oversized_allocation_fails_cleanly(void)
{
    printf("-- test_oversized_allocation_fails_cleanly --\n");
    edgeflex_mem_init();
    void *p = (void *)0x1; /* sentinel to prove it gets reset to NULL */
    edgeflex_mem_status_t rc = edgeflex_mem_alloc(EDGEFLEX_POOL_SIZE * 4u, &p);
    CHECK(rc == EDGEFLEX_MEM_ERR_NO_SPACE, "oversized alloc returns NO_SPACE");
    CHECK(p == NULL, "oversized alloc resets out_ptr to NULL");

    edgeflex_mem_stats_t st;
    edgeflex_mem_get_stats(&st);
    CHECK(st.failure_count == 1, "failure_count incremented");
    CHECK(st.allocation_count == 0, "allocation_count NOT incremented on failure");
}

static void test_invalid_pointer_free(void)
{
    printf("-- test_invalid_pointer_free --\n");
    edgeflex_mem_init();

    int stack_var = 0;
    edgeflex_mem_status_t rc = edgeflex_mem_free(&stack_var); /* pointer never came from us */
    CHECK(rc == EDGEFLEX_MEM_ERR_INVALID_PTR, "free() of a foreign/out-of-pool pointer is rejected");

    rc = edgeflex_mem_free(NULL);
    CHECK(rc == EDGEFLEX_MEM_ERR_INVALID_PTR, "free(NULL) is rejected, not a crash");
}

static void test_double_free_detected(void)
{
    printf("-- test_double_free_detected --\n");
    edgeflex_mem_init();
    void *p = NULL;
    edgeflex_mem_alloc(64, &p);
    edgeflex_mem_status_t rc1 = edgeflex_mem_free(p);
    edgeflex_mem_status_t rc2 = edgeflex_mem_free(p); /* second free of same pointer */
    CHECK(rc1 == EDGEFLEX_MEM_OK, "first free succeeds");
    CHECK(rc2 == EDGEFLEX_MEM_ERR_DOUBLE_FREE, "second free is detected and rejected, not corrupting state");
}

static void test_zero_size_and_not_init(void)
{
    printf("-- test_zero_size_and_not_init --\n");
    edgeflex_mem_init();
    void *p = NULL;
    edgeflex_mem_status_t rc = edgeflex_mem_alloc(0, &p);
    CHECK(rc == EDGEFLEX_MEM_ERR_ZERO_SIZE, "alloc(0) is rejected explicitly");
}

static void test_no_out_of_bounds_across_many_cycles(void)
{
    /* Stress: many alloc/free cycles of varying sizes, always touching
     * every byte returned. Run under ASan (see Makefile) so any 1-byte
     * OOB write or use-after-free aborts the test binary immediately. */
    printf("-- test_no_out_of_bounds_across_many_cycles --\n");
    edgeflex_mem_init();
    const size_t sizes[] = {16, 300, 64, 900, 32, 1500, 8};
    int ok = 1;
    for (int cycle = 0; cycle < 50; cycle++) {
        for (size_t i = 0; i < sizeof(sizes)/sizeof(sizes[0]); i++) {
            void *p = NULL;
            if (edgeflex_mem_alloc(sizes[i], &p) == EDGEFLEX_MEM_OK) {
                memset(p, (int)(0xC0 + i), sizes[i]); /* touch every byte */
                if (edgeflex_mem_free(p) != EDGEFLEX_MEM_OK) { ok = 0; }
            }
        }
    }
    CHECK(ok, "500+ alloc/free cycles with full-region writes complete cleanly");

    edgeflex_mem_stats_t st;
    edgeflex_mem_get_stats(&st);
    CHECK(st.current_usage_bytes == 0, "pool fully drained back to 0 after all cycles (no leaks)");
    CHECK(st.peak_usage_bytes <= EDGEFLEX_POOL_SIZE, "peak usage never exceeds pool size");
}

int main(void)
{
    printf("EdgeFlex AI - Dynamic Memory Manager - host unit tests\n");
    printf("Pool size under test: %d bytes\n\n", EDGEFLEX_POOL_SIZE);

    test_init_state();
    test_basic_alloc_free();
    test_layer_by_layer_reuse();
    test_full_pool_condition();
    test_oversized_allocation_fails_cleanly();
    test_invalid_pointer_free();
    test_double_free_detected();
    test_zero_size_and_not_init();
    test_no_out_of_bounds_across_many_cycles();

    printf("\n%d checks run, %d failed.\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
