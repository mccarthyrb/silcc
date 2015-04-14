/* ccompfn.c
 * Old name: cc1b.c  04/86	CC Compiling Functions	AB
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
 *                     Name changed to cc1b.c
 *
 * This module provides the following global routines:
 *
 * tblcreate() -- Allocate memory for all dynamically allocated tables,
 *							using tblalloc.
 *
 *	tblalloc(count, size) -- A safe malloc(). Bails out with error message
 *	unsigned count, size;		if insufficient memory.
 *
 *	max_heap() -- Compute how much heap remains (up to size of 1 segment)
 *
 * storechar( ch ) -- Store one element into the internal table.
 * SSINT ch;          If the table is full, set tblfull to TRUE.
 *
 * srchlen( tp1 ) -- Return the length of the match of a change.
 * tbltype tp1;
 *
 * compilecc() -- Prompt for CC table and handle it if there is one.
 *						If the table is umcompiled, compile it as it is read in.
 *
 * cctsetup() -- Initialize and set up pointer tables for changes,
 *						 fonts, defines, and groups.
 *
 * cctsort() -- Sort each group in the change table first by first character
 *						then by length of change.	(Longest change first)
 *
 */

#if defined(UNIX)
#include <unistd.h>
#endif

#ifdef _WINDOWS
#include "windows.h"
#endif

#if defined(_WinIO)
#include "winio.h"
#else
#include <stdio.h>
#endif
/* #include <stdlib.h> */
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
#include <io.h>
#endif
#if defined(UNIX)
#include "wtype.h"
#endif
#include "cc.h"
#include "c8type.h"
#if defined(_WINDOWS) || defined(UNIXSHAREDLIB)
#include "cctio.h"
#endif
#ifndef min
#define min(x,y) ((x<y)?x:y)	/* return smaller of x and y */
#endif
#if defined(_WINDOWS) || defined(UNIXSHAREDLIB)
#define xgetc(x) wgetc(x)
#define xungetc(x,y) wungetc(x,y)
#define xfeof(x) wfeof(x)
#else  
#define xgetc(x) getc(x)    
#define xungetc(x,y) ungetc(x,y)
#define xfeof(x) feof(x)
#endif

/* Static variables */
static char *names[4] =
    {
        "Stores:  ",
        "Switches:",
        "Groups:  ",
        "Defines: "
    };

/* Static functions defined in this module */
static void check_symbol_table(void);
static void dump_symbol_table(void);
static void discard_symbol_table(void);
static void fontmsg(void);
static bool goes_before(tbltype, tbltype);


/*****************************************************************************/
static void check_symbol_table() /* Check symbol table for possible typos */
/*****************************************************************************/

/*
 * Description:
 *						Check the symbol table for symbols which were referenced
 *						  but not defined, which usually indicates a
 *						  misspelled name.
 *
 * Return values: none
 *
 * Globals input:
 *						 sym_table -- symbol table array
 *
 * Globals output: none
 *
 * Error conditions: If a symbol is found that is referenced but not
 *							  defined, an appropriate warning message will be
 *							  displayed on the screen.
 *
 * Other procedures called: none
 *
 */

{
    int i;  /* Loop index */
    register CC_SYMBOL *sym_pntr;  /* Pointer for traversing the lists */

    for (i = 0; i < 4; i++ )
    {
        if ( (sym_pntr = sym_table[i].list_head) != NULL )
        {
            while ( sym_pntr != NULL )				/* Check the symbol table */
            {
                if ( sym_pntr->use == REFERENCED )
                {
                    switch( i )
                    {
                    case STORE_HEAD:
                        // do not print warning message if we are looking at an
                        // out command with a predefined store
                        if (( storepre[sym_pntr->number] != 0 ) &&
                                (strncmp(sym_pntr->name, "cccurrentdate", 13) == 0 ))
                            break;
                        if (( storepre[sym_pntr->number] != 0 ) &&
                                (strncmp(sym_pntr->name, "cccurrenttime", 13) == 0 ))
                            break;
                        if (( storepre[sym_pntr->number] != 0 ) &&
                                (strncmp(sym_pntr->name, "ccversionmajor", 14) == 0))
                            break;
                        if (( storepre[sym_pntr->number] != 0 ) &&
                                (strncmp(sym_pntr->name, "ccversionminor", 14) == 0))
                            break;
                        Process_msg(errorFunction, 32, 0,
                                    (long unsigned) sym_pntr->name);
                        break;
                    case SWITCH_HEAD:
                        Process_msg(errorFunction, 33, 0,
                                    (long unsigned) sym_pntr->name);
                        break;
                    case GROUP_HEAD:
                        Process_msg(errorFunction, 34, 0,
                                    (long unsigned) sym_pntr->name);
                        break;
                    case DEFINE_HEAD:
                        Process_msg(errorFunction, 35, 0,
                                    (long unsigned) sym_pntr->name);
                        break;
                    } /* End--switch */

                } /* End--if (referenced-but-undefined symbol) */

                sym_pntr = sym_pntr->next;
            } /* End--traverse the symbol table */

        } /* End--if non-empty symbol table */

    } /* End--for */

} /* End--check_symbol_table */

int get_symbol_number(char * symbol_name, int iSymbolTable)
{
    register CC_SYMBOL *sym_pntr;  /* Pointer for traversing the lists */

    for (sym_pntr= sym_table[iSymbolTable].list_head; sym_pntr != NULL; sym_pntr = sym_pntr->next)
        if (strcmp(symbol_name, sym_pntr->name) == 0)
            break;

    if (sym_pntr)
        return sym_pntr->number;
    else
        return -1;
}

/*****************************************************************************/
static void dump_symbol_table()	  /* Dump symbol table for debug display */
/*****************************************************************************/

/*
 * Description:
 *						Write the symbol table to the screen.	If a table-header
 *							(i.e.--stores, switches, groups, or defines) is empty
 *							say that there are "None" of that category.
 *
 * Return values: none
 *
 * Globals input:
 *						 sym_table -- symbol table array
 *
 * Globals output: none
 *
 * Error conditions: none
 *
 * Other procedures called: none
 *
 */

{
    int i;  /* Loop index */
    register CC_SYMBOL *sym_pntr;  /* Pointer for traversing the lists */
    int  echo_to_stdout;			/* Boolean, TRUE == echo debug info to stdout */

    echo_to_stdout = !isatty( fileno( msgfile ));

    for (i = 0; i < 4; i++ )
    {
        if ( (sym_pntr = sym_table[i].list_head) != NULL )
        {
            msg_printf("%s  Number  Name\n", names[i]);  /* Column headings */
            if ( echo_to_stdout )
                fprintf( msgfile, "%s  Number  Name\n", names[i] );

            while ( sym_pntr != NULL )				/* Print the symbol table */
            {
                msg_printf("\t   %3d     %s\n",
                           sym_pntr->number, sym_pntr->name );
                if ( echo_to_stdout )
                    fprintf(msgfile, "\t   %3d     %s\n",
                            sym_pntr->number, sym_pntr->name );

                sym_pntr = sym_pntr->next;
            } /* End--traverse the symbol table */
        } /* End--if non-empty symbol table */
        else
        {
            msg_printf("%s\tNone\n", names[i]);		  /* Nothing there */
            if ( echo_to_stdout )
                fprintf(msgfile, "%s\tNone\n", names[i] );
        }
    } /* End--for */

} /* End--dump_symbol_table */

/*****************************************************************************/
static void discard_symbol_table()	  /* Discard symbol table */
/*****************************************************************************/

/*
 * Description:	Free symbol table
 *
 * Return values: none
 *
 * Globals input:
 *						 sym_table -- symbol table array
 *
 * Globals output:	none
 *
 * Error conditions: none
 *
 * Other procedures called: none
 *
 */

{
    int i;  /* Loop index */
    register CC_SYMBOL *sym_pntr;  /* Pointer for traversing the lists */
    register CC_SYMBOL *temp_pntr;

    /* For all 4 symbol tables, loop through freeing all entries */
    for (i = 0; i < 4; i++ )
    {
        sym_pntr = sym_table[i].list_head;
        while ( sym_pntr != NULL )
        {
            free(sym_pntr->name);
            temp_pntr = sym_pntr;
            sym_pntr = sym_pntr->next;
            free(temp_pntr);
        }
        sym_table[i].list_head = NULL;
    } /* End--for */

} /* End--discard_symbol_table */

/****************************************************************************/
void *tblalloc(count, size)						/* Allocate tables */
/****************************************************************************/
unsigned count,	/* # of objects in the requested table */
size;		/* Length of the objects in bytes */

/* Description -- Try to allocate the requested table.
 *						If unsuccessful
 *							Display an error message.
 *							Exit with exit code = BADEXIT.
 *
 *						Return a pointer to the table.
 *
 * Return values: Return a pointer to requested table.
 *
 * Globals input: none
 *
 * Globals output: none
 *
 * Error conditions:  If malloc fails we will be unable to continue,
 *								so display an informative message and exit
 *								to the operating system.
 *
 * Other functions called: none
 *
 */

{
    void *mp;  /* Pointer to the table (returned by malloc) */
#ifdef DEBUG
    fprintf(msgfile,"tblalloc -- count = %u, size = %u\n", count, size);
#endif
    mp = malloc( (unsigned) (count * size));	/* Try to allocate the table */

    if ( mp == NULL )
    {
        Process_msg(errorFunction, 36, 0, 0);
        bailout(BADEXIT, FALSE);				/*  and bail out		 */
    }
    return(mp);			/* Return a pointer to the table */

} /* End--tblalloc */

/****************************************************************************/
void tblcreate()													 /* Create tables */
/****************************************************************************/

/* Description:
 *						Allocate the tables, using tblalloc (above).
 *
 * Return values: All dynamically allocated tables allocated.
 *
 * Globals input: backbuf -- ring buffer for backup
 *						cngpointers -- pointers to changes in table
 *						match -- match area for searching
 *  (MS only)		tabs -- tabs array
 *
 * Globals output: All of those input, pointing to actual data areas
 *
 * Error conditions: If we encounter an error allocating anything but
 *							  table, tblalloc will exit to the OS for us.
 *
 * Other functions called: tblalloc -- allocate a table
 *
 */

{
#if defined(_WINDOWS) || defined(UNIXSHAREDLIB)
    //these should work well for DOS EXE or FUNC versions
    // perhaps we should replace the older code after memory alloc testing
    if (backbuf == NULL)										//7.4.15
        backbuf = (SSINT *) tblalloc(BACKBUFMAX, sizeof(*backbuf));
    if (cngpointers == NULL)
        cngpointers = (tbltype *) tblalloc(MAXCHANGES+1, sizeof(*cngpointers));
    if (match == NULL)
        match = (SSINT *) tblalloc(MAXMATCH, sizeof(*match));
#else	
    //this is the older DOS EXE code
    backbuf = (SSINT *) tblalloc(BACKBUFMAX, sizeof(*backbuf));
    cngpointers = (tbltype *) tblalloc(MAXCHANGES+1, sizeof(*cngpointers));
    match = (SSINT *) tblalloc(MAXMATCH, sizeof(*match));
#endif	

} /* End--tblcreate */

#if defined(_WINDOWS) || defined(UNIXSHAREDLIB)
/****************************************************************************/
void tblfree()					/* Free all memory used by tables */ //7.4.15
/****************************************************************************/

/* Description:
 *				Free all memory used by various tables, allocated by
 *					tblcreate() and compilecc() and symbol table compilation
 *
 * All the malloc/free code needs to be tested by a memory debugger
 *
 * Return values: none
 *
 * Globals input:	backbuf -- ring buffer for backup
 *					cngpointers -- pointers to changes in table
 *					match -- match area for searching
 *                  table -- compiled CC table
 *					sym_table --
 *
 * Globals output: Global pointers to the former tables are set to NULL
 *
 * Error conditions: none
 *
 * Other functions called: symb
 *
 */

{
    //allocated by tblcreate()
    if (backbuf != NULL)										//7.4.15
    {
        free (backbuf);
        backbuf = NULL;
    }
    if (cngpointers != NULL)
    {
        free (cngpointers);
        cngpointers = NULL;
    }
    if (match != NULL)
    {
        free (match);
        match = NULL;
    }

    //allocated by compilecc()
    if (table != NULL)
    {
        free (table);
        table = NULL;
    }

    //allocated by symbol table compilation
    discard_symbol_table();

} /* End--tblfree */
#endif

/************************************************************************/
unsigned int max_heap()		/* Compute size of remaining heap space */
/************************************************************************/
/*
 * DESCRIPTION: Repeatedly calls 'malloc' and 'free' until it has found
 * the largest malloc-able buffer.  The size of this buffer is
 * returned.  The buffer is freed.
 *
 * RETURN VALUES: Returns an UNSIGNED int containing size (in bytes) of
 * largest malloc-able buffer.
 *
 * GLOBALS INPUT:		none
 * GLOBALS OUTPUT:		none
 * ERROR CONDITIONS:	none
 */
/************************************************************************/
{
    char *buffer;                    /* buffer pointer */
    unsigned int high, low, t;       /* loop controls  */

#if defined(_WINDOWS) || defined(_WindowsExe)  || defined(UNIXSHAREDLIB)
    //for computers of the 1990's, try MAX_ALLOC (nearly 64K) first,
    // and use that size if successful
    if (
        (buffer = (char *) malloc((size_t)MAX_ALLOC)) != NULL)			//7.4.15
    {
        free (buffer);
        return MAX_ALLOC;
    }
#endif    
    //try various smaller sizes
    high = MAX_ALLOC;           /* Highest amount to request */
    low = 0;                    /* Lowest amount */
    while ( low < (high-1) )    /* Loop until only 1-byte difference */
    {
        t = low + (high-low)/ 2; /* Compute trial buffer size */
        if ((buffer = (char *)malloc(t)) != NULL) /* Attempt to allocate that size */
        {						/* If successful... */
            free(buffer);		/* ...free the buffer and... */
            low = t;				/* ...raise the 'low' value */
        }
        else						/* else... */
            high = t;			/* ...lower the 'high' value */
    }

    /* When finished, the 'low' value is the largest malloc-able buffer */
    /* Subtract out RESERVEDSPACE so as not to totally exhaust heap. Bail out
    if there is too little heap space available. */
    if ( low > (unsigned) RESERVEDSPACE )
        low -= RESERVEDSPACE;	/* Leave space for file buffers */
    /*  and prntr's tables */
    else
    {
        Process_msg(errorFunction, 37, 0, (unsigned long) low);
        bailout(BADEXIT, FALSE);
    }

    return(low);
} /* End--max_heap */

/****************************************************************************/
void storechar(ch)					/* Store a char in the internal table */
/****************************************************************************/
SSINT ch;

/* Description -- If there is room in the table
 *							Store ch in the table.
 *						Else
 *							Set the boolean tblfull to TRUE.
 *
 * Return values: none
 *
 * Globals input: tloadpointer -- pointer for storing into table
 *                tablelimit -- address of last element of table
 *						tblfull -- boolean indicating whether there's room
 *										 left in table
 *
 * Globals output: tloadpointer -- if character was successfully stored,
 *												 it will point to the next position
 *												 in table, otherwise it will be unchanged
 *
 *						 tblfull -- if the table fills up, it will be set to TRUE.
 *
 * Error conditions: if table fills up, tblfull will be set to TRUE
 *
 * Other functions called: none
 *
 */

{
    if ( tloadpointer < tablelimit )
        *tloadpointer++ = ch;			/* Store the char in the table */
    else
        tblfull = TRUE;					/* Set error flag */
} /* End--storechar */

/****************************************************************************/
int srchlen(register tbltype tp1)									/* Get length of match of change */
/****************************************************************************/

/* Description -- Go through the table, starting at tp1, until we find
 *						a wedge, return the length from tp1 to the wedge. Count
 *						each full character as SRCHLEN_FACTOR, count any() as
 *						SRCHLEN_FACTOR, count prec() or fol() as 1, and count wd()
 *						and cont() as 2.
 *
 * Return values: returns length of the match.
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
    register tbltype tp2;  /* Pointer for going through the table */
    register int length;   /* Length to be returned */

    length = 0;

    for ( tp2 = tp1; *tp2 != SPECWEDGE; tp2++)  /* Find a wedge */
    {
        switch (*tp2)
        {
        case FOLCMD:		/* fol(): count as 1, skip parameter byte */
        case PRECCMD:		/* prec(): count as 1, skip parameter byte */
        case FOLUCMD:		/* folu(): count as 1, skip parameter byte */
        case PRECUCMD:		/* precu(): count as 1, skip parameter byte */
            length++;
            tp2++;
            break;

        case TOPBITCMD:   /* Top bit set: count as 1 full element */
        case ANYCMD:		/* any(): count as 1 full char, skip param. byte */
        case ANYUCMD:		/* anyu(): count as 1 full char, skip param. byte */
            length+=SRCHLEN_FACTOR;
            tp2++;
            break;

        case CONTCMD:		/* cont(): count as 2, skip parameter byte */
        case WDCMD:			/* wd(): count as 2, skip parameter byte */
        case WDUCMD:			/* wd(): count as 2, skip parameter byte */
            length+=2;
            tp2++;
            break;

        default:				/* ordinary character: count as 1 full character */
            length+=SRCHLEN_FACTOR;
        }
    }

    return( length );  /* Return the length from tp1 to the wedge */

} /* End--srchlen */

/****************************************************************************/
bool chk_prec_cmd(void)
/****************************************************************************/
/* Function: chk_prec_cmd
 *
 * Parameters:  none
 *
 * Description -- handle occurences of prec() command before match string
 *				If a prec() command is found at the beginning of a line,
 *				we wait until after the string or any() command before compiling
 *				the prec() command(s) into memory.
 *
 * Globals input: parsepntr parse2pntr precparse
 *
 * Globals output: parse2pntr precparse
 *
 * Error conditions: none
 *
 * Other functions called: none
 *
 * Return values: TRUE if str points to prec() command
 * Function added 2/95 by BJY for 7.4.30
 */

{
    char *saveparse2;

    if (!strncmp( parsepntr, "prec(", 5))
    {
        if (!precparse)
            precparse = parsepntr;	// Save position of first prec() command for later compilation
        return TRUE;
    }
    else
    {
        storeelement(TRUE);
        if (precparse)
        {
            saveparse2 = parse2pntr;	// Save pointer for later restoration
            parse2pntr = precparse;
            while (1)				// go store prec() command(s) now
            {
                parse( &parsepntr, &parse2pntr, TRUE);
                if (parse2pntr >= saveparse2)
                    break;
                storeelement(TRUE);
            }
            parse2pntr = saveparse2;	// restore position in line
        }
        return FALSE;
    }
}

/****************************************************************************/
void compilecc()									 /* Initialize CC part of program */
/****************************************************************************/

/* This procedure is the initialization of the Consistent Change part of the
program. */

/* Description:
 *						Initialize the Consistent Change part of the program,
 *					including loading the change table if there is one and
 *					compiling it if necessary.
 *
 * Return values: none
 *
 * Globals input: 
 *						sym_table -- table used for symbolic names within the
 *												change table
 *						tablelimit -- highest valid address in table
 *						fontsection -- boolean: TRUE == compiling font section
 *																	 of table
 * (MS only)		mandisplay -- boolean: TRUE == echo MS code to screen
 *						uppercase -- boolean:
 *						caseless -- boolean: TRUE == ignore case on input text
 *						debug -- boolean: TRUE == give debug display
 *						mydebug -- boolean:
 *						tablefile -- pointer to CC table input file,
 *											also used for compiled output file
 *						filenm -- file name buffer
 *						namelen -- length of contents of filenm
 *						tblarg -- table name if input on command line
 *						notable -- boolean: TRUE == no change table
 *						parsepntr -- pointer into input line
 *						parse2pntr -- pointer into input line
 *						was_math -- boolean: TRUE == last command parsed was a
 *																 math operator
 *																(add, sub, mul, div)
 *
 * Globals output:  All booleans set.
 *						  Pointers in unknown state.
 *
 * Error conditions: If a file is not found or is too small,
 *							  the user will be given another chance.
 *							If there is an error while compiling the table,
 *							  an appropriate error message will be displayed.
 *
 * Other functions called:
 *                         storechar -- store an element in the table
 *									inputaline -- read a line of input from
 *														the table file
 *									wedgeinline -- is there a > in the input line?
 *									parse -- find the next logical group in
 *												  the input line
 *									storeelement -- store an element in the internal
 *															change table
 *									cctsetup -- set up the pointers for the
 *													  internal change table
 *									cctsort -- sort the array of pointers into the
 *													 change table
 *									fontmsg -- display an appropriate message because
 *													 we found a font section
 *  (debugging only)			cctdump -- dump the internal change table to the
 *													 screen
 *
 */

{
    unsigned tablemax;		  /* largest valid subscript +1 for table */
    unsigned tablesize = 0;		  /* actual loaded size of table */
    SSINT ch;
    int i;		/* Miscellaneous loop index */
    int before;
    int after;
#if !defined(_WINDOWS) && !defined(UNIXSHAREDLIB)
    char *optptr;					/* pointer to option strings */
#endif
    int check_prec;		// 7.4.30 BJY
    char *errPtr;

    /* Allocate space for compiled CC table (freeing the old one first
    if necessary) */
    if (table != NULL)
        free(table);
#ifdef _WINDLL
    // in DLL mode just go for the maximum size, don't mess around
    tablemax = MAX_ALLOC / sizeof(SSINT);
#else
    tablemax = max_heap() / sizeof(SSINT);  /* Get as much space as available */
#endif
    table = (SSINT *) tblalloc(tablemax, (sizeof(SSINT)));
    tablelimit = table + tablemax;		/* Initialize limits */

    /* Initialize internal booleans */
    fontsection = FALSE;
    was_math = FALSE;
    was_string = FALSE;
    uppercase = FALSE;
    caseless = FALSE;
    debug = mydebug = 0;			 /* init debugs false */
    pstrBuffer= strBuffer;
#if defined(_WINDLL)
    memset(TableLineIndexes, 0, sizeof(TableLineIndexes));
#endif		
    /* Initialize the symbol table */
    for ( i = 0; i < 4; i++ )
    {
        sym_table[i].list_head = NULL;
        sym_table[i].next_number = MAXARG;
    }

    bytset(storepre, 0, NUMSTORES+1);   // do this before processing cc table

    errors = FALSE;			 /* initialize */
    tblfull = FALSE;
    doublebyte_mode = FALSE;       /* initialize doublebyte due to early scan  */
    doublebyte_recursion = FALSE;
    doublebyte1st = doublebyte2nd = 0;
    utf8encoding= FALSE;
    tloadpointer = table;
    cngpend = cngpointers;

#if defined(_WINDOWS) || defined(UNIXSHAREDLIB)
    if (lpszCCTableBuffer)
    {
        tablefile= NULL;
    }
    else
    {
        strcpy( filenm, tblarg );           /* copy name from command line */
        namelen = strlen( filenm );

        if ((tablefile= wfopen(filenm,BINREAD)) == NULL)
        {
            if ( (strchr(filenm, '.') == NULL)
                    || (strrchr( filenm, '.') == (strchr(filenm, DIRSEP) - 1)) )
            {
                strcat( filenm, ".CCT" );	/* Try an extension of .CCT */
                tablefile= wfopen(filenm,BINREAD);
            }

            if (tablefile == NULL)
            {
#if !defined(UNIX)
                upcase_str( filenm );		/* force output arg to upper case */
#endif
                errPtr = &filenm[0];
                Process_msg(errorFunction, 55, 0, (long unsigned) errPtr);
                errors = TRUE;                                      //7.4.15
                return;
            }

        }
    }
    notable = FALSE;
#else
    strcpy( filenm, tblarg );           /* copy name from command line */
    namelen = strlen( filenm );
    do
    {

        tablefile = NULL;
        if (namelen != 0)		  /* check for no table */
        {
#if !defined(UNIX)
            optptr = strchr( filenm, '/' );		/* look for slash option */
            if ( optptr )
            {
                if ( tolower( *(optptr+1) ) == 'd' )
                {
                    debug = TRUE;					/* set debug option */
                    single_step = TRUE;
                }
                else
                {
                    Process_msg(errorFunction, 54, 0, (long unsigned) optptr);
                }
                *optptr = '\0';			/* terminate file name before option */
            }
#endif
            if ( (tablefile=fopen(filenm,BINREAD)) == NULL )
            {
                if ( (strchr(filenm, '.') == NULL)
                        || (strrchr( filenm, '.') == (strchr(filenm, DIRSEP) - 1)) )
                {
                    strcat( filenm, ".CCT" );	/* Try an extension of .CCT */
                    tablefile = fopen( filenm, BINREAD);
                }
            }
            if ( tablefile == NULL )
            {
#if !defined(UNIX)
                upcase_str( filenm );		/* force output arg to upper case */
#endif
                errPtr = &filenm[0];
                Process_msg(errorFunction, 55, 0, (long unsigned) errPtr);
#if !defined(NO_QUESTIONS)
                getname("Enter CC Table name : ", filenm);
                namelen = strlen( filenm );
#else
                bailout( BADEXIT, FALSE );
#endif                
            }
        }
    } while ( (tablefile == NULL) && (namelen != 0) );

    if ( tablefile == NULL || namelen == 0 )
        bailout( BADEXIT, FALSE );

    if (namelen == 0)
    {
        notable = TRUE;
        storechar(SPECPERIOD);
    }
    else
        notable = FALSE;
#endif

    ch = xgetc(tablefile);

    if (ch == EOF)
        storechar(SPECPERIOD);
    else
    {
        /* see if we have precompiled table (by looking at first byte)   */
        if ((ch & 0x80) && ((ch & 0xFF) != 0xEF))
            /* precompiled tables have one byte per value, if high bit is
            	on then it was a command, so turn on high half of it       */
        {                             /* precompiled table            */
            ch = ch | 0xff00;             /* turn this back into command  */
            storechar(ch);                /* read it directly             */

            while ( (ch = xgetc(tablefile)) != EOF )   /* into table area  */
            {
                if ( ch & 0x80 )           /* if meant to be a command     */
                    ch = ch | 0xff00;       /* turn this back into command  */
                else                       /* old compiled tables had      */
                    if ( ch == CTRLZ )      /*  control Z, make that into   */
                        ch = ENDFILECMD;     /*   the new ENDFILECMD instead */
                storechar(ch);
            }
        }
        else										/* table not precompiled */
        {
			/* Check for Byte Order Mark and ignore if necessary */
			if ((ch & 0xFF) == 0xEF)
			{
				SSINT ch1=xgetc(tablefile);
				if ((ch1 & 0xFF) == 0xBB)
				{
					SSINT ch2= xgetc(tablefile);
					if ((ch2 & 0xFF) != 0xBF)
					{
						xungetc(ch2, tablefile);
						xungetc(ch1, tablefile);
						xungetc(ch, tablefile);
					}

				} else {
					xungetc(ch1, tablefile);
					xungetc(ch, tablefile);
				}
			} else {
				xungetc(ch, tablefile);
			}
#if defined(_WINDLL)
            iCurrentLine= 0;
#endif

            while (!xfeof(tablefile))
            {											/* compile table */
                inputaline();
#if defined(_WINDLL)
                iCurrentLine++;
#endif

#ifdef DEBUG
                if ( debug == 1 )
                    fprintf(msgfile, "%s\n", line);
#endif
                parse2pntr = line;

                if (!wedgeinline())
                {
                    flushstringbuffer(FALSE);
#if defined(_WINDLL)
                    if (TableLineIndexes[iCurrentLine - 1] == 0)
                    {
                        if (strlen(line) > 0)
                            TableLineIndexes[iCurrentLine - 1]= (unsigned)((long)tloadpointer - (long)table) / sizeof(SSINT);
                        else
                            TableLineIndexes[iCurrentLine - 1]= ~0u;	// Neil Mayhew - 6 Nov 98
                    }
#endif					
                }
                else
                {										/* search parse */
                    flushstringbuffer(FALSE);
                    storechar( SPECPERIOD);
                    check_prec = TRUE;		// 7.4.30 BJY
                    precparse = NULL;       // 7.4.30
#if defined(_WINDLL)
                    TableLineIndexes[iCurrentLine - 1]= (unsigned)((long)tloadpointer - (long)table) / sizeof(SSINT);
#endif					

                    for (;;)
                    {
                        parse( &parsepntr, &parse2pntr, TRUE);
                        if ( !*parsepntr || *parsepntr == WEDGE )
                            break;
                        if (check_prec)		// 7.4.30 BJY Handle prec() command(s) before string
                            check_prec = chk_prec_cmd();
                        else
                            storeelement( TRUE);
                        if ( parsepntr > parse2pntr )
                            parse2pntr = parsepntr;	/*The parse may have gone
                        						* beyond the current element
                        						*/
                    }
                    flushstringbuffer(TRUE);
                    storechar( SPECWEDGE);
                }
                for (;;)
                {									/* replacement parse */
                    parse( &parsepntr, &parse2pntr, TRUE);
                    if ( !*parsepntr )
                        break;
                    storeelement( FALSE);
                    if ( parsepntr > parse2pntr )
                        parse2pntr = parsepntr;		/*The parse may have gone
                    							* beyond the current element
                    							*/
                }
                if (fontsection)
                    break;
            }
            flushstringbuffer(FALSE);
            storechar(SPECPERIOD);
        }
    }
#if defined(_WINDOWS) || defined(UNIXSHAREDLIB)
    if (tablefile)
    {
        wfclose(tablefile);						/* recover file BUFFER */
        tablefile= NULL;
    }
#else
    fclose(tablefile);						/* recover file BUFFER */
#endif			
#if defined(_WINDLL)
    TableLineIndexes[iCurrentLine]= (unsigned)((long)tloadpointer - (long)table) / sizeof(SSINT);

    for (i= iCurrentLine; i > 0;)
    {
        i--;

        if (TableLineIndexes[i] == -1)
            TableLineIndexes[i]= TableLineIndexes[i + 1];
    }
#endif					
    if ( fontsection )					/* Old style fonts */
        fontmsg();

    if (tblfull)
    {
        Process_msg(errorFunction, 56, 0, 0);
        errors = TRUE;		/* Mark this as a fatal error */
    }

    if (errors)
    {
        //      Process_msg(errorFunction, 57, 0, 0);
    }
    else
    {														/* error flag check */

        /* Now shrink allocation for 'table' down to miminum */
        tablesize = tloadpointer - table;
        table = realloc(table, tablesize * sizeof(SSINT));
        tablelimit = table + tablesize;
        tloadpointer= tablelimit; // ADDED 7-31-95 DAR

        /* Check for symbols that were referenced, but not defined */
        check_symbol_table();

        /* Setup pointers into table[] and sort cngpointers */
        if ( cctsetup() )
            cctsort();
    }
    if ( debug == 1 )
    {
        dump_symbol_table();							/* Dump the symbol table for
        													*	 symbolic names
        													*/
        msg_printf("%d changes, %d maximum\n",
                   cngpend-cngpointers, MAXCHANGES);
        msg_printf("%u total bytes, %u total elements, %u maximum\n",
                   tablesize, tablesize / 2, tablemax);

#ifdef DEBUG
        cctdump();
#endif
        before = (BEFORE_CNT - 6) / 2;
        after = BEFORE_CNT - before - 6;
        for (i = before; i > 0; i--)
            msg_putc('-');
        msg_puts("OUTPUT");
        for (i = after; i > 0; i--)
            msg_putc('-');
        msg_putc('v');
        before = (AFTER_CNT - 6) / 2;
        after = AFTER_CNT - before - 6;
        for (i = before; i > 0; i--)
            msg_putc('-');
        msg_puts("INPUT");
        for (i = after; i > 0; i--)
            msg_putc('-');
        msg_putc('\n');
    }
#if !defined( _WINDOWS) && !defined(UNIXSHAREDLIB)
    else		/* If not in debug mode, discard the symbol table */
        discard_symbol_table();
#endif

} /* End--compilecc */

/****************************************************************************/
int cctsetup()										/* Set up pointers into table */
/****************************************************************************/
{
    /* Description:
     *						Go through and set up the internal representation of the
     *					changes table.
     *
     * Return values:  TRUE == everything is set up OK
     *						FALSE == there was a problem somewhere
     *
     * Globals input: 
     *						errors -- Boolean: TRUE == table contains errors
     *						fontstart -- table of pointers to the start of fonts
     *						fontsection -- Boolean: TRUE == table contains old-style
     *																	 fonts
     *						maintablend -- pointer to the end of the changes
     *											  part of the table
     *						tloadpointer -- pointer to the end of what was read
     *												into table
     *						table -- storage area for changes and fonts
     *						defarray -- table of pointers to DEFINEs
     *						cngpend -- pointer to the end of the table of
     *										 change pointers
     *						cngpointers -- table of pointers to changes
     *						groupbeg -- table of pointers to the beginning of groups
     *						maxsrch -- length of longest search string
     *						groupend -- table of pointers to the end of groups
     *
     * Globals output: if there were no problems, everybody is set up.
     *						 if there was a serious problem, errors is set to TRUE.
     *
     * Error conditions: if the table was too big, FALSE will be returned.
     *							if group one was missing or a group is multiply defined,
     *							  FALSE will be returned and
     *							  errors will be set to TRUE, indicating a major error.
     *							Other errors, such as a font section when running CC,
     *							  will only generate warning messages for the user.
     *
     * Other functions called: fontmsg -- display an appropriate message because
     *													 we found a font section in the table
     *
     */

    register tbltype tp;
    register tbltype *ptp;
    cngtype cp;
    int group, i, j;

    /* Look for 'unsorted' and 'binary' and 'doublebyte' commands in 'begin'
    section.  Set global flags accordingly */
    unsorted = FALSE;
    binary_mode = FALSE;
    doublebyte_mode = FALSE;
    utf8encoding = FALSE;
    if (table[0] == SPECWEDGE)		/* If there is a begin section */
    {									/* scan through it */
        for (tp = table+1; *tp != SPECPERIOD; tp++)
        {
            if (*tp == UNSORTCMD)
                unsorted = TRUE;
            else if (*tp == BINCMD)
                binary_mode = TRUE;
            else if (*tp == UTF8CMD)
                utf8encoding = TRUE;
            else if (*tp == DOUBLECMD)
            {
                tp++;                      /* point to doublebyte argument */
                *tp = *tp & 0x00ff;        /* turn off any possible high bits */
                doublebyte_mode = TRUE;
                doublebytestore(*tp);      /* pass argument */
                tp--;                      /* restore tp    */
            }
        }
        // binary is incompatible and not needed with doublebyte mode
        if (( binary_mode == TRUE ) && ( doublebyte_mode == TRUE ))
        {
            binary_mode = FALSE;
            Process_msg(errorFunction, 99, 0, 0);
        }

    }

    maintablend = tloadpointer;							/* set main table end */
    for ( tp = table; tp < tloadpointer; tp++ )
    {
        if (*tp == FONTCMD)			/* Set table end to beginning of font section */
        {
            fontmsg();						/* We have old-style fonts */
            maintablend = tp;
            break;
        }
    }

    for (ptp = defarray; ptp <= defarray+MAXARG; )		/* clear define array */
        *ptp++ = NULL;

    cngpend = cngpointers;								/* build changepointers array */

    for ( tp = table; tp < maintablend-1; )
    {
        if ( *tp++ == SPECPERIOD )
        {
            if ( *tp == DEFINECMD )				/* It's a define, so save its addr */
                defarray[*(tp+1)] = tp + 2;	 /*  in define array */
            else
            {
                if ( cngpend - cngpointers < MAXCHANGES )  /* Must be a change */
                    *cngpend++ = tp;								/*  so save its addr */
                else
                {
                    Process_msg(errorFunction, 38, 0, (long unsigned) MAXCHANGES);
                    return( FALSE);						/* Error */
                }
            } /* End--else */
        } /* End--if */
    } /* End--for */

    for (i = 1; i <= MAXGROUPS; i++)  /* Initialize group array */
        groupbeg[i] = NULL;

    group = 0;

    maxsrch = 0;

    for ( cp = cngpointers; cp < cngpend; cp++)
    {
        tp = *cp;							/* Find length of longest search */
        j = srchlen( tp ) / SRCHLEN_FACTOR;
        if ( j > maxsrch )
            maxsrch = j;

        if ( (tp >= table+3) && (*(tp-3) == GROUPCMD) )  /* Valid group start? */
        {
            *(tp-3) = SPECPERIOD;	 /* Eff remove GROUPCMD */

            if ( group != 0 )
            {
                groupend[group] = cp;	/* Close previous group */
            }
            else
            {
                groupbeg[1] = cngpointers;			/* First group is implied group 1 */
                groupend[1] = cp;
            }

            /* If the group has already been defined */
            /*  AND it's non-empty, squawk */

            group = *(tp-2);
            if ( (groupbeg[group] != NULL) && (groupend[group] != groupbeg[group]) )
            {
                Process_msg(errorFunction, 39, 0,
                            (long unsigned) sym_name(group, GROUP_HEAD));
                errors = TRUE;												/* Major error */
                return(FALSE);
            }
            else
                groupbeg[group] = cp;		/* Save the start of the current group */
        }
    }

    if ( group == 0 )					/* If no groups, set up group 1 */
    {
        group = 1;
        groupbeg[1] = cngpointers;
    }
    groupend[group] = cngpend;			/* Close last group */

    // increase maxsrch so debug display window will display at least 500 characters
    if (maxsrch < MINMATCH)
        maxsrch= MINMATCH;

    return( TRUE);		/* No errors detected */

} /* End--cctsetup */

/****************************************************************************/
static void fontmsg()	  /* Handle font section appropriately */
/****************************************************************************/

/* Description -- If CC or MS and only doing CC
 *							Display "...ignored..." message.
 *						Else
 *							Display "...not supported..." message.
 *
 *							Set global errors to TRUE.
 *
 * Globals output: errors -- set to TRUE if MS and running full MS
 *
 */
{
    Process_msg(errorFunction, 40, 0, 0);
}

/****************************************************************************/
void cctsort()														/* Sort CC table */
/****************************************************************************/

/* Description -- Go through the changes table, group by group and sort
 *						the changes primarily in ascending order of first letter
 *						of match and secondarily in descending order of length
 *						of match.
 *						
 *						Find the beginning of left-executes for the group
 *						by going through the group, using j, until
 *						either the first letter of a change has the
 *						high bit set or we hit the end of the group.
 *
 *						Save the beginning of left-executes in groupexeq[group].
 *
 * Return values: none
 *
 * Globals input: groupbeg -- table of pointers to the beginning of groups
 *						groupxeq -- table of pointers to the beginning of
 *										  the left-execute portions of groups
 *
 * Globals output: groupbeg -- the changes within each group will be sorted
 *											numerically by ASCII sequence for
 *											constant match strings and by length
 *											with longest match first within each
 *											character, and any left-executes at the end
 *											of the group.
 *						 groupxeq -- each non-NULL element of groupxeq will point
 *											to the first left-execute for the
 *											corresponding group.
 *
 * Error conditions: there shouldn't be any
 *
 * Other functions called: goes_before -- compares two changes and returns
 *										TRUE if first should go before second
 *
 */

{

    register cngtype k;	/* Inner loop pointer */
    register cngtype j;	/* Outer loop pointer */
    tbltype hold;			/* Pointer to change pointer be inserted */
    int group;  			/* Current group # */

    for ( group = 1; group <= MAXGROUPS; group++ )
    {
        if ( groupbeg[group] != NULL)		/* If group exists... */
        {										/* ...sort the changes */
            /* NOTE: The following is a simple insertion sort. It has the
            	desirable property that is is stable -- that is, changes
            	with matching keys are maintained in the order in which
            	they were found */
            for (j = groupbeg[group] + 1; j < groupend[group]; j++)
            {
                hold = *j;			/* Save pointer to insert */
                for (	k = j - 1;
                        k >= groupbeg[group] && goes_before(hold, *k);
                        k--)
                {
                    *(k+1) = *k;	/* Move other pointers up as needed */
                }
                *(k+1) = hold;		/* Insert the saved pointer */
            }

            /* Set up pointer to the first left-execute */

            for (j = groupbeg[group]; j < groupend[group]; j++)
                if ( **j & HIGHBIT )
                    break;				/* Found one! */
            groupxeq[group] = j;

        } /* End--group exists */
    } /* End--loop through group table */
} /* End--cctsort */

/************************************************************************/
char *sym_name(number, type)	/* Return symbolic name for a given number */
/************************************************************************/
int number;			/* Number of store, switch, group or define */
int type;			/* 0,1,2 or 3 to indicate which store, switch, group */
/* or define, respectively */
{
    static char name_buffer[20];	/* Buffer used if no symbolic name exists */
    register CC_SYMBOL *sym_pntr;	/* Pointer for traversing the list */

    /* Search appropriate symbol table for a matching number */
    sym_pntr = sym_table[type].list_head;
    while ( sym_pntr != NULL )
    {
        if (sym_pntr->number == number)		/* If matching number found... */
            return( sym_pntr->name );			/* return pointer to name */
        else
            sym_pntr = sym_pntr->next;			/* try next in list */
    }

    /* If number was not found, then return number converted to a string */
    sprintf(name_buffer, "%d", number);
    return( name_buffer );
}

/****************************************************************************/
static bool goes_before(x,y)		/* Compare two changes */
/****************************************************************************/
tbltype x;			/* first change */
tbltype y;			/* second change */

/* Description -- Compares two changes and returns TRUE if x should sort
 *						before y in the table. NOTE: This function is 
 *
 * Return values: TRUE if x goes before y, FALSE otherwise
 *
 * Globals input:	unsorted -- if TRUE then compare only first letter,
 *										otherwise compare first letter, then length
 *										of match.
 *
 * Globals output: none
 *
 * Error conditions: none
 *
 * Other functions called:	srchlen -- how long is the match?
 *
 */
{
    unsigned int xltr;      /* First letter of change x */
    unsigned int yltr;      /* First letter of change y */

    /* Compute first letter of each change. If change starts with a command
    	(e.g., any(1) "xyz" > ...) then the first letter code is HIGHBIT */
    /* Following 2 lines changed by DAR/DRC 4/24/96 and 5/8/96.
       Changed to do sorting correctly, and then to be unsigned.     */
	/* if *x or *y is SPECWEDGE, then we have a zero length string on the left side and it
	   always be sorted at the end */
	xltr = min( (unsigned) (*x & 0xFFFF), (unsigned) HIGHBIT);
	yltr = min( (unsigned) (*y & 0xFFFF), (unsigned) HIGHBIT);

    /* If first letters match, then compare search lengths */
    if (xltr == yltr && !unsorted)
        return( srchlen(x) > srchlen(y) );
    else
        return( xltr < yltr );
}

#ifdef DEBUG
/****************************************************************************/
void dumpchar(ch)  /* (Debugging routine) */ /* Dump one character */
/****************************************************************************/
char ch;
{
    /* dump one character */
    if ( ch <= 32 || ch >= 127 )
        fprintf( msgfile, "%d ", ch);
    else
        fprintf( msgfile, "%c ", ch);
}

/****************************************************************************/
void cctdump()						  /* (Debugging routine) */ /* Dump CC table */
/****************************************************************************/
{
    /* diagnostic printout of changes table */

    tbltype tp;
    tbltype *ptp;
    cngtype cp;
    int group;

    fprintf( msgfile, "Dump of changes table:\n");
    fprintf( msgfile,
             "table=%u maintablend=%u tloadpointer=%u maxsrch=%u MAXMATCH=%u\n",
             table, maintablend, tloadpointer, maxsrch, MAXMATCH);

    if ( *table = SPECWEDGE )
    {									/* Dump the "begin" statement */
        fprintf( msgfile, " begin: " );
        tp = table;
        do
        {
            dumpchar( *tp );
        } while ( *tp++ != SPECPERIOD );
        fprintf(msgfile, "\n" );
    } /* End--dump the "begin" statement */

    for ( ptp = defarray; ptp <= (defarray + MAXARG); ptp++)
    {
        if ( *ptp != NULL )
        {
            fprintf( msgfile, "define(%d): ", ptp-defarray );
            tp = *ptp;
            do
            {
                dumpchar( *tp );
            } while ( *tp++ != SPECPERIOD );
            fprintf( msgfile, "\n");
        } /* End--dump a define */
    } /* End--dump the define array */

    for ( cp=cngpointers; cp < cngpend; cp++ )
    {
        tp = *cp;
        fprintf( msgfile, "%3d %6u: ", cp-cngpointers, tp);
        do
        {
            dumpchar( *tp);
        } while ( *tp++ != SPECPERIOD );
        fprintf( msgfile, "\n");
    }

    for (ptp = defarray; ptp <= defarray+MAXARG; ptp++)
        if ( *ptp != NULL )
            fprintf( msgfile, "define(%d) start=%u:\n", ptp-defarray, *ptp);

    for (group = 1; group <= MAXGROUPS; group++)
        if (groupbeg[group]!=NULL)
            fprintf( msgfile, "group(%d): start=%u xeq=%u end=%u\n",
                     group, groupbeg[group]-cngpointers,
                     groupxeq[group]-cngpointers,
                     groupend[group]-cngpointers);

    fflush( msgfile );			/* Just in case the dump was all we wanted */
} /* End--cctdump */

#endif /* DEBUG */

/* END */
