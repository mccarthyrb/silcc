/* ccexec.c
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
 *                     Name changed to cc2d.c
 *  04-Mar-91      DJE Fixed PRECCMD processing so that it backed around
 *                     the input buffer correctly. (ver cc7b.4e)
 *  30-Apr-91		 BLR prec() command was failing when it had to back around
 *							  the end of backbuf[].
 *	25-Jan-95		BJY Fixed DUPCMD and OUTCMD in execute() to check uppercase
 * 
 * This module provides the following global routines:
 *
 * debugwrite( ch ) -- Write the character for debug (/d) option display.
 * int ch;
 *
 * displbefore() -- Do the "before change" and "show match" portions of
 *							 the debug display.
 *
 * displafter() -- Do the "after change" portion of the debug display.
 *
 * check_kbd()	-- Check for ^C or ^S and bail out if necessary.
 *
 * writestore( j ) -- Write the contents of the specified store to the screen
 *     or file.
 * int j;
 *
 * execute( mlen, tpx ,beginflag) -- Execute the replacement part of a change.
 * int mlen;  Length of matched string 
 * SSINT *tpx;   Start of string to execute
 * int beginflag;  If TRUE we are executing the 'begin' section
 *
 * int anycheck( mc ) -- Boolean function: Is mc equal to any of the 
 *     SSINT mc;                             characters in the specified
 *															store?
 *
 * int contcheck() -- Boolean function: Is the next part of the trial match
 *														equal to the contents of the
 *														specified store?
 *
 * int leftexec() -- Execute commands on the left side of the WEDGE.
 *							  Returns TRUE if command gives a successful match,
 *							  FALSE otherwise.
 *
 * startcc() -- Start up Consistent Changes.
 *
 * fillmatch() -- Fill match area, execute begin section if there is one
 *
 * ccin() -- Do CC until either: one character has been output or
 *											one change has been performed.
 *
 * gmatch( group ) -- Check through one group for a possible match.
 * int group;
 *
 * int cmatch( cp, ofs ) -- Attempt to match a change, letter by letter.
 * cngtype cp;
 * int ofs;
 *
 *
 *
 * completterset() -- Compute current first-letter set for matching.
 *
 */

#if defined(_WinIO)
#include "windows.h"
#include "winio.h"
#include "wmhandlr.h"
#else
#include <stdio.h>
#if defined(_WindowsExe) || defined(_WINDLL)
#include "windows.h"
#include "ccdll\cctio.h"
#endif
#endif
#include <string.h>
#if defined(_WINDLL) && !defined(WIN32) && !defined(STATICLINK)
#include "ccmalloc.h"
#else
#if defined(UNIX)
#include <stdlib.h>
#else
#include <malloc.h>
#endif
#endif

#if !defined(UNIX)
#include <conio.h>
#endif

#if defined(UNIX)
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif


/* #include <stdlib.h> */
#if defined(UNIX)
#include "wtype.h"
#endif
#include "cc.h"
#include "c8type.h"
#include "keybrd.h"
#include "msgput.h"
#include "utf8.h"
#if defined (_WINDLL)
static unsigned FindLineNumber(tbltype matchposition);
#endif
#if defined(UNIX)

int key_waiting(void)
{
	struct timeval tv;
	fd_set in;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_ZERO(&in);
	FD_SET(0, &in);
	return (select(1, &in, 0, 0, &tv) > 0);    
}

int get_keystroke()
{
	int c;
	read(0, &c, 1);
	
	return c;
}
#endif

#if !defined(_WINDOWS) && !defined(UNIXSHAREDLIB)
/****************************************************************************/
void debugwrite(register int ch)		  /* Display a char in debug display (/d option) */
/****************************************************************************/
{
    if (ch == CARRIAGERETURN)
        msg_putc('\270');							/* Upper corner */
    else if (ch == CTRLZ)
        msg_putc('\260');							/* Checkerboard */
    else
        msg_putc(ch);
}/* End--debugwrite */

/****************************************************************************/
void displbefore()  /* Do debug display of BEFORE performance of change */
/****************************************************************************/

/* This procedure does the debug display before the performance of a
 *  change.
 *
 * Description -- Write a <NEWLINE>, to be sure we're at the beginning
 *						  of a line  (output may be going to SCREEN: ).
 *
 *						If currently storing
 *							Display the contents of the store, on a single line.
 *
 *						Display up to BEFORE_CNT characters of the ring buffer.
 *
 *						If currently at a match
 *							Display the match, in inverse video.
 *
 *						Display the next AFTER_CNT characters of input.
 *
 *						Write a <NEWLINE>.
 *
 * Return values: none
 *
 * Globals input: curstor -- number of active store, if storing
 *						storebegin -- array of pointers to the beginning of stores
 *						storend -- array of pointers to the end of stores
 *						backinptr -- input pointer for the ring buffer
 *						backoutptr -- output pointer for the ring buffer
 *						matchpntr -- pointer to the beginning of a match
 *						mchptr -- pointer to the end of a match
 *
 * Globals output: none
 *
 * Error conditions: none
 *
 * Other functions called: debugwrite -- display a char on the screen
 *
 */

{
    register tbltype tp;  /* Working pointer for going through the store */
    register SSINT *cp;   /* Working pointer for displaying everything else */
    int offset;

    if (curstor != 0)
    {										 /* Display current contents of the store */
        msg_printf("Store %s contains: [", sym_name(curstor, STORE_HEAD));
        for ( tp = storebegin[curstor]; tp < storend[curstor]; )
            debugwrite( *tp++);
        msg_puts("]\n");
    }

    /* Display up to BEFORE_CNT characters from the ring buffer */
    offset = (backinptr - backbuf) - BEFORE_CNT;
    if (offset < 0)
        offset += BACKBUFMAX;
    cp = backbuf + offset;
    while (cp != backinptr)
    {
        debugwrite( *cp++);
        if (cp >= backbuf+BACKBUFMAX)
            cp = backbuf;
    }

    msg_printf(CRT_REVERSE);

    /* Show the match */
    for (cp = matchpntr - cnglen; cp < matchpntr; cp++)
        debugwrite( *cp);

    /* End inverse video */
    msg_printf(CRT_REGULAR);

    /* Display up to the next AFTER_CNT chars of input */
    for (cp = matchpntr; cp < matchpntr+AFTER_CNT-cnglen; cp++)
        debugwrite( *cp);
    msg_putc('\n');

} /* End--displbefore */

/****************************************************************************/
void displafter()		  /* Do debug display of AFTER performance of change */
/****************************************************************************/

/* This procedure does the debug display after a change is performed.
 *
 * Description -- Display up to BEFORE_CNT chars from the ring buffer.
 *
 *						Display the AFTER_CNT chars starting at the current match,
 *						  followed by a <NEWLINE>.
 *
 *						Display the numbers of any switches set and whatever
 *						  groups are currently being searched.
 *
 *						If storing
 *							Display the current contents of the active store.
 *
 * Return values: none
 *
 * Globals input: backinptr -- input pointer for the ring buffer
 *						backoutptr -- output pointer for the ring buffer
 *						matchpntr -- pointer to the current match
 *						switches -- array of booleans to represent switches
 *						numgroupson -- number of groups currently being searched
 *						curgroups -- array of group-numbers for groups
 *											currently being searched
 *						curstor -- number of active store, if currently storing
 *
 * Globals output: none
 *
 * Error conditions: none
 *
 * Other functions called: debugwrite -- display a char on the screen
 *
 */

{
    register tbltype tp;
    register SSINT *cp;
    register int j;
    char *cpcp;
    int offset;
    int first;
    int ch;

    /* Display up to BEFORE_CNT characters from the ring buffer */
    offset = (backinptr - backbuf) - BEFORE_CNT;
    if (offset < 0)
        offset += BACKBUFMAX;
    cp = backbuf + offset;
    while (cp != backinptr)
    {
        debugwrite( *cp++);
        if (cp >= backbuf+BACKBUFMAX)
            cp = backbuf;
    }

    /* Display the next AFTER_CNT chars after the match */
    for (cp = matchpntr; cp < matchpntr+AFTER_CNT; )
        debugwrite( *cp++);
    msg_putc('\n');

    /* Display active groups */
    msg_printf("Active groups: ");
    for (first = TRUE, j = 1; j <= numgroupson; j++)
    {
        cpcp = sym_name(curgroups[j], GROUP_HEAD);  /* Get symbolic name */
        if (first)
        {
            msg_printf("%s", cpcp);
            first = FALSE;
        }
        else
            msg_printf(", %s", cpcp);
    }
    msg_putc('\n');	/* newline */

    /* Display the switches (if any) that are set */
    for (first = TRUE, j = 0; j <= MAXARG; j++)
        if (switches[j])
        {
            cpcp = sym_name(j, SWITCH_HEAD);   /* Get symbolic name */
            if (first)
            {
                msg_printf("Switches set: %s", cpcp);
                first = FALSE;
            }
            else
                msg_printf(", %s", cpcp);
        }
    if (!first)
        msg_putc('\n');	/* newline */

    /* Display the contents of the active store */
    if (curstor != 0)
    {
        msg_printf("Store %s contains: [", sym_name(curstor, STORE_HEAD));
        for ( tp = storebegin[curstor]; tp < storend[curstor]; )
            debugwrite( *tp++);
        msg_puts("]\n");
    }

    /* Display extra blank line */
    msg_putc('\n');											  /* <NEWLINE> */

    /* Check keyboard for ^S, ^C or possible change back to single step */
    check_kbd();

    /* Check for single step mode */
    if (single_step)
    {
        msg_puts("Press any key for next step or <ESC> for continuous: ");
        ch = get_keystroke();
        if (ch == ESCAPE)
        {
            single_step = FALSE;
            msg_puts("\nPress any key to resume single step mode.\n");
        }
        else if (ch == CTRLC)
            bailout(BADEXIT, TRUE);
        msg_putc('\n');
    }

} /* End--displafter */

/************************************************************************/
void check_kbd()		/* Check for ^C or ^S */
/************************************************************************/
{
    int ch;

    /* If there is a key waiting, then check it for ^C or ^S. */
    if (key_waiting())
    {
        ch = get_keystroke();			/* Read the keystroke */
        if (ch == CTRLS)					/* If it's a ^S then... */
        {									/* wait for another keypress */
            while (!key_waiting())
                ;
            ch = get_keystroke();		/* and read whatever it is */
        }
        else
            single_step = TRUE;			/* any other key turns on single step */

        if (ch == CTRLC)					/* If it's a ^C then bail out */
            bailout(BADEXIT, TRUE);		/* (possibly even after ^S) */
    }

}
/* END of check_kbd */
#endif

/****************************************************************************/
void writestore(j)			  /* Write contents of store j to the screen */
/****************************************************************************/
register int j;

/* Description:
 *						This procedure writes out on the screen the contents of the
 *					specified store.
 *
 * Return values: none
 *
 * Globals input: storebegin -- array of pointers to the beginning of stores
 *						storend -- array of pointers to the end of stores
 *
 * Globals output: none
 *
 * Error conditions: none
 *
 * Other functions called: none
 *
 */

{
    register tbltype tp;  /* Working pointer for going through the store */

    for ( tp = storebegin[j]; tp < storend[j]; tp++ )
        if ( *tp == CARRIAGERETURN && !binary_mode)
            write_putc('\n');
        else
            write_putc(*tp);

} /* End--writestore */

/****************************************************************************/
SSINT * execute(int mlen, SSINT * tpx, int beginflag)	/* Execute replacement part of a change */
/****************************************************************************/
/* Description:
 *						This procedure executes the replacement part of a change,
 *					including output and switches.  It is used both for
 *					begin and for replacements.
 *
 * Return values: point to where table was executed
 *
 * Globals input: uppercase -- boolean: TRUE == uppercase character
 *						switches -- array of booleans, each representing
 *										  one switch
 *						curstor -- if non-zero, number of the currently active store
 *						storend -- array of pointers to the ends of stores
 *						storebegin -- array of pointers to the beginnings of stores
 *						storeoverflow -- boolean: TRUE == overflow has occurred in a
 *																		store
 *						storeact -- array of pointers to stores which are currently
 *										  being used in matches
 *						setcurrent -- boolean: TRUE == the set of first letters of
 *																	stores being used in
 *																	matches is up to date
 *						backinptr -- input pointer for the ring buffer
 *						backoutptr -- output pointer for the ring buffer
 *						matchpntr -- pointer to where to start looking for a match
 *						match -- pointer to start of current match
 *						stacklevel -- stack pointer for stack used in executing
 *											 DEFINEs
 *						numgroupson -- number of groups currently being searched
 *											  when looking for a match
 *						defarray -- array of pointers to the beginning of DEFINEs
 *						stack -- array used as a stack for executing DEFINEs
 *						eof_written -- boolean: TRUE == CC has output EOF, so
 *																	 all groups should be
 *																	 turned off
 *
 * Globals output: everything updated as a result of executing the current
 *							change
 *
 * Error conditions:  If an error occurs, a message will be displayed on
 *								the screen.  In some cases, an error flag will be
 *								set.
 *
 * Other functions called: tblskip -- skip to the END for this change
 *									output -- move a byte out of the backup buffer
 *									storematch -- compare a store to the next
 *														 thing in the change table
 *									groupinclude -- add a group to the list of
 *															groups being searched
 *									groupexclude -- remove a group from the list of
 *															groups being searched
 *									writestore -- write the contents of a store
 *														 to the screen
 *									displbefore -- do the "before" debug display
 *									displafter -- do the "after" debug display
 *									ccmath -- execute math functions
 *													(incr, add, sub, mul, div)
 *
 */

{
    register SSINT ch, *cp;        /* current character from table */
    register SSINT tempch;         /* character from dup or store buffer */
    register int i;                /* Generic loop index */
    register unsigned j, k;        /* Unsigned loop indices */
    register tbltype tp;
    SSINT cha;                     /* next character in table */
    bool bOmitDone;                /* true only if fwd or omit done with this match */
#if defined(_WINDLL)
    int bContinue;
#endif

    if (!dupptr)
    {
        dupptr = malloc(mlen * sizeof(SSINT));
        memcpy(dupptr, matchpntr - mlen, mlen * sizeof(SSINT));          /* Save initial matchpntr */
    }
#if !defined(_WINDOWS) && !defined(UNIXSHAREDLIB)
    if ( debug == 1 )											/* Display match */
        displbefore();
#endif		
    //	stacklevel = 0;
    bOmitDone = FALSE;

    while (( *tpx != SPECPERIOD) &&
#if defined(_WINDLL)
            (nUsedOutBuf < nMaxOutBuf) &&
            !bNeedMoreInput &&
#endif
            ( !bOmitDone ))
    {
#if defined(_WINDLL)
        if (lpExecuteLineCallback != NULL)
        {
            unsigned iLine;

            iLine= FindLineNumber(tpx);

            if (iLine != iCurrentLine)
            {
                iCurrentLine= iLine;
#if !defined(DEBUGLIB)
                SaveState(hActiveCCTable);
#endif
                bContinue= (*lpExecuteLineCallback)(hActiveCCTable, iCurrentLine);
#if !defined(DEBUGLIB)
                RestoreState(hActiveCCTable);
#endif				
                if (!bContinue)
                {
                    free(dupptr);
                    dupptr= NULL;
                    longjmp(abort_jmp_buf, 0);
                }
            }
        }
#endif		   
        ch= *tpx++;

        if ( (ch & HIGHBIT) == 0 )
        {											/* Not command, so output */
            if (uppercase)
            {
                ch = (char)toupper(ch);
                uppercase = FALSE;
            }
            /* if we are in doublebyte mode and if the first character
               we are looking at qualifies as the first half of a doublebyte
               pair then look at the next byte, if it also qualifies then
               store the pair of bytes as one doublebyte element.  Otherwise
               do business as normal (store one char as one SSINT element).
            */
            if ( (doublebyte_mode==TRUE) && (ch >= doublebyte1st))
            {
                cha = *tpx++;
                if ( cha >= doublebyte2nd )
                    output((SSINT)((ch * 256) + ((int) (cha & 0x00ff))));  /* combine both as one */
                else
                {
                    output(ch);
                    tpx--;                  /* back up for next time thru */
                }
            }
            else
                output(ch);
        }
        else
        {													/* See if command */
            switch (ch)
            {
            case IFCMD:											/* If */
                if ( !switches[*tpx++] )
                    tblskip( &tpx);
                break;
            case IFNCMD:										/* Ifn */
                if ( switches[*tpx++] )
                    tblskip( &tpx);
                break;
            case ELSECMD:										/* Else */
                tblskip( &tpx);
                break;
            case ENDIFCMD:										/* Endif */
                break;
            case SETCMD:										/* Set */
                switches[*tpx++] = TRUE;
                break;
            case CLEARCMD:										/* Clear */
                switches[*tpx++] = FALSE;
                break;
            case STORCMD:										/* Store */
                curstor = *tpx++;
                storend[curstor] = storebegin[curstor];
                storeoverflow = FALSE;
                if ( storeact[curstor] )
                    setcurrent = FALSE;
                break;
            case APPENDCMD:									/* Append */
                curstor = *tpx++;
                break;
            case ENDSTORECMD:									/* Endstore */
                curstor = 0;
                break;
            case PUSHSTORECMD:
                PushStore();
                break;
            case POPSTORECMD:
                PopStore();
                break;
            case OUTCMD:											/* Out */
                curstor = 0;						/* Prevent storing during output */

            case OUTSCMD:											/* Outs */
                i = *tpx++;										/* Get store # */
                k = storend[i] - storebegin[i];		/* Get # of chars in store */

                // if nothing in the store and predefined flag is non-zero then
                // we are to output one of the predefined output values
                if (( k == 0 ) && ( storepre[i] != 0 ))
                {
                    resolve_predefined(i);
                    for ( j = 0; j < strlen(predefinedBuffer); j++)
                        output((SSINT) predefinedBuffer[j]);
                }

                /*   Explicit array indexing is used here because input may be going */
                /* to a lower-numbered store.  This could cause the contents of		*/
                /* store[i] to move elsewhere in memory, giving erroneous results.	*/

                for ( j = 0; j < k; j++ )
                {
                    tempch = *(storebegin[i] + j);	// 7.4.16
                    if (uppercase)					// first char may need capitalized
                    {                           //
                        tempch = toupper( tempch);  //
                        uppercase = FALSE;          //
                    }                           //
                    /* if we are in doublebyte mode and if the element we are
                       looking at has non-empty high order half, then output
                       it as two separate bytes.
                    */
                    if ( (doublebyte_mode==TRUE) && (tempch / 256 != 0) )
                    {
                        cha = (tempch >> 8) & 0x00ff;  /* get hi order half of doublebyte char */
                        tempch = tempch & 0x00ff; /* keep low order half here  */
                        output(cha);              /* output high order half first */

                        /* If after we have done all our operations to the stored data
                           it no longer matches the doublebyte input criteria, still
                           split it back to two bytes and output it, but give warning
                           to the user that (s)he may have hosed up their data.  */
                        if ((cha < doublebyte1st) ||
                                ((cha == doublebyte1st) && (tempch < doublebyte2nd)))
                            Process_msg(errorFunction, 17, 0, 0);
                    }
                    output( tempch );
                }
                break;
            case BACKCMD:                                      /* Back */
                BackCommand(&tpx, utf8encoding);
                break;
            case BACKUCMD:                                      /* Back utf-8 encoding */
                BackCommand(&tpx, TRUE);
                break;
            case DUPCMD:													/* Dup */
                uppercase = FALSE;	/* 7.4.16 ensure following literal is lowercase */
                for ( cp = dupptr; cp < dupptr + mlen; cp++)
                    output( *cp);

                if (endoffile_in_match)
                {
                    curstor = 0;
                    numgroupson = 0;		  /* All groups off for any further endfile */
                    setcurrent = FALSE;
                    eof_written = TRUE;
                }

                break;
            case IFEQCMD:													/* Ifeq */
                if ( storematch(&tpx) != 0 )
                    tblskip(&tpx);
                break;
            case IFNEQCMD:													/* Ifneq */
                if ( storematch(&tpx) == 0 )
                    tblskip(&tpx);
                break;
            case IFGTCMD:													/* Ifgt */
                if ( storematch(&tpx) <= 0 )
                    tblskip(&tpx);
                break;
            case IFNGTCMD:                                     /* Ifngt */
                if ( storematch(&tpx) > 0 )
                    tblskip(&tpx);
                break;
            case IFLTCMD:                                      /* Iflt */
                if ( storematch(&tpx) >= 0 )
                    tblskip(&tpx);
                break;
            case IFNLTCMD:                                     /* Ifnlt */
                if ( storematch(&tpx) < 0 )
                    tblskip(&tpx);
                break;
            case LENCMD:
                LengthStore(&tpx);
                break;
            case IFSUBSETCMD:
                if ( IfSubset(&tpx) == 0 )
                    tblskip(&tpx);
                break;
            case NEXTCMD:													/* Next */
                while ( *tpx++ != SPECWEDGE)
                    ;
                break;
            case BEGINCMD:													/* Begin */
                break;
            case ENDCMD:													/* End */
                break;
            case REPEATCMD:												/* Repeat */
                while ( *(tpx - 1) != SPECWEDGE && *--tpx != BEGINCMD )	// 7b.43 ALB stop at wedge if no begin found
                    ;
                break;
            case USECMD:													/* Use */
                numgroupson = 0;	  					/* Compile puts incl next */
                break;
            case INCLCMD:													/* Include */
                groupinclude( *tpx++ );
                break;
            case EXCLCMD:													/* Exclude */
                groupexclude( *tpx++ );
                break;
            case GROUPCMD:													/* Group */
                Process_msg(errorFunction, 41, 0, 0);
                break;
            case DOCMD:														/* Do */
                if (stacklevel >= STACKMAX)
                    Process_msg(errorFunction, 42, 0,
                                (long unsigned) STACKMAX);
                else
                {
                    if ((tp = defarray[*tpx++]) == NULL)
                        Process_msg(errorFunction, 26, 0,
                                    (long unsigned) sym_name(*(tpx-1), DEFINE_HEAD));
                    else
                    {
                        stack[++stacklevel] = tpx;
                        tpx = tp+1;
                    }
                }
                break;
            case TOPBITCMD:								/* Number with top bit on */
                output((SSINT)((*tpx++) | HIGHBIT));
                break;
            case FWDCMD:                                       /* Fwd  */
            case OMITCMD:                                      /* Omit */
                FwdOmitCommand(&tpx, &bOmitDone, (bool) (ch == FWDCMD), utf8encoding);
                break;
            case FWDUCMD:                                       /* Fwd utf-8 */
            case OMITUCMD:                                      /* Omit utf-8 */
                FwdOmitCommand(&tpx, &bOmitDone, (bool) (ch == FWDUCMD), TRUE);
                break;
            case INCRCMD:													/* Incr */
                incrstore( *tpx++ );
                break;
            case DECRCMD:                                                                                                   /* Decr */
                decrstore( *tpx++ );
                break;
            case ADDCMD:													/* Add */
            case SUBCMD:													/* Sub */
            case MULCMD:													/* Mul */
            case DIVCMD:													/* Div */
            case MODCMD:													/* Mod */
                ccmath( ch, &tpx );
                break;
            case WRITCMD:													/* Write */
#if  !defined(_WINDOWS) && !defined(UNIXSHAREDLIB)
                while ( TRUE )
                {
                    if ( *tpx == TOPBITCMD )                           // 7b.4r4 ALB Fix write of upper ASCII char
                        write_putc( (char)(*++tpx | HIGHBIT) );   // 7.4.15 ALB Further fix of write of upper ASCII
                    else if ( *tpx & HIGHBIT || *tpx == CTRLZ )
                        break;
                    else if ( *tpx == CARRIAGERETURN  && !binary_mode)
                        write_putc('\n');
                    else
                        write_putc(*tpx);
                    tpx++;
                }
                break;
#else
                //             for windows (including windows DLL) ignore the write command
                //             (we do not want to put the target out to the screen)
                while ( TRUE )
                {
                    if ( *tpx & HIGHBIT || *tpx == CTRLZ )
                        break;
                    tpx++;
                }
                break;
#endif					
            case WRSTORECMD:												/* Wrstore */
                //             for windows (including windows DLL) ignore wrstore command
#if !defined(_WINDOWS) || !defined(UNIXSHAREDLIB)
                i = *tpx;                           /* Get store # */
                k = storend[i] - storebegin[i];		/* Get # of chars in store */

                // if nothing in the store and predefined flag is non-zero then
                // we are to wrstore one of the predefined output values
                if (( k == 0 ) && ( storepre[i] != 0 ))
                {
                    resolve_predefined(i);
                    for ( j = 0; j < strlen(predefinedBuffer); j++)
                    {
                        if ( writefile )
                            fputc(predefinedBuffer[j], writefile);
                        else
                            putc(predefinedBuffer[j], msgfile);
                    }
                    tpx++;                 // increment beyond this element
                }
                else
                    writestore( *tpx++);   // write the store
#else
                tpx++;                              // skip this element
#endif
                break;
            case READCMD:													/* Read */
#if !defined(_WINDOWS) && !defined(UNIXSHAREDLIB)
                //             for windows (including windows DLL) ignore the read command
                while ((*line = (char)get_char()) != '\n')
                    output(*line);
#endif					
                break;
            case CASECMD:													/* Caseless */
                if (beginflag)
                {
                    if (doublebyte_mode)
                        Process_msg(errorFunction, 1, 0, 0);
                    else
                        caseless = TRUE;
                }
                else
                    Process_msg(errorFunction, 45, 0, 0);
                break;
            case UTF8CMD:													/* Caseless */
                if (beginflag)
                {
                    if (doublebyte_mode)
                        Process_msg(errorFunction, 1, 0, 0);
                    else
                        utf8encoding = TRUE;
                }
                else
                    Process_msg(errorFunction, 45, 0, 0);
                break;
            case BINCMD:												/* Binary mode */
                if (!beginflag)
                    Process_msg(errorFunction, 46, 0, 0);
                break;
            case DOUBLECMD:                                                                                         /* Doublebyte mode */
                if (!beginflag)
                    Process_msg(errorFunction, 47, 0, 0);
                ch = (*tpx++);   /* skip past the following (already processed) argument */
                break;
            case UNSORTCMD:                                 /* Unsort mode */
                if (!beginflag)
                    Process_msg(errorFunction, 48, 0, 0);
                break;
            case ENDFILECMD:
                curstor = 0;
                numgroupson = 0;		  /* All groups off for any further endfile */
                setcurrent = FALSE;
                eof_written = TRUE;
                break;


            default:
                Process_msg(errorFunction, 49, 0,
                            (long unsigned) cmdname((char)ch, FALSE) );
            }		  /* end of switch */
        }			/* else */

        /* unstack any do in effect */
        while ( (stacklevel > 0) && ((ch=(*tpx)) == SPECPERIOD) )
            tpx = stack[stacklevel--];

    } /* End--while */

    if (*tpx == SPECPERIOD ||
            bOmitDone)
    {
        free(dupptr);
        dupptr= NULL;
        tpx= NULL;
    }

    if ( (numgroupson == 0) && !eof_written )
    {													/* NO active groups--oops! */
        Process_msg(errorFunction, 50, 0,
                    (long unsigned) sym_name(cgroup, GROUP_HEAD) );
        bailout(BADEXIT, FALSE);					/* Bail out! */
    }

#if !defined(_WINDOWS)  && !defined(UNIXSHAREDLIB)
    if ( debug == 1 )
        displafter();
#endif
    return tpx;

} /* End--execute */

/****************************************************************************/
int anycheck(SSINT mc)                                                /* Is item in the specified store? */
/****************************************************************************/

/* Description:
 *   This procedure checks to see if the argument is equal to any of the
 * characters in the specified store area.  If it is, it returns TRUE,
 * otherwise FALSE.	This procedure executes any(), fol(), prec() & wd().
 *
 * Return values:  TRUE == char equals one of the chars in the specified store
 *						FALSE == no match
 *
 * Globals input: storebegin -- array of pointers to the beginnings of stores
 *						storend -- array of pointers to the ends of stores
 *
 * Globals output: none
 *
 * Error conditions: none
 *
 * Other functions called: none
 *
 */

{
    register SSINT *wptr,           /* Working pointer */
    *endptr;  /* Pointer to end of store */

    endptr = storend[*tblptr];				 /* End of store, for loop termination */

    for ( wptr = storebegin[*tblptr]; wptr < endptr; wptr++ )
    {
        if ( caseless )
        {
            if ( tolower( mc ) == tolower( *wptr ) )
                return( TRUE );					/* Successful caseless match */
        }
        else											/* Case is important */
        {
            if ( mc == *wptr )
                return(TRUE);						/* Successful case-sensitive match */
        }
    } /* End--for */

    return(FALSE);								/* No match -- failure */
} /* End--anycheck */

/****************************************************************************/
int anyutf8check(SSINT * mcptr)             /* Is item in the specified store? */
/****************************************************************************/
/* Description:
 *   This procedure checks to see if the argument is equal to any of the
 * characters in the specified store area.  If it is, it returns TRUE,
 * otherwise FALSE.	This procedure executes any(), fol(), prec() & wd().
 *
 * Return values:  TRUE == char equals one of the chars in the specified store
 *						FALSE == no match
 *
 * Globals input: storebegin -- array of pointers to the beginnings of stores
 *						storend -- array of pointers to the ends of stores
 *
 * Globals output: none
 *
 * Error conditions: none
 *
 * Other functions called: none
 *
 */

{
    register SSINT *wptr,           /* Working pointer */
                   *endptr;         /* Pointer to end of store */ 
	register int i;					/* Index into UTF-8 character */
	register int utf8charactersize; /* Size of UTF-8 character */

    endptr = storend[*tblptr];				 /* End of store, for loop termination */
	utf8charactersize = UTF8AdditionalBytes((UTF8)*mcptr) +1;
    for ( wptr = storebegin[*tblptr]; wptr < endptr; wptr+= (UTF8AdditionalBytes((UTF8)*wptr) +1))
    {
        for (i= 0; i < utf8charactersize; i++)
			if ( mcptr[i] != wptr[i])
				break;
		if (i == utf8charactersize)
			return (TRUE);
    } /* End--for */

    return(FALSE);								/* No match -- failure */
} /* End--anyutf8check */

SSINT * utf8characterfrombackbuffer(SSINT** buffer)
{
	static SSINT utf8character[MAXUTF8BYTES];
	SSINT * bptr;
	SSINT * charptr;

	bptr = *buffer;
	charptr = utf8character + MAXUTF8BYTES;

	do 
	{
		bptr = bptr > backbuf ? bptr - 1 : backbuf + BACKBUFMAX - 1;
		*--charptr = *bptr;
	} while ((*bptr & 0xC0) == 0x80 &&
			 bptr != backoutptr);

	*buffer = bptr;
	
	return charptr;
}

/****************************************************************************/
int contcheck()		 /* Is next part of match == contents of store? */
/****************************************************************************/

/* Description:
 *   This procedure tests the next part of the match to see if it is equal to
 * the content of the specified store.  If it is, it returns TRUE,
 * otherwise FALSE.
 *
 * Return values:  TRUE == current input matches contents of the
 *									  specified store
 *						FALSE == no match
 *
 * Globals input: mchptr -- pointer to the current position within the
 *										potential match
 *						storebegin -- array of pointers to the beginnings of stores
 *						storend -- array of pointers to the ends of stores
 *
 * Globals output: mchptr -- if everything matched, set to the address of the
 *										 next char beyond the match
 *
 * Error conditions: none
 * 
 * Other functions called: none
 *
 */

{
    register SSINT *mp,      /* Pointer to input being matched */
    *tp,	 /* Pointer to contents of store being matched */
    *tp9;  /* Pointer to the end of the store being matched */

    mp = mchptr;								/* Match pointer */

    tp9 = storend[*tblptr];					/* End of store, for loop termination */

    for ( tp = storebegin[*tblptr]; tp < tp9;  tp++ )
    {
        if ( caseless )
        {
            if ( tolower( *mp ) != tolower( *tp ) )
                return( FALSE );								/* Caseless failure */
        }
        else
        {
            if ( *mp != *tp )
                return( FALSE);								/* Case-sensitive failure */
        }
        mp++;
    } /* End--for */

    mchptr = mp;							/* Update pointer to trial match */

    return( TRUE);								/* This part matched */
} /* End--contcheck */

/****************************************************************************/
int leftexec(int ofs)					 /* Execute commands on left side of WEDGE */
/****************************************************************************/

/* Description:
 *   This procedure executes commands found on the left of the WEDGE. It
 * expects the command to be in the global cngletter.
 *
 * Return values:  TRUE == successful match
 *						FALSE == no match
 *
 * Globals input: cngletter -- the current command to be executed
 *						mchptr -- pointer to the current trial match
 *						backinptr -- input pointer for the ring buffer
 *						backbuf -- array used for the ring buffer
 *						tblptr -- pointer into the current change
 *						curstor -- if storing, store #, else zero
 *
 * Globals output: mchptr -- advanced based on the success of the match
 *						 tblptr -- advanced to the character following the command
 *										(store number for FOL, PREC, WD, and CONT)
 *										(char with high-bit stripped off for TOPBITCMD)
 *
 * Error conditions: if an illegal command is found,
 *							  an error message will be displayed to the screen
 *
 * Other functions called: anycheck -- is the specified char in the store?
 *									contcheck -- does the next part of the input
 *														match the contents of the
 *														specified store?
 *
 */

#define MAX_STACKED_PREC 10

{
    int f;		/* Return value for anycheck */
    int prec_cnt;	/* Count of fol()'s or prec()'s executed in succession */
	int prec_index; /* current prec command being processed */
    int i;		/* Handy-dandy loop indices */
	int mchoffset;      /* offset into match character buffer */
    tbltype tp_temp;	/* Local copy of tblptr */
	bool storeexhausted;
	SSINT * utf8char = NULL;

    SSINT prec_array[ MAX_STACKED_PREC ];
    /* Array of store #'s for stacked prec()'s
    *   to allow for reversing the order,
    *   so that they work in the order that
    *   they were entered
    *		 (instead of right to left)
    */
	SSINT * precptr; /* Pointer into buffer for the current character for an
					  * anyutfcheck during a prec check 
					  */


    if (cngletter != ENDFILECMD)
    {
        tblptr++;			 /* Assumes all commands here have one arg */
        /* except ENDFILECMD                      */

        if (mchptr == matchpntrend &&
		    cngletter != PRECCMD &&
		    cngletter != PRECUCMD) /* all commands except ENDFILECMD and PRECMD need something in the match
                                               buffer to process, return if the match buffer is empty */
            return FALSE;
    }


    switch ( cngletter )
    {
    case ANYUCMD:
    case ANYCMD:
		if (utf8encoding || cngletter == ANYUCMD)
		{
			f = anyutf8check(mchptr);

			if (f)
				mchptr+= (UTF8AdditionalBytes((UTF8)*mchptr) + 1);
		}
		else
		{
			f = anycheck(*mchptr);

			if (f)
				mchptr++;			
		}
        return f;

    case ENDFILECMD:
        if ((bEndofInputData) && (mchptr == matchpntrend))
        {
            endoffile_in_mch[ofs]= TRUE;

            return TRUE;
        }
        else
            return FALSE;
	case FOLUCMD:
    case FOLCMD:													/* Fol */
        mchoffset = 0;

		if (utf8encoding || cngletter == FOLUCMD)
			f = anyutf8check( mchptr );
		else
			f = anycheck(*mchptr);

        for ( ;; )					/* Do forever, exit is via return */
        {
            /* If we get a failure at any point, the match has failed,
            	*   so return
            	*/
            if ( !f || ( (*(tblptr + 1) != FOLCMD) && (*(tblptr + 1) != FOLUCMD)) )
                return( f );
            else
            {
                tblptr += 2;		/* Skip the current store # and the next FOL */
				if (utf8encoding || cngletter == FOLUCMD)
					mchoffset+= UTF8AdditionalBytes((UTF8)*(mchptr + mchoffset)) + 1;					
				else
					mchoffset++;
            }
			if (utf8encoding || cngletter == FOLUCMD)
				f = (f && anyutf8check( mchptr + mchoffset ));
			else
				f = (f && anycheck( *(mchptr + mchoffset) ));

        } /* End--do forever */

    case PRECUCMD:													/* Prec */
    case PRECCMD:													/* Prec */
        /*
         * Go through and load prec_array from the bottom,
         *  with <store-#>, so that they will be processed
         *  in reverse order.  This will produce the effect most people
         *  will expect:
         *					 store(1) 'a' endstore
		 *					 store(2) 'b' endstore
		 *
         *   'c' prec(1) prec(2)  will match the c in 'abc'
         */

        for ( i = MAX_STACKED_PREC-1, prec_cnt = 1; ; i--, prec_cnt++)
        {
            if ( i >= 0 )
                prec_array[i] = *tblptr;
            if ( *(tblptr + 1) != PRECCMD &&
                 *(tblptr + 1) != PRECUCMD)
                break;					/* Found the end of the stacked prec()'s */
            else
            {
                tblptr += 2;		/* Skip the store # and then next PRECCMD */
            }
        } /* End--load prec()'s into the array */

        if ( prec_cnt > MAX_STACKED_PREC )		/* Too many in a row */
        {
            Process_msg(errorFunction, 51, 0,
                        (long unsigned) MAX_STACKED_PREC);

            /* Only use the first <n>, like we just told the user
            	*/
            prec_cnt = MAX_STACKED_PREC;
            i = 0;
        }

        tp_temp = tblptr;
        tblptr = &prec_array[i];		/* Execute out of the array */

        /* Now try them */
		f = TRUE; 

		if (!curstor || storend[curstor] == storebegin[curstor])
		{
			storeexhausted = TRUE;
			precptr = backinptr;
		}
		else
		{
			storeexhausted = FALSE;
			precptr = storend[curstor];
		}

        for ( prec_index = 0; (prec_index < prec_cnt) && f
                ; prec_index++, tblptr++ )
        {
			if (!storeexhausted)
			{
				if (precptr == storebegin[curstor])
				{
					storeexhausted= TRUE;
					precptr = backinptr;
				}
				else
				{
					precptr--;

					if (utf8encoding || cngletter == PRECUCMD)
					{
						while (precptr > storebegin[curstor] && 
							   ((*precptr) & 0xC0) == 0x80)
							precptr--;

						utf8char = precptr;
					}
				}				
			}

			if (storeexhausted)
			{
				if (precptr == backoutptr) /* BOF condition */
					f = FALSE;
				else
				{
					if (utf8encoding || cngletter == PRECUCMD)
						utf8char = utf8characterfrombackbuffer(&precptr);
					else
						precptr = (precptr > backbuf) ? precptr-1 :  backbuf + BACKBUFMAX - 1;
				}
			}

			if (utf8encoding || cngletter == PRECUCMD)
				f = f && anyutf8check( utf8char );
			else
				f = f && anycheck( *precptr );
        } /* End--try prec()'s */

        tblptr = tp_temp;
        return( f );

    case WDUCMD:														/* Wd */
    case WDCMD:														/* Wd */
        if ( curstor && (storebegin[curstor] != storend[curstor]) )
        {
			precptr= storend[curstor] - 1;

			if (utf8encoding || cngletter == WDUCMD)
			{
				while (precptr > storebegin[curstor] && 
					   ((*precptr) & 0xC0) == 0x80)
					precptr--;

				utf8char = precptr;
			}        
		}
        else
        {
			precptr = backinptr;
			if (precptr == backoutptr) /* BOF condition */
				f = FALSE;
			else
			{
				if (utf8encoding || cngletter == WDUCMD)
					utf8char = utf8characterfrombackbuffer(&precptr);
				else
					precptr = (precptr > backbuf) ? precptr-1 :  backbuf + BACKBUFMAX - 1;
			}
        }

		if (utf8encoding || cngletter == WDUCMD)
	        return( anyutf8check( utf8char ) && anyutf8check(mchptr) );
		else
	        return( anycheck(*precptr) && anycheck(*mchptr) );

    case CONTCMD:													/* Cont */
        return( contcheck() );

    case TOPBITCMD:                    /* High-bit element */
        if ( (*tblptr | HIGHBIT) == (*mchptr++ & 0xffff) )
            return( TRUE );
        else
            return( FALSE );

    default:															/* Bad */
        Process_msg(errorFunction, 52, 0,
                    (long unsigned) cmdname((char)cngletter, TRUE) );
        break;
    }
    return( FALSE);

} /* End--leftexec */

/****************************************************************************/
void startcc()																/* Start up CC */
/****************************************************************************/

/* Description:
 *                Set everything up for CC.
 *
 * Return values: none
 *
 * Globals input: switches -- array of booleans, each representing a switch
 *						storebegin -- array of pointers to the beginnings of stores
 *						storend -- array of pointers to the ends of stores
 *						curstor -- if storing, the number of the currently
 *										 active store
 *						storeoverflow -- boolean: TRUE == overflow has occurred
 *																		in a store
 *						eof_written -- boolean: TRUE == CC has output EOF
 *						notable -- boolean: TRUE == no change table
 *						backinptr -- input pointer for the ring buffer
 *						backoutptr -- output pointer for the ring buffer
 *						backbuf -- pointer to the beginning of the ring buffer
 *						table -- array containing the change table
 *
 * Globals output: switches -- all entries set to FALSE
 *						 storebegin -- all entries set to store_area
 *						 storend -- all entries set to store_area
 *						 curstor -- set to zero
 *						 storeoverflow -- set to FALSE
 *						 eof_written -- set to FALSE
 *						 setcurrent -- set to FALSE
 *						 backinptr -- set to the beginning of the ring buffer
 *						 backoutptr -- set to the beginning of the ring buffer
 *						 backbuf -- entire ring buffer set to SPACEs
 *
 * Error conditions: none
 *
 * Other functions called: bytset -- set a block of memory to the given
 *													one-byte value
 *                         ssbytset -- set a block of memory to the given
 *                                     two-byte value
 *                           groupinclude -- add a group to the list currently
 *															being searched
 *
 */

{
#if !defined(_WINDOWS) && !defined(UNIXSHAREDLIB)
    static SSINT *store_area = NULL; /* Store area (dynamically allocated) */
#endif
    unsigned storemax;					/* Size of store area */
    register int i;		/* Loop index for initializing storage pointers */

    bytset( switches, FALSE, MAXARG+1);					/* Clear all switches */

    if (store_area != NULL)
        free(store_area);
    storemax = max_heap() / sizeof(SSINT);   /* Get as much space as available */
    store_area = (SSINT *) tblalloc(storemax, (sizeof(SSINT)));  /* Allocate store area */
    storelimit = store_area + storemax;        /* Initialize storelimit */
    for (i = 0; i <= NUMSTORES; i++)				/* Initialize storage pointers */
        storebegin[i] = storend[i] = store_area;

    curstor = 0;										/* Clear storing and overflow */
    storeoverflow = FALSE;
    iStoreStackIndex = 0;

    setcurrent = FALSE;									/* Letterset not current */

    eof_written = FALSE;						/* We haven't written EOF yet */

    numgroupson = 0;
    if (groupbeg[1] != 0)
        groupinclude(1);						/* Start in group 1, if it exists */

    if (notable)								/* If no table show, no groups on */
        numgroupson = 0;

    backinptr = backoutptr = backbuf;		 	/* Initialize backup buffer */
    ssbytset( backbuf, (SSINT)' ', BACKBUFMAX);      /* (SPACEs so debug looks ok) */


} /* End--startcc */

/****************************************************************************/
void fillmatch()                          /* Fill match area, execute begin */
/****************************************************************************/
/* Description:
 *                fill up match area, execute the BEGIN if there is one.
 *
 * Return values: none
 *
 * Globals input: tloadpointer -- pointer to the next available space beyond
 *												the change table array
 *						numgroupson -- number of groups currently being searched
 *						curgroups -- array of group numbers to be checked
 *						notable -- boolean: TRUE == no change table
 *						matchpntr -- pointer to the place to start the next
 *											attempted match
 *						table -- array containing the change table
 *						debug -- table debugging flag
 *
 * Globals output: numgroupson -- set to either zero (no table or no group 1)
 *										 or one (group one exists)
 *						 matchpntr -- set to the beginning of the match area
 *
 * Error conditions: none
 *
 * Other functions called: inputchar -- get a char from the input file
 *									execute -- execute the replacement part of a change
 *
 */

{
    register int i;		   /* Loop index for initializing storage pointers */
    register tbltype tpntr; /* For finding the physical first group if
    								 *	 defaulting to an empty group
    								 */

    if (executetableptr)
        executetableptr= execute( 0, executetableptr, TRUE );
    else
    {
        refillinputbuffer();

        if (table[0] == SPECWEDGE)							/* Do begin command */
        {
            matchlength= 0;
            executetableptr= execute( 0, &table[1], TRUE );
        }
    }

    if (!executetableptr)
    {
        bBeginExecuted= TRUE;

        if ( numgroupson == 1 )
            i = curgroups[1];					/* Get the currently active group */

        if ( (numgroupson == 1) && (groupbeg[i] == groupend[i]) )
        {
            /*   Start at the beginning of the table and
            * go until we find the beginning of a group.

            		* INTERNALLY (!!) this is indicated by two
            * SPECPERIODs with only one byte between,
            * because the GROUPCMD was replaced with a
            * SPECPERIOD at setup time.
            *
            * The next byte will contain the number of
            * first group in the table.
            */
            for ( tpntr = table; tpntr < tloadpointer; tpntr++ )
            {
                if ( (*tpntr == SPECPERIOD) && ( *(tpntr + 2) == SPECPERIOD) )
                {
                    curgroups[1] = (int) *(tpntr + 1);	/* Get the group number */
                    break;
                }
                if ( *tpntr == TOPBITCMD )
                    tpntr++;						/* Next char is a high-bit literal
                								* so skip it, to avoid a possible
                								* "false match"
                								*/
            } /* End--search for first group */

            if ( curgroups[1] == i )	/* No explicitly-defined group in table */
            {
                if (tloadpointer - table > 2)
                    Process_msg(errorFunction, 23, 0,
                                (long unsigned) sym_name(i, GROUP_HEAD) );
                else
                    Process_msg(errorFunction, 136, 0,
                                (long unsigned) sym_name(i, GROUP_HEAD) );
            }
        } /* End--starting to default to an empty group */

        /*   Do the after display here, now that the real
        * starting group has been established
        */

        if ( !numgroupson && !notable )
        {
            Process_msg(errorFunction, 25, 0, 0);
            bailout(BADEXIT, FALSE);
        }
    }
} /* End--fillmatch */

#if defined(_WINDOWS) || defined(UNIXSHAREDLIB)
/****************************************************************************/
void storefree()		  /*free memory allocated for stores */
/****************************************************************************/
// This function can be called after cc has run one (or more) times.
{
    if (store_area != NULL)
    {
        free (store_area);
        store_area = NULL;
    }
}

#endif

#if defined(_WindowsExe)
/****************************************************************************/
void check_for_msgs(void)
/****************************************************************************/
{
    MSG msg;

    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}
#endif

void refillinputbuffer(void)
{
    int nbytesleft;
    register SSINT *cp;   /* Working pointer for replenishing match buffer */
    SSINT ch;

#if !defined(_WINDOWS) && !defined(UNIXSHAREDLIB)
    long percent_complete;
#endif

    nbytesleft = matchpntrend - matchpntr;

    if (nbytesleft < maxsrch && !bEndofInputData)
    {
        /* Shift the active buffer contents to the beginning */
        /*  of the buffer, to allow some space */

        if (nbytesleft)
            cp = ssbytcpy( match+BACKBUFMAX, matchpntr, nbytesleft);
        else
            cp = match+BACKBUFMAX;

#if !defined(_WINDOWS) && !defined(UNIXSHAREDLIB)
        /* display percent complete status message if user set the switch */
        if ( percent_print )
        {
            percent_complete = (infileposition * 100) / infilelength;

            if (percent_complete >= nextstatusupdate)
            {

                msg_printf("% 3ld%% complete", percent_complete);

                /* 10/24/95 DRC   if MS Visual C++ then just put out carriage return  */
#if !defined(_MSC_VER) && !defined(UNIX)
                gotoxy(1, wherey());
#else
                msg_putc('\r');
#endif
                while (percent_complete >= nextstatusupdate)
                    nextstatusupdate+= STATUSUPDATEINCREMENT;
            }
        }
#endif

#if defined(_WinIO)
        wmhandler_yield();
#else
#if defined(_WindowsExe)
        check_for_msgs();
#endif
#endif
        while ( cp < match+MAXMATCH-(MAXUTF8BYTES-1) && !bEndofInputData && !bNeedMoreInput)			/* Refill the emptied space */
        {
            ch = inputchar();

            // this actually checks "end of data" for this call in DLL case
            if (!bEndofInputData && !bNeedMoreInput)
                *cp++ = ch;

            // Make sure we have a complete UTF8 character
            if (utf8encoding && ((ch & 0xC0) == 0xC0))
            {
                int AdditionalBytes;

                for (AdditionalBytes= UTF8AdditionalBytes((UTF8)ch); AdditionalBytes != 0 && !bEndofInputData && !bNeedMoreInput; AdditionalBytes--)
                {
                    ch = inputchar();

                    if (!bEndofInputData && !bNeedMoreInput)
                        *cp++ = ch;
                }
            }
        }

        matchpntr = match+BACKBUFMAX;   /* Update pointer */
        matchpntrend = cp;
    }
}

#if defined(_WINDLL)
static unsigned FindLineNumber(tbltype matchposition)
{
    unsigned iLine;
    unsigned matchoffset;

    iLine= 0;
    matchoffset= (unsigned)((long)matchposition - (long)table) / sizeof(SSINT);

    while (TableLineIndexes[iLine] <= matchoffset)
        iLine++;

    return iLine;
}
#endif

/****************************************************************************/
void ccin()		/* Do CC until one char output or one table entry performed */
/****************************************************************************/

/* Description:
 *   This procedure does consistent changes until one character has been
 * output, or until one entry in the changes table has been performed.
 *
 * Return values: none
 *
 * Globals input: firstletter -- first letter of the potential match
 *						matchpntr -- pointer into the match buffer
 *						caseless -- boolean: TRUE == ignore case of first letters
 *																 of matches
 *						uppercase -- boolean: TRUE == first letter of the current
 *																  match is upper case
 *						setcurrent -- boolean: TRUE == set of first letters of
 *																	possible matches is
 *																	up to date
 *						storeact -- array of booleans, TRUE == the corresponding
 *																			 store is being used
 *																			 in a left-executable
 *																			 function
 *						letterset -- array of booleans, TRUE == there is a change
 *																			 beginning with the
 *																			 corresponding letter
 *						curgroups -- array of group numbers for groups currently
 *											being searched, stored in the order that the
 *											groups are to be searched
 *						matchpntr -- pointer to the logical beginning of the
 *											match buffer
 *						match -- pointer to the physical beginning of the
 *									  match buffer
 *
 * Globals output: firstletter -- contains the first letter to be checked
 *												for a possible match, set to lower case
 *												if we are doing a caseless search and
 *												it was actually upper case
 *						 uppercase -- if TRUE, firstletter was actually upper case
 *											 but we are ignoring case for matches
 *						 setcurrent -- set to TRUE if the set of first letters is
 *											  up to date
 *						 matchpntr -- updated
 *
 * Error conditions: none
 *
 * Other functions called: completterset -- set up the array of first letters
 *															 of matches
 *									gmatch -- go through a group and try to find
 *													a match
 *									output -- move a byte out of the backup buffer
 *                         ssbytcpy --  move a block of memory
 *
 */

{
    int cg;				/* Group number for stepping through the active groups */
    int match_choice;
#if defined(_WINDLL)
    BOOL bContinue;
#endif
    match_choice= -1;

    /* first check if we were in the middle of a replacement */
    if (executetableptr != NULL)
    {

        executetableptr= execute(matchlength, executetableptr, FALSE);		/* Execute replacement */

        /* if we still haven't finished with the replacement then return */
        if (executetableptr != NULL)
            return;
    }

    /* Be sure enough characters are in match buffer */
    /*  to allow for the longest possible match */
    refillinputbuffer();

    if (bNeedMoreInput)
        return;

    /* if nothing in match buffer then first letter is not really valid
    	don't bother with first letter in this case */
    if ((matchpntrend - matchpntr) != 0)    // ADDED 08-01-95 DAR
    {
        firstletter = *matchpntr;						/* Set up first letter */

        if (caseless) {											/* Caseless */
            if ( isupper(firstletter) )
            {
                firstletter = (char)tolower( firstletter );
                uppercase = TRUE;
            }
            else
                uppercase = FALSE;
		}
        if ( !setcurrent && !storeact[curstor] )
            completterset();				/* Don't recompute while storing */

        if ( letterset[(firstletter & 0xff)] || !setcurrent )
            for (cg = 1; cg <= numgroupson; cg++ )		/* Step through groups */
                if ((match_choice= gmatch(curgroups[cg])) >= 0)
                    break;
    }
    else
    {
        for (cg = 1; cg <= numgroupson; cg++ )		/* Step through groups */
            if ((match_choice= gmatch(curgroups[cg])) >= 0 )
                break;
    }

    if (match_choice >= 0)
    {
        matchlength= mchlen[match_choice];
        stacklevel= 0;
#if defined(_WINDLL)
        if (lpMatchLineCallback != NULL)
        {
            iCurrentLine= FindLineNumber(tblxeq[match_choice]);
#if !defined(DEBUGLIB)
            SaveState(hActiveCCTable);
#endif
            bContinue= (*lpMatchLineCallback)(hActiveCCTable, iCurrentLine);
#if !defined(DEBUGLIB)
            RestoreState(hActiveCCTable);
#endif			
            if (!bContinue)
                longjmp(abort_jmp_buf, 0);

        }
#endif		
        executetableptr= execute(matchlength, tblxeq[match_choice], FALSE);		/* Execute replacement */
    }
    else
    {
        if ((matchpntrend - matchpntr) == 0)
        {
#ifdef _WINDLL
            // if we have no more input data
            if ( bEndofInputData )
            {
                curstor = 0;
                numgroupson = 0;       /* All groups off for any further endfile */
                setcurrent = FALSE;
                eof_written = TRUE;
            }
#else
            curstor = 0;
            numgroupson = 0;       /* All groups off for any further endfile */
            setcurrent = FALSE;
            eof_written = TRUE;

#endif
        }
        else
            output( *matchpntr++);					/* No match, so output a char */
    }

} /* End--ccin */

/****************************************************************************/
int gmatch(group)										  /* Check through one group */
/****************************************************************************/
int group;

/* Description:
 *						Go through one group and attempt to find a match.
 *					If we find one, execute it.  If multiple matches are found,
 *					execute the longest one.  If 2 matches of equal length are
 *					found, but one contains a left-executable function (cont(),
 *					wd(), etc.), execute the change without the left-executable
 *					function.
 *
 * Return values:  TRUE == We found a match and executed the change
 *						FALSE == No match found
 *
 * Globals input: groupbeg -- array of pointers to beginnings of changes
 *										  for groups
 *						groupxeq -- array of pointers to the beginnings of
 *										  changes containing left-executable functions
 *										  for groups
 *						groupend -- array of pointers to the end of changes
 *										  for groups
 *						mchlen -- array of lengths of matches found, one length
 *										for regular changes and one for changes
 *										containing left-executable functions
 *						firstletter -- first letter to be checked for matches
 *						matchpntr -- pointer into the match area
 *						cgroup -- group from which currently executing change came
 *
 * Globals output: mchlen -- if match found for that particular type of
 *										 change, contains length of match,
 *										 otherwise, contains -1
 *						 matchpntr -- if a change was found, updated to point to
 *											 the next unchecked character
 *						 cgroup -- if a change was found, group from which it came
 *
 * Error conditions: none
 *
 * Other functions called: cmatch -- try to match a change, letter by letter
 *									execute -- execute the replacement part of a change
 *
 */

{
    register cngtype clp;	/* Pointer to beginning of regular changes */
    register cngtype cxp;	/* Pointer to beginning of left-executable changes */
    register cngtype clp9;	/* Pointer to end of regular changes */
    register cngtype cxp9;	/* Pointer to end of left-executable changes */
    int match_choice;		/* type of match found */

#ifdef DEBUG
    msg_printf("--gmatch now checking group %s\n",
               sym_name(group, GROUP_HEAD));
#endif

    endoffile_in_mch[0] = FALSE;
    endoffile_in_mch[1] = FALSE;

    clp = groupbeg[group];					/* Initialize change pointers */
    clp9 = cxp = groupxeq[group];
    cxp9 = groupend[group];

    mchlen[0] = mchlen[1] = -1;

    /* if nothing in match buffer then first letter is not really valid
    	don't check for match in this case */
    if ((matchpntrend - matchpntr) != 0)    // ADDED 08-01-95 DAR
    {
        /* Find first change in table, if any, */
        /*  with correct starting letter */
        while ( (clp < clp9) && (**clp < firstletter)  )           // 7b.4r5 ALB Fix Windows GPF on clp
            clp++;

        /* Match changes starting with correct letter */

        while ( (clp < clp9) && (**clp == firstletter) && !cmatch(clp, 1) )   // 7b.4r5 ALB Fix Windows GPF on clp
            clp++;
        /* Match changes starting with left-executable function */

    }
    while ( (cxp < cxp9) && !cmatch(cxp,0) )
        cxp++;

    /* Examine mchlen[0] and mchlen[1] to see what matches were found. */
    /*
    NOTE: Use the following logic: There are two types of matches, type 0 are
    left-execute matches (i.e., leftmost element on match side was a command),
    and type 1 are regular matches (i.e., leftmost element is a letter).
    Either or both types may have been successfully matched in this group.
    If only one type of match was found, then choose that match. If both types
    were found, then compare their lengths to decide which match should be
    used. If their lengths are the same (or the 'unsorted' flag is TRUE)
    then choose the match which is physically first in the table, otherwise
    choose the longest of the two matches.
    */
    match_choice = -1;		/* Assume no match */

    if (endoffile_in_mch[0])
        mchlen[0]++;

    if (endoffile_in_mch[1])
        mchlen[1]++;

    if (mchlen[0] >= 0)			/* Was type 0 match found? */
    {								/* yes */
        if (mchlen[1] >= 0)				/* Was type 1 match also found? */
        {									/* yes, both types were found. */
            if (mchlen[0] == mchlen[1] || unsorted)	/* If lengths are same */
            {													/* or unsorted, then... */
                if (tblxeq[0] < tblxeq[1])					/* choose first in table */
                    match_choice = 0;
                else
                    match_choice = 1;
            }
            else				/* If lengths are different, choose longest */
            {
                if (mchlen[0] > mchlen[1])
                    match_choice = 0;
                else
                    match_choice = 1;
            }
        }
        else		/* Type 0 found, type 1 not found */
            match_choice = 0;		/* obviously choose 0 */
    }
    else if (mchlen[1] >= 0)		/* Was type 1 found? */
        match_choice = 1;

    if (endoffile_in_mch[0])
        mchlen[0]--;

    if (endoffile_in_mch[1])
        mchlen[1]--;

    /* Upon dropping out of the above logic, if match_choice == -1, then no
    match was found, otherwise match_choice indicates which match is to
    be used */
    if (match_choice >= 0)
    {							/* Match was found */

        matchpntr += mchlen[match_choice];	/* Adjust match pointer */
        cnglen = mchlen[match_choice];		/* Tell debug how long match is */
        cgroup = group;		/* Tell the world which group the match is in */
        endoffile_in_match= endoffile_in_mch[match_choice];

        //		execute(mchlen[match_choice],
        //			tblxeq[match_choice], FALSE);		/* Execute replacement */
        //		return(TRUE);
    }
    //	else
    //		return(FALSE);			/* No match */
    return match_choice;

} /* End--gmatch */

/****************************************************************************/
int cmatch(cp, ofs)						/* Match a change, letter by letter */
/****************************************************************************/
register cngtype cp;		/* Pointer to the match we're trying */
int ofs;						/* Offset into the change at which to start */
/*  (0 for left-executable matches,  */
/*	1 for regular matches, since we have already */
/*	matched the first letter)		*/

/* Description:
 *						Attempt to match a change, letter by letter,
 *					starting ofs bytes into the change.
 *
 * Return values:  TRUE == we have a match
 *						FALSE == no match
 *
 * Globals input: tblptr -- working pointer to the change
 *						mchptr -- working pointer into the match area
 *						matchpntr -- pointer to the beginning of the current
 *											trial match
 *						cngletter -- copy of the current character from the table
 *						mchlen -- array of lengths of successful matches, one entry
 *										for regular changes and
 *										one for left-executable changes
 *						tblxeq -- array of pointers to replacement strings for
 *										successful matches, one entry
 *										for regular changes and
 *										one for left-executable changes
 *
 * Globals output: tblptr -- if successful match, points to the WEDGE,
 *										 otherwise, contents are unimportant
 *						 mchptr -- if successful match, points to the next
 *										 character beyond the end of the match,
 *										 otherwise, contents are unimportant
 *						 mchlen -- if successful match,
 *										 mchlen[ofs] contains the length of the match,
 *										 otherwise, unchanged
 *						 tblxeq -- if successful match,
 *										 tblxeq[ofs] points to the replacement part
 *										 of the change,
 *										 otherwise, unchanged
 *						 cngletter -- contains the last character from the table
 *											 that was tested for a match
 *
 * Error conditions: none
 *
 * Other functions called: leftexec -- try to match using a left-executable
 *													  function
 *
 */

{
    tblptr = *cp + ofs;							/* Pointer into the table */
    mchptr = matchpntr + ofs;						/* Pointer into the match area */

    for (;;)									/* Do forever (exit is via return) */
    {

#ifdef DEBUG
        msg_printf("--cmatch, cngletter = %02X, *tblptr = %02X\n",
                   (cngletter & 0xff), (*tblptr & 0xff));
#endif
        cngletter= *tblptr;

        if (cngletter  & HIGHBIT )
        {
            if ( cngletter == SPECWEDGE )
            {															 /* Successful match */
                mchlen[ofs] = mchptr - matchpntr;		/* Length of match */
                tblxeq[ofs] = tblptr + 1;				/* Pointer to replacement */
                return( TRUE );
            }
            else
            {
                if ( !leftexec(ofs) )				/* Try a left-executable function */
                    return( FALSE );
            }
        }
        else if ( mchptr == matchpntrend ||
                  cngletter != *mchptr)
            //					 (!caseless && cngletter != *mchptr) ||
            //					 (caseless &&  cngletter != tolower(*mchptr)))			/* Compare raw letters */  /* FIX FOR CASELESS DAR 10-29-96 */
            return( FALSE );
        else
            mchptr++;

        tblptr++;						/* On to the next char in the table */

    } /* End--for */

} /* End--cmatch */

/****************************************************************************/
void completterset()					 /* Compute first-letter set for matching */
/****************************************************************************/

/* Description:
 *						Go through the letterset array, setting each element for
 *					which there is a change starting with that letter to TRUE.
 *					If there is a null-match ("" or '') set every element to TRUE.
 *
 * Return values: none
 *
 * Globals input: setcurrent -- boolean: TRUE == letter set is up to date
 *						letterset -- array of booleans: TRUE == there is a match
 *																				that begins with
 *																				that letter
 *						storeact -- array of booleans: TRUE == that store is used in
 *																				a left-executable
 *																				function
 *						curgroups -- array of group numbers for groups currently
 *											being searched for possible matches
 *						groupbeg -- array of pointers to the beginning of groups
 *						groupxeq -- array of pointers to the beginning of the
 *										  left-executable portions of groups
 *						groupend -- array of pointers to the ends of groups
 *
 * Globals output: setcurrent -- set to TRUE
 *						 letterset, storeact -- correct for the currently
 *														active groups
 *
 * Error conditions: none
 *
 * Other functions called: bytset -- set a block of memory to a specified
 *													one-byte value
 *                         ssbytset -- set a block of memory to a specified
 *                                     two-byte value
 *
 */

{
    register tbltype tp;		/* Working pointer into storage area */
    tbltype tp9;				/* Working pointer to the end of the store */
    /*  currently being checked */
    register cngtype cp;		/* Working pointer for going through the changes */
    cngtype clp9;				/* Working pointer to the end of the */
    /*  regular changes for the current group */
    cngtype cxp9;				/* Working pointer to the end of the current group */

    /* Note:  The above variables are declared separately because */
    /*				DECUS C does not allow regular typedef's	*/
    SSINT ch;                                               /* Local temp for comparisons */
    int cg,						/* Index into curgroups array */
    group;				  /* Current group being checked */

    setcurrent = TRUE;						/* Say letter set is up to date */

    bytset( letterset, FALSE, sizeof(letterset));    /* Re-initialize arrays */
    bytset( storeact, FALSE, sizeof(storeact));

    for (cg = 1; cg <= numgroupson; cg++ )				/* Step through groups */
    {
        group = curgroups[cg];					/* Set up working pointers */
        cp = groupbeg[group];					 /*  for this group */
        clp9 = groupxeq[group];
        cxp9 = groupend[group];

        /* Compile first letters of changes starting with character */

        while ( cp < clp9 )
            letterset[(**cp++ & 0x00ff)] = TRUE;

        /* Compile first letters of changes starting with function */

        while ( cp < cxp9 )
        {
            tp = *cp++;
            while ( (ch=(*tp++)) != SPECWEDGE )
            {
                switch ( ch )
                {
                case ANYCMD:												/* any */
                case ANYUCMD:												/* any */
                    storeact[*tp] = TRUE;
                    tp9 = storend[*tp];
                    for ( tp = storebegin[*tp]; tp < tp9; )
                        letterset[(*tp++) & 0xFF] = TRUE;
                    goto L60;
                case CONTCMD:												/* cont */
                    storeact[*tp] = TRUE;
                    letterset[*storebegin[*tp] & 0xFF] = TRUE;
                    goto L60;
                case FOLCMD:												/* fol */
                case PRECCMD:												/* prec */
                case WDCMD:													/* wd */
                case FOLUCMD:												/* fol */
                case PRECUCMD:												/* prec */
                case WDUCMD:												/* wd */
                    tp++;
                    break;						/* Doesn't affect 1st letter */

                case TOPBITCMD:                  /* High-bit element */
                    /* DAR use only lower byte of letter.
                    	 Otherwise letterset would have to be 64K */
                    letterset[*tp & 0x00ff] = TRUE;
                    goto L60;
                default:
                    if ( (ch & HIGHBIT) == 0 )
                    {
                        letterset[(ch & 0x00ff)] = TRUE;
                        goto L60;
                    }
                } /* End--switch */

            } /* End--while (ch != SPECWEDGE) */

            /* Any starting character is game */
            bytset( letterset, TRUE, sizeof(letterset));   /* (null match) */
            goto L90;
L60:		;
        } /* End--while (cp < cxp9) */

    } /* End--for (step through curgroups array) */
L90:
    ;

} /* End--completterset */


/****************************************************************************/
void execcc()			  /* Execute CC */
/****************************************************************************/
{

#ifndef _WINDLL
    while (!eof_written)                        /* Go until EOF */
    {
        ccin();
    }

    while ( backoutptr != backinptr )
    {
        writechar();
    }
#else
    while (!eof_written && !bNeedMoreInput && !bOutputBufferFull)
    {
        ccin();
    }

    if (eof_written)
        while (( backoutptr != backinptr ) && (!bOutputBufferFull ))
        {
            writechar();
        }
#endif
}

/* END */
