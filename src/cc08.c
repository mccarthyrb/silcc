/* CC08.C */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(UNIX)
#include <dos.h>
#endif
#include <assert.h>
#ifdef _WINDOWS 
#include "windows.h"
#include "ccdll\cctio.h"
#endif
#if defined(UNIX)
#include "wtype.h"
#include "cctio.h"
#endif
#include "cc.h"
#include "c8type.h"
#include "keybrd.h"
#include "cc08.h"

#ifdef TEST_makeArgv
#define safalloc(a)  malloc(a) 
#if !defined(UNIX)
#include <malloc.h> 
#endif
#endif


#ifdef _WINDOWS


/****************************************************************************/
int makeArgv( char *pszCmdLine, char*** papszArg, int* pcArg)
/****************************************************************************/
/* pszCmdLine - commandLine passed to the function
   papszArg - a pointer to a char** variable you will probably name "argv". 
   makeArgv() allocates an array of pointers and sets *papszArg to point to it. 
   pcArg - a pointer to an int variable you will probably name "argc".
   makeArgv() sets *pArg to the number of vectors in the array

 * Description: This procedure processes a string containing command-line
 *				commands.  	It will ultimately produce a
 *				look alike for argc and argv so that those procedures set up
 *				to use command line arguments need never know the difference.
 *
 *          NOTE:  No special code is provided to free the allocated memory.
 *
 * Return values: A return from this procedure indicates success.
 *
 * Globals input: none
 *
 * Globals output: none
 *
 * Error conditions: none 
 *
 * Other procedures called: none
 * */

{
    char *pchCmdLine,		/* pointer into given command line */
    *pchBuf,		  /* pointer into buffer used for getting an arg */
    *pchArgStr;		  /* pointer into  allocated memory for arg strings  */
    char pBuf[81];			  /* input buffer */
    int cLenStr;			  /* length of most recently size-up string */
    unsigned cAllocation;	  /* # of bytes to allocate in the malloc call */
    int	cArgs,iArg;            /* count, index of arguments */
    char **apszArg;       /* array of pointers (to be allocated) */

    /* allocate for 2 pointers (argv[0] and NULL final element) */
    cAllocation = ( (unsigned) sizeof(char *) ) * 2;
    cAllocation += sizeof(char);	/* for null string for arg 0 */

    /* Start of sizing pass */

    cArgs = 1;

    pchCmdLine = pszCmdLine;
    while ( *pchCmdLine )
    {
        /* Skip white space */
        while (*pchCmdLine && isspace(*pchCmdLine))
            pchCmdLine++;
        /* Get the string */
        pchBuf = pBuf;
        while (*pchCmdLine && !isspace(*pchCmdLine))
            *pchBuf++ = *pchCmdLine++;
        *pchBuf = '\0';

        cLenStr = strlen(pBuf);
        if ( cLenStr  )
        {						  /* Allow for string */
            cAllocation += (unsigned) (cLenStr + 1);
            /* Allow for apszArg element */
            cAllocation += (unsigned) sizeof(char *);

            cArgs++;			  /* Update count of valid arguments */
        }

    } /* End--while */

    /* Allocate memory */
    apszArg = (char **) malloc( cAllocation );

    /* Start of copying pass */

    pchArgStr = (char *) &apszArg[cArgs + 1]; /* beginning of string space */
    pchCmdLine = pszCmdLine;

    *pchArgStr = '\0';         /* a nul string */
    *apszArg = pchArgStr++;	  /* set element 0 of apszArg */

    for ( iArg = 1; iArg < cArgs; iArg++ )
    {
        /* Skip white space */
        while (*pchCmdLine && isspace(*pchCmdLine))
            pchCmdLine++;
        /* Get the string */
        pchBuf = pBuf;
        while (*pchCmdLine && !isspace(*pchCmdLine))
            *pchBuf++ = *pchCmdLine++;
        *pchBuf = '\0';

        cLenStr = strlen(pBuf);
        if ( cLenStr )	/* If non-empty string */
        {
            apszArg[iArg] = pchArgStr; /* set next arg pntr */
            strcpy(pchArgStr, pBuf);
            pchArgStr += cLenStr + 1; /* bump pointer */
        }
    } /* End--for */
    assert (pchArgStr == (char *)apszArg + cAllocation);

    apszArg[cArgs] = NULL;	  /* last element of apszArg */

    /* return the results */
    *papszArg = apszArg;
    *pcArg = cArgs;
    return(0);
}


/****************************************************************************/
FILE* fdMessageFile ()
/****************************************************************************/
/* opens the file that receives the CC messages (in DOS, msgs go to stdout) */
{
    return  fopen ("cc08log.txt","w");
}

void CloseMessageFile(FILE *fd)
{
    fclose (fd);
}

#endif
