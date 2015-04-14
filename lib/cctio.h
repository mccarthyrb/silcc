#if !defined(_CCTIO_H)
#define _CCTIO_H

#define BUFFERSIZE 2048
#define UNGETBUFFERSIZE 100

typedef struct
{
    char Buffer[BUFFERSIZE+UNGETBUFFERSIZE];
    unsigned iBufferFront;
    unsigned iBufferEnd;
    BOOL fEndOfFile;
#if defined(UNIX)
	FILE * hfFile;
#else
    HFILE hfFile;
#endif
    BOOL fTextMode;
    BOOL fWrite;
    long FilePos;
    UINT fuMode;
} WFILE;

WFILE * wfopen(char * pszCCTFile, char * mode);
int wfclose(WFILE * stream);
int wfeof(WFILE * stream);
int wgetc(WFILE * stream);
int wungetc(int ch, WFILE * stream);
int wfputc(int ch, WFILE * stream);
int wfflush(WFILE * stream);
#endif
