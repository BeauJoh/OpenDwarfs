#ifndef __COMMON_MEMORY_H__
#define __COMMON_MEMORY_H__

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__MACH__)
void* memalign(int alignment, int size);
#else
#include <malloc.h>
#endif

#ifdef __cplusplus
}
#endif

#endif// __COMMON_MEMORY_H__
