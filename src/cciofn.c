/* cciofn.c
 * Old name: cc2c.c  12/85	Input/Output Routines  AB
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
 * 20 July 1984	 GT - OUT()  added multi-height graphics
 * 23 Jan  1987	 MT - out()  deleted and replaced with calls to
 *											either out_string or out_char
 *  1 Apr  1988	 MT - incrstore()  deleted and replaced with ccmath(),
 *													which handles add, sub, mul, div,
 *													in addition to incr
 *  15-Nov-89		 ACR Removed RT11 and CP/M stuff. Ported to Microsoft C.
 *  17-Jan-90		 ACR Added incrstore() back in again for compatibility.
 *  13-Feb-91      DJE CC only version. Removed SHARP and MS stuff.
 *                     Name changed to cc2c.c
 *  26-Oct-95      DRC - Added decrstore() to handle decr (decrement keyword)
 *
 * This module provides the following global routines:
 *
 * int yes(void) -- Read a line from stdin and return TRUE if the first
 *						non-blank character is Y or y. Otherwise, return FALSE.
 *
 * storch( store, ch ) -- Store ch at the end of store.
 * int store;
 * SSINT ch;
 *
 * writechar() -- Write a single character from the backup buffer.
 *
 * SSINT inputchar() -- Input a char.  On end of line return CARRIAGERETURN,
 *								on EOF return <CTRL-Z>
 *
 * tblskip( tblpnt ) -- Skip over table entries until an else, endif, or
 * SSINT **tblpnt;      end of entry is found.
 *
 * storematch( tblpnt ) -- Compare the contents of the store specified in
 * SSINT **tblpnt;         the next char in the table with the subsequent
 *									string or store (if following command is cont).
 *									Return -1 (less than), 0 (equal), or 1 (greater
 *									than).
 *
 * incrstore( store ) -- Increment ASCII number in store.
 * int store;
 *
 * decrstore( store ) -- Decrement ASCII number in store.
 * int store;
 *
 * doublebytestore( string ) -- Verifies format of argument to doublebyte command
 * SSINT string;                         and stores arguments as appropriate.
 *
 * ccmath( tblpnt ) -- handle add, sub(tract), mul(tiply), div(ide) and mod(ulo)
 * SSINT  operator,       Command to execute 
 *        **tblpnt;       Pointer to the table pointer/
 *
 * groupinclude(int group ) -- Include another group of changes to be used.
 *
 * groupexclude(int group ) -- Exclude a group of changes.
 *
 */
#ifdef _WINDOWS
#include "windows.h"
#endif
#if defined(UNIXSHAREDLIB)
#include "wtype.h"
#endif

#if defined(_WinIO)
#include "winio.h"
#else
#include <stdio.h>
#endif
#if defined(UNIX)
#include "wtype.h"
#endif
#if defined(_WINDOWS) || defined(UNIXSHAREDLIB)
#include "cctio.h"
#endif
#include <string.h>
#include <stdlib.h>
#include "cc.h"
#include "c8type.h"

/* Static variables */
static char *err_math[4] =  { "non-number",
                              "number greater than 2,000,000,000",
                              "overflow",
                              "divide by zero" };
static char *zero = "an empty store";

/* Indices into the above array */
#define NON_NUMBER		0
#define BIG_NUMBER		1
#define OVERFLOW			2
#define DIVIDE_BY_ZERO	3

/* Prototype for static functions */
static void math_error(char *, char *, char, int, int);

/****************************************************************************/
static void math_error( op_1, op_2, operation, err_type, storenum)
/****************************************************************************/
char *op_1, *op_2;   /* Operand strings */
char operation;      /* Operator (+, -, *, /) */
int err_type,			/* Index into the err_math array (above) */
storenum;			/* Number of the store to be accessed */
/*
 * Description:	Print a warning message on screen.
 *
 * Globals input: cgroup -- group the currently-executing change is in
 *
 */

{
    MSG_STRUCT_S_S *structss;
    MSG_STRUCT_S_C_S *structscs;

    if ( *op_1 == '\0' )
        op_1 = zero;
    if ( *op_2 == '\0' )
        op_2 = zero;

    Msg_s_s.string1 = err_math[err_type];
    Msg_s_s.string2 = sym_name(cgroup, GROUP_HEAD);
    structss = &Msg_s_s;
    Process_msg(errorFunction, 19, 0, (long unsigned) structss);
    Msg_s_c_s.string1 = op_1;
    Msg_s_c_s.char1 = operation;
    Msg_s_c_s.string2 = op_2;
    structscs = &Msg_s_c_s;
    Process_msg(errorFunction, 20, 0, (long unsigned) structscs);
    Process_msg(errorFunction, 6, 0,
                (long unsigned) sym_name(storenum, STORE_HEAD));
}

#if !defined(_WINDOWS) && !defined(UNIXSHAREDLIB)
/****************************************************************************/
int yes(void)						 /* Did the user respond with a Y or a y? */
/****************************************************************************/

/* Description:
 *						This function reads a line from the terminal and returns
 *					TRUE if it starts with Y or y, otherwise returns FALSE.
 *
 * Return values:  TRUE == Y or y was the first non-SPACE, non-TAB char
 *									  on the line
 *						FALSE == something else was the first non-SPACE,
 *									  non-TAB char
 *
 * Globals input: none
 *
 * Globals output: none
 *
 * Error conditions: none
 *
 * Other functions called: none
 *
 */

{
    register int answer;  /* Input character */

    /* Get first character not SPACE or TAB */
    while ( (answer = get_char()) == ' ' || answer == '\t')
        ;

    if (answer != '\n')
        while ( get_char() != '\n' )					/* Ignore rest of line */
            ;

    return(tolower(answer) == 'y');

} /* End--yes */
#endif

/****************************************************************************/
void storch(store, ch)		/* Store a char at the end of the current store */
/****************************************************************************/
register int store;
SSINT ch;

/* Description:
 *						This procedure stores character ch at the end of the
 *					store. It first checks to see if there is room in the store.
 *					If there is not, it tries to make room by moving everything
 *					above up 10 characters. If it can't, it gives a message, and
 *					sets storeoverflow to prevent repeating the message until
 *					non-overflow.
 *
 * Return values: none
 *
 * Globals input: setcurrent -- boolean: TRUE == letter set for searches
 *																	is up to date
 *						storeact -- array of pointers to stores being used
 *										  in matching, if a store is not being used
 *										  its pointer will be set to NULL
 *						storebegin -- array of pointers to the beginning of stores
 *						storend -- array of pointers to the end of stores
 *						storeoverflow -- boolean: TRUE == we have overflowed the
 *																		store area
 *
 * Globals output: setcurrent -- if store was the target of a match,
 *											  setcurrent will be set to FALSE
 *						 storebegin, storend -- if we had to shift things down
 *														  the pointers will be updated
 *						 storeoverflow -- if overflow occurred
 *												  storeoverflow will be set to TRUE
 *
 * Error conditions: if this call to storch caused overflow,
 *							  storeoverflow will get set to TRUE and
 *							  an error message will be displayed
 *
 * Other functions called: None.
 *
 */

{
    register tbltype tp;					/* Working pointer */
    register tbltype end_of_store;	/* Pointer to the end of store */

    /* Note:  The above variables are declared separately because */
    /*				DECUS C does not allow regular typedef's	*/

    int i;

#ifdef DEBUG
    msg_printf("\n--storch called with ch=%02X (%c), store=%s\n",
               (ch & 0xff),ch, sym_name(store, STORE_HEAD));
#endif

    if ( storeact[store] )			  /* Is store being used in a match? */
        setcurrent = FALSE;				  /* Yes, letterset is no longer current */

    end_of_store = storend[store];						/* Point to end of store */

    if (end_of_store >= storebegin[(store + 1)]) {  /* If store needs to move up */
        if (storebegin[NUMSTORES] + 11 >= storelimit)
        {															/* If overflow */

            if (!storeoverflow)		/* Only give overflow message once */
            {
                Process_msg(errorFunction, 4, 0,
                            (long unsigned) sym_name(store, STORE_HEAD));

#ifndef _WINDOWS
                tp = storebegin[store];						/* Dump store to the screen */
                if ( tp < end_of_store )
                    while ( *tp != CARRIAGERETURN )
                        msg_putc(*tp++);
                msg_putc('\n');
#endif
                storeoverflow = TRUE;			/* Avoid repeating the error message */
            }
            return;
        }
        else													 /* Move up to make room */
        {
            /* Shift contents */
            tp = storebegin[NUMSTORES];

            do
            {
                *(tp+10) = *tp;
            } while	(tp-- > end_of_store);


            for ( i = store + 1; i <= NUMSTORES; i++ )  /* Now update pointers */
            {
                storebegin[i] += 10;
                storend[i] += 10;
            }
        } /* End--else (move up to make room) */
	}
    *storend[store]++ = ch;       /* Add the char to store */

    /* storend[store] += 1; */

} /* End--storch */

/****************************************************************************/
void writechar()							/* Write a char from the backup buffer */
/****************************************************************************/

/* Description:
 *						This procedure writes a single character from the backup
 *					buffer.	If just CC, it goes to the output file.
 *					If Manuscripter, it goes into Manuscripter's buffer.
 *
 * Return values: none
 *
 * Globals input: backoutptr -- output pointer for the backup buffer
 *						backbuf -- pointer to the beginning of the backup buffer
 *  (MS only)		mandisplay -- boolean: TRUE == echoing MS code to the screen
 *
 * Globals output: backoutptr -- updated
 *
 * Error conditions: none
 *
 * Other functions called:	out_char -- write a char to the output file
 *									storch -- add a char to a store
 *
 */

{
    if (*backoutptr == CARRIAGERETURN && !binary_mode)		/* If NEWLINE... */
        out_char((SSINT)'\n');                     /* output a newline */
    else
        out_char(*backoutptr);           /* else output the char itself */

    if (++backoutptr >= backbuf+BACKBUFMAX)	/* Handle wraparound */
        backoutptr = backbuf;						/* in the ring buffer */

} /* End--writechar */

/****************************************************************************/
void output(ch)											/* Output or store a char */
/****************************************************************************/
register SSINT ch;

/* Description:
 *						This procedure outputs a char or stores the char if
 *					currently storing.  On output, if the char is a CR it outputs
 *					a NEWLINE.	 If the char is a <CTRL-Z> for EOF, it turns off
 *					storage and all groups.
 *
 * Return values: none
 *
 * Globals input: curstor -- if non-zero, the number of the currently
 *										 active store
 *						numgroupson -- number of groups currently active
 *                setcurrent -- boolean: TRUE == letter set for searches
 *																	is current
 *						backinptr -- input pointer for the backup buffer
 *						backoutptr -- output pointer for the backup buffer
 *						backbuf -- address of the start of the backup buffer
 *						eof_written -- boolean: TRUE == CTRLZ has been output
 *																	 by CC, so numgroupson
 *																	 SHOULD be zero
 *
 * Globals output: curstor -- set to zero on EOF
 *						 numgroupson -- set to zero on EOF
 *						 setcurrent -- set to FALSE on EOF
 *						 eof_written -- set to TRUE on EOF
 *						 backinptr -- updated
 *						 backoutptr -- updated
 *
 * Error conditions: on EOF (char == <CTRL-Z>)
 *							  any active store and groups will be turned off
 *
 *							if the buffer fills up, writechar will be called to free
 *							  up one space
 *
 * Other functions called: storch -- add a char to a store
 *									writechar -- move a char out of the backup buffer
 *
 */

{
#if FALSE
    if (ch == CTRLZ)
    {											/* On <CTRL-Z> close and end */
        curstor = 0;
        numgroupson = 0;		  /* All groups off for any further endfile */
        setcurrent = FALSE;
        eof_written = TRUE;
    }
#endif
    if (ch > 128)
        eof_written= FALSE;

    if (curstor > 0)								/* See if storing */
        storch(curstor, ch);
    else
    {												/* Put char in ring buffer */
        *backinptr++ = ch;
        if (backinptr >= backbuf+BACKBUFMAX)
            backinptr = backbuf;
        if (backinptr == backoutptr)				/* If backbuffer full */
            writechar();
    }

} /* End--output */

/****************************************************************************/
SSINT inputchar()     /* Input a char.  Return <CR> on EOL, <CTRL-Z> on EOF */
/****************************************************************************/

/* Description:
 *						This function inputs a char.	On <END-OF-LINE> it inputs
 *                a CARRIAGERETURN. On EOF it inputs a CTRLZ.
 *
 * Return values: on EOF, return <CTRL-Z>
 *						on EOL, return <CARRIAGERETURN>
 *                otherwise, return input char as an SSINT.
 *
 * Globals input: bEndofInputData -- boolean: TRUE == we're at EOF
 *						infile -- pointer to the input file
 *						filenm -- file name buffer
 *						namelen -- length of file name
 *						argvec -- argvec from command line
 *						inparg -- index into argvec
 *
 * Globals output: bEndofInputData -- updated
 *						 infile -- if we hit EOF on a file, it will either be NULL
 *										 or point to the next file
 *						 filenm -- if we hit EOF on a file, it will either be empty
 *										 or contain the name of the next file
 *						 namelen -- updated
 *						 inparg -- if we hit EOF
 *										 on a file, it will be incremented
 *
 * Error conditions: if we hit EOF, and the requested file was not found
 *							  an error message will be displayed and the user
 *							  will be prompted again
 *
 * Other functions called:
 *
 */

{
    register SSINT ch;  /* Input value from getc */
    SSINT ch2;          /* used to see if we have doublebyte "character" */

    if (bEndofInputData)					/* Return <CTRL-Z> on EOF */
        return(CTRLZ);

L1:

#ifndef _WINDLL
    while ((ch = getc(infile)) == EOF)		/* EOF processing */
    {
        /* if we find eof while looking for possible second byte of
           doublebyte "character" then return -1 which will always
           not be a valid second byte for a doublebyte character.
        */
        if(doublebyte_recursion)
            return(-1);

        if (infile) {
            fclose(infile);									/* Close current file */
            infile = NULL;
        }

        /* display status message */
        if (!noiseless)
            Process_msg(errorFunction, 7, 0, 0);

        get_next_infile( );			/* get next input file */

        if (!infile)				/* no more input files */
        {
            bEndofInputData = TRUE;
            return(CTRLZ);
        }

    } /* End--while ( EOF ) */

    infileposition++;

#else
    // If the last character in the previous input buffer was a possible first
    // half of a doublebyte character, then get that back as first input char
    if (bSavedInChar)
    {
        bSavedInChar = FALSE;         // turn off again for next time through
        ch = inSavedChar & 0xFF;      // use last char from previous buffer
    }
    else
    {
        if (nInSoFar == nInBuf)
        {
            if(lpInProc)
            {
                lpInBuf = lpInBufStart;     // point to start of input buffer again
                // store length info from input callback function as global variable

                nInBuf = (*lpInProc)
                         (lpInBufStart, INPUTBUFFERSIZE, &lDLLUserInputCBData);

                nInSoFar= 0;

                if (nInBuf == -1)
                {
                    nInBuf= 0;
                    bPassedAllData = TRUE;
                }
                else if ( nInBuf == 0)
                    bPassedAllData = TRUE;
                else
                    bPassedAllData = FALSE;
            }
            else
            {
                if (bPassedAllData)
                    nInBuf= 0;
                else
                {
                    bNeedMoreInput= TRUE;
                    return -1;
                }
            }
        }

        if (nInBuf == 0)
        {
            bEndofInputData = TRUE;

            if(doublebyte_recursion)
                return(-1);
            else
                return(CTRLZ);
        }

        nInSoFar++;
        ch= (*lpInBuf++) & 0x00ff;
    }
#endif

    if ((doublebyte_mode) && (ch >= doublebyte1st)
#ifdef _WINDLL
            // do this so don't get premature efosw and lose final byte in file
            && ( nInSoFar < nInBuf )
#endif
            && (!doublebyte_recursion))
    {
        doublebyte_recursion = TRUE;   /* turn on temporarily to prevent further recursion */
        ch2 = inputchar() & 0x00ff;
        doublebyte_recursion = FALSE;  /* reset to its normal state   */
        /* if second byte matches criteria also, then make them a doublebyte,
           unless first is newline, and second is EOF.  */
        if (( ch2 >= doublebyte2nd ) &&
                (( ch != '\n' ) || ( ch2 != 0x00ff )))
        {
            ch = ch * 256 + ch2;                /* treat two bytes as one   */
            return(ch);                         /* return doublebyte "char" */
        }
        else
        {
            if (ch2 != 0x00ff)          /* do not put char back if hit eof   */
            {
#ifndef _WINDLL
                ungetc( ch2, infile );   /* put char back for the next call   */
                infileposition--;        /* reset file position for next time */
#else
                lpInBuf--;               // "put back" char for the next call
                nInSoFar--;              // reset buffer position counter again
#endif
            }
        }
    }
#ifdef _WINDLL
    else
    {
        // if we are in doublebyte mode and this is the last character in
        // the input buffer and it is a possible first byte of a doublebyte
        // pair then store it, and denote that we have stored it so that
        // we can pick it up first thing the next time through here
        // (only do this if bPassedAllData says there will be a next time thru).
        if ((doublebyte_mode) && (ch >= doublebyte1st)
                && (!bPassedAllData)
                && ( nInSoFar == nInBuf ) && (!doublebyte_recursion))
        {
            inSavedChar = (char) ch;    // save the final character in buffer
            bSavedInChar = TRUE;        // denote we have saved an input char
            bEndofInputData = TRUE;     // denote that we are done with buffer
            nInSoFar++;                 // count this last char
            lpInBuf++;                  // point beyond this last character
        }
    }
#endif

    if (binary_mode)
        return(ch);
    else
    {
        switch (ch)					/* Process the character */
        {
        case '\0':							/*   Ignore NULs, because they will hang
            								  * the new font system, which deals with
            								  * NUL-terminated strings
            								  */
#ifdef _WINDLL
            // in DLL case if we have exceeded buffer just return NUL (zero)
            if ( nInSoFar > nInBuf )
                return(ch);
#endif

        case CARRIAGERETURN:				/* Ignore carriage returns */
            goto L1;
        case '\n':							/* use line feed as NEWLINE character */
            /* if possible second byte of a doublebyte
               character (doublebyte_recursion TRUE)
               is newline, then return it immediately */
            if (doublebyte_recursion)
                return('\n');            /* we want this kept as is   */

#ifndef _WINDLL
            infileposition++;           /* two bytes for a new line in text mode */

            if ( (ch = getc( infile )) != EOF )
                ungetc( ch, infile );   /* put char back for the next call   */
#else
            if ((nInSoFar < nInBuf)
                    && ((ch = (SSINT)(*lpInBuf++ & 0x00ff)) == EOF))
            {
                nInSoFar++;                   // denote we read another byte
            }
            else
                lpInBuf--;                    // back up in buffer to this byte
#endif

            return(CARRIAGERETURN);
        default:
            return(
                      ch);
        }
    }
} /* End--inputchar */

/****************************************************************************/
void tblskip(tblpnt)  /* Skip to end-of-entry, else, or endif in table entry */
/****************************************************************************/
SSINT **tblpnt;

/* Description:
 *						This procedure skips over table entries until an else,
 *					endif, or end of entry is found.
 *
 * Return values: if end of entry, *tblpnt points to the SPECPERIOD,
 *						  otherwise, *tblpnt points to the char just beyond
 *						  the command
 *
 * Globals input: none
 *
 * Globals output: none
 *
 * Error conditions: none
 *
 * Other functions called: none
 *
 */

{
    register SSINT *tp;              /* Working pointer into table */
    register SSINT ch;            /* Temp storage for char from table */
    int level;					 /* Level of nesting of begin and end blocks */

    level = 0;												/* Initializations */
    tp = *tblpnt;

    for ( ;; )									/* Do forever	(exit is via a break) */
    {
        ch = *tp++;							/* Get current char for comparison */
        if ( ch == SPECPERIOD )
        {													/* End of entry */
            tp--;										/* Back up to end of entry */
            break;											/* Go exit */
        }
        else if ( ch == BEGINCMD )				/* Deeper nesting */
            level++;
        else if ( ch == ENDCMD )
        {
            if ( --level == 0 )				/* Don't exit at a deeper level than */
                /*  we entered */
                if ( *tp == ELSECMD )
                    tp++;							/* Point to the command beyond the ELSE */
            if ( level <= 0 )
                break;							/* Exit on END */
        }
        else if ( level == 0 )
            if ( ch == ELSECMD || ch == ENDIFCMD )  /* Exit on ELSE or ENDIF */
                break;										  /*	at the current level */
    }

    *tblpnt = tp;						/* Update tblpnt */

} /* End--tblskip */

/****************************************************************************/
int storematch(tblpnt)	/* Compare a store with next thing in table */
/****************************************************************************/
SSINT **tblpnt;  /* Address of pointer into table */

/* Description:
 *						This function compares the content of the storage area
 *					whose number is next in the table with the following string
 *					in the table (terminated by a command or begin).  If the
 *					first char following is a cont command, it picks up the store
 *					number from it and compares the two storage areas instead.
 *					It returns -1 for less than, 0 for equal and 1 for
 *					greater than.
 *
 * Return values:  1 == store > string or contents of second store
 *						 0 == store == string or contents of second store
 *						-1 == store < string or contents of second store
 *
 * Globals input: storebegin -- array of pointers to beginning of stores
 *						storend -- array of pointers to end of stores
 *
 * Globals output: none
 *
 * Error conditions: none
 *
 * Other functions called: valnum -- check for a valid numeric argument
 *                                     (returns TRUE if one found)
 *
 */

{
    register SSINT *sp, /* (fast) Pointer to beginning of store */
    *sp2;  /* (fast) Pointer to the thing to compare to
    *	(either a store or a string in the table)
    */
    SSINT *se,   /* Pointer to end of store */
    *se2,		 /* Pointer to end of store specfied in font */
    *tp,		 /* Working pointer into table */
    ch_temp;  /* Char temp for handling 8-bit stuff */

    char operand_buffer[20];  /* dummy buffer for valnum() */

    long first_val,    /* Value of first operand                           */
    second_val;     /* Value of second operand                          */

    bool bPreNumeric;  /* TRUE if have predefined store with numeric value */

    int i,             /* Index into storebegin and storend arrays         */
    ans;            /* Return value                                     */
    unsigned j;        /* index for when evaluating a predefined store     */

    tp = *tblpnt;                     /* Get working copy of table pointer */

    // if we have nothing in this store, and if the flag is on saying that
    // this is a predefined store then call routine to resolve that store

    if (( storend[*tp] == storebegin[*tp] ) && ( storepre[*tp] != 0 ))
    {
        resolve_predefined(*tp);         // resolve the predefined store
        // (puts value into predefinedBuffer)

        // see if all used characters of predefined store are numeric or not
        // if so leave bPreNumeric as TRUE and set value in first_val

        first_val = 0;
        bPreNumeric = TRUE;
        for (j = 0; j < strlen(predefinedBuffer); j++)
        {
            if (( predefinedBuffer[j] >= '0' ) && ( predefinedBuffer[j] <= '9' ))
                first_val = (first_val * 10) + (predefinedBuffer[j] - '0');
            else
                bPreNumeric = FALSE;       // denote not numeric predefined store
        }
    }
    else
    {
        predefinedBuffer[0] = 0;         // denote we have no predefined store
        bPreNumeric = FALSE;             // denote not numeric predefined store
    }

    /*
     * Check to see if both operands are numeric.
     *  If they are, handle the comparison in terms of their values.
     *
     * Note: an empty store is considered to be a valid numeric argument,
     *        with a value of ZERO
     */

    /* See if the first operand contains a valid numeric value  */
    if ((( predefinedBuffer[0] != 0) && ( bPreNumeric ))
            || (( predefinedBuffer[0] == 0 )
                && ( valnum( tp, operand_buffer, 20, &first_val, TRUE) )))
    {
        /* See if the second operand contains a valid numeric value */
        if ( valnum( (tp + 1), operand_buffer, 20, &second_val, FALSE) )
        {
            /* Both operands are numeric */
            if ( first_val > second_val )
                ans = 1;
            else if ( first_val < second_val )
                ans = -1;
            else
                ans = 0;                   /* They're equal */

            /*
            	* Skip over the match, now that we've executed it
            	*/

            tp++;                      /* Skip the store number */
            if ( *tp != CONTCMD )
            {                    	/* Second operand is a string */
                for ( ; !(*tp & HIGHBIT) || (*tp == TOPBITCMD); tp++ )
                    ;
            }
            else
                tp += 2;                /* Skip cont and its store # */

            *tblpnt = tp;        /* Update table pointer */
            return( ans );
        } /* End--both operands are numeric */
    } /* End--first operand is numeric */

    i = *tp++;						/* Get store number from first command */

    sp = storebegin[i];			/* Get begin and end pointers for store */
    se = storend[i];

    ans = -1;									/* Assume less than */
    if ( *tp == CONTCMD )
    {												/* Content command */
        tp++;
        i = *tp++;						/* Get store number from cont command */
        sp2 = storebegin[i];
        se2 = storend[i];			/* Get begin and end pointers for second store */
        for (;;)
        {
            if ( sp >= se )				/* End of first store? */
            {
                if ( sp2 >= se2 )				/* End of second also? */
                    ans = 0;								/* Must be equal */
                break;
            }
            if ( sp2 >= se2 )			/* End of second store? */
            {
                ans = 1;					/* First store is longer, so */
                break;					 /*  it is greater */
            }
            if ( *sp != *sp2 )					/* Non match? */
            {													/* Which is greater? */
                if ( (unsigned)(*sp) > (unsigned)(*sp2) ) /* DAR fixed for double byte compare  4/24/96 */
                    ans = 1;
                break;
            }
            sp++;					/* Increment pointers */
            sp2++;
        }
    }
    else
    {									/* Match string from table */

        sp2 = tp;		/* Use a fast pointer into the table */
        j = 0;         /* initialize index to predefined store (if any)  */
        for (;;)
        {
            if ( *sp2 == TOPBITCMD )  /* Handle possible elements with high bit on */
            {
                sp2++;
                ch_temp = (*sp2 | HIGHBIT);
            }
            else
                ch_temp = *sp2;

            /* end of the store?          */
            if ((( predefinedBuffer[0] != 0 ) && ( j >= strlen(predefinedBuffer) ))
                    || (( predefinedBuffer[0] == 0) && ( sp >= se )))
            {
                if ( *sp2 & HIGHBIT )       /* End of the string, too?    */
                    ans = 0;                 /*    Must be equal           */
                break;
            }
            if ( *sp2 & HIGHBIT )          /* String done?               */
            {
                ans = 1;                    /* Store is longer,           */
                break;                      /*    so the store is greater */
            }

            /* non-match?                            */
            if ((( predefinedBuffer[0] != 0 )
                    && ( (SSINT)predefinedBuffer[j] != ch_temp) )
                    || (( predefinedBuffer[0] == 0) && ( *sp != ch_temp )))
            {
                /* Which is greater?          */
                if ( predefinedBuffer[0] != 0 )
                {
                    if ( (unsigned)predefinedBuffer[j] > (unsigned)(ch_temp) ) /* DAR fixed for doublebyte compare 4/24/96 */
                        ans = 1;
                }
                else
                {
                    if ( (unsigned)(*sp) > (unsigned)(ch_temp) ) /* DAR fixed for doublebyte compare 4/24/96 */
                        ans = 1;
                }
                break;
            }

            if ( predefinedBuffer[0] != 0 )
                j++;                        /* increment pointer          */
            else
                sp++;                       /* Increment pointer          */
            sp2++;
        }

        while ( !(*sp2 & HIGHBIT)
                || (*sp2 == TOPBITCMD) ) /* Go past unmatched in table */
        {
            if ( *sp2 == TOPBITCMD )       /* Skip element with high bit on */
                sp2++;
            sp2++;
        }

        tp = sp2;
    }			  /* else */
    *tblpnt = tp;                        /* Update table pointer       */
    return(ans);
} /* End--storematch */

/****************************************************************************/
int IfSubset(tblpnt)	/* TRUE if each character in store is found in following
                      * string. */
/****************************************************************************/
SSINT **tblpnt;  /* Address of pointer into table */

/* Description:
 *						This function compares the store with the following string.
 *             The function returns TRUE if each character in the store is
 *             found in the following string.  Dupicate characters in the
 *             following string are both redudant and ignored. The store
 *             with no characters is a subset of any following string.
 *
 * Return values:  1 == Each store member is in following string
 *						 0 == If any store member is not in following string
 *
 * Globals input: storebegin -- array of pointers to beginning of stores
 *						storend -- array of pointers to end of stores
 *
 * Globals output: none
 *
 * Error conditions: none
 *
 */

{
    register SSINT *sp, /* (fast) Pointer to beginning of store */
    *sp2; /* (fast) Pointer to the thing to compare to
    *	(either a store or a string in the table)
    */
    SSINT *se,      /* Pointer to end of store */
    *se2,		 /* Pointer to end of store specfied in font */
    *tp,		 /* Working pointer into table */
    ch_temp;  /* Char temp for handling 8-bit stuff */

    int i,	 /* Index into storebegin and storend arrays */
    ans;	/* Return value */

    tp = *tblpnt;						/* Get working copy of table pointer */

    i = *tp++;						/* Get store number from first command */

    sp = storebegin[i];			/* Get begin and end pointers for store */
    se = storend[i];

    ans = 0;									/* Assume FALSE */
    if ( *tp == CONTCMD )
    {												/* Content command */
        tp++;
        i = *tp++;						/* Get store number from cont command */
        se2 = storend[i];			/* Get begin and end pointers for second store */
        for (;;)
        {
            if ( sp >= se )				/* End of first store? */
            {
                ans = 1;                /* All must have been found */
                break;
            }
            for(sp2 = storebegin[i];;)
            {
                if ( sp2 >= se2 )				/* End folowing string? */
                {
                    ans = 0;
                    break;
                }
                if ( (unsigned)(*sp) == (unsigned)(*sp2) ) /* DAR fixed for doublebyte compare 4/24/96 */
                {
                    ans = 1;						/* Found match */
                    break;
                }
                sp2++;                     /* Check next character */
            }
            if(!ans)
                break;                     /* This character failed to match */
            sp++;                         /* Match found, try next one */
        }
    }
    else
    {									/* Match string from table */

        se2 = tp;                  /* Find end of string in table */
        while ( !(*se2 & HIGHBIT)
                || (*se2 == TOPBITCMD) ) /* Go past unmatched in table */
        {
            if ( *se2 == TOPBITCMD )    /* Skip element with high bit on */
                se2++;
            se2++;
        }

        for (;;)
        {
            if ( sp >= se )				/* End of the store? */
            {
                ans = 1;						/* All must have matched */
                break;
            }
            for(sp2 = tp;;)   /* Use a fast pointer into the table */
            {
                if ( sp2 >= se2 )				/* End folowing string? */
                {
                    ans = 0;               /* Must be no match */
                    break;
                }
                if ( *sp2 == TOPBITCMD )  /* Handle possible element with high bit on */
                {
                    sp2++;
                    ch_temp = (*sp2 | HIGHBIT);
                }
                else
                    ch_temp = *sp2;

                if ( (unsigned)(*sp) == (unsigned)(ch_temp) ) /* DAR fixed for doublebyte compare 4/24/96 */
                {
                    ans = 1;             /* Match found */
                    break;
                }

                sp2++;
            }
            if(!ans)
                break;

            sp++;
        }

        tp = se2;
    }			  /* else */
    *tblpnt = tp;								/* Update table pointer */
    return(ans);
} /* End--IfSubset */

/************************************************************************/
void LengthStore(SSINT **cppTable)
/************************************************************************/
/* Description:
 *			This function returns to the output stream an ASCII string
 *       which gives the number of characters in the given store.
 *
 * Return values:  none
 *
 * Globals input: storebegin -- array of pointers to beginning of stores
 *						storend -- array of pointers to end of stores
 *
 * Globals output: none
 *
 * Error conditions: none
 *
 */
{
    long lLength;
    int i;
    SSINT *cpWorkTable;    /* Working copy of table pointer */
    char   cpAsciiLength[15];
    char  *cpAscii;

    cpWorkTable = *cppTable;	 	/* Get working copy of table pointer */

    i = *cpWorkTable++;			/* Get store number from first command */
    lLength = (long)(storend[i] - storebegin[i]);

#if defined(UNIX)
    sprintf(cpAsciiLength,"%ld", lLength);
#else
    ltoa(lLength, cpAsciiLength, 10);  /* Convert to decimal */
#endif

    cpAscii = &cpAsciiLength[0];
    while(*cpAscii)
        output(*cpAscii++);

    *cppTable = cpWorkTable;   /* Update pointer */

}

/************************************************************************/
void PushStore(void)
/************************************************************************/
/* Description:
 *			This function saves the currect storing context on a stack.  It
 *			is used in conjuction with PopStore to allow the CC programmer
 *       to write groups that don't disturbe the current storing context.
 *
 * Return values:  none
 *
 * Globals input: curstor -- number of the store currently in use.
 *                iStoreStackIndex -- depth in current stack.
 *
 * Globals output: none
 *
 * Error conditions: Store stack overflow.
 *
 */
{
    if(iStoreStackIndex >= STORESTACKMAX)
    {
        Process_msg(errorFunction, 8, 0, 0);
    }
    else
        StoreStack[iStoreStackIndex++] = curstor;
}

/************************************************************************/
void PopStore(void)
/************************************************************************/
/* Description:
 *			This function restores a previously saved storing context. It
 *			is used in conjuction with PushStore to allow the CC programmer
 *       to write groups that don't disturbe the current storing context.
 *
 * Return values:  none
 *
 * Globals input: curstor -- number of the store currently in use.
 *                iStoreStackIndex -- depth in current stack.
 *
 * Globals output: none
 *
 * Error conditions: Store stack underflow.
 *
 */
{
    if(iStoreStackIndex == 0)
    {
        Process_msg(errorFunction, 9, 0, 0);
    }
    else
        curstor = StoreStack[--iStoreStackIndex];
}

/************************************************************************/
void BackCommand(SSINT **cppTable, bool utf8)
/************************************************************************/
{
    SSINT *cpTbl, *cpTmp;
    int i;
    long lStoreValue;
    char  caOperandBuffer[20];  /* dummy buffer for valnum() */
    bool bErrorInProcessing= FALSE;

    cpTbl = *cppTable;
    i = *cpTbl++;
    if( i == VALCMD )
    {
        valnum( ++cpTbl, caOperandBuffer, 20, &lStoreValue, TRUE);
        /* Check for reasonable size */
        if((lStoreValue > 32767L) || (lStoreValue < 0))
        {
            Process_msg(errorFunction, 3, 0, (long unsigned) lStoreValue);
            lStoreValue = 1L;
        }
        i = (int)(lStoreValue & 0x7FFF);
        if (curstor == 0)
        {
            if(lStoreValue > (long)(BACKBUFMAX))
            {
                Process_msg(errorFunction, 10, 0, (long unsigned) i);
                i = 1;
            }
        }
        else
        {     /* If value is larger than the store, back the whole store */
            if(i > (storend[curstor] - storebegin[curstor]))
            {
                i = (storend[curstor] - storebegin[curstor]);
            }
        }
        cpTbl++;
    }

    if (i > 0 &&
            eof_written)
    {
        eof_written= FALSE;
        i--;
    }

    for ( ; !bErrorInProcessing && i-- > 0; )
    {
        do {
            if (curstor == 0)
            {													/* If not storing */
                if ( (backinptr == backbuf) && (backinptr != backoutptr) )
                    cpTmp = backbuf+BACKBUFMAX-1;			/* Handle back-wrap */
                else
                    cpTmp = backinptr - 1;

                if ( (backinptr == backoutptr) || (matchpntr <= match) )
                {
                    /* Error if backbuf empty or match full */
                    Process_msg(errorFunction, 0, 0, 0);
                    bErrorInProcessing= TRUE;
                    break;
                }
                else
                {
                    *--matchpntr = *cpTmp;			/* Transfer one char */
                    backinptr = cpTmp;				/* Update global pointer */
                }
            }
            /* Storing */

            else if ( storend[curstor] <= storebegin[curstor] ||
                      matchpntr <= match )
            {
                Process_msg(errorFunction, 11, 0, 0);
                bErrorInProcessing= TRUE;
                break;
            }
            else
                *--matchpntr = *--storend[curstor];

        } while (utf8 && ((*matchpntr & 0xC0) == 0x80));
    } /* end for */
    *cppTable = cpTbl;   /* Update table pointer */
}

/************************************************************************/
void FwdOmitCommand(SSINT **cppTable, bool *pbOmitDone, bool fwd, bool utf8)
/************************************************************************/
{
    SSINT *tpx;
    int i;

    tpx= *cppTable;

    if (*tpx > maxsrch)
        maxsrch= *tpx;

    refillinputbuffer();

    if (bNeedMoreInput)
    {
        tpx--;
    }
    else
    {
        for ( i = *tpx++; !eof_written && i-- > 0; )
        {
            do
            {
                // if we have already processed all the data, then skip out
                if ( matchpntr == matchpntrend )                // fix CC_041
                {
                    eof_written = TRUE;   // do not keep going any more
                    tpx++;                // skip over argument to omit or fwd
                    *pbOmitDone = TRUE;   // fwd or omit done with this match
                    break;
                }

                if ( fwd )
                    output( *matchpntr);

                matchpntr++;
            } while (utf8 && ((*matchpntr & 0xC0) == 0x80));
        }
    }
    *cppTable= tpx;
}

/************************************************************************/
void incrstore(int j)			/* Increment ASCII number in store */
/************************************************************************/
/*
 * Description -- If the store is being used in a match
 *                   Say that the letter set for matches is not current.
 *
 *                Get a local copy of the beginning address of the store.
 *
 *                Go through the store, starting at the right-most char
 *                   If char is ASCII 9
 *                      Set char to ASCII 0.
 *                   Else
 *                      Increment char.
 *                      Return.
 *
 *                (we must have overflowed)
 *
 *                Store another zero as the right-most digit.
 *                Put a 1 in as the left-most digit.
 *
 * Return values: none
 *
 * Globals input: storeact -- array of pointers for stores being used
 *                              for matching
 *                setcurrent -- boolean: TRUE == letter set for matches is
 *                                                 up to date
 *                storebegin -- array of pointers to the beginning of stores
 *                storend -- array of pointers to the end of stores
 *
 * Globals output: setcurrent -- if the store is being used in matching,
 *                                 set to FALSE
 *
 * Error conditions: if we ran out of room in the store area,
 *                     storeoverflow will be set to TRUE
 *                   if the store was being used in matching,
 *                     setcurrent will be set to FALSE
 *
 * Other functions called: storch -- add a char to a store
 *
 */
{

    SSINT *tb;            /* Local copy of the beginning address of the store */
    register SSINT *tp;   /* Working pointer */

    if ( storeact[j] )		/* Is this store being used for matching? */
        setcurrent = FALSE;

    tb = storebegin[j];		/* Get a local copy of beginning address */

    for ( tp = storend[j]; --tp >= tb; )		/* Go through the store, */
    {													/*  right to left */
        if ( *tp != '9' )
        {						/* Increment digit */
            (*tp)++;
            return;
        }
        else
            *tp = '0';			/* Make 9 into 0 */
    }

    /* Overflow has occured when for exits normally */

    storch(j, (SSINT)'0');         /* Add the right-most zero */

    *tb = '1';					/* Insert a 1 as the new left-most digit */

} /* End--incrstore */


/************************************************************************/
void decrstore(int j)                /* Decrement ASCII number in store */
/************************************************************************/
/*
 * Description -- If the store is being used in a match
 *                   Say that the letter set for matches is not current.
 *
 *                Get a local copy of the beginning address of the store.
 *
 *                Go through the store to see if is all char ASCII 0's
 *                   If so then print appropriate warning message, return.
 *
 *                Go through the store, starting at the right-most char
 *                   If char is ASCII 0
 *                      Set char to ASCII 9.
 *                   Else
 *                      Decrement char.
 *                      Return.
 *
 *
 * Return values: none
 *
 * Globals input: storeact -- array of pointers for stores being used
 *                              for matching
 *                setcurrent -- boolean: TRUE == letter set for matches is
 *                                                 up to date
 *                storebegin -- array of pointers to the beginning of stores
 *                storend -- array of pointers to the end of stores
 *
 * Globals output: setcurrent -- if the store is being used in matching,
 *                                 set to FALSE
 *
 * Error conditions: if the store was being used in matching,
 *                     setcurrent will be set to FALSE
 *
 */
{

    SSINT *tb;            /* Local copy of the beginning address of the store */
    register SSINT *tp;   /* Working pointer */

    if ( storeact[j] )		/* Is this store being used for matching? */
        setcurrent = FALSE;

    tb = storebegin[j];        /* Get a local copy of beginning address */

    tp = storend[j];

    tp--;                      /* get to first character.  */

    if (tp < tb)
    {
        Process_msg(errorFunction, 12, 0, 0);
    }
    /* Go through the store from the right end
       until we get to the leftmost character or
       the first non-zero.  Then check that character
       (be it a non-zero or the leftmost character)
       to see if we have totally zeroes.          */
    while ((tp > tb) && (*tp == '0'))              /* Go through the store  */
    {
        tp--;
    }

    if (*tp == '0')                                 /* Is store all zeros? */
    {
        Process_msg(errorFunction, 13, 0, 0);
        return;
    }

    for ( tp = storend[j]; --tp >= tb; )		/* Go through the store, */
    {													/*  right to left */
        if ( *tp != '0' )
        {                               /* Decrement digit */
            (*tp)--;
            return;
        }
        else
            *tp = '9';                      /* Make 0 into 9 */
    }

} /* End--decrstore */


/************************************************************************/
void doublebytestore (SSINT doublearg)  /* process doublebyte argument  */
/************************************************************************/
/*
 * Description -- Process doublebyte argument,
 *                However, the early scanning for doublebyte already
 *                did this, so this just evrifies that the input now
 *                looks OK, and if not puts outs warning message(s).
 *
 * Return values: none
 *
 * Globals input: doublebyte1st set to valid derived from earlier scan
 *                doublebyte2nd set to valid derived from earlier scan
 *                doublebyte_mode should already be set to TRUE
 *
 * Globals output: none
 *
 */
{
    if (doublebyte_mode != TRUE)
        Process_msg(errorFunction, 14, 0, 0);
    /* if values are non-zero (doublebyte1st non-zero), then this argument
       should match at least one of the values, else put out warning message.
    */
    if ((doublebyte1st != 0) &&
            (doublearg != doublebyte1st) &&
            (doublearg != doublebyte2nd))
        Process_msg(errorFunction, 14, 0, 0);

} /* End--doublebytestore */

/****************************************************************************/
void ccmath( operator, tblpnt )   /* Perform a mathematical operation in CC */
/****************************************************************************/
SSINT  operator,   /* Command to execute */
**tblpnt;   /* Pointer to the table pointer */

/*
 * Description:
 *						This function will perform the CC mathematical functions
 *					add, sub(tract), mul(tiply), div(ide), and mod(ulo).
 *
 * Return values: the store specified by the command will contain the
 *						  result of the operation.
 *
 * Globals input: storeact -- array of flags for stores being used
 *											for matching
 *						setcurrent -- boolean: TRUE == letter set for matches is
 *																	up to date
 *						cgroup -- group the currently-executing change is in
 *
 * Globals output: setcurrent -- if the destination store for the operation
 *												is being used in matching, set to FALSE
 *
 * Error conditions: If a command has a non-numeric argument, the store
 *							specified in the command will be unchanged.
 *
 * Other procedures called:
 *									long_abs -- absolute value of a long int
 *									valnum -- validate a numeric argument
 *
 */

{
    char sign_check,        /* Flag byte for signs of the operands */
    operation;          /* What we're doing (+, -, *, or /) */
    register SSINT *tp;     /* Copy of the table pointer */
    long int first_operand,
    second_operand,
    l_tmp_1,
    l_tmp_2,			/* Used to store absolute values of operands,
    					*	 because DECUS C doesn't correctly compare
    					*	 the results of long int functions
    					*/
    l_temp;			/* Used in checking for overflow */
    int store,					/* Store used for the first operand */
    valcode1, valcode2,  /* Return values from validation routine */
    i;

    char num_buf[12];      /* Buffer for converting result back to ASCII
    						*	(10 digits, sign, and string-terminating NUL)
    						*/
    char  op1_buf[20],
    op2_buf[20];     /* Buffers for the two operands for valnum */

    switch ( operator )
    {
    case ADDCMD:
        operation = '+';
        break;
    case SUBCMD:
        operation = '-';
        break;
    case MULCMD:
        operation = '*';
        break;
    case DIVCMD:
        operation = '/';
        break;
    case MODCMD:
        operation = '%';
        break;
    default:
        operation = '?';			/* How did we get here then? */
    } /* End--switch */

    tp = *tblpnt;			/* Get a working copy of the table pointer */
    store = *(tp);			/* Get the store # for the destination */
    if ( storeact[store] )		/* Is this store being used for matching? */
        setcurrent = FALSE;

    /* Validate the operands */
    valcode1 = valnum( tp, op1_buf, 20, &first_operand, TRUE);
    tp++;											/* Move to the second operand */
    valcode2 = valnum( tp, op2_buf, 20, &second_operand, FALSE);
    if ( (!valcode1) || (!valcode2) )
    {
        math_error( op1_buf, op2_buf, operation, NON_NUMBER, store );
        goto DONE;
    }
    if ( (valcode1 == -1) || (valcode2 == -1) )
    {
        math_error( op1_buf, op2_buf, operation, BIG_NUMBER, store );
        goto DONE;
    }

    /* Check signs of both operands */
    if ( first_operand < 0L )		/* Get sign of the first operand */
        sign_check = 0x2;
    else
        sign_check = 0;
    if ( second_operand < 0L )		/* Get the sign of the second operand */
        sign_check |= 0x1;

    /* Perform the operation */
    switch( operator )
    {
    case ADDCMD:
        if ( (sign_check != 1) && (sign_check != 2) )
        {												/* signs are the same */
            l_tmp_1 = long_abs( first_operand );
            l_tmp_2 = long_abs( second_operand );
            l_temp = l_tmp_1 + l_tmp_2;
            if ( (l_temp < 0L) || (l_temp > 1999999999L) )
            {
                math_error( op1_buf, op2_buf, operation, OVERFLOW, store );
                break;
            }
        }
        first_operand = first_operand + second_operand;
        break;

    case SUBCMD:
        if ( (sign_check != 0) && (sign_check != 3) )
        {									/* signs are different */
            l_tmp_1 = long_abs( first_operand );
            l_tmp_2 = long_abs( second_operand );
            l_temp = l_tmp_1 + l_tmp_2;
            if ( (l_temp < 0L) || (l_temp > 1999999999L) )
            {
                math_error( op1_buf, op2_buf, operation, OVERFLOW, store );
                break;
            }
        }
        first_operand = first_operand - second_operand;
        break;

    case MULCMD:
        l_tmp_1 = long_abs( first_operand );
        l_tmp_2 = long_abs( second_operand );
        l_temp = l_tmp_1 * l_tmp_2;
        if ( (l_temp < l_tmp_1) && (l_temp < l_tmp_2) )
        {
            math_error( op1_buf, op2_buf, operation, OVERFLOW, store );
            break;
        }
        first_operand = first_operand * second_operand;
        break;

    case DIVCMD:
        if ( second_operand == 0L )
        {
            math_error( op1_buf, op2_buf, operation, DIVIDE_BY_ZERO, store );
            break;
        }
        first_operand = first_operand / second_operand;
        break;

    case MODCMD:
        if ( second_operand == 0L )
        {
            math_error( op1_buf, op2_buf, operation, DIVIDE_BY_ZERO, store );
            break;
        }
        first_operand = first_operand % second_operand;
        break;
    } /* End--switch */

    /* Store the result */
    sprintf( num_buf, "%ld", first_operand );     /* Convert to ASCII */
    storend[store] = storebegin[store];

    for (i = 0; num_buf[i] != '\0'; i++)     /* put string into store */
        storch(store, (SSINT) num_buf[i]);

    /* Skip to the next command */
DONE:
    tp = (*tblpnt) + 1;				/* Skip the store # */
    if ( *tp != CONTCMD )
    {												/* Second operand is a string */
        for ( ; !(*tp & HIGHBIT) || (*tp == TOPBITCMD); tp++ )
            ;
    }
    else
        tp += 2;						/* Skip cont and its store # */

    *tblpnt = tp;			/* Update the pointer before returning */

} /* End--ccmath */

/****************************************************************************/
void groupinclude(register int group)  /* Include another group of changes in search area */
/****************************************************************************/

/* This procedure includes a new group of changes in the list of groups
 *   being searched.
 *
 * Description -- If trying to use too many groups
 *							Display an error message on the screen.
 *							Return.
 *
 *						If the group is already in use
 *							Make it the last group to be searched.
 *						Else
 *							Increment count of groups in use.
 *							Add the new group to the array of groups in use.
 *							Say that the letter set for matching is
 *							  not up to date.
 *
 * Return values: none
 *
 * Globals input: numgroupson -- number of groups currently being used
 *											  for matching
 *						curgroups -- array of group numbers for searching
 *						setcurrent -- boolean: TRUE == letter set for matching
 *																	is up to date
 *
 * Globals output: numgroupson -- if not too many groups being searched
 *												incremented by 1
 *						 curgroups -- if not too many groups being searched and
 *											 new group added as last entry
 *						 setcurrent -- if not too many groups being searched
 *											  set to FALSE
 *
 * Error conditions: if trying to search too many groups
 *							  an error message will be displayed on the screen
 *
 * Other functions called: none
 *
 */

{
    register int i;		/* Loop index for searching curgroups */
    char found;				/* Boolean: TRUE == group is currently active */


    for (i = 1, found = FALSE; i <= numgroupson && !found; i++)
    {										/* See if the group is already active */

        if ( curgroups[i] == group )
        {
            found = TRUE;						/* Group is already active, */
            while (i < numgroupson) 		/*	make it the last group searched */
            {
                curgroups[i] = curgroups[i+1];
                i++;
            }
            curgroups[numgroupson] = group;
            setcurrent = FALSE;
        }
    }

    if ( !found )
    {
        if ( numgroupson == GROUPSATONCE )
        {
            Process_msg(errorFunction, 5, 0, (long unsigned) GROUPSATONCE);
        }
        else
        {
            numgroupson++;						/* Not there, so we need to add it */
            curgroups[numgroupson] = group;

            setcurrent = FALSE;							/* Must recompute letterset */
        }
    }


} /* End--groupinclude */

/****************************************************************************/
void groupexclude(register int group)				  /* Exclude a group of changes */
/****************************************************************************/

/* This procedure excludes a group from the list of groups being searched.
 *
 * Description -- If group is not currently in use.
 *							Display an error message on the screen.
 *							Return.
 *
 *						If the group is not currently active
 *							Give a warning message.
 *						If the group is in use
 *							Delete it from the list.
 *							Decrement the count of groups in use.
 *						Say that the letter set for matching is
 *						  not up to date.
 *
 * Return values: none
 *
 * Globals input: numgroupson -- number of groups currently being used
 *                               for matching
 *						curgroups -- array of group numbers for searching
 *						setcurrent -- boolean: TRUE == letter set for matching
 *																	is up to date
 *
 * Globals output: numgroupson -- if group was in use
 *												decremented by 1
 *						 curgroups -- if the group was in use,
 *											 group deleted
 *						 setcurrent -- if the group was in use
 *											  set to FALSE
 *
 * Error conditions: if the group was not in use
 *							  an error message will be displayed on the screen
 *
 * Other functions called: none
 *
 */

{
    register int i;	/* Loop index for searching curgroups */
    char found;			/* Boolean: TRUE == found the group */


    for( i = 1, found = FALSE; i <= numgroupson; i++ )
    {																/* Is the group active? */
        if ( curgroups[i] == group )
            found = TRUE;						/* group is active */

        /* Compress curgroups to get rid of
        *	 the entry for the group
        */
        if ( found )
            curgroups[i] = curgroups[(i + 1)];
    }
    if ( found )
    {
        numgroupson--;
        setcurrent = FALSE;							/* Must recompute letterset */
    }
    else
        Process_msg(errorFunction, 2, 0,
                    (long unsigned) sym_name(group, GROUP_HEAD));

} /* End--groupexclude */

/* END */
