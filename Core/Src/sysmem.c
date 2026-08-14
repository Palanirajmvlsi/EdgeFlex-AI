/* Standard CubeIDE-style _sbrk stub. EdgeFlex itself never calls
 * malloc/free (see edgeflex_memory.c) - this only exists to satisfy
 * newlib internals used by vsnprintf. Heap is tiny and bounded. */
#include <errno.h>
#include <stdint.h>

extern uint8_t _end;       /* set by the linker script */
extern uint8_t _estack;    /* top of stack, from the linker script */
extern uint32_t _Min_Stack_Size;

static uint8_t *s_heap_end = 0;

void *_sbrk(int incr)
{
    extern uint8_t _end;
    static uint8_t *heap_end = 0;
    uint8_t *prev_heap_end;

    if (heap_end == 0) {
        heap_end = &_end;
    }
    prev_heap_end = heap_end;

    uint8_t *stack_limit = (uint8_t *)((uint32_t)&_estack - (uint32_t)&_Min_Stack_Size);
    if (heap_end + incr > stack_limit) {
        errno = ENOMEM;
        return (void *)-1;
    }

    heap_end += incr;
    (void)s_heap_end;
    return (void *)prev_heap_end;
}
