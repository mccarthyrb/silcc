/* cc.h	 MS/CC Externals and Constants	 30-Jan-86	  SMc */
#if defined(UNIXSHAREDLIB)
#define _WINDLL
#endif

#ifdef _WINDLL
#include <setjmp.h>
#endif

#if defined(_WINDOWS) || defined(UNIXSHAREDLIB)
#include "cctio.h"
#endif

#ifdef MAIN
#define GLOBAL
#define PTR_INIT =NULL
#define PTR_INITB =strBuffer
#else
#define GLOBAL extern
#define PTR_INIT
#define PTR_INITB
#endif


// the VERSIONMAJOR and VERSIONMINOR below combine to form the version of
// the release.  Note that a '.' is automatically stuck in between them.
// They should both be numeric values for a release.  (It is OK for a beta
// to have some beta level info after the number part of VERSIONMINOR).
#define VERSIONMAJOR   "8"
#define VERSIONMINOR   "1.8"

#define DATE    "July 2007"
#if defined (_WINDLL)
#define PROGRAM "CC.DLL"
#elif defined (_WINDOWS)
#define PROGRAM "CC FUNC"
#elif defined (UNIXSHAREDLIB)
#define PROGRAM "libcc.s0"
#else
#define PROGRAM "Consistent Changes"
#endif

#if defined(_WINDLL)
typedef int WINAPI CCInputProc(char *, int, long *);
typedef int WINAPI CCOutputProc(char *, int, long *);
typedef int WINAPI CCMatchLineCallback(HANDLE hCCTable, unsigned iLine);
typedef int WINAPI CCExecuteLineCallback(HANDLE hCCTable, unsigned iLine);
typedef int WINAPI CCCompileErrorCallback(char * lpszMessage, unsigned iLine, unsigned iCharacter);
typedef int WINAPI CCErrorCallback(short nMsgIndex, unsigned short wParam, unsigned long lParam, long *lpUserData);
#endif

#if !defined(_WINDLL)
typedef int CCErrorCallback(short nMsgIndex, unsigned short wParam, unsigned long lParam, long *lpUserData);
#endif

/*MS01.C Consistant Change and Manuscripter	Alan Buseman  1-Apr-83	E1
 *		  Converted from PASCAL to C Greg Trihus	June-1983
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
 *
 * Edit history:
 *  10-Aug-87		MT  Added edit history
 *  10-Aug-87		MT  Added MSONLY, to facilitate building MS without CC
 *  10-Aug-87		MT  Deleted TWOFIXEDSPACE, THREEFIXEDSPACE
 *  12-Aug-87		MT  Added MSONLY versions of store numbers and numstores
 *  17-Aug-87		MT  Added #ifdef's for some variables under MSONLY
 *  19-Nov-87		MT  Changed X to GLOBAL for readability
 *   1-Apr-88		MT  Added commands for add, subtract, multiply, and divide
 *							in CC
 *   6-Apr-88		MT  Added the #define PTR_INIT to cause compile-time
 *							initialization of pointers
 *  11-Apr-88		MT  Added excl(ude group) command
 *  16-Nov-89		ACR Removed vestiges of RT11 and CP/M support. Ported to
 *							Microsoft C. Added function prototypes for all global
 *							functions. Removed MSONLY stuff.
 *  25-Oct-91    DJE Changed MAXCHANGES from 2500 to 5000
 *  11-Nov-94    BDW Added code for function versions of CC08
 *  26-Oct-95    DRC Added code for decr(ement) operator, doublebyte support  
 *  27-Dec-95    DRC Added code for Windows DLL support  
 */

/* Provide NOREF macro for suppressing warning messages for
unreferenced function arguments */
#ifdef MICROSOFT
#define NOREF(a) a
#else
#define NOREF(a)
#endif

#if defined(_WINDOWS) || defined(UNIXSHAREDLIB)
#define NO_QUESTIONS	
#endif           

/* Equates for referencing the symbol table array */

#define STORE_HEAD	0
#define SWITCH_HEAD	1
#define GROUP_HEAD	2
#define DEFINE_HEAD	3

/* Equates for "typo detection" using CC_SYMBOL.use
 *   (i.e. catching referenced-but-undefined symbols)
 *
 * Note: these equates MUST both be non-zero, and should each reference
 *			 single bits, because of the way they are used
 */

#define REFERENCED	1
#define DEFINED		2

#define STATUSUPDATEINCREMENT 1

#ifdef _MAC
#define CARRIAGERETURN	'\r'
#else
#define CARRIAGERETURN	'\015'
#endif

#if defined(UNIX)
#define DIRSEP '/'
#else
#define DIRSEP '\\'
#endif

#define TAB             '\011'
#define CTRLZ				'\032'
#define CTRLS				'\023'
#define CTRLC				'\003'
#define FORMFEED			'\014'
#define BACKSPACE			'\010'
#define ESCAPE				'\033'
#define HT					'\011'
#define FIXEDSPACE		'#'
#define VERTICALBAR		'|'


/*
*  MSDOS
*/

/* Boolean constants */
#ifndef TRUE
#define TRUE				(1)
#endif
#ifndef FALSE
#define FALSE				(0)
#endif

/* Binary read/write parameters */
#define BINREAD			"rb"
#define BINWRITE			"wb"

/* Memory contraints */
#ifdef _WINDOWS
	#if defined(WIN32)
		#define MAX_ALLOC		65536*4 - 1
	#else
		#define MAX_ALLOC		65500		/* Maximum space to allocate */
	#endif
#else
#  ifdef UNIX
#    define MAX_ALLOC		65536*4 - 1
#  else
#    define MAX_ALLOC		65535		/* Maximum space to allocate */
#  endif
#endif

//ref MSDN Q32998
#define RESERVEDSPACE	4096		/* Reserved space for file buffers */

/* Exit flags */
#define GOODEXIT			0
#define BADEXIT			1

/* ANSI (VT-100 like) CRT Control */
#if !defined(_WindowsExe)
#define CRT_REGULAR	"\033[0m"	/* Regular */
#define CRT_REVERSE	"\033[7m"	/* Reverse video */
#else
#define CRT_REGULAR	"{"	/* Regular */
#define CRT_REVERSE	"}"	/* Reverse video */
#endif
#if defined(_WindowsExe)
#endif


/* Equates */

#define WEDGE				'>'	  /* separator of search from replacement */
#define HIGHBIT         0x8000  /* Used to test high order bit of elements */
/* in a search element */
#define LINEMAX			500	  /* line is cut off if more than max */

#define STACKMAX			10		  /* max level of do nesting */
#define STORESTACKMAX   10      /* max number of nested pushstore commands */

#define MAXCHANGES		16000	  /* max number of changes allowed */
#define MINMATCH        300     /* the minimum number of bytes in the match buffer */

/* so we can show something in the debug input buffer display */
#if defined(WIN32) || defined(UNIX)
	#define MAXMATCH			20500	  /* match area len, BACKBUFMAX+maxsrch+500 */
	/* the extra 500 are for the debug input buffer display */
	#define BACKBUFMAX		20000		/* length of backup ring buffer */
#else
	#define MAXMATCH			2500	  /* match area len, BACKBUFMAX+maxsrch+500 */
	/* the extra 500 are for the debug input buffer display */
	#define BACKBUFMAX		2000		/* length of backup ring buffer */
#endif

#define MAXARG				1000		/* maximum allowable command argument */
#define MAXDBLARG       255      /* max allowable doublebyte command arg */
#define MAXGROUPS			MAXARG	/* number of groups allowed */
#define GROUPSATONCE		25			/* number of groups allowed on at once */
#define NUMSTORES			MAXARG+1	/* number of store areas plus 1 */
#define SRCHLEN_FACTOR	10			/* Length factor for srchlen() routine */
#define MAX_STACKED_PREC 10			/* Maximum number of prec commands on left of wedge */
#define BEFORE_CNT	35				/* # of characters from backup ring */
/*   buffer to be shown in debug displays */
#define AFTER_CNT	35					/* # of characters from input buffer */
/*   to be shown in debug displays */

#define NUMBER_FILES 300
#define PATH_LEN 250             /* allows for a path and file name */

#if defined(WIN32) || defined(UNIX)
	#define PRIMARY_LEN 250
	#define EXT_LEN 80
#else
	#define PRIMARY_LEN 8
	#define EXT_LEN 3
#endif

#define ARRAY_SIZE (PRIMARY_LEN+EXT_LEN+2)
#define ARRAY_SIZE_2 (ARRAY_SIZE-1)

#ifdef _WINDLL
#define AUX_BUFFER_LEN  4096     // if we need auxiliary output buffer start
// with this size and reallocate if needed
#define   INPUTBUFFERSIZE   1024        // this is a rather arbitrary number
#define   OUTPUTBUFFERSIZE  1024        // this is a rather arbitrary number
#endif

/* Change table commands
 *
 * Note: The dis-continuities in the sequence are due to old commands
 *			  which are no longer accepted, but which should not be used in the
 *			  future.  If a new command is needed, take the next-higher
 *			  negative number, to avoid incompatibilities.
 *
 *			Name				Decimal			  Octal			Hex
 */
#define FONTCMD			(-1)			/* 377			FF */
#define SPECPERIOD		(-2)			/* 376			FE */
#define SPECWEDGE		(-3)			/* 375			FD */
#define IFCMD			(-10)			/* 366			F6 */
#define IFNCMD			(-11)			/* 365			F5 */
#define ELSECMD			(-12)			/* 364			F4 */
#define ENDIFCMD		(-13)			/* 363			F3 */
#define SETCMD			(-14)			/* 362			F2 */
#define CLEARCMD		(-15)			/* 361			F1 */
#define STORCMD			(-16)			/* 360			F0 */
#define ENDSTORECMD		(-17)			/* 357			EF */
#define OUTCMD			(-18)			/* 356			EE */
#define DUPCMD			(-19)			/* 355			ED */
#define APPENDCMD		(-20)			/* 354			EC */
#define BACKCMD			(-21)			/* 353			EB */
#define FWDCMD			(-22)			/* 352			EA */
#define OMITCMD			(-23)			/* 351			E9 */
#define NEXTCMD			(-24)			/* 350			E8 */
#define IFEQCMD			(-25)			/* 347			E7 */
#define IFNEQCMD		(-26)			/* 346			E6 */
#define INCRCMD			(-27)			/* 345			E5 */
#define BEGINCMD		(-28)			/* 344			E4 */
#define ENDCMD			(-29)			/* 343			E3 */
#define REPEATCMD		(-30)			/* 342			E2 */
#define TOPBITCMD       (-31)	        /* 341          E1 */ /* no longer needed? */
#define DEFINECMD		(-32)			/* 340			E0 */
#define DOCMD			(-33)			/* 337			DF */
#define GROUPCMD		(-34)			/* 336			DE */
#define USECMD			(-35)			/* 335			DD */
#define WRITCMD			(-36)			/* 334			DC */
#define WRSTORECMD		(-37)			/* 333			DB */
#define READCMD			(-38)			/* 332			DA */
#define INCLCMD			(-39)			/* 331			D9 */
#define CASECMD			(-41)			/* 327			D7 */
#define ANYCMD			(-42)			/* 326			D6 */
#define PRECCMD			(-43)			/* 325			D5 */
#define FOLCMD			(-44)			/* 324			D4 */
#define WDCMD			(-45)			/* 323			D3 */
#define CONTCMD			(-46)			/* 322			D2 */
#define OUTSCMD			(-47)			/* 321			D1 */
#define IFGTCMD			(-49)			/* 317			CF */
#define ADDCMD			(-50)			/* 316			CE */
#define SUBCMD			(-51)			/* 315			CD */
#define MULCMD			(-52)			/* 314			CC */
#define DIVCMD			(-53)			/* 313			CB */
#define EXCLCMD			(-54)			/* 312			CA */
#define MODCMD			(-55)			/* 311			C9 */
#define BINCMD			(-56)			/* 310			C8 */
#define UNSORTCMD		(-57)			/* 307			C7 */
#define IFSUBSETCMD		(-58)			/* 306			C6 */
#define LENCMD			(-59)			/* 305			C5 */
#define PUSHSTORECMD    (-60)			/* 304          C4 */
#define POPSTORECMD     (-61)			/* 303          C3 */
#define VALCMD          (-62)			/* 302          C2 */
#define ENDFILECMD      (-63)			/* 301          C1 */
#define DECRCMD         (-64)			/* 300          C0 */
#define DOUBLECMD       (-65)			/* 277			BF */
#define IFNGTCMD        (-66)			/* 276			BE */
#define IFLTCMD         (-67)			/* 275			BD */
#define IFNLTCMD        (-68)			/* 274			BC */
#define UTF8CMD			(-69)			/* 273			BB */
#define BACKUCMD		(-70)			/* 272			BA */
#define FWDUCMD			(-71)			/* 271			B9 */
#define OMITUCMD		(-72)			/* 270			B8 */
#define ANYUCMD			(-73)			/* 269			B7 */
#define PRECUCMD		(-74)			/* 268			B6 */
#define FOLUCMD			(-75)			/* 267			B5 */
#define WDUCMD			(-76)			/* 266			B4 */

// values follow for signaling if predefined stores found for out command
#define CCCURRENTDATE  1               /* cccurrentdate predefined store */
#define CCCURRENTTIME  2               /* cccurrenttime predefined store */
#define CCVERSIONMAJOR 3               /* ccversionmajor predefined store */
#define CCVERSIONMINOR 4               /* ccversionminor predefined store */



/* Types */

typedef signed short int SSINT; /* will work same in 16-bit or 32-bit mode */
typedef SSINT *tbltype;         /* Pointer into table[] */
typedef SSINT **cngtype;        /* Pointer into cngpointers[] */

typedef char bool;				  /* booleans stored in byte */
typedef char filenametype[80 + 1];		/* for getln routine */

/* Symbol table element for CC */
typedef struct symnode
{
    struct symnode *next;	/* Pointer to next symbol */
    char *name;					/* Pointer to name string */
    int number;					/* Number of store, switch, define, or group */
    char use;					/* Some combination of the equates REFERENCED */
    /* and DEFINED (declared below) */
} CC_SYMBOL;

typedef struct sym_tab
{
    CC_SYMBOL *list_head;	 /* Pointer to the list */
    int		  next_number;  /* Next (possibly) available number */
} SYM_TAB;


// the typedefs below are for error/warning message generation

typedef  struct msg_S_S_struct
{
    char    *string1;
    char    *string2;
} MSG_STRUCT_S_S;

typedef  struct msg_S_C_S_struct
{
    char    *string1;
    char    char1;
    char    *string2;
} MSG_STRUCT_S_C_S;


/*
 * NOTE: ANY GLOBAL VARIABLES THAT ARE CHANGED IN OR ADDED TO THE LIST OF
 *       GLOBAL VARIABLES BELOW MUST ALSO BE CHANGED THE SAME WAY IN THE
 *       STRUCTURE THAT FOLLOWS, UNLESS THEY ARE RELATED TO INPUT/OUTPUT
 *       FILES AND DO NOT AFFECT THE DLL.  THEY MUST ALSO BE REFLECTED IN
 *       THE SaveState() AND RestoreState() ROUTINES AS WELL.
 *
 */

/* debugging version variables */
#define MAXTABLELINES 32767
#if defined(_WINDLL)
GLOBAL unsigned iCurrentLine;     /* current line in cc table */
GLOBAL HANDLE hTableLineIndexes;    /* handle of array of indexes into the compiled cc table for the current line */
GLOBAL unsigned * TableLineIndexes; /* array of indexes into the compiled cc table for the current line */
GLOBAL CCMatchLineCallback * lpMatchLineCallback;
GLOBAL CCExecuteLineCallback * lpExecuteLineCallback;
GLOBAL CCCompileErrorCallback * lpCompileErrorCallback;
#endif

/* Consistent Change global variables */

GLOBAL SYM_TAB sym_table[4];      /* the array itself  */
GLOBAL filenametype filenm;       /* input name from keyboard */
GLOBAL int namelen;               /* length of filename */
GLOBAL bool cmdlni;               /* True if input on command line */
GLOBAL bool usestdin;             /* True if we are to use stdin for input */
GLOBAL bool usestdout;            /* True if we are to use stdout for output */
GLOBAL char *tblarg PTR_INIT;     /* pointer to table file name if cmdlni */
GLOBAL char *outarg PTR_INIT;     /* pointer to output file name if cmdlni */
GLOBAL char *inarg PTR_INIT;      /* ponter to input file name */
GLOBAL char *writearg PTR_INIT;   /* pointer to write output file if cmdlni */
GLOBAL int inparg;                /* number in argv of current input file */
GLOBAL int argcnt;                /* holds value of argc */
GLOBAL char **argvec PTR_INIT;    /* holds value of argv */
#if defined(_WINDOWS) || defined(UNIXSHAREDLIB)
GLOBAL WFILE *tablefile PTR_INIT;  /* table file for compiled table */
#else
GLOBAL FILE *tablefile PTR_INIT;  /* table file for compiled table */
#endif
GLOBAL FILE *infile PTR_INIT;		 /* input file (changes and input) */
GLOBAL FILE *outfile PTR_INIT;	 /* output file */
GLOBAL FILE *writefile PTR_INIT;	 /* write output file */
GLOBAL FILE *keyboard PTR_INIT;	 /* Keyboard, for "wait for <RETURN>" stuff */
GLOBAL FILE *msgfile PTR_INIT;    /* all messages use this descriptor */
GLOBAL bool bEndofInputData;					 /* on if end of file */
GLOBAL long infilelength;         /* length of current input file. used for process
												 status */
GLOBAL long infileposition;       /* position in current input file */
GLOBAL long nextstatusupdate;

GLOBAL char  nullname[1];			 /* "" */
GLOBAL long  outsize;             /* size of output disk file -1 otherwise  */

GLOBAL char line[LINEMAX+1];      /* input line (changes and input)         */
GLOBAL char strBuffer[LINEMAX+1]; /* buffer to be used for error message    */
GLOBAL char * pstrBuffer PTR_INITB; /* pointer to above buffer for messages */

#ifndef NO_QUESTIONS			
GLOBAL bool prompt_for_input;		/* on if input files are to be ask for */
GLOBAL char outnamebuf[80+1];		/* so we don't have to deal with
											 *	mallocing space */
GLOBAL char innamebuf[80+1];
GLOBAL char ccnamebuf[80+1];
#endif

/* vars specific to compile */
GLOBAL char *parsepntr PTR_INIT,
*parse2pntr PTR_INIT; /* pointers into line */
GLOBAL bool fontsection;			 /* old-style font section present in table */

GLOBAL bool notable;						 /* on if no table */
GLOBAL bool was_math;				 /*	on if last command parsed was a math
											  * operator (add, sub, mul, div, incr)
											  */
GLOBAL bool was_string;				 /*	on if last command parsed was a
											  * quoted string
											  */
GLOBAL tbltype table PTR_INIT;  /* table of changes (Dynamically allocated) */
GLOBAL tbltype tablelimit PTR_INIT; /* tablemax variable for MS and CC */
GLOBAL tbltype tloadpointer PTR_INIT;	/* pointer for storing into table */
GLOBAL tbltype maintablend PTR_INIT;	/* end of main table before fonts */

GLOBAL SSINT *storelimit PTR_INIT;      /* Limit of store area */

GLOBAL char switches[MAXARG+1];           /* switches */

GLOBAL tbltype storebegin[NUMSTORES+1];	/* beg ptrs to store in table[] */
GLOBAL tbltype storend[NUMSTORES+1];		/* end ptrs +1 to store in table[] */
GLOBAL char storeact[NUMSTORES+1];			/* TRUE -> store active in matching */
GLOBAL char storepre[NUMSTORES+1];        /* predefined store found for out
                                             command if this is nonzero   */

GLOBAL int curstor;						 /* current storing area */
GLOBAL int iStoreStackIndex;         /* store() stack pointer */

GLOBAL int storeoverflow;                              /* prevents repeating message */
GLOBAL int doublebyte1st;            /* first boundary value for doublebyte mode     */
GLOBAL int doublebyte2nd;            /* optional second boundary for doublebyte mode */

GLOBAL cngtype groupbeg[MAXGROUPS+1];	 /* beginning of group */
GLOBAL cngtype groupxeq[MAXGROUPS+1];	 /* beginning of leftexec's of group */
GLOBAL cngtype groupend[MAXGROUPS+1];	 /* beginnings and ends of groups */
GLOBAL int curgroups[GROUPSATONCE+1];	 /* current groups in use */
GLOBAL int cgroup;						 /* group from which currently executing
												  *	change came
												  */
GLOBAL int numgroupson;					 /* number of groups currently on */
GLOBAL char letterset[256];          /* current set of "active" match chars */
GLOBAL int setcurrent;					 /* TRUE -> letterset current */

GLOBAL tbltype defarray[MAXARG+1];	 /* defines for do */
GLOBAL SSINT *stack[STACKMAX+1];     /* stack for do command */
GLOBAL int stacklevel;					 /* level of do nesting */

GLOBAL SSINT *backbuf PTR_INIT;           /* ring buffer for backup */
GLOBAL SSINT *backinptr PTR_INIT;                 /* pointers into backbuf */
GLOBAL SSINT *backoutptr PTR_INIT;
GLOBAL SSINT *dupptr PTR_INIT;          /* Pointer into match buffer for 'dup' command */
GLOBAL tbltype *cngpointers PTR_INIT;/* change ptrs into table[] */
GLOBAL tbltype *cngpend PTR_INIT;	 /* last cngpointer +1 */
GLOBAL tbltype executetableptr PTR_INIT; /* Pointer into replacement side of a change when
														* an execute terminates because of a full output buffer */
GLOBAL SSINT *match PTR_INIT;                    /* match area for searching */
GLOBAL SSINT *matchpntr PTR_INIT;                /* pointer into match area */
GLOBAL SSINT *matchpntrend PTR_INIT;

GLOBAL int maxsrch;						 /* length of longest search string */

GLOBAL bool errors;						 /* set if errors in table */
#if defined(_WINDOWS) || defined(UNIXSHAREDLIB)
GLOBAL bool bFileErr;					/* set if errors opening files */
#endif
GLOBAL bool tblfull;						 /* set if table too large */
GLOBAL bool eof_written;				 /* set when CC outputs CTRLZ */
GLOBAL bool debug, mandisplay, mydebug, single_step;	/* debug mode switches */
GLOBAL bool caseless, uppercase;		/* caseless mode switches */
GLOBAL bool unsorted;			/* TRUE if table is processed unsorted */
GLOBAL bool binary_mode;		/* TRUE if table is processed in binary mode */
GLOBAL bool doublebyte_mode;            /* TRUE if using double byte input mode */
GLOBAL bool doublebyte_recursion;       /* TRUE only for recursive call */
GLOBAL bool utf8encoding;		/* TRUE if input/output is UTF-8 encoded */
GLOBAL bool quiet_flag;			/* quiet mode flag */
GLOBAL bool noiseless;			/* noiseless mode flag (no beeps or messages) 7.4.22 BJY */
#ifndef _WINDOWS              
GLOBAL bool percent_print;    /* TRUE means print percent complete numbers  */
#endif
GLOBAL int hWndMessages;

/* global variables that were originally specific to one source file  */

/* cciofn.c formerly static variable */
GLOBAL int StoreStack[STORESTACKMAX];  /* Used by PushStore and PopStore */

/* ccexec.c formerly static variables */
GLOBAL int mchlen[2];       /* xeq, letter match lengths */
GLOBAL int matchlength;     /* length of last match */
GLOBAL tbltype tblxeq[2];              /* xeq, letter table pointer */
GLOBAL tbltype tblptr;         /* working pointer into table */
/* Note:  The above variables are declared separately because */
/*          DECUS C does not allow regular typedef's  */
GLOBAL SSINT *mchptr;                  /* working pointer into match */
GLOBAL SSINT firstletter, cngletter;   /* letters during matching */
GLOBAL int cnglen;   /* Length of change actually being executed */
/*  used for debug display */
GLOBAL int endoffile_in_mch[2]; /* used to flag an endoffile on the left side of a possible match
												 added 7/31/95 DAR */
GLOBAL int endoffile_in_match; /* used to flag an endoffile on the left side of a match
                                    added 7/31/95 DAR */
GLOBAL char predefinedBuffer[50];    /* used to resolve predefined stores  */
#if defined(_WINDOWS) || defined(UNIXSHAREDLIB)
GLOBAL SSINT *store_area;  /* Store area (dynamically allocated) */ //7.4.15 BDW store_area now has file scope rather than scope within ccstart()
#endif

/* ccmsdo.c formerly static variable */
GLOBAL int org_break_flag;
GLOBAL bool concat;        /* concatenated output flag */
GLOBAL bool bAppendFlag;      // Append mode flag
GLOBAL bool bOutFileExists;   // TRUE is there is already a file by that name
GLOBAL char *cpOutMode;       // Records appending or writing
GLOBAL bool bWriteFileExists;   // TRUE is there is already a file by that name
GLOBAL char *cpWriteOutMode;    // Records appending or writing

/* filewc.c formerly static variable */
GLOBAL char (*fn_arr_pnt)[ARRAY_SIZE];          /* array of input file names */
GLOBAL char (*pad_arr_pnt)[ARRAY_SIZE_2];       /* used to sort the above */
GLOBAL char inpath[PATH_LEN];        /* path specification from input file */
GLOBAL char *inpath_end;        /* pointer to end of input file path */
GLOBAL char outpath[PATH_LEN];       /* path specification from output file */
GLOBAL char *outpath_end;       /* pointer to end of output file path */
GLOBAL int files_read;              /* number of input files to be processed */
GLOBAL int *gpindex;
GLOBAL int ind_incr;                    /* index to next input file */

/* cccomp.c formerly static variable */
GLOBAL char keyword[20+1];         /* keyword for identification          */
GLOBAL bool begin_found;           /* TRUE == we have a begin statement   */

/* cccompfn.c formerly static variable */
GLOBAL char *precparse;            /* used by chk_prec_cmd and compilecc  */
GLOBAL char *lpszCCTableBuffer;

/* ccstate.c and other DLL formerly static variable */
// errorFunction is error routine (NULL default means use our own routine)

GLOBAL bool bBeginExecuted;          // TRUE if begin statement was executed or it does not exist
GLOBAL bool bNeedMoreInput;
GLOBAL CCErrorCallback * errorFunction PTR_INIT;
#if defined _WINDLL

GLOBAL HANDLE hActiveCCTable;
GLOBAL jmp_buf abort_jmp_buf;   // used in bailout processing

GLOBAL int nInBuf;              // size of the cc input buffer passed to DLL
GLOBAL int nInSoFar;            // number of bytes passed in so far this call
GLOBAL bool bPassedAllData;     // true if we have already been passed all of
// our input data from user callback routine
GLOBAL char *lpOutBuf PTR_INIT; // points to cc output buffer for DLL
GLOBAL char *lpOutBufStart;     // start of CC output buffer when CC back end
GLOBAL int nMaxOutBuf;          // maximum size of CC output buffer for DLL
GLOBAL int nUsedOutBuf;         // used size of CC output buffer for DLL
GLOBAL CCInputProc *lpInProc;   // input procedure to fill up DLL input
GLOBAL CCOutputProc *lpOutProc; // output procedure to take up DLL output
GLOBAL bool bOutputBufferFull;
GLOBAL bool bMoreNextTime;      // TRUE if more data but filled output buffer
GLOBAL bool bFirstDLLCall;      // TRUE if first call to CCGetBuffer DLL call
GLOBAL HANDLE hInputBuffer;     // handle for CC DLL input buffer area
GLOBAL HANDLE hOutputBuffer;    // handle for CC DLL output buffer area
GLOBAL char *lpInBuf PTR_INIT;  // points to cc input buffer for DLL
GLOBAL char *lpInBufStart;      // points to start of cc input buffer for DLL
GLOBAL bool bSavedDblChar;      // TRUE if saved second half of doublebyte
// char that needs to be output to the user
GLOBAL char dblSavedChar;       // saved second half of doublebyte character
GLOBAL bool bSavedInChar;       // TRUE if saved possible first half double-
// byte char from last byte of input buffer
GLOBAL char inSavedChar;        // saved possible first half of doublebyte
GLOBAL char *lpAuxBufStart;     // start of special auxiliary output buffer
GLOBAL char *lpAuxOutBuf;       // next write position in auxiliary buffer
GLOBAL char *lpAuxNextToOutput; // next aux buffer position to be outputted
GLOBAL bool bAuxBufUsed;        // true if we have allocated aux buf to use
GLOBAL bool bProcessingDone;    // true if DLL return completion rc to user
GLOBAL bool bPassNoData;        // true if DLL needs no data next time called
GLOBAL int nCharsInAuxBuf;      // length of used (filled) part of aux buffer
GLOBAL int iAuxBuf;
GLOBAL int nAuxLength;          // length of special auxiliary output buffer
GLOBAL HANDLE hAuxBuf;          // handle for special auxiliary output buffer
GLOBAL HANDLE hStoreArea;       // handle for our store area
GLOBAL long lDLLUserInputCBData;   // user data passed to input callback
GLOBAL long lDLLUserOutputCBData;  // user data passed to output callback
GLOBAL long lDLLUserErrorCBData;   // user data passed to error callback
GLOBAL WFILE *fUserInFile;       // file for input file name passed in to DLL
GLOBAL WFILE *fUserOutFile;      // file for output file name passed in to DLL
GLOBAL HANDLE hCCParent;        // handle for calling program
GLOBAL char cctpath[PATH_LEN+1];   // cct file path (includes null at end)
#endif

// following are structures used to pass message arguments.  These
// correspond to message defines etc in ccerror.h

GLOBAL  MSG_STRUCT_S_S Msg_s_s;
GLOBAL  MSG_STRUCT_S_C_S Msg_s_c_s;

GLOBAL  char errorLine[3*LINEMAX];    // place to store syntax error message

/*
 * NOTE: ANY GLOBAL VARIABLES THAT ARE CHANGED IN OR ADDED ABOVE
 *       MUST ALSO BE ADDED OR CHANGED THE SAME WAY IN THE
 *       STRUCTURE THAT FOLLOWS, UNLESS THEY ARE RELATED TO INPUT OR
 *       OUTPUT OR OTHERWISE DO NOT AFFECT THE DLL.  THEY MUST ALSO BE
 *       REFLECTED IN THE SaveState() AND RestoreState() ROUTINES.
 *
 */

typedef struct cc_global_vars
{
#if defined(_WINDLL)
    unsigned iCurrentLine;     /* current line in cc table */
    HANDLE hTableLineIndexes;  /* handle of array of indexes into the compiled cc table for the current line */
    CCMatchLineCallback * lpMatchLineCallback;
    CCExecuteLineCallback * lpExecuteLineCallback;
    CCCompileErrorCallback * lpCompileErrorCallback;
#endif 
    SYM_TAB sym_table[4];      /* the array itself  */
    char *tblarg;              /* pointer to table file name if cmdlni */
#ifdef _WINDLL
    WFILE *tablefile;           /* table file for compiled table */
#else
    FILE *tablefile;           /* table file for compiled table */
#endif
    bool bEndofInputData;                /* on if end of file */
    long nextstatusupdate;
    char  nullname[1];         /* "" */
    char line[LINEMAX+1];      /* input line (changes and input) */
#ifndef NO_QUESTIONS			
    bool prompt_for_input;     /* on if input files are to be ask for */
    char outnamebuf[80+1];     /* so we don't have to deal with
    										 *	mallocing space */
    char innamebuf[80+1];
    char ccnamebuf[80+1];
#endif
    /* vars specific to compile */
    char *parsepntr,
    *parse2pntr;    /* pointers into line */
    bool fontsection;          /* old-style font section present in table */
    bool notable;                 /* on if no table */
    bool was_math;             /*   on if last command parsed was a math
    										  * operator (add, sub, mul, div, incr)
    										  */
    bool was_string;           /*   on if last command parsed was a
    										  * quoted string
    										  */
    tbltype executetableptr; /* Pointer into replacement side of a change when
    								* an execute terminates because of a full output buffer */
    tbltype table;             /* table of changes (Dynamically allocated) */
    tbltype tablelimit;        /* tablemax variable for MS and CC */
    tbltype tloadpointer;      /* pointer for storing into table */
    tbltype maintablend;       /* end of main table before fonts */
    SSINT *storelimit;         /* Limit of store area */
    char switches[MAXARG+1];           /* switches */
    tbltype storebegin[NUMSTORES+1];   /* beg ptrs to store in table[] */
    tbltype storend[NUMSTORES+1];      /* end ptrs +1 to store in table[] */
    char storeact[NUMSTORES+1];        /* TRUE -> store active in matching */
    char storepre[NUMSTORES+1];        /* predefined store found for out command
                                          if element of this is nonzero    */
    int curstor;                  /* current storing area */
    int iStoreStackIndex;         /* store() stack pointer */
    int storeoverflow;                              /* prevents repeating message */
    int doublebyte1st;            /* first boundary value for doublebyte mode     */
    int doublebyte2nd;            /* optional second boundary for doublebyte mode */
    cngtype groupbeg[MAXGROUPS+1];   /* beginning of group */
    cngtype groupxeq[MAXGROUPS+1];   /* beginning of leftexec's of group */
    cngtype groupend[MAXGROUPS+1];   /* beginnings and ends of groups */
    int curgroups[GROUPSATONCE+1];   /* current groups in use */
    int cgroup;                      /* group from which currently executing
                                      *   change came
                                      */
    int numgroupson;              /* number of groups currently on */
    char letterset[256];          /* current set of "active" match chars */
    int setcurrent;               /* TRUE -> letterset current */
    tbltype defarray[MAXARG+1];   /* defines for do */
    SSINT *stack[STACKMAX+1];     /* stack for do command */
    int stacklevel;               /* level of do nesting */
    SSINT *backbuf;               /* ring buffer for backup */
    SSINT *backinptr;             /* pointers into backbuf */
    SSINT *backoutptr;
    SSINT *dupptr;               /* Pointer into match buffer for 'dup' command */
    tbltype *cngpointers;         /* change ptrs into table[] */
    tbltype *cngpend;             /* last cngpointer +1 */
    SSINT *match;                 /* match area for searching */
    SSINT *matchpntr;             /* pointer into match area */
    SSINT *matchpntrend;
    int maxsrch;                  /* length of longest search string */
    bool errors;                  /* set if errors in table */
#if defined(_WINDOWS) || defined(UNIXSHAREDLIB)
    bool bFileErr;               /* set if errors opening files */
#endif
    bool tblfull;                 /* set if table too large */
    bool eof_written;             /* set when CC outputs CTRLZ */
    bool debug, mandisplay, mydebug, single_step;  /* debug mode switches */
    bool caseless, uppercase;    /* caseless mode switches */
    bool unsorted;         /* TRUE if table is processed unsorted */
    bool binary_mode;      /* TRUE if table is processed in binary mode */
    bool doublebyte_mode;            /* TRUE if using double byte input mode */
    bool doublebyte_recursion;       /* TRUE only for recursive call */
    bool utf8encoding;		/* TRUE if input/output is UTF-8 encoded */
    bool quiet_flag;       /* quiet mode flag */
    bool noiseless;        /* noiseless mode flag (no beeps or messages) 7.4.22 BJY */
    int hWndMessages;
    /* global variables that were originally specific to one source file  */
    /* cciofn.c formerly static variable */
    int StoreStack[STORESTACKMAX];  /* Used by PushStore and PopStore */
    /* ccexec.c formerly static variables */
    int mchlen[2];       /* xeq, letter match lengths */
    int matchlength;     /* length of last match */
    tbltype tblxeq[2];              /* xeq, letter table pointer */
    tbltype tblptr;         /* working pointer into table */
    SSINT *mchptr;                  /* working pointer into match */
    SSINT firstletter, cngletter;   /* letters during matching */
    int cnglen;   /* Length of change actually being executed */
    /*  used for debug display */
    int endoffile_in_mch[2]; /* used to flag an endoffile on the left side of a possible match
    											 added 7/31/95 DAR */
    int endoffile_in_match; /* used to flag an endoffile on the left side of a match
                                       added 7/31/95 DAR */
#if defined(_WINDOWS) || defined(UNIXSHAREDLIB)
    SSINT *store_area;  /* Store area (dynamically allocated) */ //7.4.15 BDW store_area now has file scope rather than scope within ccstart()
#endif
    /* cccomp.c formerly static variable */
    char keyword[20+1];         /* keyword for identification          */
    bool begin_found;           /* TRUE == we have a begin statement   */
    /* cccompfn.c formerly static variable */
    char *precparse;            /* used by chk_prec_cmd and compilecc  */
    /* ccstate.c formerly static variable */
#ifdef _WINDLL
    CCErrorCallback * errorFunction;
    int nInBuf;                 // size of the cc input buffer passed to DLL
    int nInSoFar;               // number of bytes passed in so far this call
    bool bPassedAllData;        // true if we have already been passed all of
    // our input data from user callback routine
    char *lpOutBuf;             // points to cc output buffer for DLL
    char *lpOutBufStart;        // start of CC output buffer when CC is back end
    int nMaxOutBuf;             // maximum size of CC output buffer for DLL
    int nUsedOutBuf;            // used size of CC output buffer for DLL
    CCInputProc *lpInProc;      // input procedure to fill up DLL input
    CCOutputProc *lpOutProc;    // output procedure to take up DLL output
    bool bMoreNextTime;         // TRUE if more data but filled output buffer
    bool bNeedMoreInput;
    bool bOutputBufferFull;
    bool bFirstDLLCall;         // TRUE if DLL is being called the first time
    bool bBeginExecuted;        // TRUE if begin statement was executed or it does not exist
    HANDLE hInputBuffer;        // handle for CC DLL input buffer area
    HANDLE hOutputBuffer;       // handle for CC DLL output buffer area
    char *lpInBuf;              // points to cc input buffer for DLL
    char *lpInBufStart;         // points to start of cc input buffer for DLL
    bool bSavedDblChar;         // TRUE if saved second half of doublebyte
    // char that needs to be outputted to the user
    char dblSavedChar;          // saved second half of doublebyte character
    bool bSavedInChar;          // TRUE if saved possible first half double-
    // byte char from last byte of input buffer
    char inSavedChar;           // saved possible first half of doublebyte
    char *lpAuxBufStart;        // start of special auxiliary output buffer
    char *lpAuxOutBuf;          // next write position in auxiliary buffer
    char *lpAuxNextToOutput;    // next aux buffer position to be outputted
    bool bAuxBufUsed;           // true if we have allocated aux buf to use
    bool bProcessingDone;       // true if DLL return completion rc to user
    bool bPassNoData;           // true if DLL needs no data next time called
    int nCharsInAuxBuf;         // length of used (filled) part of aux buffer
    int iAuxBuf;
    int nAuxLength;             // length of special auxiliary output buffer
    HANDLE hAuxBuf;             // handle for special auxiliary output buffer
    HANDLE hStoreArea;          // handle for our store area
    long lDLLUserInputCBData;   // user data passed to input callback function
    long lDLLUserOutputCBData;  // user data passed to output callback function
    long lDLLUserErrorCBData;   // user data passed to error callback function
    HANDLE hCCParent;           // handle for calling program
    char cctpath[PATH_LEN+1];   // cct file path (includes null at end)
    WORD wDS;
#endif

} GLOBAL_VARS_STRUCT;


/* Function prototypes for global functions */
/* cc.c */
void banner(void);	// 7.4.05 ALB
#ifdef _WINDLL
void CallCCMainPart();
#endif
#if defined _WINDOWS || defined(UNIXSHAREDLIB)
void freeMem(void);
#endif

#ifdef _WINDLL
/* ccstate.c */
HANDLE SaveState(HANDLE hInHandle);
void RestoreState(HANDLE hGlobin);
int CallOutputCallback(HANDLE hActiveCCTable);
#endif

/* ccutil.c */
int odd(int);
int a_to_hex(char);
char *bytcpy(char *, char *, int);
SSINT *ssbytcpy(SSINT *, SSINT *, int);
char *bytset(char *, char, int);
SSINT *ssbytset(SSINT *, SSINT, int);
void resolve_predefined(int index);
long long_abs(long);
int valnum(SSINT *, char *, int, long *, char);
void bailout(int, int);
void upcase_str( char * );

/* ccompfn.c */
void tblcreate(void);
#if defined(_WINDOWS) || defined(UNIXSHAREDLIB)
void tblfree(void);
#endif
void *tblalloc(unsigned, unsigned);
unsigned max_heap(void);
void storechar(SSINT);
int srchlen(tbltype);
void compilecc(void);
void loadfont(void);
int cctsetup(void);
void cctsort(void);
void dumpchar(char);
void cctdump(void);
void show_pf_error(int);
void show_sf_error(int);
char *sym_name(int, int);
int get_symbol_number(char * symbol_name, int iSymbolTable);

/* ccomp.c */
void err(char *);
void inputaline(void);
void parse(char **, char **, int);
bool wedgeinline(void);
void stornoarg(char, char, char);
void storarg(char, char, char);
void storoparg(char, char, char);
void stordbarg(char, char, char);
void buildnum(int *);
void buildkeyword(void);
void storeelement(int);
char *cmdname(char, int);
void flushstringbuffer(int);

/* cciofn.c */
int yes(void);
void storch(int, SSINT);
void writechar(void);
void output(SSINT);
SSINT inputchar(void);
void tblskip(SSINT **);
int storematch(SSINT **);
int IfSubset(SSINT **tblpnt);
void LengthStore(SSINT **cppTable);
void PushStore(void);
void PopStore(void);
void BackCommand(SSINT **cppTable, bool utf8);
void FwdOmitCommand(SSINT **cppTable, bool *pbOmitDone, bool fwd, bool utf8);
void incrstore(int);
void decrstore(int);
void doublebytestore(SSINT);
void ccmath(SSINT, SSINT **);
void groupinclude(register int);
void groupexclude(register int);

/* ccexec.c */
void debugwrite(int);
void displbefore(void);
void displbefore(void);
#if !defined(_WINDOWS) && !defined(UNIXSHAREDLIB)
void displafter(void);
void check_kbd(void);		/* Check for ^C or ^S */
#endif
void writestore(int);
SSINT * execute(int, SSINT *, int);
int anycheck(SSINT);
int contcheck(void);
int leftexec(int);
void startcc(void);
void fillmatch(void);
#if defined(_WINDOWS) || defined(UNIXSHAREDLIB)
void storefree(void);											//7.4.15
#endif
void ccin(void);
int gmatch(int);
int cmatch(cngtype, int);
void completterset(void);
void refillinputbuffer(void);
void execcc(void);

/* ccout.c */
void out_char(SSINT);
#ifdef _WINDLL
void out_char_buf(char);
#endif

/* msgput.c */
int msg_putc(int);
int msg_puts(char *);
int msg_printf(char *,...);
int Process_msg(CCErrorCallback * ,
                short, short unsigned, long unsigned);

/* ccmsdo.c */
void sysinit(int);
void syscleanup(void);
int get_char(void);
void cmdlnparse(int, char *[]);
char *get_string(char *);
void usage(void);
void openinfile(char *);
bool openoutfile(char *);
void openwritefile(char *);
void get_next_infile( void );
bool openfilepair( bool );
void getname( char *, char * );
void beep(int, int);

/* filewc.c */
char *get_first_name( int, char *, char ** );
char *get_next_name( char ** );

/* END */
