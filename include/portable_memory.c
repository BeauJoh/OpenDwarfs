
#include "portable_memory.h"
#include <stdlib.h>
#include <assert.h>

void* memalign(int alignment, int size){
    void* memory;
    int err = posix_memalign(&memory, alignment, size);
    assert(!err);
    return memory;
}

