#if !defined(_WTYPE_H)
#define _WTYPE_H
typedef int                 BOOL;

typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef unsigned long       DWORD;

typedef unsigned int        UINT;

typedef signed long         LONG;
typedef const void *        HANDLE;
typedef const void *		HINSTANCE;
typedef char * LPSTR;
typedef void * LPVOID;
#define WINAPI
#define FAR
#define _export

#define GlobalLock(x) x
#define GlobalAlloc(x,y) malloc(y)
#define GlobalFree(x) free((void *)x)
#define GlobalReAlloc(x, y, z) realloc(x, y)
#endif
