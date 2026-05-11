#pragma once

#include <stdio.h>

#define MN_TRACE_CPU 0
#define MN_TRACE_PPU 0
#define MN_TRACE_BLARGG 0

#ifndef MN_TRACE
#define MN_TRACE 1
#endif

#ifndef MN_ERROR
#define MN_ERROR 1
#endif

#ifdef MN_TRACE
#define tracef(...) printf(__VA_ARGS__)
#else
#define tracef(...) ((void)0)
#endif

#ifdef MN_ERROR
#define errorf(...) fprintf(stderr, __VA_ARGS__)
#else
#define errorf(...) ((void)0)
#endif
