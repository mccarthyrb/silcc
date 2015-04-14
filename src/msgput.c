/* msgput.c - Routines for sending messages to screen
 *
 * Copyright (c) 1980-1996, 2015 by SIL International
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * Change History:
 *	01/24/90		ACR		Original. Created to encapsulate all message writing
 *								so that it if the screen has been initialized with
 *								the screen font library, the appropriate sf.lib
 *								calls can be made.
 *  13-Feb-91  DJE      CC only version. Removed SHARP and MS stuff.
 *   5-Nov-91  DJE      Added the write_putc function. Also changed such
 *                      that all msg_* messages go to stdout instead od stderr.
 *
 *	The following entry points are included:
 *
 * write_putc(int ch)             Put a character on the screen or in file
 *
 * msg_putc(ch)                   Put a character on the screen
 *	int c;
 *
 * msg_puts(s)                    Put a string on the screen
 *	char *s;
 *
 * msg_printf(fmt,...)            Put a formatted message on the screen
 *	char *fmt;
 *
 * Process_msg(int (*) (char *),  Put a formatted message out
 *   short, short unsigned,
 *   long unsigned              
 *
 */

#ifdef _WINDOWS
#include "windows.h"
#endif
#if defined(_WinIO)
#include "winio.h"
#else
#include <stdio.h>
#include <stdarg.h>
#endif
#if defined(UNIX)
#include "wtype.h"
#endif
#include "cc.h"
#if defined(UNIXSHAREDLIB)
#define _WINDLL
#define MessageBox(w,x,y,z) fprintf(stderr, "%s\n", x)
#define MessageBeep(x) fprintf(stderr, "***>")
#elif defined(UNIX)
#undef _WINDLL
#endif
#include "msgput.h"
#include "ccerror.h"

/************************************************************************/
int write_putc(int ch)		/* write or wrstore character to screen or file */
/************************************************************************/
{
    if( writefile )
        return( fputc(ch, writefile) );  /* write to file if there is one */
    else
        return( putc(ch, msgfile) );	/* else send ch to stdout */
}

/************************************************************************/
int msg_putc(ch)		/* Put a character to screen */
/************************************************************************/
int ch;				/* Character to put */
{
    return( putc(ch, msgfile) );	/* else send ch to stdout */
}

/************************************************************************/
int msg_puts(s)			/* Put a string to screen */
/************************************************************************/
char *s;			/* String to put */
{
    return( fputs(s, msgfile) );		/* else do fputs to stdout */
}

/************************************************************************/
int msg_printf( 		/* Write formatted message to screen */
    char * fmt,       /* Format String */
    ... )
// The old messages that were common between DOS and
// windows now use Process_msg (below).  This routine
// is now only used for DOS-specific messages, or
// in Windows only when we are in debugging mode.

/************************************************************************/
{
#if defined(_WINDLL) && !defined(UNIX)
    char *sss;
    char messageBuffer[200];
#endif
    va_list arg_ptr;			/* Pointer to variable number of arguments */
#if !defined(_WINDLL) || defined(UNIX)
    va_start(arg_ptr, fmt);		/* Initialize 'arg_ptr' */
    return( vfprintf(msgfile, fmt, arg_ptr) );	/* else send msg to stdout */
}
#else
    sss = (char *) &messageBuffer[0];
    va_start(arg_ptr, fmt);		/* Initialize 'arg_ptr' */
    vsprintf(sss, fmt, arg_ptr);   /* put message to this string  */
    MessageBeep(MB_ICONINFORMATION);
    MessageBox(NULL, sss, "Consistent Changes", MB_ICONINFORMATION);
    return(0);
}
#endif

/************************************************************************/
int Process_msg(CCErrorCallback * customErr,
                short nMsgIndex, short unsigned wParam, long unsigned lParam)
/************************************************************************/
/*
 *  NOTE: See comments at the start of ccerror.h that describe what
 *        this routine does, and how users would write their own
 *        error checking routine in Windows DLL mode.
 *
 */
{
    int errorType;

#ifdef _WINDLL
    int rc;
    char *msg_ptr;
    char msg_buffer[200];


    msg_ptr = (char *) &msg_buffer[0];

    // if hWindow is not NULL then we are not to use our own error handling
    // routine here, but instead we are to use user's customer error handler
    if (customErr != NULL)
    {
        rc = (* customErr) (nMsgIndex, wParam, lParam, &lDLLUserErrorCBData);
        return(rc);
    }
#endif


    errorType = errortable[nMsgIndex].errtype;
    switch (errorType)
    {


    case MSG_S_S:
#ifdef _WINDLL
        sprintf(msg_ptr, errortable[nMsgIndex].errmsg,
                ((MSG_STRUCT_S_S *) lParam)->string1,
                ((MSG_STRUCT_S_S *) lParam)->string2);
        break;
#else
        return( fprintf(msgfile, errortable[nMsgIndex].errmsg,
                        ((MSG_STRUCT_S_S *) lParam)->string1,
                        ((MSG_STRUCT_S_S *) lParam)->string2));
#endif

    case MSG_S_C_S:
#ifdef _WINDLL
        sprintf(msg_ptr, errortable[nMsgIndex].errmsg,
                ((MSG_STRUCT_S_C_S *) lParam)->string1,
                ((MSG_STRUCT_S_C_S *) lParam)->char1,
                ((MSG_STRUCT_S_C_S *) lParam)->string2);
        break;
#else
        return( fprintf(msgfile, errortable[nMsgIndex].errmsg,
                        ((MSG_STRUCT_S_C_S *) lParam)->string1,
                        ((MSG_STRUCT_S_C_S *) lParam)->char1,
                        ((MSG_STRUCT_S_C_S *) lParam)->string2));
#endif

    case MSG_S:
#ifdef _WINDLL
        sprintf(msg_ptr, errortable[nMsgIndex].errmsg, (char *) lParam);
        break;
#else
        return( fprintf(msgfile, errortable[nMsgIndex].errmsg,
                        (char *) lParam ) );
#endif

    case MSG_LD:
#ifdef _WINDLL
        sprintf(msg_ptr, errortable[nMsgIndex].errmsg,
                (long signed) lParam);
        break;
#else
        return( fprintf(msgfile, errortable[nMsgIndex].errmsg,
                        (long signed) lParam ) );
#endif

    case MSG_noparms:
    default:            // We should never just default actually!
#ifdef _WINDLL
        sprintf(msg_ptr, errortable[nMsgIndex].errmsg);
        break;
#else
        return( fprintf(msgfile, errortable[nMsgIndex].errmsg));
#endif

    }


    // actually put out the Windows DLL info/warning messages here
#ifdef _WINDLL
    if (errortable[nMsgIndex].msgtype == WARNING_MESSAGE)
    {
        MessageBeep(MB_ICONINFORMATION);
        rc= MessageBox(NULL, msg_ptr, "CC - Consistent Changes", MB_ICONINFORMATION | MB_OKCANCEL);
    }
    else
    {
        MessageBeep(MB_ICONHAND);
        rc= MessageBox(NULL, msg_ptr, "CC - Consistent Changes", MB_ICONHAND | MB_OKCANCEL);
    }
#if !defined(UNIXSHAREDLIB)
    if (rc == IDCANCEL)
        if (abort_jmp_buf)
            longjmp(abort_jmp_buf, BADEXIT);
#endif
    return(0);
#endif
}

/* END */
