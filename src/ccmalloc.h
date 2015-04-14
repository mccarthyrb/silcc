#if !defined(CCMALLOC_C)
#define malloc(a) CCMalloc((a))
#define free(a)   CCFree((a))
#define strdup(a) CCStrDup((a))
#define realloc(memblock, size) CCRealloc((memblock), (size))
void * WINAPI CCMalloc(size_t size);
void WINAPI CCFree(void * memblock);
char * WINAPI CCStrDup(char * s);
void * WINAPI  CCRealloc(void * memblock, size_t size);
#else
#include <malloc.h>
#endif


