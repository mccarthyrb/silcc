/**************************************************************************/
/*								   FILEWC.C 							  */
/**************************************************************************/

/*
 *	FILEWC.C, version 1.0a, 10-Oct-91
 *	Copyright SIL Inc, 1991 
 */

/*
 *	This module contains the code for processing wildcarded input and
 *	output file specifications.  Global entry points are:
 *		get_first_name( )
 *		get_next_name( )
 */



/* Preprocessor directives */
#ifdef _WINDOWS 
#include <windows.h>
#endif	
#include <stdio.h>
#include <string.h>
#if !defined(UNIX)
#include <dos.h>
#endif
#include <stdlib.h> 
#include <errno.h>
#include <ctype.h>
#ifdef _WINDOWS
#include "ccw\ccwin.h"
#include "filewc.h"
#else
#include "msgput.h"
#include "cc.h"   
#endif
#if defined(UNIX)
#include "wtype.h"
#endif
#if defined(WIN32)
#include <io.h>
#define find_t _finddata_t
#endif

#if !defined(UNIX)
typedef int  (__cdecl fcmp)(const void	*, const void  *);
#else
typedef int  (fcmp)(const void	*, const void  *);
#define strcmpi strcasecmp
#endif

/* Function prototypes - external functions */
char *get_first_name( int sort_switch, char *inp, char **oup );
char *get_next_name( char **oup );

/* Function prototypes - static functions */
static int get_filenames ( char inp[] );
static int valid_wildcard_pair( char *, char * );
static void sort_filenames( void );
static void pad_file_names( char (*pad_arr_pnt)[ARRAY_SIZE_2] );
static int comp_strings(const int *, const int * );
static void *safalloc( unsigned size );
static char *split_path( char *, char * );
static void create_outfile_name( char *out_spec, char *in_spec,
                                 char *infile, char *out_mod );
static void wildcard_fill( char *out_org, char *mod, char *in_org, char *in );
static int wildcard_names_ok( char *inspec, char *outspec);
#ifdef _WINDOWS
static void upcase_str( char * );	/* convert string to upper case */
#endif

char *get_first_name( int sort_switch, char *inp, char **oup )
/* --------------------------------------------------------------------- */
/* Arguments
 * *inp - a string with full path specification of file.ext with possible
 *			* or ? wild cards in file.ext
 * sort_switch - indicates whether or not directory ordered file processing was
 *		specified (versus lexicographical processing)
 * **oup - pointer to a string containing path name of output file, with
 *			possible * or ? wild cards.
 *
 * Results
 *	**oup - Returned as a pointer to a valid output file.ext, 
 *			with * or ? wild cards expanded appropriately.
 * return value - pointer to actual input file to be processed by CC
 *					NULL if no filename could be extracted (error)
 *
 * Global variables referenced or modified -  
 *		fn_arr_pnt, ind_incr, index, inpath, outpath, inpath_end, outpath_end,
 *		files_read, bFileErr
 *
 * Description
 *		Calls get_filenames to get the set of files which match the input 
 *		file spec. passed in.  Calls sort_filenames to sort them, if 
 *		sort_switch is set.  Calls create_outfile_name to make up the name 
 *		of the first output file.  Copies first input and output file names
 *		to inpath and outpath, sets appropriate pointers to inpath and outpath.
 *
 * Programmer - Rod Davis, JAARS
 *			Modifications by Beth Reitz, JAARS, 20-Sep-91
 */
{
    int x = 0; /* Increment variables */
#ifndef _WINDOWS
    MSG_STRUCT_S_S *structss;	 // parms for error routine
#endif
    

#ifndef _WINDOWS 
#if !defined(UNIX)
    if (!strcmpi(inp, *oup) && !IsValidDevice(inp))
#else
    if (!strcmpi(inp, *oup))
#endif
    {
        if (*inp != '-') {
          Process_msg(errorFunction, 21, 0, 0);
          return(NULL);
        }
    }
#else
    if (!strcmpi(inp, *oup))
    {
        MessageBox(NULL, "Input and output files must be different!",
                    "Consistent Changes", MB_ICONHAND);
        return(NULL);
    }
#endif

    /* if there are wildcard characters in the output specification,
    * check to see if wildcard specifications match between input and
    * output files
     */
    if ( ( strchr( *oup, '*') || strchr( *oup, '?' ) )
            && valid_wildcard_pair( inp, *oup ) == 0 ) 	   // 7.4.03 ALB Added
    {
#ifndef _WINDOWS 
        Msg_s_s.string1 = inp;
        Msg_s_s.string2 = *oup;
        structss = &Msg_s_s;
        Process_msg(errorFunction, 22, 0, (long unsigned) structss);
#else
        sprintf(msgBuffer, "Wildcard mismatch between input \"%s\" and output \"%s\"\n", inp, *oup);
        MessageBox(NULL, msgBuffer, "Consistent Changes", MB_ICONHAND);
#endif
        return( NULL );
    }

    /* Allocate memory for array of pointers to file names */
    fn_arr_pnt = safalloc( sizeof(char) * NUMBER_FILES * ARRAY_SIZE );

    /*	Get files from directory */
    if (!get_filenames( inp ) )
    {
        //get_filenames produced one of several error messages
        return(NULL);
    }

    /* Allocate array of indices into array allocated for filenames above.	*/
    /* The number of indices will be the same as the number of files_read	*/
    /* returned in the call to get_filenames above.							*/
    gpindex = (int *) safalloc( (files_read) * sizeof(int) );

    for (x = 0; x < files_read; x++)
        gpindex[x] = x;

    /* Sort files lexicographically if sort_switch is set */
    if (sort_switch)
        sort_filenames( );

#if !defined(WIN32) && !defined(UNIX)
    /* make sure both input and output file specs are in upper case, so
    that comparisons will not fail based on case alone. Relevant for Win16
    only since Win32 supports upper and lower case filenames */

    upcase_str( inp );			 /* make input file spec uppercase */
    upcase_str( *oup ); 		/* make output file spec uppercase */
#endif

    /* find beginning of file names at end of path specs. */
    inpath_end = split_path( inpath, inp );
    outpath_end = split_path( outpath, *oup );

    if (outpath_end != outpath && !**oup)	// 7.4.20 BJY
    {									// Output name can't end with backslash
#ifndef _WINDOWS 
        msg_printf( "Output file specification \"%s\" is incomplete\n", outpath);
#else
        sprintf(msgBuffer, "Output file specification \"%s\" is incomplete\n", outpath);
        MessageBox(NULL, msgBuffer, "Consistent Changes", MB_ICONHAND);
#endif
        return( NULL );
    }

    /* Set gpindex[ind_incr] to point to first sorted input filename */
    ind_incr = 0;

#if !defined(WIN32) && !defined(UNIX)
    /* This checks for 8.3 filenames. This is only relevant for 16 bit apps */
    if (!wildcard_names_ok( inp, *oup)) 	// 7.4.25 BJY
        return(NULL);
#endif

    /* create first output file name */
    create_outfile_name( *oup, inp, fn_arr_pnt[ gpindex[ ind_incr ] ],
                         outpath_end );
    *oup = outpath; 	/* set up for return */
    strcpy( inpath, fn_arr_pnt[ gpindex[ind_incr] ] ); /* copy to inpath */
    return( inpath );

} /* End of function get_first_name */


int wildcard_names_ok( char *inspec, char *outspec)
/* --------------------------------------------------------------------- */
/* Arguments
 * *inspec - pointer to a string containing original input file specification
 * *outspec - pointer to a string containing original output file spec
 *
 * return value - TRUE if wild card expansion on all files produces filenames
 *					within the DOS limits. (8.3)  FALSE if an output filename
 *					is too long.
 *
 * Global variables referenced - fn_arr_pnt, gpindex
 *
 * Description
 *		If both input and output file specifications have a *, then see if
 *		  expansion of the wildcards into the output filenames generated will
 *		  produce filenames longer then 8 by 3.
 *
 * Programmer - Brian Yoder, JAARS	Added for 7.4.25
 */
{
    int i, error;
    char *p, fname[20];
#ifndef _WINDOWS 
    MSG_STRUCT_S_S *structss;	 // parms for error routine
#endif
    error = FALSE;

    if (!(strchr(inspec, '*') && strchr(outspec, '*'))) // input and output specs have stars
        return TRUE;

    for (i=0; i<files_read; i++)
    {
        create_outfile_name( outspec, inspec, fn_arr_pnt[gpindex[i]], fname);
        p = strchr(fname, '.'); 	// does name have an extension?
        if (p)
        {
            if ((p-fname > 8))		// base name longer than 8 chars?
                error = TRUE;
            else if (strlen(p+1) > 3)	// extension longer than 3 chars?
                error = TRUE;
        }
        else if (strlen(fname) > 8) // no extension, name longer than 8 chars?
            error = TRUE;
        if (error)
        {
#ifndef _WINDOWS   
            Msg_s_s.string1 = fn_arr_pnt[gpindex[i]];
            Msg_s_s.string2 = outspec;
            structss = &Msg_s_s;
            Process_msg(errorFunction, 24, 0, (long unsigned) structss);
#else
            sprintf(msgBuffer, "Input filename \"%s\" too long for available characters in output filename \"%s\"\n",
                    fn_arr_pnt[gpindex[i]], outspec);
            MessageBox(NULL, msgBuffer, "Consistent Changes", MB_ICONHAND);
#endif
            return FALSE;
        }
    }
    return TRUE;
}

int valid_wildcard_pair( char *in_pat, char *out_pat )
/* --------------------------------------------------------------------- */
/* Arguments:
 * *in_pat, - pointer to pattern string specifying the input files
 * *out_pat - pointer to pattern string specifying the output files
 *
 * Results:
 * return value - TRUE - the pair can be used by wildcard
 *				  routines FALSE - the pair are incompatible for the
 *				  wildcard routines. (i.e. the routines will bomb or
 *				  there will be an overlap in the input and output
 *				  files)
 *
 * Global variables referenced or modified: None.
 *
 * Description:
 *
 *		Determine if the following pair can be used for valid wildcard
 *		operations. Checks used are 1) if wildcard type character
 *		match (? or *) 2) if the positions of the wildcard characters
 *		are the same 3) that no other characters follow a star other
 *		than the filename/file extension separator or end of filename
 *		(one of those DOS gotchas) 4) if there is a difference between
 *		the input specification and output specification, so that
 *		there will be no mapping of input file to output file by
 *		wildcard_fill().
 *
 *		Added extra allowance for the position of star may be
 *		different for output file specification.
 *
 * Programmer: Peter Mielke */


{
    char *in_sav = in_pat;		// 7.4.28 BJY
    char *out_sav = out_pat;
    char *p;					// 7.4.27 BJY
    char in_flag, out_flag; 	// 7.4.25 BJY
    int valid = 1;				  /* innocent until proven guilty */
    int i;

    in_flag = out_flag = 0; 	// 7.4.25 BJY

    if ((p=strrchr(in_pat,DIRSEP)) || (p=strchr(in_pat,':'))) // 7.4.27 BJY Don't include pathnames in evaluation
        in_pat = p+1;

    if ((p=strrchr(out_pat,DIRSEP)) || (p=strchr(out_pat,':')))	// 7.4.27 BJY
        out_pat = p+1;

    /* treat filename and extension separately */

    for( i = 0; valid && i < 2 && ( *in_pat || *out_pat ); i++ ) {

        /* while we have not come to the end of the patterns (null or
         * extension separator) */

        while ( valid && ( ( *in_pat != 0 && *in_pat != '.' )
                           || ( *out_pat != '.' && *out_pat != 0 ) ) ) {

            /* these defines are used to reduce the amount of code during
             * character type comparison */

#define QUEST 1
#define STAR 2
#define NORM 4
#define CROSS(a,b) ((a << 3) | b)
#define TYPE(a) ( a == '?' ? QUEST : a == '*' ? STAR : NORM )

            /* compare the input and output characater types */

            switch ( CROSS( TYPE( *in_pat ), TYPE( *out_pat ) ) ) {
            case CROSS( QUEST, NORM ):
                        case CROSS( STAR, QUEST ):
                            case CROSS( QUEST, STAR ):
                                    valid = 0;		  /* these relationships result in a loss of
                							* information in the transformation */
                break;
            case CROSS( NORM, NORM ):
                            break;
            case CROSS( STAR, STAR ):
                            /* check for DOS gotcha */
                            out_pat++; in_pat++;
                if ( *out_pat != '.' && *out_pat )
                    valid = 0;
                else if ( *in_pat != '.' && *in_pat )
                    valid = 0;
                in_flag |= i+1; 	// 7.4.25 BJY
                out_flag |= i+1;	// 7.4.25
                break;

            case CROSS( STAR, NORM ):	// begin 7.4.25 BJY

                            if ( *(in_pat+1) != '.' && *(in_pat+1) )	// 7.4.28 BJY  '*' must be at end of name or ext
                                valid = 0;
                /* if we are not at the end of the output pattern go over
                * the star again for the next character in the output
                * pattern */

                if ( *out_pat != 0 && *out_pat != '.' )
                    in_pat--;
                in_flag |= i+1;
                break;					// end 7.4.25 BJY

            case CROSS( NORM, STAR ):
                            if ( *(out_pat+1) != '.' && *(out_pat+1) )	// 7.4.28 BJY  '*' must be at end of name or ext
                                valid = 0;

                /* if we are not at the end of the input pattern go over
                * the star again for the next character in the input
                * pattern */

                if ( *in_pat != 0 && *in_pat != '.' )
                    out_pat--;
                out_flag |= i+1;		// 7.4.25 BJY

                /* all other patterns are allowed and fall through */
            }

            if( (in_pat < in_sav) || (*in_pat && *in_pat != '.') ) in_pat++;
            if( (out_pat < out_sav) || (*out_pat && *out_pat != '.') ) out_pat++;
        }

        /* skip over file extention separator (if it exists) */

        if( *in_pat ) in_pat++;
        if( *out_pat ) out_pat++;
    }

    if (in_flag && out_flag && ((!(in_flag & out_flag)) // 7.4.25 BJY
                                || (in_flag > out_flag)))			// 7.4.29 BJY
        return 0;		// don't allow bad * combination e.g. out.* in*.txt



    /* one final override: if there have been no difference
    * encountered during our scan the pair is rejected as an input
    * match will be not changed.
     */

    return valid;
}

char *get_next_name( char **oup )
/* --------------------------------------------------------------------- */
/* Arguments
 * **oup - pointer to a string containing path name of output file, with
 *			possible * or ? wild cards.
 *
 * Results
 *	**oup - Returned as a pointer to a valid output file.ext, 
 *			with * or ? wild cards expanded appropriately.
 * return value - pointer to actual input file to be processed by CC;
 *						NULL when no more to do.
 *
 * Global variables referenced or modified -  fn_arr_pnt, ind_incr, gpindex
 *			inpath, outpath
 *
 * Description
 *		Increment ind_incr to index next input file.  Create a new output
 *		file name to correspond to this input file.
 *
 * Programmer - Rod Davis, JAARS
 *		Modifications by Beth Reitz, JAARS, Sept, 1991
 */
{
#ifndef _WINDOWS
    char* tmp;	// 7.4.06 ALB
#endif


    ind_incr++; 		/* index to next input file name */
    if ( ind_incr < files_read )		/* if more to do */
    {
        if ( strchr( *oup, '*') || strchr( *oup, '?' ) )
        {		/* if any wildcards in output file name */
            create_outfile_name( NULL, NULL, fn_arr_pnt[ gpindex[ind_incr] ],
                                 outpath_end ); 	/* create new output file name */
            *oup = outpath; 				/* set pointer for return */
        }
        /* copy input file name to inpath */
        strcpy( inpath_end, fn_arr_pnt[ gpindex[ind_incr] ] );
        return( inpath );
    }

    /* no more to do */
    if ( fn_arr_pnt )			/* if not already freed */
    {
        free( fn_arr_pnt ); 	/* free allocated spaces */
        free(gpindex);
        fn_arr_pnt = NULL;
    }

#ifndef _WINDOWS   // this does not apply in Windows
    /* psm */ 		// 7.4.06 ALB Put in Peter's code
    /* well, we may not be done just yet. if there are still things on
     * the command line go through them as well
     */
    while( inparg < argcnt )
    {
        tmp = get_first_name( TRUE, argvec[inparg++], oup );
        if( tmp )
            return( tmp );
    }
#endif	  // _WINDOWS

#ifndef NO_QUESTIONS
    /* psm */
    /* then again we still may not be done. This is if we have not
     * given any filenames on the command line (for those that like the
     * old style of prompting) */

#ifndef _WINDOWS			  // turn this off, do not prompt in windows
    if ( prompt_for_input )
    {
        FILE *tmpfp;

        for (;;)
        {
            if (!noiseless) // 7.4.22 BJY
                beep(800,100);

            msg_printf( "Next Input file (<RETURN> if no more)? " );
            get_string( innamebuf );

            if ( !*innamebuf )
                return NULL;

            if ( ( tmpfp = fopen( innamebuf, "r" ) ) != NULL )
            {
                fclose( tmpfp );

                return get_first_name( TRUE, innamebuf, oup );
            }

            /* see if it is a wildcard */
            else if ( strchr( innamebuf, '*' ) || strchr( innamebuf, '?' ) )
            {
                tmp = get_first_name( TRUE, innamebuf, oup );
                if ( tmp )
                    return tmp;
            }

            msg_printf( "%s not found.\n", innamebuf );
        }
    }
#endif	  //  _WINDOWS
#endif	  //  NO_QUESTIONS

    return( NULL ); 			/* indicate done */

} /* End of function get_next_name */


/* --------------------------------------------------------------------- */
int get_filenames( char inp[] )
/* Arguments
 *		inp - file spec (with possible wild cards), used to get a list of
 *				all matching files from the indicated directory.
 *
 * Results
 *		returns TRUE for success, FALSE for failure
 *
 * Global variables referenced or modified - fn_arr_pnt, files_read
 *
 * Description
 *		Calls _dos_findfirst and _dos_findnext to get appropriate files.
 *
 * Programmer - Rod Davis, JAARS
 */
{
#if !defined(UNIX)
#if defined(WIN32)
    HANDLE hFile;
    WIN32_FIND_DATA filedata;
#else
    struct find_t filedata;  /* Structure of type find_t */
#endif


    files_read = 0;

    /* get first name from DOS */

#if defined(WIN32)

    if ((hFile= FindFirstFile(inp, &filedata)) == INVALID_HANDLE_VALUE )
    {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
        {

#else
    if (_dos_findfirst(inp, _A_NORMAL, &filedata) != 0 )
    {
        if (errno == ENOENT)
        {
#endif
#ifndef _WINDOWS	
            Process_msg(errorFunction, 27, 0, (long unsigned) inp);
#else
            sprintf(msgBuffer, "CC input file  %s  does not exist in specified path!\n", inp);
            MessageBox(NULL, msgBuffer, "Consistent Changes", MB_ICONHAND);
#endif
            return(0);
        }
        else
            return(0);
    } /* EndIf */

#if defined(WIN32)

    strcpy( fn_arr_pnt[files_read++], filedata.cFileName ); /* copy to fn_arr_pnt */  // 7.4.28 BJY
    while ( FindNextFile(hFile, &filedata) )
#else
    strcpy( fn_arr_pnt[files_read++], filedata.name ); /* copy to fn_arr_pnt */  // 7.4.28 BJY

    while ( _dos_findnext(&filedata) == 0 )
#endif
    {											/* till no more file names */
        if (files_read < NUMBER_FILES)				/* if enough room */
        {
#if defined(WIN32)
            strcpy( fn_arr_pnt[files_read++], filedata.cFileName ); /* copy to fn_arr_pnt */  // 7.4.28 BJY
#else
            strcpy( fn_arr_pnt[files_read++], filedata.name ); /* copy to fn_arr_pnt */ // 7.4.28 BJY
#endif
        }
        else										/* array is full */
        {
#ifndef _WINDOWS	
            Process_msg(errorFunction, 28, 0,
                        (long unsigned) NUMBER_FILES);
#else
            sprintf(msgBuffer, "Total number of files must not exceed %ld\n",
                    NUMBER_FILES);
            MessageBox(NULL, msgBuffer, "Consistent Changes", MB_ICONHAND);
#endif
            return(0);
        }
    }

    return(1);				/* success */
#else
	files_read = 0;
	strcpy( fn_arr_pnt[files_read++], inp );
	return(1);				/* success */
#endif
} /* End of function get_filenames	*/


/* --------------------------------------------------------------------- */
void sort_filenames( void )
/* Arguments - none
 *
 * Results
 *		modifies index array 
 *
 * Global variables referenced or modified - index
 *
 * Description
 *		Creates pad_arr_pnt and calls pad_file_names to facilitate sorting
 *		numbered files.  Uses DOS qsort() to sort pad_arr_pnt.
 *
 * Programmer - Rod Davis, JAARS
 */
{
    pad_arr_pnt = safalloc( sizeof(char) * files_read * ARRAY_SIZE_2 );

    pad_file_names( pad_arr_pnt );

    qsort( (void **)gpindex, (size_t)files_read, (size_t)sizeof( gpindex[0] ),
           (fcmp *)comp_strings );

    free(pad_arr_pnt);

} /* End of function sort_filenames */


/* --------------------------------------------------------------------- */
int comp_strings( const int *index1, const int *index2 )
/* Arguments
 *		index1 and index2 - pointers to two ints, which in turn index two
 *			entries in pad_arr_pnt.
 *
 * Results
 *		Returns the result of comparing the two indicated entries:
 *			> 0 string at index1 is lexically greater than string at index2
 *			== 0  strings at index1 and index2 are the same
 *			< 0   string at index1 is lexically less than string at index2
 *
 * Global variables referenced or modified - None
 *
 * Description - calls strcmp to compare the indicated strings.
 *
 * Programmer - Rod Davis, JAARS
 */
{

    return(strcmp( pad_arr_pnt[*index1], pad_arr_pnt[*index2] ));

} /* End of function comp_strings */


/* DJE code follows */

char *pad_string(char *pfn, char **ppad, int z);

/* --------------------------------------------------------------------- */
void pad_file_names( char (*pad_arr_pnt)[ARRAY_SIZE_2] )

/* Arguments
 * char *fn_arr_pnt - array of unsorted DOS file names
 *
 * Results
 * char *pad_arr_pnt - result array of unsorted names padded with blanks to
 *					   the standard length of the primary and extension
 *					   field.  In addition, any trailing digits in a
 *					   primary or extension name are moved to the end
 *					   of the respective field.
 *
 * Global variables referenced or modified - None
 *
 * This subroutine takes the the file names in the array fn_arr_pnt and
 * first copies them into pad_arr_pnt array. In the process of copying it
 * pads both the primary and extension names out to 8 and 3 respectively
 * with blanks.
 *
 * Next each name is inspected and any trailing numerical digits are
 * moved out the end of the primary name or extension name field.
 *
 * The purpose of the subroutine is to set up the pad_arr_pnt array with names
 * such that a subsequent sort will have names such as MARK2.TXT sort
 * before MARK10.TXT.
 *
 * Programmer - Dan Edwards, JAARS, 25 Apr 91
 */
{
    int x;
    char *pfn, *ppad;

    for ( x = 0; x < files_read; x++ )	 /* Iterate thru file names */
    {
        ppad = pad_arr_pnt[x];	  /* ptr into padded name array */
        pfn = fn_arr_pnt[x];	  /* ptr into file name array */
        pfn = pad_string(pfn, &ppad, PRIMARY_LEN);	/* pad primary name */
        if(*pfn == '.')    /* 1st call shd leave *pfn pointing at . */
            pfn++;			/* step over the . */
        pfn = pad_string(pfn, &ppad, EXT_LEN);		/* pad extension */
    }
}

/* --------------------------------------------------------------------- */
char *pad_string(char *pfn, char **ppad, int z)
/* Arguments
 * char *pfn - pointer to DOS file name as NUL terminated ASCII string with
 *		  a . seperating the primary and extension names.
 * int z - number of characters in this field (8 for primary, 3 for ext)
 * Results
 * char * - ptr to the next unused character in the pfn string
 * char **ppad - location of the ptr into the padded string array
 *				 This result is returned to give the next unused char in
 *				 the padded result string.
 * Algorithm
 * 1. Copy z chars from pfn string to ppad string.	During the copy
 *	  keep track of the number of trailing numerical digits (if any).
 *	  Stop copying if a . or NUL is reached and pad the rest of the
 *	  ppad string with blanks.
 * 2. If there are any trailing numerical digits and blanks where used
 *	  used to pad the ppad stiring then start at the end of the ppad
 *	  string and move backwards until the first numerical digit in the
 *	  backwards direction is found.  Starting at that point continue
 *	  moving backwards thru the ppad string and swap the characters
 *	  at the end of the string with the numerical digits as they are
 *	  found.
 * Example - after 2 calls from the subroutine pad_file_names (above)
 *	  pfn name "MARK3.TXT" becomes "MARK   3.TXT"
 *			   "MARK.4"    becomes "MARK    .  4"
 *
 * Programmer - Dan Edwards, JAARS, 25 Apr 91
 */
{
    char *ppad1, *ppad2, *ppad3;
    int y, i, n;
    char ch;

    ppad1 = *ppad;				   /* copy of the argument (not really nec) */
    for(y = 0, n = 0; y < z; y++)  /* loop for length of field */
    {
        ch = *pfn++;			/* get next char and adv ptr */
        if(ch  == '.' || ch == '\0')
        {
            pfn--;				 /* Don't step past . or NUL */
            *ppad1++ = ' '; 	/* Pad string out with blanks */
            continue;
        }
        *ppad1++ = ch;	   /* copy ch in ppad array and adv ptr */
        if(isdigit(ch))
            n++;				 /* count trailing digits */
        else					 /* Here if non-digit is found */
            n = 0;			 /* reset trailing digit count on non-digit */
    }
    ppad2 = ppad3 = ppad1 - 1;	/* ptr to last char in field */
    if(n && *ppad2 == ' ')	   /* test for trailing digits fol by blank */
    {
        while(*(--ppad2) == ' ')
            ;			   /* step back over trailing spaces */
        for(i = 0; i < n; i++)	/* loop for number of trailing digits */
        {
            ch = *ppad2;	/* exchange char for trailing blanks */
            *ppad2-- = *ppad3;
            *ppad3-- = ch;	/* step each ptr backwards and repeat for */
        }				/* number of trailing digits */
    }
    *ppad = ppad1;		 /* return ptr to next unused char of padded string */
    return(pfn);		 /* return ptr to next unused char of pfn string */
}

/* end of DJE code */

/* --------------------------------------------------------------------- */
void create_outfile_name( char *out_spec, char *in_spec,
                          char *infile, char *out_mod )
/* Arguments
 *	out_spec = original output file spec, from user.
 *	in_spec  = original input file spec, from user.
 *	infile	 = expanded name of corresponding input file
 *	out_mod  = location for expanded output file name
 *
 * Results
 *	out_mod = expanded output file name
 *
 * Global variables referenced or modified - None
 *
 * Description
 *		If a user's input file and/or output file spec have been passed
 *	in, copy them to the appropriate local static buffer for future
 *	reference.	If they were not passed in, then assume they were set
 *	on a previous call, and use the contents of the buffers.
 *		Wildcard expansion is done first on the primary name and then
 *	on the extension.  This routine breaks the input file spec, output file
 *	spec and expanded input file name into primary name and extension, and
 *	calls wildcard_fill() to create the primary name and extension for the
 *	output file.
 *
 * Programmer - Beth Reitz, June 1991
 *
 */

{
    static char insp_buff[PATH_LEN]; /* local buff for original input spec */
    static char *in_org;			 /* pointer to the buffer */
    static char *in_org_ext;		 /* pointer to the extension */

    static char outsp_buff[PATH_LEN];  /* local buff for original output spec */
    static char *out_org;			/* pointer to the buffer */
    static char *out_org_ext;		/* pointer to the extension */

    char *in_ext;				/* pointer to input file name extension */
    char *mod_ext;				/* pointer to expanded file name extension */

    if ( in_spec )					/* if an input spec passed in */
    {
        in_org = &insp_buff[0]; 	/* init pointer to buffer */
        strcpy( in_org, in_spec );	/* copy spec to local buffer */
        in_org_ext = strchr( in_org, '.' ); /* find end of primary name */
        if ( in_org_ext ) 
	{
            *in_org_ext++ = '\0';			  /* make it null terminated */
	}
    }
    if ( out_spec ) 				/* if an output spec passed in */
    {
        out_org = &outsp_buff[0];	/* init pointer to buffer */
        strcpy( out_org, out_spec); /* copy spec to local buffer */
        out_org_ext = strchr( out_org, '.' );	/* find end of primary name */
        if ( out_org_ext ) 
	{
            *out_org_ext++ = '\0';				/* make it null terminated */
	}
    }

    in_ext = strrchr( infile, '.' ); 		/* find end of primary name */
    if ( in_ext && in_ext[1] != DIRSEP)
        *in_ext = '\0'; 					/* make it null terminated */
    else
        in_ext = NULL;

    wildcard_fill( out_org, out_mod, in_org, infile );	/* fill primary name */

    /* Primary name cannot be zero length.	 If it is now, copy corresponding
    	input file name. */

    if	( strlen( out_mod ) == 0 )
        out_mod = strcpy( out_mod, infile );

    /* Replace removed dots, then expand wildcards in the file extension */

    if ( in_ext )					/* if input file had an extension */
        *in_ext++ = '.';				/* replace dot removed above */
    if ( out_org_ext )					/* if original had an extension */
    {
        mod_ext = out_mod + strlen( out_mod );	/* find end of result */
        *mod_ext++ = '.';								/* put the dot in */

        /* fill extension */
        wildcard_fill( out_org_ext, mod_ext, in_org_ext, in_ext );
    }
}

/* --------------------------------------------------------------------- */
void wildcard_fill( char *org_out, char *mod, char *org_in, char *in )

/* Arguments
 *
 *	org_out 	pointer to original output file spec or extension --
 *					 MUST be null-terminated.
 *	mod 		location for modified output file name or extension
 *	org_in		pointer to what the user typed in as input file spec --
 *					 MUST be null-terminated.
 *	in			pointer to corresponding input file name or extension --
 *					 MUST be null-terminated.
 *
 * Results
 *	The expanded primary name or extension is copied to the location
 *	pointed to by mod
 *
 * Global variables referenced or modified - None
 *
 * Description
 *	  Process each character in out_org.
 *	  If a '?', compare org_in with in to determine what character the
 *				? matches.	Copy that character, if any, to mod.
 *	  If a '*', compare org_in with in to determine what character(s) the
 *				* matches.	Copy those characters, if any, to mod.
 *	  Otherwise, copy the character to mod.
 *
 * Programmer - Beth Reitz, June 1991
 * Modified by Dan Edwards, October 1991, to match DOS RENAME behavior.
 */
{
    char ch;			/* character from org being processed */
    char *org_outp; 	/* working pointer to org_out */
    char *org_inp;		/* working pointer to org_in */
    char *modp; 		/* working pointer to mod */
    char *inp;			/* working pointer to in */
    char cInTemp;		/* next character from in */
    int baselen;		/* length of spec up to * */	// 7.4.25 BJY

    org_outp = org_out; 	/* init working pointers */
    org_inp = org_in;
    modp = mod;
    inp = in;
    if (inp)		// 7.4.28 BJY  There might not be an extension
        cInTemp = *inp; // start with 1st char in inp string
    else
        cInTemp = '\0';

    while ( (ch = *org_outp++) != '\0' )	   /* step through original output spec */
    {
        if( cInTemp )		 /* check to make sure not at end of in string */
            cInTemp = *inp++;	/* step down expanded input name string */

        if ( ch == '?' )   /* 0 or 1 char match */
        {
            if ( cInTemp )		   /* if something is in inp at this position */
                *modp++ = cInTemp;		 /* copy replacement for ? to mod */
        }
        else if ( ch == '*' )	/* multi-char match */
        {
            /* Copy the rest of the characters from inp to modp
             */
            if (inp && strchr(org_in, '*'))    // * in input spec as well?	begin 7.4.25 BJY
            {
                baselen = strchr(org_in, '*') - org_in;
                if (baselen < (int)strlen(in))	// nothing to copy if name doesn't reach '*' */
                {
                    inp = in + baselen; 	// point to where wildcard started
                    if (inp)		// 7.4.28 BJY
                        cInTemp = *inp++;
                }
                else
                    cInTemp = '\0';
            }							// end 7.4.25 BJY
        }
        else		  /* a regular character */
            *modp++ = ch;	 /* copy to mod */
    }

    *modp = '\0';		/* terminate result */

}

/******************************************************************/
char *split_path( char *pathbuff, char *name )
/*----------------------------------------------------------------
* Arguments --
*	  pathbuff - location of buffer into which to copy path string
*	  name - name of file, possibly with full path
* Return values -- 
*	  path in pathbuff
*	  name points to name without path
*	  returns pointer to next char after path in pathbuff - will
*		 insert file name here.
* Description --
*	  Find the last \ in name.	Copy all chars up to and including that
*	  \ to pathbuff.  Remaining characters are the file name.  Shift
*	  them in the buffer so that name points to them.
* Globals input -- none
* Globals output -- none
* Error conditions -- none
* Programmer - Beth Reitz, June 1991
*/
/******************************************************************/
{
    char *t;
    int slen;

    t = strrchr( name, DIRSEP );		/* find the last DIRSEP */
    if (!t) 	// 7.4.27 BJY Handle situation like a:file.ext (no backslash)
        t = strchr( name, ':');
    if ( t )
    {
        slen = t - name + 1;				/* number of chars in path */
        strncpy( pathbuff, name, slen );	/* copy path into buffer */
        strcpy( name, t+1 );				/* shift remaining filename so that
        										it is now pointed to by 'name' */
    }
    else
        slen = 0;					/* no path */

    return( pathbuff + slen );

}

/******************************************************************/
void *safalloc( unsigned size ) 	 /* malloc with check for failure */
/*----------------------------------------------------------------
* Description -- call malloc() to allocate the memory
* Return values -- pointer to allocated area
* Globals input -- none
* Globals output -- none
* Error conditions -- Terminates the program if the malloc fails.
*/
/******************************************************************/
{
    void *temp; 			/* pointer to return */

    temp = malloc( size );	/* try to allocate area */
    if ( temp == NULL ) 	/* if failed */
    {
#ifndef _WINDOWS   
        Process_msg(errorFunction, 29, 0, 0);
#else
        MessageBox(NULL, "Memory allocation error, exiting program\n",
                   "Consistent Changes", MB_ICONHAND);
#endif
        exit( 3 );			/* bail out */
    }
    else
        return( temp ); 	/* return pointer to area */

}

#ifdef _WINDOWS
/******************************************************************/
void upcase_str( char *s )		/* force string to upper case */
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
        s++;								/* guard against side effects -
        									   toupper is a macro */
    }

}
#endif


//	 END
