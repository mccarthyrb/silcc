/* ccout.c
 * Old name: cc2e.c  12/85 Output Routines   AB
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
 * CHANGE HISTORY:
 *  15-Nov-89		 ACR Removed RT11 and CP/M stuff. Ported to Microsoft C.
 *  13-Feb-91      DJE CC only version. Removed SHARP and MS stuff.
 *                     Name changed to cc2e.c
 *  02-Jan-96      DRC added support for Windows DLL and out_char_buf routine
 *
 * This module provides the following global routines:
 *
 * out_char( ch ) -- Output a single character
 * SSINT ch;
 *
 *
 *
 *
 * This module provides the following local routine:
 *
 * out_char_buf( char inchar ) -- Output a single character to output buffer
 *                                or if applicable to auxiliary output buffer
 *
 *
 */

#ifdef _WINDOWS
#include "windows.h"
#endif

#if defined(_WinIO)
#include "winio.h"
#else
#include <stdio.h>
#endif
#include <string.h>
#if defined(UNIX)
#include "wtype.h"
#include <stdlib.h>
#endif
#include "cc.h"


/****************************************************************************/
void out_char( ch )                           /* Output a single character  */
/****************************************************************************/
SSINT ch;          /* Char to be output */
{
    SSINT ch_first;             /* first of doublebyte pair of characters */

#ifndef _WINDLL
    if ( outsize != -1L )		 /* If (outsize == -1) then unlimited size */
    {

        if ( outsize != 0 )
            outsize--;			/* update available length */
        else
        {
#ifdef _WINDOWS                                                   	//7.4.15
            Process_msg(errorFunction, 15, 0, 0);
            bFileErr = TRUE;
            return;
#else
            /* Get another output file */
            fclose( outfile );
            Process_msg(errorFunction, 16, 0, 0);
            getname("Enter new output file for next part of output: ",
                    filenm);
            openoutfile(filenm);
#endif
        }
    }
#endif

    /* if we are in doublebyte mode, and high order half of element
       is non-zero, then output doublebyte element as two bytes    */

    if (doublebyte_mode && ((ch / 256 != 0) || (doublebyte1st == 0 && doublebyte2nd == 0)))
    {
        ch_first = (ch >> 8) & 0x00ff;
#ifdef _WINDLL
        out_char_buf((char) ch_first);  // output first of two characters
#else
        fputc((char)ch_first, outfile); // output first of two characters
#endif
        ch = ch & 0x00ff;

        /* If after we have done everything the doublebyte characters
           that we put out as two characters no longer match the doublebyte
           input criteria, then alert the user with warning message.  */
        if ((ch_first < doublebyte1st) ||
                ((ch_first == doublebyte1st) && (ch < doublebyte2nd)))
            Process_msg(errorFunction, 17, 0, 0);
#ifdef _WINDLL
        // if we likely will just overflow our output buffer by one byte
        // (the second half of a doublebyte element) then just store that
        // byte and set a flag, and pick it up next time, instead of
        // allocating space etc to improve performance.
        if (( nUsedOutBuf >= nMaxOutBuf ) &&
                ( nCharsInAuxBuf == 0 ) && ( !bSavedDblChar ))
        {
            dblSavedChar = (char) ch;   // save character to output next time
            bSavedDblChar = TRUE;       // indicate we have a saved character
        }
        else
            out_char_buf((char) ch);    // output the character
    }
    else
    {
        out_char_buf((char) ch);       // output the character
    }
#else
    }
    fputc((char)ch, outfile);   /* output the character to output file */
#endif
}

#ifdef _WINDLL
/****************************************************************************/
void out_char_buf(char inchar )               /* Output a single character  */
/****************************************************************************/
/*
 *
 * Description: This is a Windows DLL-specific subroutine of the out_char()
 *              function above.  With the DLL we want to output to the user
 *              only a specific number of bytes at a time.  We can usually
 *              control this nicely with our logic, but at times like when
 *              we output a lot of data at EOF or when a match is found etc
 *              we will need to put out a lot of data.  If we would overflow
 *              our regular Windows DLL output buffer, then we will instead 
 *              put that to a separate buffer, and later copy that into the
 *              regular output buffer.  This routine checks for that case,
 *              and if we need to do that it allocates (or enlarges) the 
 *              new buffer, and moves the data there as appropriate.  Or in
 *              the usual case it will just copy data to the output buffer.
 *
 *
 */
{
    if (bOutputBufferFull)     // if would "overflow" output buffer
    {

        if ( nAuxLength > 0 )           // if we have already allocated area
        {
            if ( nCharsInAuxBuf >= nAuxLength )  // if area not big enough
            {                                 // then make it bigger
                nAuxLength = nAuxLength * 2;      // try doubling the size
                hAuxBuf = GlobalReAlloc((void *)hAuxBuf, nAuxLength,
                                        GMEM_ZEROINIT);
                if (hAuxBuf == NULL)
                {
                    errors = TRUE;
                    Process_msg(errorFunction, 60, 0, 0);
                    return;
                }
            }
        }
        else                            // need to allocate special area
        {
            nAuxLength = AUX_BUFFER_LEN; // set this as initial aux buffer size
            hAuxBuf = GlobalAlloc(GPTR, nAuxLength);  // allocate space

            if (hAuxBuf == NULL)
            {
                errors = TRUE;
                Process_msg(errorFunction, 61, 0, 0);
                return;
            }
            lpAuxBufStart = (char *)GlobalLock(hAuxBuf);  // lock space
            if (lpAuxBufStart == NULL)
            {
                errors = TRUE;
                Process_msg(errorFunction, 62, 0, 0);
                return;
            }
            lpAuxOutBuf = lpAuxBufStart;
            lpAuxNextToOutput = lpAuxBufStart;
        }
        *lpAuxOutBuf++ = inchar;        // put char to our special area
        nCharsInAuxBuf++;               // increment special area count
        bAuxBufUsed = TRUE;             // denote special area is used
    }
    else                               // would not overflow output buffer
    {
        *lpOutBuf++ = inchar;           // output character to output buffer
        nUsedOutBuf++;                  // count used bytes of output buffer

        if (nUsedOutBuf >= nMaxOutBuf)
        {
            if (lpOutProc)
            {
                CallOutputCallback(hActiveCCTable);


                nUsedOutBuf = 0;             // note we emptied buffer

                lpOutBuf = lpOutBufStart;    // point to start of buffer again
            }
            else
                bOutputBufferFull= TRUE;
        }
    }
}

#endif

/* END */
