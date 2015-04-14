/* ccomp.c
 * Old name: cc1c.c  12/85	CC Compile Routines	AB
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
 * Change history:
 *   2-Nov-86		 MT  Changed inputaline and storeelement to handle
 *								 8-bit input
 *  14-Jan-87		 MT  Deleted storefontptr and fontcompile
 *  17-Feb-87		 MT  Fixed storeelement to catch an illegal search string
 *								 in the begin statement
 *  24-Apr-87		 MT  Changed inputaline to work properly under CP/M
 *  12-Aug-87		 MT  Changed to not compile for MS-without-CC
 *  18-Jan-88		 MT  Added code for symbolic names in CC
 *  27-Jan-88		 MT  Added code for hexadecimal constants in CC
 *							  Added code for decimal numbers in CC
 *   1-Apr-88		 MT  Added code for add, sub, mul, div in CC
 *  15-Nov-89		 ACR Removed RT11 and CP/M stuff. Ported to Microsoft C.
 *  13-Feb-91      DJE CC only version. Removed SHARP and MS stuff.
 *                     Name changed to cc1c.c
 *
 * This module provides the following global routines:
 *
 * err( message ) -- Display a 3-line error message in the form of
 * char *message;		  current line, pointer to current parsing position,
 *							  error message.
 *
 * inputaline() -- Read a line into the array line,
 *							up to EOF, \n, or FORMFEED.
 *
 *						 " and ' are treated as quote chars, anything
 *							between a matched pair of them is passed through
 *							unquestioned.
 *
 *						 Each horizontal TAB outside of quotes is converted to
 *							a SPACE.
 *
 *						 A line longer than LINEMAX chars will generate an error.
 *
 * parse( pntr, pntr2, errormessage)
 * char **pntr, **pntr2;
 * int errormessage;
 *				This procedure parses an element of line. It begins looking at
 *			*pntr2.	Within the procedure, *pntr2 points to the current char
 *			being looked at. It ends with pntr set to the beginning of the next
 *			element, and *pntr2 set to the position one after the end of the next
 *			element. It gives error messages only if errormessage is TRUE.
 *
 * wedgeinline() -- Boolean: return TRUE if there is a wedge in the current
 *										 input line.
 *
 * stornoarg( comand) -- Store a command that takes no argument in the
 * char comand;				changes table.
 *
 * storarg( comand) -- Store a command that takes an argument (numeric)
 * char comand;			 in the changes table.
 *
 * storoparg( comand) -- Store a command that optionally takes an argument
 * char comand;				in the changes table.  If no argument is given,
 *									the command is stored with an argument of one (1).
 *
 * stordbarg( comand) -- Store doublebyte command, that takes 0-2 arguments,
 * char comand;				in the changes table.  If no argument is given,
 *									the command is stored with an argument of one (1).
 *
 * parsedouble( flag) -- Parse an argument to the doublebyte command, in   
 * int *flag;              either decimal, hexadecimal,or the octal default.
 *
 * buildkeyword() -- Copy a (suspected) keyword into the array keyword
 *							  so it can be checked against the list of keywords.
 *
 * storeelement( search) -- Decode and store the next element in the
 *										changes table.
 * int search;  Boolean --  TRUE == we're on the search side of a change
 *									FALSE == we're on the replacement side
 *
 * cmdname(cmd, search) -- Returns name of a command given the command code.
 * char cmd;	The command code
 * int search;  Boolean --  TRUE == we're on the search side of a change
 *									FALSE == we're on the replacement side
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
#include <stdlib.h>

#if defined(_WINDLL) && !defined(WIN32) && !defined(STATICLINK)
#include "ccmalloc.h"
#else
#if defined(UNIX)
#include <stdlib.h>
#else
#include <malloc.h>
#endif
#endif

#if defined(UNIX)
#include "wtype.h"
#endif
#include "cc.h"
#include "c8type.h"
#include "utf8.h"

/* Prototype for static functions defined in this module */
static int hex_decode(char *);
static int ucs4_decode(char *);
static void decimal_decode(int *);
static void octal_decode(int *);
static int symbol_number(int, char);
static int search_table(int, char, char *, int, int);
static SSINT parsedouble(int *);
static char * ptokenstart;
/* Local equates--for type of search in the symbol table */
#define NAME_SEARCH		1
#define NUMBER_SEARCH	0

/* Variables particular to this module */

#define NULLFP (int (*)(char, char, char)) 0		/* NULL function pointer */

struct cmstruct
{
    char	 *cmname;			/* Name of operation */
    void	(*cmfunc) (char, char, char);		 /* Function to call to compile it */
    char	cmcode;				/* Compiled code */
    char	symbolic;			/* Non-zero == command can take symbolic arguments */
    char	symbol_index;		/* If (symbolic)
    								*	 index into the symbol table array
    								*/
};

/* Search Arguments (left side) */

struct cmstruct cmsrch[] =
    {
        { "nl",		  stornoarg, CARRIAGERETURN, FALSE,			0 },
        { "tab",	  stornoarg, TAB,            FALSE,			0 },
        { "endfile",  stornoarg, ENDFILECMD,	  FALSE,			0 },
        { "define",   storarg,	 DEFINECMD,		  DEFINED,		DEFINE_HEAD },
        { "any",	  storarg,	 ANYCMD,		  REFERENCED,	STORE_HEAD },
        { "prec",	  storarg,	 PRECCMD,		  REFERENCED,	STORE_HEAD },
        { "fol",	  storarg,	 FOLCMD,		  REFERENCED,	STORE_HEAD },
        { "wd",		  storarg,	 WDCMD,			  REFERENCED,	STORE_HEAD },
        { "anyu",	  storarg,	 ANYUCMD,		  REFERENCED,	STORE_HEAD },
        { "precu",	  storarg,	 PRECUCMD,		  REFERENCED,	STORE_HEAD },
        { "folu",	  storarg,	 FOLUCMD,		  REFERENCED,	STORE_HEAD },
        { "wdu",	  storarg,	 WDUCMD,		  REFERENCED,	STORE_HEAD },
        { "cont",	  storarg,	 CONTCMD,		  REFERENCED,	STORE_HEAD },
        { NULL,		  NULL,		0,				  FALSE,			0 }
    };

/* Replacement Arguments (right side) */

struct cmstruct cmrepl[] =
    {
        { "nl",		  stornoarg, CARRIAGERETURN, FALSE,			0 },
        { "tab",		  stornoarg, TAB,            FALSE,			0 },
        { "endfile",  stornoarg, ENDFILECMD,	  FALSE,			0 },
        { "cont",	  storarg,	 CONTCMD,		  REFERENCED,	STORE_HEAD },
        { "if",		  storarg,	 IFCMD,			  REFERENCED,	SWITCH_HEAD },
        { "ifn",		  storarg,	 IFNCMD,			  REFERENCED,	SWITCH_HEAD },
        { "else",	  stornoarg, ELSECMD,		  FALSE,			0 },
        { "endif",	  stornoarg, ENDIFCMD,		  FALSE,			0 },
        { "set",		  storarg,	 SETCMD,			  DEFINED,		SWITCH_HEAD },
        { "clear",	  storarg,	 CLEARCMD,		  DEFINED,		SWITCH_HEAD },
        { "begin",	  stornoarg, BEGINCMD,		  FALSE,			0 },
        { "endstore", stornoarg, ENDSTORECMD,	  FALSE,			0 },
        { "store",	  storarg,	 STORCMD,		  DEFINED,		STORE_HEAD },
        { "append",   storarg,	 APPENDCMD,		  DEFINED,		STORE_HEAD },
        { "out",		  storarg,	 OUTCMD,			  REFERENCED,	STORE_HEAD },
        { "outs",	  storarg,	 OUTSCMD,		  REFERENCED,	STORE_HEAD },
        { "dup",		  stornoarg, DUPCMD,			  FALSE,			0 },
        { "back",	  storoparg, BACKCMD,		  FALSE,			0 },
        { "next",	  stornoarg, NEXTCMD,		  FALSE,			0 },
        { "ifeq",	  storarg,	 IFEQCMD,		  REFERENCED,	STORE_HEAD },
        { "ifneq",	  storarg,	 IFNEQCMD,		  REFERENCED,	STORE_HEAD },
        { "ifgt",	  storarg,	 IFGTCMD,		  REFERENCED,	STORE_HEAD },
        { "ifngt",    storarg,   IFNGTCMD,       REFERENCED,  STORE_HEAD },
        { "iflt",     storarg,   IFLTCMD,        REFERENCED,  STORE_HEAD },
        { "ifnlt",    storarg,   IFNLTCMD,       REFERENCED,  STORE_HEAD },
        { "end",		  stornoarg, ENDCMD,			  FALSE,			0 },
        { "repeat",   stornoarg, REPEATCMD,		  FALSE,			0 },
        { "group",	  storarg,	 GROUPCMD,		  DEFINED,		GROUP_HEAD },
        { "do",		  storarg,	 DOCMD,			  REFERENCED,	DEFINE_HEAD },
        { "incl",	  storarg,	 INCLCMD,		  REFERENCED,	GROUP_HEAD },
        { "excl",	  storarg,	 EXCLCMD,		  REFERENCED,	GROUP_HEAD },
        { "fwd",		  storoparg, FWDCMD,			  FALSE,			0 },
        { "omit",	  storoparg, OMITCMD,		  FALSE,			0 },
        { "incr",	  storarg,	 INCRCMD,		  DEFINED,		STORE_HEAD },
        { "decr",     storarg,   DECRCMD,        DEFINED,     STORE_HEAD },
        { "add",		  storarg,	 ADDCMD,			  DEFINED,		STORE_HEAD },
        { "sub",		  storarg,	 SUBCMD,			  DEFINED,		STORE_HEAD },
        { "mul",		  storarg,	 MULCMD,			  DEFINED,		STORE_HEAD },
        { "div",		  storarg,	 DIVCMD,			  DEFINED,		STORE_HEAD },
        { "mod",		  storarg,	 MODCMD,			  DEFINED,		STORE_HEAD },
        { "write",	  stornoarg, WRITCMD,		  FALSE,			0 },
        { "wrstore",  storarg,	 WRSTORECMD,	  REFERENCED,	STORE_HEAD },
        { "read",	  stornoarg, READCMD,		  FALSE,			0 },
        { "caseless", stornoarg, CASECMD,		  FALSE,			0 },
        { "binary",   stornoarg, BINCMD,         FALSE,       0 },
        { "doublebyte",stordbarg,DOUBLECMD,      FALSE,       0 },
        { "unsorted",stornoarg,  UNSORTCMD,      FALSE,       0 },
        { "ifsubset", storarg,	 IFSUBSETCMD,	  REFERENCED,	STORE_HEAD },
        { "len",		  storarg,	 LENCMD,			  REFERENCED,	STORE_HEAD },
        { "utf8",     stornoarg, UTF8CMD,		  FALSE,			0 },
        { "backu",	  storoparg, BACKUCMD,		  FALSE,			0 },
        { "fwdu",	  storoparg, FWDUCMD,		  FALSE,			0 },
        { "omitu",	  storoparg, OMITUCMD,		  FALSE,			0 },
        { NULL,		  NULL,		0,				  FALSE,			0 }
    };

/****************************************************************************/
void err(message)											/* Display an error message */
/****************************************************************************/
char *message;

/* Description -- Give a 3-line error message in the form of:
 *
 *						  current line
 *						  pointer to current parsing position
 *						  error message
 *
 *						Set errors to TRUE.
 *
 * Return values: none
 *
 * Globals input: errors -- global error flag
 *
 * Globals output: errors set to TRUE
 *
 * Error conditions: if this routine gets called, there must be one.
 *
 * Other functions called: none
 *
 */
{
    MSG_STRUCT_S_S *structss;
    register char *cp;         // points to the source of the error info
    register char *lp;         // points to the target of the error info
#if !defined(_DOS) && !defined(UNIX)
    if (lpCompileErrorCallback != NULL)
    {
        lpCompileErrorCallback(message, iCurrentLine, ptokenstart - line);
    }
    else
#endif
    {
        lp= errorLine;

        for ( cp = line; *cp != '\0'; )          // copy current (erroneous) line
        {
            *lp++ = *cp++;
        }
        *lp++ = '\n';

        for ( cp = line; cp++ < parse2pntr; )    // Blanks until
            *lp++ = ' ';                          // current parsing position
        *lp++ = '^';                             // ^ shows where error is
        *lp++ = '\0';                            // (end of string)

        Msg_s_s.string1 = &errorLine[0];
        Msg_s_s.string2 = message;
        structss = &Msg_s_s;
        Process_msg(errorFunction, 53, 0, (unsigned long) structss);
    }

    errors = TRUE;

} /* End--err */

/****************************************************************************/
void inputaline()				  /* Input a line and set len to its length */
/****************************************************************************/

/* Description -- Read in a line until \n, formfeed, or EOF found.
 *						  NULs and <CR>s are ignored.
 *						  Horizontal TABs are converted to single SPACEs.
 *
 *						If line is too long
 *							Display an error message, using err.
 *							Return.
 *
 *						If character read is " or '
 *							Treat as quote char and pass everything up
 *							  to the next occurrence of the same char
 *							  without checking for SPACEs or TABs.
 *
 * Return values: none
 *
 * Globals input: line -- current input line
 *
 * Globals output: line -- contains new input line, terminated by a \0
 *
 * Error conditions: input line longer than LINEMAX chars will cause an error
 *							  and only get the first LINEMAX chars.
 *
 * Other functions called: err -- display an error message
 *
 */

{
    register SSINT ch,      /* Input character */
    quotechar;	/* Close-quote character we're looking for */
    register char *lp;  /* Pointer into line */

    lp = line;				/* Initialize local variables */
    quotechar = '\0';

#if defined(_WINDOWS) || defined(UNIXSHAREDLIB)
    while ((ch = wgetc(tablefile)) != EOF)
#else
    while ((ch = getc(tablefile)) != EOF)
#endif
    {
        ch &= 0xff;

        if ( ch == CTRLZ )							/* CTRLZ is internal EOF marker */
            break;

        if (ch == '\0' || ch == CARRIAGERETURN)  /* Ignore NUL or <CR> */
            ;													/*  in source text */

        else if (ch == '\n' || ch == FORMFEED)  /* End-of-line characters */
            break;
        else if ( lp >= line+LINEMAX)		/* Line too long? */
        {
            *lp = '\0';
            err("Line too long, end cut off");
            break;
        }
        else
        {
            if (quotechar != '\0')	/* Check for close-quote */
            {
                if (ch == quotechar)		/* Close quote found, ignore it */
                    quotechar = '\0';		 /*  and say we're no longer in quotes */
            }
            else
            {
                if (ch == HT)				/* Horizontal TAB to space */
                    ch = ' ';
                else if (ch == '"' || ch == '\'')  /* Open quote */
                    quotechar = ch;
            }
            *lp++ = (char)(ch & 0xff); /* Put the char into line */
        } /* End--else */

    } /* End--while */

    /* trim white space from end of line */
    while (lp > line && isspace(*(lp-1)))
        lp--;

    *lp = '\0';							/* Mark the end of the line */

} /* End--inputaline */

/****************************************************************************/
void parse( pntr, pntr2, errormessage) /* Parse an element of a line */
/****************************************************************************/
char **pntr,	/* Pointer to pointer to beginning of element found */
**pntr2;	/* Pointer to pointer to character just beyond element */
int errormessage;		/* Boolean: TRUE == output an error message if
							 *							 an unmatched quote is found
							 */

/* Description:
 *						Parse an element of a line in the change table, skipping
 *					over white space (TAB, SPACE, FF).	Handle quoted strings, but
 *					do not allow a line break inside a quoted string.
 *
 * Return values: *pntr points to the beginning of the element found
 *						*pntr2 points to the character just beyond the end
 *						  of the element
 *
 * Globals input: line -- current input line
 *
 * Globals output: none altered
 *
 * Error conditions: an unmatched quote will give *pntr pointing to the
 *							  open quote and *pntr2 pointing to the \0 at the
 *							  end of line.
 *							*pntr == *pntr2 implies that nothing was found
 *													(blank line or a comment)
 *
 * Other functions called: err -- display an error message
 *
 */

{
    register char *lp,	 /* Pointer to beginning of element */
    *lp2;   /* Pointer to next char beyond end of element */

    lp = *pntr2;								/* Set starting pointer */

    while ( (*lp == ' ') || (*lp == '\t') )		/* Skip spaces */
        lp++;
    lp2 = lp;

    if ( *lp == '"' || *lp == '\'' )			/* Quoted string */
    {
        while ( *(++lp2) && (*lp != *lp2) )			/* (skip to end of string) */
            ;
        if ( *lp2 )		/* Point lp2 to next char beyond the end of element */
            lp2++;
        else
            if ( errormessage )
            {
                ptokenstart= lp2;
                err( "Unmatched quote");
            }
    }
    else if (*lp == WEDGE)							/* A wedge */
        lp2++;		/* end of wedge is next character */
    else if (*lp == '(')
    {
        if ( errormessage )
        {
            ptokenstart= lp;
            err("Parenthesized parameter(s) must be adjacent to a keyword");
        }
        while ( *(++lp2) && (*lp2 != ')') )		/* Skip to closing paren */
            ;
        if (*lp2 == ')')
            lp2++;			/* Skip past closing paren (if any) */
    }
    else												/* Keyword */
        while ( *lp2										/* (move to end of it) */
                && ( strchr( " \t>'\"", *(++lp2) ) == NULL) )
            ;

    if ( ((lp+1) == lp2) && (toupper(*lp) == 'C') )			/* Comment */
        lp = lp2 = line + strlen(line);					 /*  (skip to end of it) */

    *pntr = lp;
    *pntr2 = lp2;
    ptokenstart= lp;
}/* End--parse */

/****************************************************************************/
bool wedgeinline()								 /* Is there a WEDGE in the line? */
/****************************************************************************/

/* Description -- Set 2 local pointers to point to the beginning of line.
 *
 *						Parse each element of line until we find either
 *						  a WEDGE or end-of-line.
 *
 *						If we found end-of-line
 *							Return FALSE.
 *						Else	(we must have found a WEDGE)
 *							Return TRUE.
 *
 *  Note:  Parse must be used rather than simply checking for a WEDGE
 *				 because otherwise we might find a WEDGE that was actually
 *				 inside a quoted string.
 *
 * Return values:  TRUE == There is a > in line
 *						FALSE == There is no > in line
 *
 * Globals input: line -- current input line
 *
 * Globals output: none altered
 *
 * Error conditions: none
 *
 * Functions called: parse -- get the next logical group from the input line
 *
 */

{
    char *wp, *wp2;			/* pointers used for parse calls */

    wp = wp2 = line;
    while ( *wp && *wp != WEDGE )  /* Look for either a WEDGE or end-of-line */
        parse( &wp, &wp2, FALSE);
    return( *wp != '\0' );

} /* End--wedgeinline */

/****************************************************************************/
void stornoarg(comand, dummy1, dummy2) /* Store commands which take no args */
/****************************************************************************/
char	comand;	/* Command to store */
char	dummy1, dummy2;	/* These are only used by storarg, but they
								 *   are passed to this procedure because it is called
								 *   using a pointer to a function.
								 */

/* Description -- If the comand has no arguments
 *							Store it in the table, using storechar.
 *						Else
 *							Give an error message, using err.
 *
 * Return values: none
 *
 * Globals input: parsepntr -- pointer to the last element parsed
 *
 * Globals output: none altered
 *
 * Error conditions: If there was an error, errors will be set to TRUE
 *							  via a call to err.
 *
 * Other functions called: storechar -- store an element into the internal
 *														change table
 *									err -- display an error message
 *
 */

{
    NOREF(dummy1);
    NOREF(dummy2);
    if ( *parsepntr == ' ' || !*parsepntr )
        storechar( comand);
    else
        err( "Illegal parenthesis");

} /* End--stornoarg */

/****************************************************************************/
void storarg( comand, sym_args, table_head ) /* Store commands taking args */
/****************************************************************************/
char	comand,		/* Command to be stored */
sym_args,	/* Boolean: TRUE == command can take symbolic arguments */
table_head; /* Index into the symbol table */

/* Description:
 *						This procedure stores commands with arguments such as
 *					if, set, and clear.	It is used during compiling to parse
 *					out these arguments.  It first checks for the open paren,
 *					then stores the number.
 *
 * Return values: none
 *
 * Globals input: parsepntr -- pointer to the current element being decoded
 *
 * Globals output: parsepntr -- updated
 *
 * Error conditions: any errors will set errors to TRUE via err.
 *
 * Other functions called: storechar -- store an element into the internal
 *														change table
 *									err -- display an error message
 *
 */

{
    register SSINT number;   /* number for building argument */
    int flag;                /* Flag for no digits between parentheses */
    char *pPredefined;       /* used to check for predefined store for out */
    bool bPredefinedFound;   /* TRUE if we find a predefined store for out */
    int  nPredefinedType;    /* if a predefined special store is found for
                                out command, this saves what type it is    */

    if ( *parsepntr != '(' )
    {
        ptokenstart= parsepntr;
        err("Missing parenthesis");
        return;
    }

    for ( ;; )			/* Do forever (exit is via a break, below) */
    {
        bPredefinedFound = FALSE;        // have not found special store
        nPredefinedType = 0;             // no special store type found yet
        if ( sym_args )
        {										/* Command can take symbolic arguments */

            // check to see if we have predefined stores with any of the
            // commands that support the predefined stores
            if (( comand == OUTCMD ) || ( comand == OUTSCMD )
                    || ( comand == IFEQCMD ) || ( comand == IFNEQCMD )
                    || ( comand == IFGTCMD ) || ( comand == IFNGTCMD )
                    || ( comand == IFLTCMD ) || ( comand == IFNLTCMD )
                    || ( comand == WRSTORECMD ))
            {
                pPredefined = parsepntr;   // point to delimeter before argument
                pPredefined++;             // point to argument for the command

                // do not continue checking for predefined stores if what we
                // are pointing at is not long enough for them.  Since the
                // predefined stores all have lengths of 13 or 14 bytes, and are
                // followed by a ',' or ')', then we can test for 14 bytes.
                if ( strlen(pPredefined) >= 14 )
                {
                    if ( strncmp(pPredefined, "cccurrentdate", 13) == 0 )
                    {
                        bPredefinedFound = TRUE;
                        nPredefinedType = CCCURRENTDATE;
                    }
                    else if ( strncmp(pPredefined, "cccurrenttime", 13) == 0 )
                    {
                        bPredefinedFound = TRUE;
                        nPredefinedType = CCCURRENTTIME;
                    }
                    else if ( strncmp(pPredefined, "ccversionmajor", 14) == 0 )
                    {
                        bPredefinedFound = TRUE;
                        nPredefinedType = CCVERSIONMAJOR;
                    }
                    else if ( strncmp(pPredefined, "ccversionminor", 14) == 0 )
                    {
                        bPredefinedFound = TRUE;
                        nPredefinedType = CCVERSIONMINOR;
                    }
                }
            }

            if ( (number = symbol_number( table_head, sym_args )) != 0 ) /* Get the number */
            {
                flag = 1;				/* Say everything is OK for below */
            }
            else
                return;					/* We had an error */

        } /* End--command takes symbolic arguments */
        else
        {
            if ( comand == DOUBLECMD )
                number = parsedouble( &flag );  /* set number and flag here */
            else
            {
                number = flag = 0;   /* Initialize for decoding number */
                while ( *++parsepntr >= '0' && *parsepntr <= '9' )
                {
                    number = 10 * number + *parsepntr - '0';   /* Decode the number */
                    flag = 1;                     /* (say we found a digit) */
                }
            }
        } /* End--command doesn't take symbolic arguments */

        if ( !flag )							/* Just 2 parentheses */
            err("Bad number");
        else if (( number > MAXARG ) && (comand != DOUBLECMD))  /* Other errors */
            err( "Number too big");
        else if ( number == 0 )
            err( "Number cannot be zero");
        else
        {
            /* Do doublebyte "pre-parsing" here.  We need to know early
               what out doublebyte args are, so determine that here.
            */
            if ( comand == DOUBLECMD)
            {
                if ( number > MAXDBLARG )
                    err( "Doublebyte number too big");
                else if ( doublebyte1st == 0)
                    doublebyte1st = number;
                else
                    if ( doublebyte2nd == 0)
                        doublebyte2nd = number;
                    else
                        Process_msg(errorFunction, 18, 0, 0);
            }
            storechar( comand);           /* store command  */
            storechar( number);           /* store argument */

            // if we found a predefined store with the out command, mark that
            if ( bPredefinedFound == TRUE )
                storepre[number] = nPredefinedType;

            if ( *parsepntr == ',')							/* Multiple arguments */
                continue;
            else if ( *parsepntr != ')' )
            {
                ptokenstart= parsepntr;
                err( "No close parenthesis");			/* Error */
            }
            else
                parsepntr++;							/* Skip the close parenthesis */
        } /* End--else */

        break;											/* Exit from the loop */
    }

} /* End--storarg */

/****************************************************************************/
void storoparg( comand, dummy1, dummy2) /* Store commands taking optional args */
/****************************************************************************/
char	comand;		/* Command to be stored */
char	dummy1, dummy2;	/* These are only used by storarg, but they
								 *   are passed to this procedure because it is called
								 *   using a pointer to a function.
								 */

/* Description:
 *						This procedure stores commands with optional arguments,
 *                                      namely fwd, back, and omit.
 *                                      It stores an argument
 *                                      of 1 if none is found.
 *
 * Return values: none
 *
 * Globals input: parsepntr -- pointer to the current element
 *
 * Globals output: table updated
 *
 * Error conditions: any errors will be reported by storarg
 *
 * Other functions called: storarg -- store a command having an argument
 *                         storechar -- store an element into the internal
 *														change table
 *
 */

{
    NOREF(dummy1);
    NOREF(dummy2);
    if ( *parsepntr == '(' )
        storarg(comand, FALSE, 0);				/* Arg with the command */
    else
    {
        storechar(comand);				/* No arg, so use a default of 1 */
        storechar(1);
    }

} /* End--storoparg */

/****************************************************************************/
void stordbarg( comand, dummy1, dummy2) /* Store dooublebyte command, args  */
/****************************************************************************/
char	comand;		/* Command to be stored */
char	dummy1, dummy2;	/* These are only used by storarg, but they
								 *   are passed to this procedure because it is called
								 *   using a pointer to a function.
								 */

/* Description:
 *                This procedure stores the doublebyte command, which can take
 *                                      0, 1, or 2 arguments.  It stores an argument
 *                                      of 1 if none is found.
 *                This routine is separated out since we need to do the
 *                parsing for this command right at first so we know whether
 *                to make doublebyte elements out of the store argument.
 *
 * Return values: none
 *
 * Globals input: parsepntr -- pointer to the current element
 *
 * Globals output: table updated
 *
 * Error conditions: any errors will be reported by storarg
 *
 * Other functions called: storarg -- store a command having an argument
 *                         storechar -- store an element into the internal
 *														change table
 *
 */

{
    NOREF(dummy1);
    NOREF(dummy2);
    doublebyte_mode = TRUE;

    if ( *parsepntr == '(' )
    {
        storarg(comand, FALSE, 0);				/* Arg with the command */
    }
    else
    {
        storechar(comand);				/* No arg, so use a default of 1 */
        storechar(1);
    }

} /* End--stordbarg */

/****************************************************************************/
static SSINT parsedouble ( myflag)            /* parse doublebyte arguments */
/****************************************************************************/
int *myflag;      /* Return 1 for found valid digit, 0 otherwise  */

/* Description:
 *                This procedure does the parsing for the arguments for  
 *                the doublebyte command.  This is passed whatever is
 *                within the parenthesis and commas.  The default is to 
 *                parse in octal, but we will parse in decimal or hex if
 *                the input is preceeded by 'd' or 'x' (or 'D' or 'X').
 *
 * Return values: returns the numeric value parsed
 *                *myflag is set to 0, or 1 if found valid digit(s)
 *
 * Globals input: parsepntr -- pointer to the current element
 *
 * Globals output: parsepntr is updated to point beyond last valid digit
 *
 * Error conditions: any errors will be reported by storarg
 *
 */

{
    int answer = 0;    /* value to return  */
    int num;

    *myflag = 0;       /* denote no valid digits found yet */

    if ( ( *++parsepntr == 'd' ) || ( *parsepntr == 'D' ) )
    {
        /* user wants number treated as a decimal number */
        while ( *++parsepntr >= '0' && *parsepntr <= '9' )
        {
            answer = 10 * answer + *parsepntr - '0';   /* Decode number */
            *myflag = 1;                      /* (say we found a digit) */
        }
        return(answer);
    }

    else if ( ( *parsepntr == 'x' ) || ( *parsepntr == 'X' ) )
    {
        /* user wants number treated as hexadecimal number */
        num = a_to_hex(*++parsepntr);   /* get numeric value of hex char */
        while ( (num >= 0) && (num <= 15) )
        {
            answer = 16 * answer + num;     /* Decode hex number */
            *myflag = 1;                    /* (say we found a digit) */
            num = a_to_hex(*++parsepntr);   /* get numeric value of hex char */
        }
        return(answer);

    }

    else
    {
        /* default to assuming octal number      */
        while ( *parsepntr >= '0' && *parsepntr <= '9' )
        {
            /* first check to validate we have legal octal number */
            if ( ( *parsepntr == '8') || ( *parsepntr == '9' ) )
                err( "Error - doublebyte argument is illegal octal number");
            answer = 8 * answer + *parsepntr - '0';    /* Decode number */
            *myflag = 1;                      /* (say we found a digit) */
            parsepntr++;
        }
        return(answer);
    }
}

/****************************************************************************/
static void octal_decode(register int *number)			/* Decode an octal number */
/****************************************************************************/

/* Description:
 *						Go through the current element, which is assumed to be
 *					in octal, and build an 8-bit binary number.
 *
 * Return values: number will be set to the result
 *
 * Globals input: parsepntr -- pointer to the current element being parsed
 *
 * Globals output: parsepntr -- updated
 *
 * Error conditions: any errors will be reported via err.
 *
 * Other functions called: err -- display an error message
 *
 */

{
    *number = 0;						/* Initialize number */
    while ( parsepntr < parse2pntr )		/* Decode the whole thing */
    {
        if ( *parsepntr >= '0' && *parsepntr <= '7' )
        {
            *number = (*number * 8) + (*parsepntr++ - '0');
            if ( *number > 255 )
            {
                err( "Octal number too big, must not exceed 377");
                return;
            }
        }
        else
        {
            err( "Invalid octal digit");
            return;
        }
    } /* End--while */
} /* End--octal_decode */

/****************************************************************************/
static int hex_decode(char * pstr)  /* Decode a hexadecimal sequence        */
/****************************************************************************/

/*
 * Description:
 *		Check the length of the string (which can be of any length) to find out
 * how many digits we have.  If there is an odd number of digits,
 * right-justify the number (the first byte will have a left nibble of zero).
 * The result is a string in pstr
 *		If we encounter an invalid hex digit, display an error message.
 *
 * Note: When we get here, we know we have a leading 'x' or 'X', followed
 *			by at least one valid hex digit.
 *
 *
 *
 * Return values: length of resulting string
 *
 * Globals input: parsepntr -- pointer to the beginning of the current element
 *						parse2pntr -- pointer to just beyond the current element
 *
 * Globals output: parsepntr -- updated based on what we found
 *
 * Error conditions: If an invalid hex digit is found, an error message will
 *							be displayed and errors will be set to TRUE, via the
 *							routine err().
 *
 * Other procedures called: err -- display an error message
 *									 a_to_hex -- convert an ASCII char to hex
 *
 */
{
    register char next_byte;   /* The next "byte" (element) to store */
    char * pstrstart= pstr;

    parsepntr++;				/* Skip over the leading 'x' */

    if ( odd( parse2pntr - parsepntr ) )
    {
        next_byte = a_to_hex( *parsepntr++ );  /* Handle the odd digit */
        *pstr++= next_byte;
    }

    next_byte = 0;

    while ( parsepntr < parse2pntr )
    {
        if ( isxdigit( *parsepntr ) )
            next_byte = next_byte | (((a_to_hex( *parsepntr++ )) << 4) & 0x00ff);
        else
        {
            err( "Invalid hexadecimal digit" );
            return 0;
        }

        if ( isxdigit( *parsepntr ) )
        {
            next_byte = next_byte | (a_to_hex( *parsepntr++ ) & 0x00ff);
        }
        else
        {
            err( "Invalid hexadecimal digit" );
            return 0;
        }

        *pstr++= next_byte;
        next_byte = 0;                /* clear this out for next time */
    } /* End--while */

    return pstr - pstrstart;

} /* End--hex_decode */

/****************************************************************************/
static int ucs4_decode(char * pstr)  /* Decode a usc4 string converting it to utf8 */
/****************************************************************************/

/*
 * Description:
 *		Check the length of the string (which can be of any length) to find out
 * how many digits we have.  If there is an odd number of digits,
 * right-justify the number (the first byte will have a left nibble of zero).
 * The result is a string in pstr
 *		If we encounter an invalid hex digit, display an error message.
 *
 * Note: When we get here, we know we have a leading 'u' or 'u', followed
 *			by at least one valid hex digit.
 *
 *
 *
 * Return values: length of resulting string
 *
 * Globals input: parsepntr -- pointer to the beginning of the current element
 *						parse2pntr -- pointer to just beyond the current element
 *
 * Globals output: parsepntr -- updated based on what we found
 *
 * Error conditions: If an invalid hex digit is found, an error message will
 *							be displayed and errors will be set to TRUE, via the
 *							routine err().
 *
 * Other procedures called: err -- display an error message
 *									 a_to_hex -- convert an ASCII char to hex
 *
 */
{
    register unsigned char next_byte;   /* The next "byte" (element) to store */
    UCS4 ch;
    int count;

    parsepntr++;				/* Skip over the leading 'u' */

    if ( odd( parse2pntr - parsepntr ) )
        ch = a_to_hex( *parsepntr++ );  /* Handle the odd digit */
    else
        ch= 0;

    next_byte = 0;


    while ( parsepntr < parse2pntr )
    {
        if ( isxdigit( *parsepntr ) )
            next_byte = next_byte | (((a_to_hex( *parsepntr++ )) << 4) & 0x00ff);
        else
        {
            err( "Invalid UCS4 digit" );
            return 0;
        }

        if ( isxdigit( *parsepntr ) )
        {
            next_byte = next_byte | (a_to_hex( *parsepntr++ ) & 0x00ff);
        }
        else
        {
            err( "Invalid UCS4 digit" );
            return 0;
        }

        ch= ch << 8 | next_byte;
        next_byte = 0;                /* clear this out for next time */
    } /* End--while */

    count= UCS4toUTF8(ch, pstr);

    return count;

} /* End--ucs4_decode */

/****************************************************************************/
static void decimal_decode( number )	 /* Decode a decimal number */
/****************************************************************************/
register int *number;
/*
 * Description:
 *			Skip the leading 'd' (indicating decimal). Then go through the
 *		string, decoding one digit at a time.	If a non-digit is encountered
 *		before the end of the element, give an error message and return.
 *
 * Note: We know before we get called that we have a leading 'd' or 'D',
 *			followed by at least one digit.
 *
 * Return values: *number will contain whatever was decoded.
 *
 * Globals input: parsepntr -- pointer to the beginning of the current element
 *						parse2pntr -- pointer to just beyond the current element
 *
 * Globals output: parsepntr -- updated based on what we found
 *
 * Error conditions: If an invalid digit is found or the number is greater than
 *							255, an error message will be displayed
 *							and errors will be set to TRUE.
 *
 * Other procedures called: err -- display an error message
 *
 */
{
    parsepntr++;			/* Skip over the 'd' or 'D' */

    *number = 0;			/* Initialize target */

    while ( parsepntr < parse2pntr )				/* Decode the whole thing */
    {
        if ( isdigit( *parsepntr ) )
        {
            *number = (*number * 10) + (*parsepntr++ - '0');
            if ( *number > 255 )
            {
                err( "Decimal number too big, must be less than 256");
                return;
            }
        }
        else
        {
            err( "Invalid decimal digit");
            return;
        }
    } /* End--while */
} /* End--decimal_decode */

/****************************************************************************/
void buildkeyword()			/* Copy suspected keyword into the array keyword */
/****************************************************************************/
/* Description -- Loop until we get to the end of the current element
 *						  (loop exit is via a break)
 *
 *							If current character is (
 *								Break.
 *							If we're not fixing to overflow the array keyword
 *								Copy the character into keyword.
 *							Increment parsepntr (pointer into current element).
 *							Put a string-terminating \0 at the current end
 *							  of keyword.
 *
 * Return values: none
 *
 * Globals input: keyword -- array used for checking keywords
 *						parsepntr -- pointer to the current element
 *						parse2pntr -- pointer to just beyond the current element
 *
 * Globals output: keyword -- now contains the suspected keyword
 *						 parsepntr -- points to one char beyond the end of
 *											 the suspected keyword
 *
 * Error conditions: if the suspected keyword will overflow the array keyword,
 *							  the excess will be ignored
 *
 * Other functions called: none
 *
 */

{
    register char *kp;

    for ( kp = keyword; parsepntr < parse2pntr; )
    {
        if ( *parsepntr == '(' )		/* End of suspected keyword */
            break;

        if ( kp < keyword+sizeof(keyword)-1 )	/* Don't overflow array */
            *kp++ = (char)tolower(*parsepntr);

        parsepntr++;	/* Keep moving through the element, whether we're */
        /*  storing what we see or not */

    }
    *kp = '\0';						/* Put terminator in keyword array */

} /* End--buildkeyword */

/****************************************************************************/
void storestring(int search, char * string, int sLen)
/****************************************************************************/
/*
 *  NOTE: This written by DAR 4/96
 *
 */

{
    SSINT *valstart = NULL;        /* Start of a quoted string, for valnum()      */
    char valbuf[20];        /* Dummy buffer for valnum()                   */
    long val_dummy;         /* Dummy for valnum()                          */
    SSINT double_value;     /* Accumulate potential doublebyte elements    */
    int number;
    char *tmp;

    if ( was_math )
        valstart = tloadpointer;        /* Set up to check the string to
    							* see if it's a valid number
    							*/
    was_string = TRUE;

    /* store low order byte (and zero out high order byte if any)       */
    while ( sLen )
    {
        /* if doublebyte mode and we are on the left side of replacement */
        if (( doublebyte_mode ) && ( search ) && sLen > 1)
        {
            /* first get possible first half of doublebyte pair, and
            	then test whether these two meet the criteria or not    */
            double_value = (SSINT) (*string & 0x00ff);
            string++;
            sLen--;

            if (( ((SSINT) (*string & 0x00ff)) >= doublebyte2nd ) &&
                    ( double_value >= doublebyte1st ))
            {
                double_value = (double_value * 256)
                               | ((SSINT) (*string & 0x00ff));

                string++;
                sLen--;

                if ( double_value & HIGHBIT )
                    storechar( TOPBITCMD );

                storechar((SSINT) (double_value & 0x7fff));

            }
            else
                /* doublebyte, but did not meet both criteria tests     */
            {
                storechar((SSINT)(double_value & 0x00ff));
            }
        }
        else
        {
            storechar ( (SSINT)((*string) & 0x00ff));
            string++;
            sLen--;
        }
    }

    if ( was_math )
    {                /* Be sure it's a valid number in the
        						* range -1,999,999,999 to +1,999,999,999
        						*/

        storechar( SPECPERIOD ); /* Temporarily put a command after the string */

        if ( (number = valnum( valstart, valbuf, 20, &val_dummy, FALSE))
                != TRUE )
        {
            if ( number == -1 )
            {
                /* this hack adjusts the cursor in the error message to point
                   either to the end of the failing item, or at least to point
                   before the subsequent item (instead of pointing after the
                   subsequent item which gives the user very bad information). */
                tmp = parse2pntr;
                parse2pntr = parsepntr;  /* back up to start of this entry */
                parse2pntr--;            /* back up                        */
                parse2pntr--;            /*   to end of prior element      */
                err( "Number too big, must be less than 2,000,000,000" );
                parse2pntr = tmp;        /* restore this to where we were */
            }
            else
            {
                /* this hack adjusts the cursor in the error message to point
                   either to the end of the failing item, or at least to point
                   before the subsequent item (instead of pointing after the
                   subsequent item which gives the user very bad information). */
                tmp = parse2pntr;
                parse2pntr = parsepntr;  /* back up to start of this entry */
                parse2pntr--;            /* back up                        */
                parse2pntr--;            /*   to end of prior element      */
                err( "Invalid number for arithmetic" );
                parse2pntr = tmp;        /* restore this to where we were */
            }
        }

        tloadpointer--;          /* "Un-store" the SPECPERIOD from above */
        was_math = FALSE;
    } /* End--previous command was a math operator */
} /* End--storestring */

/****************************************************************************/
void flushstringbuffer(int search)
/****************************************************************************/
/*
 *   Note: This written by DAR 4/96
 *
 */
{
    int sLen;

    if (pstrBuffer != strBuffer)
    {
        sLen = pstrBuffer - strBuffer;
        storestring(search, strBuffer, sLen);
        pstrBuffer = strBuffer;
    }
    else
        // if zero-length string, still denote it wi a string.    7/96
        was_string = TRUE;
} /* End--flushstringbuffer */

/****************************************************************************/
void storeelement(search)	/* Store a parsed element in internal change table */
/****************************************************************************/
int search;  /* Boolean --  TRUE == we're on the search side of a change, */
/* FALSE == we're on the replacement side */

/* Description:
 *		This procedure stores a parsed element in the internal change table.
 * It recognizes quoted strings, octal numbers, and keywords.	It gives an
 * error message if the element is bad.  It uses search to tell if it is
 * storing a search element or a replacement element.
 *
 * Return values: none
 *
 * Globals input: parsepntr -- pointer to the beginning of the current element
 *						parse2pntr -- pointer to just beyond the current element
 *						keyword -- array used for comparing suspected keywords
 *						fontsection -- boolean: TRUE == the table has a font section
 *						tloadpointer -- pointer to the next empty byte in the table
 *						table -- pointer to the beginning of the table
 *                was_math -- boolean: TRUE == last command was a math command
 *                                               (add, sub, mul, div)
 *                was_string -- boolean: TRUE == last command was a quoted string
 *
 * Globals output: parsepntr -- updated based on what we found
 *						 keyword -- may contain a suspected keyword
 *						 fontsection -- may be set to TRUE if we found the beginning
 *												of the font section of the table
 *						was_math -- updated
 *						was_string -- updated
 *
 * Error conditions: any error found will result in errors being set to TRUE
 *							  and an appropriate error message being displayed
 *
 * Other functions called: storechar -- store an element in the internal
 *														change table
 *									octal_decode -- decode an ASCII octal number
 *									hex_decode -- decode an ASCII hexadecimal number
 *									buildkeyword -- copy a suspected keyword into
 *															the keyword array for checking
 *									storarg -- store a command with a numeric argument
 *									valnum -- validate a numeric argument for
 *													the arithmetic routines
 *									err -- display an error message
 *
 */

{
    int number,             /* General purpose integer variable            */
    last_command;        /* Previous command stored in the table        */
    int ch;                 /* Single character                            */

    struct cmstruct *cmpt;
    int nbytes;

    /* If we have already processed the begin, compilecc should have    */
    /*  stored a SPECWEDGE as the first char of the table.  Otherwise   */
    /*  we have an error */
    if ( begin_found && (tloadpointer == table) )
    {
        err( "Illegal search string in begin statement");
        return;
    }

    if ( *parsepntr == '"' || *parsepntr == '\'' )
    {                                                /* Quoted string   */
        ++parsepntr;
        nbytes= parse2pntr - parsepntr - 1;

        if (nbytes > 0)
        {
            strncpy(pstrBuffer, parsepntr, nbytes);
            pstrBuffer+= nbytes;
        }
    }
    else if ( *parsepntr == '0' )  /* If starts with 0 it must be a number */
    {
        parsepntr++;	/* Skip over the '0' */
        if ( (ch = tolower(*parsepntr)) == 'x' )
        {
            nbytes= hex_decode(pstrBuffer);
            pstrBuffer+= nbytes;
        }
        else if (ch == 'u')
        {
            nbytes= ucs4_decode(pstrBuffer);
            pstrBuffer+= nbytes;
        }
        else
        {
            if (ch == 'd')
                decimal_decode( &number );			/* Decimal code number */
            else
                octal_decode( &number );			/* Octal code number */
            *pstrBuffer++= number & 0xFF;
        }						// moved to fix problem with 0x syntax 4/7/99
    }
    else if ( *parsepntr >= '0' && *parsepntr <= '7')
    {											/* Octal number */
        octal_decode( &number );
        *pstrBuffer++= number & 0xFF;

    }
    else if ( (tolower( *parsepntr ) == 'd') && isdigit( *(parsepntr + 1)) )
    {																/* Decimal number */
        decimal_decode( &number );
        *pstrBuffer++= number & 0xFF;
    }
    else if ( (tolower(*parsepntr) == 'x') && isxdigit( *(parsepntr + 1)) )
    {																/* Hexadecimal number */
        nbytes= hex_decode(pstrBuffer);
        pstrBuffer+= nbytes;
    }
    else if ( (tolower(*parsepntr) == 'u') && isxdigit( *(parsepntr + 1)) )
    {
        nbytes= ucs4_decode(pstrBuffer);
        pstrBuffer+= nbytes;
    }
    else if ( *parsepntr == '(' )	/* Bogus parameter list */
        ;							/* simply ignore */
    else
    {																/* Keyword */
        flushstringbuffer(search);
        buildkeyword();
        cmpt = NULL;
        if ( search )
        {															/* Search side */
            if ( !strcmp(keyword,"begin") )	/* begin */
            {
                if ( tloadpointer == table+1 )
                {
                    tloadpointer--;
                    begin_found = TRUE;					/* We have a begin statement */
                }
                else
                    err( "Begin command not first in table");
            }
            else
                cmpt = cmsrch;						/* Search the search-side array */
        }
        else
        {															/* Replacement side */
            if ( (tloadpointer == table) && !strcmp(keyword,"begin") )
            {
                err( "No wedge on \'begin\' line");
            }
            else if ( !strcmp(keyword,"use") )
            {										/* Use */
                storechar( USECMD );
                storarg( INCLCMD, REFERENCED, GROUP_HEAD );
            }
            else if ( !strcmp(keyword,"font") )
            {												/* Font */
                fontsection = TRUE;
                storechar( SPECPERIOD);						/* Terminate last change */
            }
            else
                cmpt = cmrepl;					 /* Search the replacement-side array */
        }

        if ( cmpt != NULL )
        {
            for ( ; cmpt->cmname; cmpt++ )				/* Look for a match */
                if ( !strcmp(keyword, cmpt->cmname) )
                {
                    number = cmpt->cmcode;       // Get command code for comparisons
#if defined(_WINDOWS) || defined(UNIXSHAREDLIB)  // ignore read command for windows                                     
                    if (number == READCMD)       // 7.4.15
                        return;
#endif
                    if (number != CONTCMD)
                    {
                        if ( was_math )
                            err( "Illegal command following arithmetic operator" );
                        else
                        {
                            if ( tloadpointer >= (table + 2) )
                            {
                                last_command = *(tloadpointer - 2);
                            }
                            else
                            {
                                last_command = 0;			/* Nothing there yet */
                            }

                            if ( !(was_string || number == '\r') &&
                                    ((last_command == IFEQCMD) ||
                                     (last_command == IFGTCMD) ||
                                     (last_command == IFNGTCMD) ||
                                     (last_command == IFLTCMD) ||
                                     (last_command == IFNLTCMD) ||
                                     (last_command == IFNEQCMD) ||
                                     (last_command == IFSUBSETCMD) ))
                                err( "Illegal command following comparison operator" );
                        }
                    } /* End--command was not cont */

                    if ( was_math )
                    {								/* Last command was a math operation */
                        was_math = FALSE;
                    }

                    /*   Store the command,
                    * using the function
                    * from the search table
                    */
                    (*(cmpt->cmfunc))
                    ( cmpt->cmcode, cmpt->symbolic, cmpt->symbol_index );

                    was_string = FALSE;
                    /* Did we just process a math command? */
                    if (	number == ADDCMD ||
                            number == SUBCMD ||
                            number == MULCMD ||
                            number == DIVCMD ||
                            number == MODCMD )
                        was_math = TRUE;
                    else
                        was_math = FALSE;

                    return;
                }

            err( "Unrecognized keyword");
        } /* End--look for a match */
        was_string = FALSE;
    } /* End--keyword */

} /* End--storeelement */

/****************************************************************************/
char *cmdname(cmd, search)	/* Get name for given command code */
/****************************************************************************/
char cmd;		/* Command code */
int search;		/* Boolean --  TRUE == we're on the search side of a change, */
/* FALSE == we're on the replacement side */
{
    struct cmstruct *cmpt;

    /* Point to proper search table */
    if (search)
        cmpt = cmsrch;
    else
        cmpt = cmrepl;

    /* Look for a matching command code */
    for ( ; cmpt->cmname; cmpt++ )
    {
        if (cmd == cmpt->cmcode)
            return(cmpt->cmname);
    }

    /* If no match was found return a bad name */
    return("UNKNOWN");
}

/*************************************************************************/
static int symbol_number( sym_type, sym_use ) /* Get number for symbolic name */
/*************************************************************************/
int sym_type;	/* Index into the sym_table array (0..3) */
char sym_use;	/* One of two equates DEFINED or REFERENCED, from ms01.h */

/*
 * Description:
 *						Parse the next element within a command, search the
 *			symbol table for it and add it to the table if necessary.  Return
 *			the number (1..MAXARG) for the element.
 *
 *  Note: if the element is a numeric value, the number returned may or may not
 *				be the same as the element, since the number may have already been
 *				used.
 *						  (Talk about confusion!)
 *
 * Return values: if successful, return an integer in the range 1..MAXARG
 *						if error, return 0.
 *
 * Globals input:
 *						parsepntr -- pointer to the current element being decoded
 *						sym_table -- symbol table head array
 *
 * Globals output:
 *						parsepntr -- updated
 *						sym_table -- updated
 *
 * Error conditions: if an error occurs, an error message will be displayed
 *							  via the procedure err(), and 0 will be returned.
 *
 * Other procedures called:
 *									err -- display an error message
 *
 */

{
    register char *beginning_of_symbol,  /* Pointers to name/number */
    *end_of_symbol;

    register CC_SYMBOL *cursym;  /* Used for traversing symbol table */

    CC_SYMBOL *sym_temp;		/* Used while adding a new element to the table */

    char replaced_char;		/* Temp for char beyond end of symbol */
    int overload;				/* Boolean: TRUE == numeric value of the element
    								*							has already been assigned
    								*/

    int  numval;		  /*	 (possible) numeric value, if an actual numeric arg
    							* was given
    							*/

    int i;		/* Integer temp */

    overload = FALSE;		/* Initialize locals */

    /* Find the beginning of the name/number */

    for ( beginning_of_symbol = ++parsepntr; ; beginning_of_symbol++ )
        if ( (*beginning_of_symbol == CARRIAGERETURN)
                || ( *beginning_of_symbol == ',' ) || ( *beginning_of_symbol == ')' )
                || !isspace( *beginning_of_symbol ) )
        {
            if ( (beginning_of_symbol == parsepntr)
                    && ( *beginning_of_symbol == ')' ) )
            {
                ptokenstart= beginning_of_symbol;
                err( "Missing parameter" );	  /* Nothing there */
                return( 0 );
            }
            else
                break;												/* Found the beginning */
        }

    /* Now find the end */

    for ( end_of_symbol = beginning_of_symbol; ; end_of_symbol++ )
        if ( (*end_of_symbol == CARRIAGERETURN)
                || ( *end_of_symbol == ',' ) || ( *end_of_symbol == ')' )
                || isspace( *end_of_symbol ) )
            break;

    /*   Temporarily convert it to a
    	* NUL-terminated string
    	*/
    replaced_char = *end_of_symbol;
    *end_of_symbol = '\0';

    numval = atoi( beginning_of_symbol );	/*  Try converting to a numeric value */

    if ( (i = search_table( sym_type, sym_use,
                           beginning_of_symbol, 0, NAME_SEARCH )) != 0 )
    {
        *end_of_symbol = replaced_char;	 /* Get rid of the NUL */

        while ( isspace( *end_of_symbol ) )			/* Skip trailing white space */
            end_of_symbol++;

        parsepntr = end_of_symbol;			 /* Update parsepntr */
        return( i );
    }

    /* Number already assigned? */
    overload = search_table( sym_type, sym_use, NULL, numval, NUMBER_SEARCH );

    if ( (sym_temp = (CC_SYMBOL *) malloc( sizeof(CC_SYMBOL) )) != NULL )
    {
        sym_temp->next = NULL;				/* Mark the new end of the list */

        if ( !overload && (numval != 0) && (numval <= MAXARG) )
            sym_temp->number = numval;			 /* Use its actual numeric value */
        else
        {											 /* Assign it one */
            i = sym_table[ sym_type ].next_number;
            /* Find the next available number */
            while ( search_table( sym_type, sym_use, NULL, i, NUMBER_SEARCH ) )
                i--;
            sym_temp->number = i;
            if ( i <= 0 )
            {									/* No number available */

                *end_of_symbol = replaced_char;		/* Get rid of the NUL */
                switch( sym_type )
                {
                case STORE_HEAD:
                    err( "Too many stores" );
                    break;
                case SWITCH_HEAD:
                    err( "Too many switches" );
                    break;
                case DEFINE_HEAD:
                    err( "Too many defines" );
                    break;
                case GROUP_HEAD:
                    err( "Too many groups" );
                    break;
                }	/* End--invalid number */

                return( 0 );		/* Error return */
            }
            /* Update next available number */
            sym_table[ sym_type ].next_number = --i;
        }

        sym_temp->name = (char *) malloc(strlen(beginning_of_symbol) + 1);
        if ( sym_temp->name != NULL )
        {
            strcpy( sym_temp->name, beginning_of_symbol );	/* Copy the name */

            *end_of_symbol = replaced_char;			/* Get rid of the NUL */
        }
        else
        {
            *end_of_symbol = replaced_char;			/* Get rid of the NUL */

            err( "Unable to allocate space for symbolic name" );	/* No more space */
            return( 0 );
        }
        sym_temp->use = sym_use;						/* Set initial usage */

    } /* End--successful allocation of new symbol */
    else
    {
        *end_of_symbol = replaced_char;			/* Get rid of the NUL */

        err( "Unable to allocate space for symbolic name");
        return( 0 );			  /* Error, unable to allocate space */
    }

    if ( (cursym = sym_table[ sym_type ].list_head) == NULL )
        sym_table[ sym_type ].list_head = sym_temp;	 /* List was empty */
    else
    {
        while ( cursym->next != NULL )		 /* Find the end of the list */
            cursym = cursym->next;

        cursym->next = sym_temp;	/* Add the element on to the end of the list */
    }

    while ( isspace( *end_of_symbol ) )		/* Skip trailing white space */
        end_of_symbol++;

    parsepntr = end_of_symbol;		/* Update parsepntr */

    return( sym_temp->number );	 /* Return the number for the new element */
} /* End--symbol_number */

/*************************************************************************/
static int
search_table( table_index, sym_usage, sym_name, sym_num, type_of_search)
/*************************************************************************/
int table_index;	  /* index into sym_table array */
char sym_usage,	  /* either REFERENCED or DEFINED */
*sym_name;	  /* if (type_of_search) name to be searched for */
int sym_num;		  /* if ( !type_of_search ) number to be searched for */
int type_of_search; /* 1 == search for name match
							* 0 == search for number match
							*/
/*
 * Description:	Search the symbol table, looking for a match of either
 *					 names (type_of_search == 1) or numbers (type_of_search == 0).
 *
 * Return values:
 *					 If a match is found, return the number of the symbol,
 *					 otherwise return zero.
 *
 *					 If searching by name and a match is found,
 *						OR the usage into the use field of the symbol
 *
 * Globals input: sym_table -- symbol table head array
 *
 * Globals output: none
 *
 * Error conditions: if no match is found, zero will be returned
 *
 * Other procedures called: none
 *
 */
{
    register CC_SYMBOL *list_pointer;  /* Pointer for traversing the list */

    list_pointer = sym_table[ table_index ].list_head;

    for ( ; list_pointer != NULL; list_pointer = list_pointer->next )
    {
        if ( type_of_search )
        {												 /* Looking for a name match */
            if ( !strcmp( list_pointer->name, sym_name ) )
            {
                /* Add the current usage to the use field
                *	 of the symbol, to allow checking for
                *	 used-but-not-defined symbols
                *		(typically caused by typos)
                */
                list_pointer->use = list_pointer->use | sym_usage;
                return( list_pointer->number );			  /* names match */
            }
        }
        else
        {												 /* Looking for a number match */
            if ( list_pointer->number == sym_num )
                return( list_pointer->number );			  /* numbers match */
        }
    } /* End--for */

    return( 0 );	/* No match found */

} /* End--search_table */

/* END */
