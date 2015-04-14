/* ccmsdo.c   MS/CC MSDOS-Specific Routines		3-Feb-86		SMc
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
 *                     Name changed to cc1c.c
 *	 19-Jul-91      BLR Removed all code relating to file questions.
 *                     Files are now specified via the command line only.
 *  10-Oct-91      DJE Revised filename wildcard handling to match DOS
 *                     RENAME.  Also add -a (append) switch to the command
 *                     line processing.
 *   5-Nov-91      DJE Added -w command line option to allow the write and
 *                     wrstore commands to write to an output file.
 *
 * This module provides the following system-dependent global routines:
 *
 * sysinit( argc ) -- System-dependent intialization code.
 * int argc;
 *
 * syscleanup() -- System-dependent cleanup routine.
 *
 * cmdlnparse( argc, argv ) -- Interpret the command line.
 * int argc;
 * char *argv[];
 *
 * usage() -- Display a "usage..." message.
 *
 * get_next_infile() - get name of next input file and open it.
 *
 * openfilepair() - get names of next input and output files, and open them.
 *
 * openinfile() -- Open an input file.
 *
 * openoutfile() -- Open an output file.
 *
 * openwritefile() -- Open the write output file (if any).
 *
 * get_char() -- Read a character from the keyboard
 *
 */

#if defined(_WINDOWS) || defined(_WIN32)
#include "windows.h"
#else
#include <stdio.h>
#endif

#if defined(_WinIO)
#include "winio.h"
#else
#include <stdio.h>
#endif
#if !defined(UNIX)
#include <io.h>
#endif
#include <stdlib.h>
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
#include <dos.h>
#include <conio.h>	// 7.4.22 BJY
#endif
#include <assert.h>  // BDW
#include <time.h>	// 7.4.22 BJY 
#include <fcntl.h>

#if defined(UNIX)
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#define stricmp strcasecmp
#endif

#include "cc.h"
#include "c8type.h"
#include "keybrd.h"

/* Defines */
#define GET_CHAR_BUFF_SIZE		100

/* Static variables */
static filenametype last_outname = {'\0'};

#define NMDEV 20					  /* 4-Nov-91  number of valid device names */
static char *device[NMDEV] =	  /* 23-oct-85 valid device names */
    {									  /* 4-Nov-91 Added COM4 */
        "CON",  "CON:",
        "PRN",  "PRN:",
        "AUX",  "AUX:",
        "LPT1", "LPT1:",
        "LPT2", "LPT2:",
        "COM1", "COM1:",
        "COM2", "COM2:",
        "COM3", "COM3:",
        "COM4", "COM4:",
        "NUL",  "NUL:"
    };

/* Prototypes for static functions in this module */
static int cmdfile(char *);
static long fsize(char *);
static char *prompt_for( char *, bool, bool );

int IsValidDevice(char * filenm)
{
    int namelen;
    int i;
    int okdev;

    namelen= strlen(filenm);

    okdev = FALSE;
    for (i = 0; i < NMDEV && !okdev; ++i)
        if (stricmp(filenm, device[i]) == 0)
            okdev = TRUE;
    return okdev;
}


/****************************************************************************/
void sysinit(argc)	 /* Initialization routine  (system-dependent routine) */
/****************************************************************************/
int argc;	/* Argc from main */

{
#if !defined(_WINDOWS) && !defined(UNIX)
    NOREF(argc);
    /* Allow ^C at any dos call */
    org_break_flag = break_flag(TRUE);
#endif	
} /* End--sysinit */

/****************************************************************************/
void syscleanup()				/* Cleanup routine  (system-dependent routine) */
/****************************************************************************/

{
    if (outfile)	// 7.4.17 BJY
        fclose(outfile);
    if( writefile )
        fclose(writefile);

} /* End--syscleanup */

#ifndef _WINDOWS
/************************************************************************/
int get_char()			/* Read a character from the keyboard */
/************************************************************************/
{
    return( getc(keyboard) );
}

/************************************************************************/
char *
get_string(char* buff)	/* Read a string of characters from the keyboard */
/************************************************************************/
{
    register char *bp;	/* Buffer pointer */
    register int ch;		/* A character to read */
    register int count = 0;	/* count of characters read */	// 7.4.18 BJY

    bp = buff;		/* Point to start of buffer */
    while ( (ch = get_char()) != '\n' 		/* Read up to a newline */
            && count++ < 80)  // 7.4.18 BJY  don't overwrite buffer
        *bp++ = (char) ch;						/* storing as we go */
    *bp = '\0';					/* Terminate the string */
    return(buff);				/* Return the buffer */
}
#endif

/****************************************************************************/
static int cmdfile(char *name)
/****************************************************************************/

/* Description: This procedure processes a file containing command-line
 *				commands.  The file may be in any order and may contain
 *				zero or more commands per line.	It will ultimately produce a
 *				look alike for argc and argv so that those procedures set up
 *				to use command line arguments need never know the difference.
 *
 * Return values: A return from this procedure indicates success.
 *
 * Globals input: argvec -- global substitute for argv
 *						argcnt -- global substitute for argc
 *
 * Globals output: argvec -- points to a look alike vector built up from the
 *									  command file
 *						 argcnt -- set to one more than the number of valid elements 
 *									  in argvec
 *
 * Error conditions: If the requested file is not found, an error exit will be
 *								made out to the operating system.
 *
 * Other procedures called: none
 *
 */

{
    register char *p,			/* pointer to the buffer allocated for a string */
    *q;		 /* pointer into input buffer used while reading */
    FILE *linefile;
    char buf[81];				/* input buffer */
    int current_string;		/* length of most recently size-up string */
    unsigned total_allocation;  /* # of bytes to allocate in the malloc call */

    total_allocation = (unsigned) sizeof(char *);	/* Initial requirement */
    total_allocation *= 2;					/* (argv[0] and NULL final argument) */

    if ( (linefile = fopen(name, "r")) != NULL )
    {																		/* Successful open */
        for (argcnt = 1; !feof(linefile); )
        {
            /* Skip white space */
            for (q = buf; !feof(linefile) && isspace(*q = (char)getc(linefile));)
                ;

            if (!feof(linefile))                            /* get the string  */
                for (*++q = (char)getc(linefile); !feof(linefile) && !isspace(*q); )
                    *++q = (char)getc(linefile);

            *q = '\0';			/* NULL terminate it so strlen will be happy */

            if ( (current_string = strlen(buf)) != 0 )
            {												/* Allow for string */
                total_allocation += (unsigned) (current_string + 1);
                /* Allow for argvec element */
                total_allocation += (unsigned) sizeof(char *);

                argcnt++;			/* Update count of valid arguments */
            }

        } /* End--for */

        fclose(linefile);									/* End of sizing pass */

        /* Start of copying pass */

        if ( (linefile = fopen(name, "r")) == NULL )
        {
            msg_printf("\nUnable to reopen %s\n", name);
            bailout(BADEXIT, FALSE);
        }

        /* Allocate memory */
        if ( (argvec = (char **) malloc( total_allocation )) == NULL )
        {
            msg_printf("\nUnable to allocate memory for list file\n");
            bailout(BADEXIT, FALSE);
        }

        *argvec = nullname;				/* First and last elements of argvec */
        argvec[argcnt] = NULL;

        p = (char *) &argvec[argcnt + 1];		/* beginning of string space */

        for ( argcnt = 1; !feof(linefile); )
        {
            /* Skip white space */
            for (q = buf; !feof(linefile) && isspace(*q = (char)getc(linefile));)
                ;

            if (!feof(linefile))                            /* get the string  */
                for (*++q = (char)getc(linefile); !feof(linefile) && !isspace(*q); )
                    *++q = (char)getc(linefile);

            *q = '\0';			/* NULL terminate it so strlen will be happy */

            if ( (current_string = strlen(buf)) != 0 )			/* If non-empty string */
            {
                argvec[argcnt++] = p;						/* set next arg pntr */

                strcpy(p, buf);								/* copy the string */

                p += current_string + 1;					/* bump pointer */
            }
        } /* End--for */

        fclose(linefile);				/* End of allocation pass */
    } /* End--if (successful open) */
    else
    {
        msg_printf("%s not found\n", name);
        bailout(BADEXIT, FALSE);
    }

    return(0);
}

/****************************************************************************/
void cmdlnparse(argc, argv)							  /* Parse the command line */
/****************************************************************************/
int argc;				  /* argc from main */
register char	*argv[];		/* argv from main */

/*
 * Description:
 *						Parse the command line and handle any options from it.
 *					Set up the globals argcnt and argvec, which are functional
 *					replacements for argc and argv, respectively.
 *
 * Return values: none
 *
 * Globals input: 
 *						nullname -- array containing just a \0
 *						argcnt -- global copy of argc
 *						argvec -- global copy of argv
 *						outarg -- pointer to name of output file from command line
 *                writearg -- pointer to name for write and wrstore output
 *                            from command line
 *						tblarg -- pointer to name of change table from command line
 *						fntarg -- pointer to name of font file
 *						inparg -- if input file(s) specified on the command line,
 *										index into argv of current input file
 *
 * Globals output: cmdlni -- set according to what we found
 *						 nullname -- first element set to \0
 *						 argcnt, argvec -- contain argc and argv, respectively
 *						 outarg -- updated
 *                 writearg -- updated
 *						 tblarg -- updated
 *						 fntarg -- updated
 *						 screen_out -- set TRUE if output to "screen:"
 *						 inparg -- updated
 *
 * Error conditions: if an invalid option is specified or a filename is left
 *							  out, an error message and a "usage..." message will be
 *							  displayed on the screen.
 *
 * Other procedures called: cmdfile -- process the -i option
 *									 usage -- display a "Usage:" message and exit
 *													to the operating system
 *
 *
 ************************** NOTE RE: STDIN and STDOUT ****************
 * Prior to version 7.4g, user could specify stdin and/or stdout by 
 * saying that the input or output file name was - .  I have taken
 * this out for now, but if it has to be added again look first in this
 * routine.  I THINK that if you can get cmdlnparse to accept the -, and 
 * store it as outarg or argvec[inparg], all the other routines will
 * cooperate.  That is, openinfile and openoutfile still recognize - as
 * stdin or stdout, respectively.
 *						BLR 24-Sep-91
 *
 */

{
    int base;		  /* Index into argv */
    int line_switch; /* Switch being decoded from command line */

    quiet_flag = bAppendFlag = FALSE;         /* quiet and append is off */
    noiseless = FALSE;        // 7.4.22 BJY
    percent_print = FALSE;    // 8.0 default is do not print percent complete
    usestdin = FALSE;
    usestdout = FALSE;
    nullname[0] = '\0';								/* Initialze nullname */

    if (argc > 1 && *argv[1] == '?')
    {
        banner();                           // 7.4.05 ALB
        usage();
    }											/* Help option */

    argcnt = (char)argc;								/* Make argc, argv global */
    argvec = argv;

    base = 1;
    /* Set up for decoding loop */
    outarg = writearg = tblarg = nullname; // 7.4r0 ALB remove fntarg

    while (base < argcnt && *argvec[base] == '-')
    {
        if (argvec[base][2])	// 7.4.26 BJY  switch letter should be null terminated
        {
            banner();
            msg_printf("Unrecognized switch %s\n", argvec[base]);
            usage();
        }
        switch (line_switch = tolower(argvec[base][1]))
        {
        case 't':													/* CC table */
            if ( tblarg == nullname )
                tblarg = argvec[++base];
            else
                base++;
            break;
            // NOTE: Took out the -s (compile CC Table) option 3/96 (CC version 8.0)
            //       Not really needed anymore to save a little space, and would have
            //       incompatibilities with binary and CTRLZ in old compiled tables
        case 'o':					  				/* Output file name */
            if ( outarg == nullname )
                outarg = argvec[++base];
            else
                base++;
            break;
        case 'w':					  				/* Write output file name */
            if ( writearg == nullname )
                writearg = argvec[++base];
            else
                base++;
            break;
        case 'i':									/* Command input from a file */
            if ( argvec == argv )
            {
                base = cmdfile(argvec[++base]);
            }
            else
            {
                banner();      // 7.4.05 ALB
                msg_printf("\nIllegal nesting of input files %s ignored\n\n", argvec[++base]);
            }
            break;

        case 'q':				 	// quiet - don't ask about overwriting
            //	output file if it exists, just delete it.
            quiet_flag = TRUE;
            break;
        case 'u':				 	// Enable UTF-8 Encoding
            utf8encoding = TRUE;
            break;

        case 'a':				 	// append - don't ask about overwriting
            //	but append to output file if it exists
            bAppendFlag = TRUE;
            break;

        case 'n':					// 7.4.22 BJY  Added noiseless switch
            noiseless = TRUE;
            break;

        case 'p':               // 8.0 DRC  Added percent_print switch
            percent_print = TRUE;
            break;

        default:															/* Invalid option */
            banner();               // 7.4.05 ALB
            msg_printf("Unrecognized switch -%c\n", line_switch);
            usage();
        } /* End--switch */
        ++base;
    } /* End--while */

    if (base > argcnt)
    {
        banner();                  // 7.4.05 ALB
        msg_puts("Missing argument\n");			/* Filename left out */
        usage();
    }

    argvec[argcnt] = nullname;

    inparg = (char)base;				/* Index to next un-processed argument */

    /* Check to see if we have all required files */
    if ( tblarg == nullname )
    {
        banner();                   // 7.4.05 ALB
#ifdef NO_QUESTIONS			
        msg_puts("\nNo table file specified.\n\n");
        usage();
#else
        strcpy( ccnamebuf, prompt_for( "Changes file", TRUE, TRUE ) );
        tblarg = ccnamebuf;
#endif
    }

#ifndef NO_QUESTIONS			
    prompt_for_input = FALSE;
#endif
    if ( inparg >= argcnt )
    {
#ifdef UNIX
	inparg--;	
	argvec[inparg]="-";
#else
#ifdef NO_QUESTIONS			
        banner();               // 7.4.05 ALB
        msg_puts("\nNo input file specified\n\n");
        usage();
#else
        prompt_for_input = TRUE;
#endif
#endif
    }
    if ( outarg == nullname )
    {
#ifdef UNIX
	outarg="-";
#else
        banner();                 // 7.4.05 ALB
#ifdef NO_QUESTIONS			
        msg_puts("\nNo output file specified\n\n");
        usage();
#else
        strcpy( outnamebuf, prompt_for( "Output file", FALSE, FALSE ) );
        outarg = outnamebuf;
#endif
#endif
    }

#if !defined(UNIX)
    upcase_str( outarg );      /* force output arg to upper case */
#endif

    if ( strchr( outarg, '*' ) || strchr( outarg, '?' ) )
        concat = FALSE;      /* if there are wildcard chars in the output
                          * spec, there will be one output file 
                          * per input file */
    else
        concat = TRUE;       /* otherwise, concatenate into one output file */

    if ( strchr( writearg, '*' ) || strchr( writearg, '?' ) )
    {
        banner();                   // 7.4.05 ALB
        msg_printf("\nFile name wildcard not permitted in -w filename\n\n");
        usage();
    }
} /* End--cmdlnparse */

/****************************************************************************/
void usage()         /* Display a "usage..." message and exit to the system */
/****************************************************************************/
{
    msg_puts("Usage: CC Switches Input_files\n\n");

    msg_puts("Switches are:\n");
    msg_puts("-t\tTable name - this is required\n");
    msg_puts("-q\tQuiet - don't ask, just delete and replace output file, if it exists\n");
    msg_puts("-n\tNoiseless - don't beep or display processing messages\n"); // 7.4.22 BJY
    msg_puts("-a\tAppend - append to output file, if it exists\n");
    msg_puts("-o\tOutput filename - may contain * or ? wildcards.\n");
    msg_puts("\tIf no output file is specified, the stdout is used.\n");
    msg_puts("-i\tInput list filename - list of input files taken from file\n");   // 7.4.23 BJY Change wording
    msg_puts("-p\tPercentage - print percentage complete while processing\n");
    msg_puts("-u\tEnable UTF-8 support\n");
    msg_puts("-w\tWrite command output filename - file output for \n\
             \t\twrite and wrstore commands\n\n");

    msg_puts("Input_files are input file names that may contain \
             * or ? wildcards\n\n");
    msg_puts("\tIf no input file is specified, the stdin is used.\n");

    bailout(BADEXIT, FALSE);					/* Bail out */

} /* End--usage */

#ifndef NO_QUESTIONS
/****************************************************************************/
char *prompt_for( char *prompt, bool must_exist, bool cc_table )
/****************************************************************************/

/* prompt_for() return a filename for a given task
 *
 * if must_exist is TRUE then loop until we have a valid filename
 * otherwise check if we want to append to a file (only dealing with
 * output file). With cc_table TRUE, append ".cct" and see if that
 * results in a valid file name.
 *
 * Returns: string containing valid filename.
 *
 * NOTE: that a pointer to a static area is returned that is
 * overwritten the next time prompt_for() is called. */

{
    static char buf[80+1];		  /* so we don't have to deal malloc()
    										* and free() */
    FILE *tmp;
    int done = FALSE;
    int okdev = FALSE;			  /* if outputfile valid MS-DOS device */
    int i;

    while( ! done )
    {
        msg_printf( "%s? ", prompt );
        get_string( buf );

        /* do not allow null input */

        if( buf[0] == '\0' )
        {
            msg_printf( "%s required.\n", prompt );
            continue;
        }

        if ( (tmp = fopen( buf, "r") ) != NULL )
        {
            fclose( tmp );
            if( must_exist )
                done = TRUE;
            else
            {
                /* Check to see if the name is a valid MS-DOS device */

                for (i = 0; i < NMDEV; ++i)
                    if ( stricmp(buf, device[i] ) == 0)
                    {
                        okdev = TRUE;
                        done = TRUE;
                        break;
                    }
#ifdef JUNK
                if ( ! okdev && ! bAppendFlag )
                {
                    msg_printf( "%s %s already exists. Replace it? [No] ", prompt, buf );
                    if( yes() )
                        done = TRUE;
                }
#else
                done = TRUE;
#endif						
            }
        }
        else
        {
            if ( strchr( buf, '*' ) || strchr( buf, '?' ) )
                done = TRUE;		  /* allow wildcard names */
            else if ( ! cc_table && buf[0] == '-' && buf[1] == 0 )
                msg_printf( "'-' not allowed for %s when being prompted.\n",
                            prompt); /* do not allow - for stdin or stdout */
            else if ( cc_table )	  /* Try an extension of .CCT */
            {
                if ( ( strchr(filenm, '.') == NULL )
                        || (strrchr( filenm, '.') == (strchr(filenm, DIRSEP) - 1)) )
                {
                    strcat( buf, ".CCT" );
                    if ( (tmp = fopen( buf, "r") ) != NULL )
                    {
                        fclose( tmp );
                        done = TRUE;
                    }
                }
                if ( ! done )
                    msg_printf( "%s %s not found.\n", prompt, buf );
            }
            else if ( must_exist )
                msg_printf( "%s %s not found.\n", prompt, buf );
            else
                done = TRUE;
        }
    }

    return buf;
}
#endif

/****************************************************************************/
void get_next_infile( )
/****************************************************************************/

/* Description:
 *		This routine is called when a new input file is needed.  It opens
 *		a new input file only if there is another input file to be processed 
 *		into the currently open output file.
 *		If the concat flag is false, then we are not concatenating, so there
 *		will be no more input files for this output file.  Otherwise, call
 *		get_next_name to get the next input file, and open it.
 *
 * Return values: none
 *		infile == NULL means that there are no more input files associated
 *					 with the current outfile.
 *
 * Globals input: next_inname and next-outname; last_outname; infile
 *
 * Globals output: next_inname and next-outname; infile
 *
 */
{
    char *inname;		/* pointer to next input file name */
    char **outnameptr;		/* pointer to next output file name string */
    filenametype outnamebuff;		/* buffer for next output file name */
    char *nbptr = outnamebuff;		/* for use where a char ** is needed */

    if ( !concat )		/* not concatenating - no more input files for
        							the current output file */
    {
        if (infile)
        {
            fclose( infile );
            infile = NULL;
        }
    }

    else		/* we are concatenating - see if there is another input file */
    {
        cpOutMode = "Writing";
        strcpy( outnamebuff, outarg );	/* get output spec from cmd line */
        outnameptr = &nbptr;
        inname = get_next_name( outnameptr );	/* get pair of file names */
        if ( !inname || !*inname )			/* no more files at all */
            infile = NULL;
        else			/* open the input file */
        {
            openinfile( inname );
            if (!noiseless)	// 7.4.21 BJY  Show for all files  7.4.22 noiseless
                // && ( strchr( argvec[inparg-1], '*') || strchr( argvec[inparg-1], '?' ) ) ) // 7.4.08 ALB test for wildcard
                msg_printf("Processing input file %s; %s to output file %s\n",
                           inname, cpOutMode, last_outname);
        }
    }

}

/****************************************************************************/
bool openfilepair(firsttime)		/* open input and output file */
bool firsttime;		/* TRUE first time this routine is called */
/****************************************************************************/

/* Description:
 *		Call get_first_name or get_next_name to get the next pair of file
 *		names to be opened.  If there are no more names, but there is
 *		another input argument from the command line, call get_first_name
 *		with that argument.  Now, if there are no more file names, we are
 *		done.  Otherwise, open the files.
 *		Explanatory note: This is NOT called when only a new input file is 
 *		needed when output is being concatenated into one output file
 *		 - get_next_infile() handles that.
 *
 * Return values:
 *		TRUE - files were opened; more to be processed
 *		FALSE - done processing.
 *
 * Globals input: infile, outfile, concat flag
 *
 * Globals output: infile, outfile, writefile, bFileErr
 *
 * Note: bFileErr can be set by get_first_name(),
 *		openinfile(), openwritefile(), or openoutfile()
 */

{
    char *inname;						/* input file name */
    char **outnameptr;				/* pointer to pointer to outnamebuff */
    filenametype outnamebuff;		/* buffer for output file name */
    char *nbptr = outnamebuff;		/* for use when char** is needed */

    inname = NULL;                       // ALB 7.4.13

    strcpy( outnamebuff, outarg );	/* get output spec from command line */
    outnameptr = &nbptr;

#ifndef NO_QUESTIONS              // ALB 7.4.11
    /* psm */
    if ( prompt_for_input )
    {
        if ( firsttime )
        {
            strcpy( innamebuf, prompt_for( "Input file", TRUE, FALSE ) );
            inname = get_first_name( TRUE, innamebuf,	outnameptr );
        }
        else	// 7.4.28 BJY
        {
            strcpy( outnamebuff, outarg ); /* get output spec again */	// begin 7.4.24 BJY
            if (!inname && (strchr(nbptr, '*') || strchr(nbptr, '?')))
            {
                prompt_for_input = FALSE;	// don't ask for more files if wildcards in output spec
                outnameptr = &nbptr;
                inname = get_next_name( outnameptr );	// get next file pair
            }
        }														// end 7.4.24 BJY
    }
    else
#endif
    {
        if ( firsttime )
            inname = get_first_name( TRUE, argvec[inparg++], outnameptr );
        else
            inname = get_next_name( outnameptr );
#ifdef _WINDOWS
        if ( !inname && inparg < argcnt ) //7.4.15 chgd to "if" from "while" loop
            //"while" apparently allowed skipping of invalid arguments
            //Now, invalid arg will set bFileErr & CC will exit
#else
        while ( !inname && inparg < argcnt ) /* no more input files */
#endif
        {						  /* try next input file spec */
            strcpy( outnamebuff, outarg ); /* get original output spec again */
            outnameptr = &nbptr;
            inname = get_first_name( TRUE, argvec[inparg++], outnameptr );
        }
    }

    if ( !inname )			/* if no input file (or if errs), we're done */
        return( FALSE );
    else						/* got an input file, so open it */
    {
        openinfile( inname );
        if ( firsttime )
        {
            openwritefile( writearg );    /* open write output file (if any) */
        }
        if ( firsttime || !concat )			/* new output file */
        {
            if ( outfile )								/* if there is an old outfile */
                fclose( outfile );						/* close old one */
            strcpy( last_outname, *outnameptr );/* remember name for next time */
            openoutfile( *outnameptr );			/* open new one */
            if ( strcmp( *outnameptr, last_outname ) != 0 )
            {	/* openoutfile changed the name */
                strcpy( outarg, *outnameptr );   /* copy override to outarg */
                strcpy( last_outname, *outnameptr );
            }
        }
#ifdef _WINDOWS

        if (bFileErr)   						// return now if err  7.4.15
            return FALSE;
#endif    
        if (!noiseless)	// 7.4.21 BJY show processing message for all files  7.4.22 BJY noiseless
            // && ( strchr( argvec[inparg-1], '*') || strchr( argvec[inparg-1], '?' ) ) ) // 7.4.08 ALB test for wildcard
            msg_printf("Processing input file %s; %s to output file %s\n",
                       inname, cpOutMode, last_outname);
    }

    return( TRUE );

}

#if defined(UNIX)
int filelength(int fileno)
{
	struct stat buf;
	fstat(fileno, &buf);
	
	return buf.st_size;
}
#endif


/****************************************************************************/
void openinfile( fname ) 			/* Open the input file */
char *fname;						/* input file name */
/****************************************************************************/

/* Description:
 *		Tries to open fname as an input file.  If it can't be opened, there
 *		is a problem because fname came from get_first_name or get_next_name,
 *		which got fname from DOS!
 *
 * Return values: none
 *
 * Globals input:
 *						argvec -- local copy of argv from main
 *						inparg -- if input file(s) specified on command line,
 *										index into argvec of current input file
 *						infile -- pointer to input file
 *						bEndofInputData -- boolean: TRUE == EOF on input file
 *
 * Globals output: filenm -- contains name of input file just opened
 *						 namelen -- updated
 *						 inparg -- if input file(s) specified on command line,
 *										 and not all have been used, incremented by one
 *						 infile -- updated
 *						 bEndofInputData -- cleared
 *						bFileErr -- set if unable to open file (FUNC ver)
 *
 * Error conditions: can't open fname
 *
 * Other procedures called:
 *
 */

{
    int len;

    len = strlen( fname ) ;
    if (fname[len - 1] == ':')			/* MS-DOS doesn't like colons */
        fname[len - 1] = '\0';			/*  on the end of device names */

#ifndef _WINDOWS                                                    //7.4.15
    if ( (len == 1) && (fname[0] == '-') )
        infile = stdin;
    else
#endif
        if (binary_mode)
            infile = fopen(fname, "rb");
        else
            infile = fopen(fname, "r");

    if (infile == NULL)	/* File not found */
    {
        msg_printf("Cannot open %s.\n", fname);
#if defined (_WINDOWS)												//7.4.15
        bFileErr = TRUE;
        return;
#else
        bailout( BADEXIT, FALSE );
#endif		
    }


    bEndofInputData = FALSE;									/* Clear EOF flag */

    infilelength= filelength(fileno(infile));

    infileposition= 0;

    nextstatusupdate= 0;

} /* End--openinfile */


/****************************************************************************/
bool openoutfile(fname )		/* Open output file */
char *fname;					/* name of output file */
/****************************************************************************/

/* Description:
 *						This procedure reads a file name and opens outfile with
 *					that name. 
 *					DOS ver first opens the name for input to see if it
 *					exists. If it does, it allows the person to force it or to
 *					give another name.
 *
 * Return values: 		TRUE if successful
 *						FALSE if errors occurred
 *
 * Globals input:
 *						outfile -- pointer to the output file
 *						outsize -- space available for the output file,
 *										 -1 indicates unlimited space
 *
 * Globals output:
 *						 outfile -- points to the output file or device
 *						 outsize -- updated
 *						bFileErr -- set if unable to open file (FUNC ver)
 *
 * Error conditions: if unable to open the requested file, an error message
 *							  will be displayed on the screen and the user will
 *							  be asked to enter a new name.
 *							if the requested file exists, the user will be given
 *							  a chance to either replace the file or enter a new
 *							  name.
 *
 * Other procedures called:
 *									 fsize -- how much space is available for the
 *													output file?
 *
 */

{
    int okdev;		 /* 23-oct-85	TRUE if outputfile valid MS-DOS device */
    int i;			/* Loop index */

    strcpy( filenm, fname );
#ifndef _WINDOWS
L1:
#endif
    namelen = strlen( filenm );

    /* Check to see if the name is a valid MS-DOS device */
    okdev = FALSE;					 /* 23-oct-85 */
    for (i = 0; i < NMDEV && !okdev; ++i)
        if (stricmp(filenm, device[i]) == 0)
        {
            okdev = TRUE;
            if (filenm[namelen - 1] == ':')
                filenm[namelen - 1] = '\0';
        }

    if (filenm[namelen - 1] != ':' && !okdev)
    {												 /* Must be a regular file */
        outsize = fsize(filenm);				/* How much space left on disk? */
        bOutFileExists = FALSE;

        outfile = fopen(filenm, "r");

        if (outfile != NULL)
        {
            fclose(outfile);
            bOutFileExists = TRUE;
            if ( !quiet_flag && !bAppendFlag )	/* if not quiet or append mode */
#if defined (_WINDOWS)
                ;     /* overwrite the existing output file */		//7.4.12
#else				
            {					/* ask user if OK to replace it */
                msg_printf("Output file %s already exists. Replace it? [No] ", filenm);
                if (!yes())
                {
                    getname("New output file name? ", filenm);
#if !defined(UNIX)
                    upcase_str( filenm );			/* force to upper case */
#endif
                    strcpy( fname, filenm );	/* copy the name for caller */
                    goto L1;
                }			/* end - if yes */
            }			/* end - if not quiet or append */
#endif
        }		/* end - if file exists */
    }		/* end - if not a device */
    else
        outsize = -1L;								  /* Must be a device */

    if ( strcmp(filenm, "-") == 0 )
    {
#if !defined(UNIX)
        outfile = msgfile;                              //7.4.12
        // if we use stdout for output file, and if we are using binary
        // mode then set stdout (output file) to binary mode
        if ( binary_mode )
            _setmode(_fileno(outfile), _O_BINARY);      // 7/1/96
#else
      outfile = stdout;
#endif
    }
    else
    {
        if( bOutFileExists && bAppendFlag )
        {
            if (binary_mode)
                outfile = fopen(filenm, "ab");
            else
                outfile = fopen(filenm, "a");
            cpOutMode = "Appending";
        }
        else
        {
            if (binary_mode)
                outfile = fopen(filenm, "wb");
            else
                outfile = fopen(filenm, "w");
            cpOutMode = "Writing";
        }
    }

    if ( outfile == NULL )
    {											  /* File not found */
        msg_printf("Cannot open %s\n", filenm);
#if defined (_WINDOWS)												//7.4.15
        bFileErr = TRUE;
        return FALSE;
#elif defined (NO_QUESTIONS)
        bailout( BADEXIT, FALSE );
#else
        getname("New output file name? ", filenm);
        strcpy(fname, filenm);
        goto L1;
#endif
    }

    return TRUE;													//7.4.12

} /* End--openoutfile */

/****************************************************************************/
void openwritefile( fname ) 	/* Open output file */
char *fname;					/* name of output file */
/****************************************************************************/

/* Description:
 *						This procedure reads a file name and opens write output
 *             file with that name. It first opens the name for input to see
 *					if it exists. If it does, it allows the person to force it or
 *             to give another name.
 *
 * Return values: none
 *
 * Globals input:
 *						writefile -- pointer to the output file
 *
 * Globals output:
 *						 writefile -- points to the output file or device
 *						bFileErr -- set if unable to open file (FUNC ver)
 *
 * Error conditions: if unable to open the requested file, an error message
 *							  will be displayed on the screen and the user will
 *							  be asked to enter a new name.
 *							if the requested file exists, the user will be given
 *							  a chance to either replace the file or enter a new
 *							  name.
 *
 * Other procedures called:
 *									 fsize -- how much space is available for the
 *													output file?
 *
 */

{
    int okdev;		/* TRUE if outputfile valid MS-DOS device */
    int i;			/* Loop index */

    if( !*fname )
        return;

    strcpy( filenm, fname );
#if !defined(UNIX)
    upcase_str( filenm );		/* force filename to upper case */
#endif
#ifndef _WINDOWS
L1:
#endif
    namelen = strlen( filenm );

    /* Check to see if the name is a valid MS-DOS device */
    okdev = FALSE;
    for (i = 0; i < NMDEV && !okdev; ++i)
        if (stricmp(filenm, device[i]) == 0)
        {
            okdev = TRUE;
            if (filenm[namelen - 1] == ':')
                filenm[namelen - 1] = '\0';
        }

    if (filenm[namelen - 1] != ':' && !okdev)
    {												 /* Must be a regular file */

        bWriteFileExists = FALSE;
        if ((writefile = fopen(filenm, "r")) != NULL)
        {
            fclose(writefile);
            bWriteFileExists = TRUE;
            if ( !quiet_flag && !bAppendFlag )	/* if not quiet or append mode */
#if defined (_WINDOWS)
                ;     /* overwrite the existing write file */  	//7.4.12
#else				
            {					/* ask user if OK to replace it */
                msg_printf("Write output file %s already exists. Replace it? [No] ", filenm);
                if (!yes())
                {
                    getname("New write output file name? ", filenm);
#if !defined(UNIX)
                    upcase_str( filenm );			/* force to upper case */
#endif
                    strcpy( fname, filenm );	/* copy the name for caller */
                    goto L1;
                }			/* end - if yes */
            }			/* end - if not quiet or append */
#endif				
        }		/* end - if file exists */
    }		/* end - if not a device */

    if ( strcmp(filenm, "-") == 0 )
#if defined(UNIX)
		writefile = stdout;
#else
        writefile = msgfile; 									//7.4.12
#endif
    else
    {
        if( bWriteFileExists && bAppendFlag )
        {
            if (binary_mode)
                writefile = fopen(filenm, "ab");
            else
                writefile = fopen(filenm, "a");
            cpWriteOutMode = "Appending";
        }
        else
        {
            if (binary_mode)
                writefile = fopen(filenm, "wb");
            else
                writefile = fopen(filenm, "w");
            cpWriteOutMode = "Writing";
        }
    }

    if ( writefile == NULL )
    {															  /* File not found */
        msg_printf("Cannot open write output file %s\n", filenm);
#if defined (_WINDOWS)												//7.4.15
        bFileErr = TRUE;
        return;
#elif defined (NO_QUESTIONS)
        bailout( BADEXIT, FALSE );
#else
        getname("New write output file name? ", filenm);
        goto L1;
#endif
    }

    msg_printf("%s write and wrstore output to file %s\n",
               cpWriteOutMode, filenm);

} /* End--openwritefile */

#ifndef _WINDOWS													//7.4.15
/****************************************************************************/
void getname( prompt, namebuff )
char *prompt;
char *namebuff;
/****************************************************************************/
/* Description:	Display prompt and read response into namebuff, stripping
 *		whitespace
 *
 * Return values: name in namebuff
 *
 * Globals input: none
 *
 * Globals output: none
 *
 * Other procedures called:
 *
 */
{
    char *p;
    register int len;

    for ( ; ; )
    {
        msg_puts( prompt );	  		/* prompt */
        get_string( namebuff );		/* read response */
        for ( p = namebuff; (*p && isspace(*p)); p++)		/* Skip white space */
            ;

        if ( *p )
        {
            if (p != namebuff)	// 7.4.19 BJY
            {
                len = strlen(p);		// 7.4.19 BJY
                bytcpy( namebuff, p, len );		/* Start line with non-space */
                namebuff[len] = '\0';	// 7.4.19 BJY  null terminate the adjusted string
            }
            return;
        }
    }

}
#endif

/****************************************************************************/
static long fsize( name )		 /* return file size in bytes */
/****************************************************************************/
char	*name;			 /* name of output file */
/*
 *  Requirements: There may be only one output file opened per device.
 *						A DOS call function 0x36 is done to get the number
 *						of free bytes left on the disk.
 *
 *  Side effects: none
 *
 *  Signals:		Bytes left on the disk.
 *
 *  Description:	Return bytes left on disk which is the free clusters
 *						multiplied by the sectors per cluster multiplied by the
 *						bytes per sector.
 */
{
#if !defined(UNIX)
#if defined(WIN32)
    DWORD SectorsPerCluster;	// address of sectors per cluster
    DWORD BytesPerSector;	// address of bytes per sector
    DWORD NumberOfFreeClusters;	// address of number of free clusters
    DWORD TotalNumberOfClusters; 	// address of total number of clusters

    GetDiskFreeSpace(
        name,	// address of root path
        &SectorsPerCluster,	// address of sectors per cluster
        &BytesPerSector,	// address of bytes per sector
        &NumberOfFreeClusters,	// address of number of free clusters
        &TotalNumberOfClusters 	// address of total number of clusters
    );

    return BytesPerSector * SectorsPerCluster * TotalNumberOfClusters;
#else
    struct diskfree_t x;	/* Structure for _dos_getdiskfree call */
    unsigned drive;		/* Drive number (0 for default, A=1, B=2, etc.) */
    /* Compute drive number according to drive letter given (if any) */
    if (name[1] == ':')
        drive = tolower( *name ) - 'a' + 1;
    else
        drive = 0;		/* No drive letter given, use 0 for default */

    _dos_getdiskfree(drive, &x);
    return (long) x.bytes_per_sector * (long) x.sectors_per_cluster *
           (long) x.avail_clusters;
#endif
#else
	return -1;
#endif

}

/****************************************************************************/
void beep( freq, duration)
int freq;		/* frequency in hertz of desired beep */
int duration;	/* desired length of beep in milliseconds */
/****************************************************************************/
/* Description:
 *		Emits a beep of the desired pitch and duration by pulsing the
 *		PC speaker with the timer chip.
 *
 * Return values:
 *		none
 *                        
 * Function added for 7.4.22 by BJY
 * Function change by DAR 1-Aug-95: Function used clock in a nontransportable
 * way. The ANSI definition of clock is clock() is that the value returned by
 * clock() must be divided by CLK_TCK (defined in TIME.H) to get the time in
 * seconds. For Microsoft C(++) CLK_TCK is 1000 so the value returned by clock()
 * is in milliseconds, so the function works. However, it is not guarenteed that
 * CLK_TCK will be 1000 for other compilers, or even for future versions of
 * Microsoft C. For Borland C, CLK_TCK is 18.2. Beep was modified to use the time
 * rather than clock. Resolution of time is only one second but since this is only called
 * by one routine to emit a beep for the next file, this should be sufficient. If CLK_TCK
 * is used, then the floating point library has to be included, bloating the code for
 * a function of dubious value anyways.
 */
{
#if !defined(WIN32) && !defined(UNIX)
    long goal;
    int timerval;


    if (!freq)
        return;

    timerval = (int)(1193181L/freq);	// value to stuff into timer chip

    outp( 0x43, 0xB6);			// prepare timer to receive frequency info

    outp( 0x42, timerval & 255);	// tell timer how fast to pulse the speaker
    outp( 0x42, timerval >> 8);

    outp( 0x61, inp(0x61) | 3);		// make sure timer is gated to speaker

    goal = time(NULL) * 1000 + (clock_t)duration;

    while (goal > time(NULL) * 1000)		// wait for specified time
        ;

    outp( 0x61, inp(0x61) & ~3);	// disconnect timer from speaker
#endif
}
/* END */
