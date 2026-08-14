/**
 ******************************************************************************
 * @file    edgeflex_memory.c
 * @brief   EdgeFlex AI - Dynamic Memory Manager implementation
 *
 * Fixed static arena + first-fit free list + immediate coalescing.
 * No libc heap. See edgeflex_memory.h for the full design rationale.
 ******************************************************************************
 */
#include "edgeflex_memory.h"
#include <string.h>

/* ------------------------------------------------------------------------ */
/* Internal block header                                                    */
/* ------------------------------------------------------------------------ */

typedef struct block_header {
    uint32_t size;              /* Usable payload size (bytes), excludes header */
    uint8_t  used;               /* 1 = live allocation, 0 = free                */
    uint8_t  was_used_before;    /* 1 = this block has been allocated at least
                                     once before -> a future alloc into it
                                     counts as a "reuse"                        */
    struct block_header *next;   /* Next block in address order (NULL = end)     */
    struct block_header *prev;   /* Previous block in address order              */
} block_header_t;

#define HEADER_SIZE  (sizeof(block_header_t))
#define MIN_SPLIT_PAYLOAD (8u)  /* don't split off slivers smaller than this */

/* block_header_t contains pointers, so its required alignment differs by
 * platform (4 bytes on the 32-bit Cortex-M4 target, 8 bytes on a 64-bit
 * host running the unit tests). Round every split boundary to whichever
 * is stricter so a header is never placed at a misaligned address on
 * either platform. Caught by UBSan on the host build - see Tests/host. */
#define HDR_ALIGN ((uint32_t)_Alignof(block_header_t))
static inline uint32_t align_up(uint32_t v, uint32_t a)
{
    uint32_t r = v % a;
    return (r == 0) ? v : (v + (a - r));
}

/* Pool storage. Static array => zero libc heap usage, deterministic address. */
static uint8_t s_pool[EDGEFLEX_POOL_SIZE] __attribute__((aligned(EDGEFLEX_MEM_ALIGNMENT)));
static block_header_t *s_head = NULL;
static bool s_initialized = false;

static edgeflex_mem_stats_t s_stats;

/* ------------------------------------------------------------------------ */
/* Helpers                                                                   */
/* ------------------------------------------------------------------------ */

static inline uint8_t *pool_lo(void) { return s_pool; }
static inline uint8_t *pool_hi(void) { return s_pool + EDGEFLEX_POOL_SIZE; }

static inline void *block_payload(block_header_t *b)
{
    return (void *)((uint8_t *)b + HEADER_SIZE);
}

/* Is `b` a header address that actually lies inside our pool and is
 * 4-byte aligned the way our allocator would have placed it? Used to
 * reject garbage/foreign pointers passed to free(). */
static bool header_is_plausible(block_header_t *b)
{
    uint8_t *addr = (uint8_t *)b;
    if (addr < pool_lo() || addr >= pool_hi()) {
        return false;
    }
    if (((uintptr_t)addr) % EDGEFLEX_MEM_ALIGNMENT != 0) {
        return false;
    }
    return true;
}

static void recompute_peak(void)
{
    if (s_stats.current_usage_bytes > s_stats.peak_usage_bytes) {
        s_stats.peak_usage_bytes = s_stats.current_usage_bytes;
    }
}

/* free_bytes = sum of *usable payload* across free blocks, NOT
 * (pool_size - current_usage) - that subtraction silently ignores header
 * overhead and over-reports how much a caller can actually allocate.
 * Bug found by Tests/host/test_memory.c::test_full_pool_condition. */
static void recompute_free_bytes(void)
{
    uint32_t total = 0;
    block_header_t *b = s_head;
    while (b != NULL) {
        if (!b->used) {
            total += b->size;
        }
        b = b->next;
    }
    s_stats.free_bytes = total;
}

/* ------------------------------------------------------------------------ */
/* Public API                                                                */
/* ------------------------------------------------------------------------ */

void edgeflex_mem_init(void)
{
    memset(s_pool, 0, sizeof(s_pool));
    memset(&s_stats, 0, sizeof(s_stats));

    s_head = (block_header_t *)s_pool;
    s_head->size = EDGEFLEX_POOL_SIZE - HEADER_SIZE;
    s_head->used = 0;
    s_head->was_used_before = 0;
    s_head->next = NULL;
    s_head->prev = NULL;

    s_stats.pool_size_bytes = EDGEFLEX_POOL_SIZE;
    s_stats.free_bytes = s_head->size;

    s_initialized = true;
}

edgeflex_mem_status_t edgeflex_mem_alloc(size_t size, void **out_ptr)
{
    if (out_ptr == NULL) {
        return EDGEFLEX_MEM_ERR_INVALID_PTR;
    }
    *out_ptr = NULL;

    if (!s_initialized) {
        return EDGEFLEX_MEM_ERR_NOT_INIT;
    }
    if (size == 0) {
        return EDGEFLEX_MEM_ERR_ZERO_SIZE;
    }

    /* Round up so every returned pointer is aligned AND, if we split,
     * the new block header we place right after it is also aligned. */
    uint32_t want = align_up((uint32_t)size, EDGEFLEX_MEM_ALIGNMENT);
    want = align_up(want, HDR_ALIGN);

    block_header_t *b = s_head;
    while (b != NULL) {
        if (!b->used && b->size >= want) {
            /* Found a fit. Split off the remainder if it's big enough to
             * be useful as its own block; otherwise hand out the whole
             * block (avoids leaving unusable slivers -> bounded fragmentation). */
            uint32_t leftover = b->size - want;
            if (leftover >= HEADER_SIZE + MIN_SPLIT_PAYLOAD) {
                uint8_t *new_block_addr = (uint8_t *)block_payload(b) + want;
                block_header_t *nb = (block_header_t *)new_block_addr;
                nb->size = leftover - HEADER_SIZE;
                nb->used = 0;
                nb->was_used_before = 0;
                nb->next = b->next;
                nb->prev = b;
                if (b->next != NULL) {
                    b->next->prev = nb;
                }
                b->next = nb;
                b->size = want;
            }

            /* Bounds check: the block we're about to hand out must be
             * fully contained within the pool. This should always hold
             * by construction, but we verify explicitly per the "every
             * allocation has bounds checking" safety rule. */
            uint8_t *payload = (uint8_t *)block_payload(b);
            if (payload < pool_lo() || (payload + b->size) > pool_hi()) {
                return EDGEFLEX_MEM_ERR_NO_SPACE; /* should be unreachable */
            }

            bool is_reuse = (b->was_used_before != 0);

            b->used = 1;
            b->was_used_before = 1;

            s_stats.allocation_count++;
            s_stats.live_block_count++;
            if (is_reuse) {
                s_stats.reuse_count++;
            }
            s_stats.current_usage_bytes += b->size;
            recompute_peak();
            recompute_free_bytes();

            *out_ptr = payload;
            return EDGEFLEX_MEM_OK;
        }
        b = b->next;
    }

    s_stats.failure_count++;
    return EDGEFLEX_MEM_ERR_NO_SPACE;
}

edgeflex_mem_status_t edgeflex_mem_free(void *ptr)
{
    if (!s_initialized) {
        return EDGEFLEX_MEM_ERR_NOT_INIT;
    }
    if (ptr == NULL) {
        return EDGEFLEX_MEM_ERR_INVALID_PTR;
    }

    block_header_t *b = (block_header_t *)((uint8_t *)ptr - HEADER_SIZE);
    if (!header_is_plausible(b)) {
        return EDGEFLEX_MEM_ERR_INVALID_PTR;
    }

    /* Confirm this header is actually one of ours by walking the list -
     * protects against a pointer that lies in-range but wasn't a block
     * boundary we created (e.g. an interior pointer). */
    block_header_t *walker = s_head;
    bool found = false;
    while (walker != NULL) {
        if (walker == b) { found = true; break; }
        walker = walker->next;
    }
    if (!found) {
        return EDGEFLEX_MEM_ERR_INVALID_PTR;
    }

    if (!b->used) {
        return EDGEFLEX_MEM_ERR_DOUBLE_FREE;
    }

    b->used = 0;
    s_stats.release_count++;
    s_stats.live_block_count--;
    s_stats.current_usage_bytes -= b->size;

    /* Coalesce with next, then with previous, to keep fragmentation bounded. */
    if (b->next != NULL && !b->next->used) {
        block_header_t *n = b->next;
        b->size += HEADER_SIZE + n->size;
        b->next = n->next;
        if (n->next != NULL) {
            n->next->prev = b;
        }
    }
    if (b->prev != NULL && !b->prev->used) {
        block_header_t *p = b->prev;
        p->size += HEADER_SIZE + b->size;
        p->next = b->next;
        if (b->next != NULL) {
            b->next->prev = p;
        }
        b = p; /* merged into predecessor */
    }

    recompute_free_bytes();

    return EDGEFLEX_MEM_OK;
}

void edgeflex_mem_get_stats(edgeflex_mem_stats_t *out_stats)
{
    if (out_stats == NULL) {
        return;
    }
    *out_stats = s_stats;
}

uint32_t edgeflex_mem_free_bytes(void)
{
    return s_stats.free_bytes;
}

uint32_t edgeflex_mem_peak_bytes(void)
{
    return s_stats.peak_usage_bytes;
}
