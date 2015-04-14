/******************************************************************
 *   Buffered File IO routines using Windows I/O
 *   These routines somewhat mimic the behaviour of the standard C
 *   library routines. They use the Windows routines to do the
 *   actual I/O. This seems to solve a problem with calling the CC
 *   DLL from WinWord 6.0 WordBasic. When a DLL is called from
 *   WordBasic, the standard I/O library is not initialized and
 *   so seems to cause problems. These routines fix that. Note that
 *   these routines only implement a subset of the standard I/O routines.
 *   In particular, any file open for output must be explicitly closed
 *   before a program exits, or the output buffer will not be flushed.
 *
 *   Written by Doug Rintoul, Oct 16, 1996
 */ 
#if defined(UNIX)
#include "wtype.h"
#define DIRSEP '/'
#else
#include <windows.h>
#define DIRSEP '\\'
#endif
#include <stdio.h>
#include <string.h>
#if defined(_WINDLL) && !defined(WIN32) && !defined(STATICLINK)
#include "ccmalloc.h"
#elif defined(UNIX)
#include <stdlib.h>
#else
#include <malloc.h>
#endif
#if defined(UNIX)
#include <stdio.h>
#include <unistd.h>
#else
#include <io.h>
#include <direct.h>
#endif
#if defined(UNIX)
#include "wtype.h"
#include "cctio.h"
#endif
#include "cc.h"
#include "cctio.h"

void CloseFile(WFILE * stream)
{
#if defined(UNIX)
    fclose(stream->hfFile);
#else
    _lclose(stream->hfFile);
#endif
}

WFILE * wfopen(char * pszFileName, char * mode)
{
    WFILE * stream;
#if defined(WINDOWS)
    UINT fuMode;
#endif
    char szFileName[PATH_LEN];
    int len;
#if defined(WINDOWS)
    LONG dist;
    if (strcspn(pszFileName, "\\:") < strlen(pszFileName))
        strcpy(szFileName, pszFileName);
    else
#endif
#if defined(UNIX)
    if (*pszFileName == DIRSEP)
        strcpy(szFileName, pszFileName);
    else
#endif
    {
        getcwd(szFileName, PATH_LEN);
        len= strlen(szFileName);
        if (szFileName[len - 1] != DIRSEP)
        {
            szFileName[len]= DIRSEP;
            szFileName[len+1]= '\0';
        }
        strcat(szFileName, pszFileName);
    }

    stream = malloc(sizeof(WFILE));

    if (stream != NULL)
    {

        stream->iBufferFront= UNGETBUFFERSIZE;
        stream->iBufferEnd= UNGETBUFFERSIZE;
        stream->fEndOfFile= FALSE;

        if (strchr(mode, 'b'))
            stream->fTextMode= FALSE;
        else
            stream->fTextMode= TRUE;
#if defined(UNIX)
		stream->hfFile = fopen(szFileName, mode);
			
		if (stream->hfFile == NULL)
		{
			free(stream);
			stream= NULL;
		} else {
			if (strchr(mode, 'a') || strchr(mode, '+') || strchr(mode, 'w'))
				stream->fWrite= TRUE;
			else
				stream->fWrite= FALSE;
		}
	}
#else
        if (strchr(mode, 'r') || strchr(mode, 'a'))
        {
            if (strchr(mode, '+') || strchr(mode, 'a'))
                fuMode= OF_READWRITE;
            else
                fuMode= OF_READ;
        }
        else if (strchr(mode, 'w'))
        {
            if (strchr(mode, '+'))
                fuMode= OF_READWRITE | OF_CREATE;
            else
                fuMode= OF_WRITE | OF_CREATE;
        }

		if (fuMode != OF_READ)
			fuMode|= OF_SHARE_DENY_WRITE;

        stream->hfFile= OpenFile(szFileName, &(stream->OpenBuff), fuMode);

        if (strchr(mode, 'a'))
        {
            if (stream->hfFile == HFILE_ERROR)
            {
                if (strchr(mode, '+'))
                    fuMode= OF_READWRITE | OF_CREATE | OF_SHARE_DENY_WRITE;
                else
                    fuMode= OF_WRITE | OF_CREATE | OF_SHARE_DENY_WRITE;

                stream->hfFile= OpenFile(szFileName,  &(stream->OpenBuff), fuMode);
            }

            if (stream->hfFile != HFILE_ERROR)
            {
                dist= _llseek(stream->hfFile, 0, SEEK_END);
            }

        }


        if (stream->hfFile == HFILE_ERROR)
        {
            free(stream);

            stream= NULL;
        }
        else
        {
            stream->fuMode= fuMode & ~OF_CREATE;

            if (fuMode & (OF_WRITE | OF_READWRITE))
                stream->fWrite= TRUE;
            else
                stream->fWrite= FALSE;

        }
    }
#endif

    return stream;
}

int wfflush(WFILE * stream)
{
#if !defined(UNIX)
    int ReturnCode;
#endif
	
    if (stream->fWrite && stream->iBufferEnd != UNGETBUFFERSIZE)
    {
#if defined(UNIX)
        fwrite(stream->Buffer + UNGETBUFFERSIZE, stream->iBufferEnd - UNGETBUFFERSIZE, 1, stream->hfFile);
		
		if (ferror(stream->hfFile))
		{
			return EOF;
		}
#else
        ReturnCode= _lwrite(stream->hfFile, stream->Buffer + UNGETBUFFERSIZE, stream->iBufferEnd - UNGETBUFFERSIZE);
        if (ReturnCode == 0)
        {
            MessageBox(NULL, "Error flushing file", "CC.DLL", MB_OK);
            return EOF;
        }
        if (ReturnCode == HFILE_ERROR)
        {
            MessageBox(NULL, "Error flushing file", "CC.DLL", MB_OK);
            return EOF;
        }
#endif		
    }

    stream->iBufferFront= UNGETBUFFERSIZE;
    stream->iBufferEnd= UNGETBUFFERSIZE;

    return 0;
}

int wfclose(WFILE * stream)
{
#if !defined(UNIX)
    int ReturnCode;
#endif
    wfflush(stream);
#if defined(UNIX)
	fclose(stream->hfFile);
#else
    ReturnCode=  _lclose(stream->hfFile);
#endif
    free(stream);
#if (UNIX)
	if (ferror(stream->hfFile))
		return EOF;
#else
    if (ReturnCode	== HFILE_ERROR)
    {
        MessageBox(NULL, "Error closing file", "CC.DLL", MB_OK);
        return EOF;
    }
#endif
    return 0;
}

BOOL wfeof(WFILE * stream)
{
    if (stream)
        return stream->fEndOfFile;
    else
    {
        if (!lpszCCTableBuffer)
            return TRUE;
        else
            return *lpszCCTableBuffer == 0;
    }
}

int wgetc(WFILE * stream)
{
    int ch;

    if (!stream)
    {
        if (!lpszCCTableBuffer || !*lpszCCTableBuffer)
            return EOF;
        else
            return (*lpszCCTableBuffer++) & 0xFF;
    }

    if (stream->fEndOfFile)
        return EOF;

    else if (stream->iBufferFront == stream->iBufferEnd)
    {
        stream->iBufferFront= UNGETBUFFERSIZE;

#if defined(UNIX)
        stream->iBufferEnd= fread(stream->Buffer + UNGETBUFFERSIZE, 1, BUFFERSIZE, stream->hfFile) + UNGETBUFFERSIZE;
#else
        stream->iBufferEnd= _lread(stream->hfFile, stream->Buffer + UNGETBUFFERSIZE, BUFFERSIZE) + UNGETBUFFERSIZE;
#endif
        if (stream->iBufferEnd == UNGETBUFFERSIZE)
        {
            stream->fEndOfFile= TRUE;
            return EOF;
        }
    }

    ch= stream->Buffer[(stream->iBufferFront)] & 0xFF;

    (stream->iBufferFront)++;

    /* if text mode ignore CR */
    if (stream->fTextMode && ch == '\r')
        return wgetc(stream);
    else
        return ch;
}

int wungetc(int ch, WFILE * stream)
{
    if (!stream)
    {
        if (!lpszCCTableBuffer)
            return EOF;
        else
        {
            *--lpszCCTableBuffer= ch;

            return ch;
        }
    }

    if (stream->iBufferFront == 0)
        return EOF;

    stream->Buffer[--(stream->iBufferFront)]= ch;

    return ch;
}

int wfputc(int ch, WFILE * stream)
{
    int rc;

#if !defined(UNIX)
    if (stream->fTextMode && ch == '\n')
        if (wfputc('\r', stream) == EOF)
            return EOF;
#endif
		
    if (stream->iBufferEnd == BUFFERSIZE + UNGETBUFFERSIZE)
    {
#if defined(UNIX)
        rc= fwrite(stream->Buffer + UNGETBUFFERSIZE, BUFFERSIZE, 1, stream->hfFile);
        if (ferror(stream->hfFile))
            return EOF;		
#else
        rc= _lwrite(stream->hfFile, stream->Buffer + UNGETBUFFERSIZE, BUFFERSIZE);
		
        if (rc == HFILE_ERROR)
            return EOF;
#endif
        stream->iBufferEnd= UNGETBUFFERSIZE;
    }
    stream->Buffer[stream->iBufferEnd]= ch;
    (stream->iBufferEnd)++;

    return ch;
}
