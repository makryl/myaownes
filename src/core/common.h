#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MN_VERSION "0.0.0"

#define MN_TRACE_CPU 0
#define MN_TRACE_PPU 0
#define MN_TRACE_APU 0
#define MN_TRACE_NESTEST 0
#define MN_TRACE_BLARGG 0

#define MN_TRACE 1
#define MN_ERROR 1

#if MN_TRACE
#define tracef(...) printf(__VA_ARGS__)
#else
#define tracef(...) ((void)0)
#endif

#if MN_ERROR
#define errorf(...) fprintf(stderr, __VA_ARGS__)
#else
#define errorf(...) ((void)0)
#endif

#define MN_CACHE_LINE alignas(64)
#define MN_INLINE __attribute__((always_inline)) inline
