#include <windows.h>
#include <stdio.h>
#include <string.h>
#define CCMALLOC_C
#include "ccmalloc.h"

void * WINAPI _export CCMalloc(size_t size)
{
    return malloc(size);
}

void WINAPI _export CCFree(void * memblock)
{
    free(memblock);
}

char * WINAPI _export CCStrDup(char * s)
{
    return strdup(s);
}

void * WINAPI _export CCRealloc(void * memblock, size_t size)
{
    return realloc(memblock, size);
}

