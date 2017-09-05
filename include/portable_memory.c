
#include "portable_memory.h"

#if defined(__MACH__)
#include <stdlib.h>
#include <assert.h>

void* memalign(int alignment, int size){
    void* memory;
    int err = posix_memalign(&memory, alignment, size);
    assert(!err);
    return memory;
}
#endif

