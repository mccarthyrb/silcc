/* ccutil.c
 * Old name: cc0b.c  12/85	Routines Contained in Root  GT
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
 *                     Name changed to cc0b.c
 *
 * This module provides several simple routines.
 *
 * Global routines in this module include:
 *
 *		odd( i ) -- Boolean:  TRUE == i is odd
 *		int i;
 *
 *		a_to_hex( ch ) -- Convert an ascii char to hex
 *		char ch;
 *
 *    char *bytcpy( d, s, n) --    Copy n bytes from s to d and return
 *    char *d, *s;                 the new value of d, which points to
 *    int n;                       the char just beyond the last one
 *                                 written to.
 *    SSINT *ssbytcpy( d, s, n) -- Copy n bytes from s to d and return
 *    SSINT *d, *s;                the new value of d, which points to
 *    int n;                       the char just beyond the last one
 *                                 written to.
 *    char *bytset( d, c, n) --    Set the n bytes starting at d to the
 *    char *d, c;                  character c and return the new value
 *    int n;                       of d, which points to the char beyond
 *                                 the last one written to.
 *
 *    SSINT *ssbytset( d, c, n) -- Set the n elements starting at d to the
 *    SSINT *d, c;                 SSINT c and return the new value
 *    int n;                       of d, which points to the element beyond
 *                                 the last one written to.
 *
 *    void resolve_predefined(int index)  Resolve value of predefined store.
 *
 *		long int long_abs( value ) -- Absolute value of a long int.
 *		long int value;
 *
 * int valnum( operand, opbuf, buflen, value, first )--Validate a numeric arg
 * register SSINT *operand;
 * char *opbuf;             Buffer for the operand, in case it's bad
 * int buflen;					 Length of opbuf
 * long int *value;
 * char first;			 Boolean: TRUE == first operand, so the first thing
 *													  we'll see is a store #
 *			Return:	 TRUE: operand is OK
 *						FALSE: not a number
 *							-1: number is too big (i.e. > 1,999,999,999)
 *
 *    void bailout(exit_code, ctrlc_flag) -- Exit with given error code.
 *    int exit_code          Error code
 *    int ctrlc_flag         True if bailing because user issued ^C
 *
 *    void upcase_str( char *s) -- convert string to upper case characters
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

#ifndef _WINDOWS
#include "keybrd.h"
#endif

#include <string.h>
#include <time.h>
#include <stdlib.h>
#if defined(UNIX)
#include "wtype.h"
#endif
#include "cc.h"
#include "c8type.h"

#ifdef _WINDLL
#include <setjmp.h>
#endif

#ifdef _WINDOWS
#include "cc08.h"
#else
extern void exit(int);
#endif

void upcase_str( char * );    /* convert string to upper case */

/****************************************************************************/
int odd(register int i)												/* Is numeric argument odd? */
/****************************************************************************/
{
    return(i & 1);
}

/****************************************************************************/
int a_to_hex(register char ch )					/* Convert an ASCII char to hex */
/****************************************************************************/
{
    ch = (char)toupper( ch );
    if ( isdigit( ch ) )
        return( ch - '0' );
    else
        return( ch - ('A' - 0x0a) );
}

/****************************************************************************/
char *bytcpy( d, s, n)							/* Forward byte move */
/****************************************************************************/
register char *d, *s;
register int n;
{
    while ( n-- > 0 )
        *d++ = *s++;
    return( d );
}

/****************************************************************************/
SSINT *ssbytcpy( d, s, n)                    /* Forward element move */
/****************************************************************************/
register SSINT *d, *s;
register int n;
{
    memmove(d, s, sizeof(SSINT) * n);
    return( d+n);
}

/****************************************************************************/
char *bytset( d, c, n)				 /* Set n bytes to the specified value */
/****************************************************************************/
register char *d;
register char c;
register int n;
{
    memset(d, c, n);
    return( d + n);
}

/****************************************************************************/
SSINT *ssbytset( d, c, n)           /* Set n elements to the specified value */
/****************************************************************************/
register SSINT *d;
register SSINT c;
register int n;
{
    int i;

    for ( i = 0; i < n; i++)
    {
        *d = c;
        d++;
    }
    return( d );
}

/****************************************************************************/
void resolve_predefined(int index)    /* Resolve value of predefined store  */
/****************************************************************************/
/*
 * Description:
 *             This routine resolves the value of predefined stores.   
 *             (Examples of these are cccurrentdate, cccurrenttime,
 *              ccversionmajor, and ccversionminor).
 *
 * Input: index -- the store number
 *
 * Globals output: predefinedBuffer -- this array gets the value of the
 *                 predefined store.  Note that this is set at the time
 *                 it is referenced, not when the change table is read.
 *
 */
{
    time_t long_time;           // seconds since time started on 1-1-1970
    struct tm *newtime;         // structure for time and date information

    switch (storepre[index])
    {
    case CCCURRENTDATE:
        time (&long_time);
        newtime = localtime(&long_time);
        sprintf(predefinedBuffer, "%02d/%02d/%4d", newtime->tm_mon + 1,
                newtime->tm_mday, 1900 + newtime->tm_year);
        break;
    case CCCURRENTTIME:
        time (&long_time);
        newtime = localtime(&long_time);
        sprintf(predefinedBuffer, "%02d:%02d:%02d", newtime->tm_hour,
                newtime->tm_min, newtime->tm_sec);
        break;
    case CCVERSIONMAJOR:
        sprintf(predefinedBuffer, "%s", VERSIONMAJOR);
        break;
    case CCVERSIONMINOR:
        sprintf(predefinedBuffer, "%s", VERSIONMINOR);
        break;
    default:
        predefinedBuffer[0] = 0;    // just put end of string marker
        break;                      // we should never default actually
    }
}

/****************************************************************************/
long int long_abs( value )		/* Absolute value of a long int */
/****************************************************************************/
long int value;
{
    if ( value < 0L )
        return( -(value) );
    else
        return( value );
} /* End--long_abs */

/****************************************************************************/
int valnum( operand, opbuf, buflen, value, first )  /* Validate numeric arg */
/****************************************************************************/
register SSINT *operand;
char *opbuf;            /* Buffer for the operand, in case it's bad */
int buflen;					/* Length of opbuf */
long int *value;
char first;			/* Boolean: TRUE == first operand, so the first thing
						 *							 we'll see is a store #
						 */
/*
 * Description:
 *						Check the operand to see if it's a valid numeric argument
 *					for the arithmetic functions.
 *						At the same time, copy the operand into opbuf so the caller
 *					can use it for displaying an error message.
 *
 * Return values:
 *						TRUE == operand is a valid number,
 *								  *value will contain its value
 *
 * Note: an empty store is assumed to have a value of zero
 *
 *						FALSE == operand is not a valid argument
 *
 *						-1 == number was too long (i.e. won't fit in a long int)
 *
 *						opbuf == the operand, as a NUL-terminated string.
 *									if the operand is longer than (buflen - 1), only
 *									  the first (buflen - 1) chars will be copied
 *
 * Globals input: storebegin -- array of pointers to the beginning of stores
 *						storend -- array of pointers to the end of stores
 *
 * Globals output: none
 *
 * Error conditions: If the operand is not a valid argument,
 *								FALSE will be returned, and *value will be undefined
 *							If the number won't fit in a long int,
 *								-1 will be returned, and *value will be undefined
 *
 * Other procedures called:
 *									atol -- convert an ASCII string to a long int
 *
 * Note: This procedure is called during compilation to check for
 *				valid numeric strings at compile time.
 */

{
    char  numbuf[20];       /* used for building the string for
                               conversion by atol */
    SSINT *end_of_operand;  /* Address of either the end of the operand
                             * or end of the store used by cont
                             */
    char firsthalf;         /* first half doublebyte pair of valid digits */

    register char *numptr;  /* Pointer for loading numbuf    */


    SSINT *opptr;  /* Pointer for finding end of operand     */

    int store,		/* store used by a cont command */
    i;				/* counter for digits (a 32-bit int is <= 10 digits) */

    buflen--;		/* Allow for the NUL */

    if ( (*operand == CONTCMD) || (*operand == VALCMD) || first )
    {
        if ( first )
            store = *operand;
        else
            store = *(++operand);
        end_of_operand = storend[store];
        operand = storebegin[store];
    }
    else
    {							/* Find the end of the operand */
        for ( opptr = operand; (!(*opptr & HIGHBIT) || (*opptr == TOPBITCMD));
                opptr++)
            ;
        end_of_operand = opptr;
    }

    if ( operand == end_of_operand )
    {
        *value = 0L;						/* Empty store or string, so value is zero */
        *opbuf = '\0';				/* Nothing there, so return an empty string */
        return( TRUE );
    }

    if ( (*operand == '-') || (*operand == '+') )		/* Catch the sign */
    {
        *opbuf++ = (char)*operand;            /* Copy it into the buffer */
        buflen--;
        numbuf[0] = (char)*operand++;
    }
    else
    {
        numbuf[0] = '+';							  /* Default to a positive number */
    }
    /* if just a + or - in the operand then this is not a valid number */

    if (operand == end_of_operand)
    {
        *opbuf = '\0';
        return( FALSE );			/* Invalid digit encountered */
    }

    for ( i = 1, numptr = numbuf + 1; (operand < end_of_operand) && (i < 19);
            operand++, numptr++, i++)
        if ( isdigit(*operand) )              /* DRC removed casting 8-19-96 */
        {
            // if we are in doublebyte mode and if the high order part of the
            // element *operand points to is also a valid digit, (and if we
            // have room in buffer for both halves of the number), then process
            // the high order firsthalf of the element before low order half
            if (doublebyte_mode == TRUE)
            {
                firsthalf = (char) (*operand >> 8);
                if (( isdigit( firsthalf )) && ( i < 18 ))
                {
                    *numptr++ = firsthalf;
                    i++;
                    if ( buflen )
                    {
                        *opbuf++ = (char)*operand;
                        buflen--;
                    }
                }
            }

            *numptr = (char)*operand;
            if ( buflen )
            {
                *opbuf++ = (char)*operand;
                buflen--;
            }
        }
        else
        {
            while( buflen && (operand < end_of_operand) )
            {														/* Get the rest of it */

                if ( *operand == TOPBITCMD )
                {
                    *opbuf++ = (*(operand + 1) | HIGHBIT);
                    operand += 2;
                }
                else
                    *opbuf++ =  (char)*operand++;
                buflen--;
            }
            *opbuf = '\0';
            return( FALSE );			/* Invalid digit encountered */
        }

    *numptr = '\0';						/* Terminate the strings */
    *opbuf = '\0';

    if ( (i > 11) || ((i == 11) && (numbuf[1] > '1')) )
        return( -1 );				/* Number was too big */

    *value = atol( numbuf );

    return( TRUE );
} /* End--valnum */

/************************************************************************/
void bailout(exit_code, ctrlc_flag)		/* Bail out with given exit code */
/************************************************************************/
int exit_code;		/* Exit code */
int ctrlc_flag;	/* TRUE if user is bailing out by pressing ^C */
{
#if !defined(UNIX)
#ifdef _WINDOWS
    CloseMessageFile(msgfile);
#ifdef _WINDLL
    if (abort_jmp_buf)
        longjmp(abort_jmp_buf, exit_code);
#else
    abort();
#endif

#else
    NOREF(ctrlc_flag);

    break_flag(org_break_flag);

    exit(exit_code);
#endif 	
#else
   exit(exit_code);
#endif 	
}

/******************************************************************/
void upcase_str( char *s )      /* force string to upper case */
/*-----------------------------------------------------------------
* Description -- Do toupper( ch ) on each character in string
* Return values -- none
* Globals input -- none
* Globals output -- none
* Error conditions -- none
*/
/******************************************************************/
{
    while( *s )
    {
        *s = (char) toupper( *s );
        s++;                                /* guard against side effects -
                                               toupper is a macro */
    }

}



/* END */
