/* cc.c	 12/85  Global Variables and Main Function  AB
 *
 * Copyright (c) 1980-1998, 2015 by SIL International
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
 *  Data structures are as follows:
 *
 *		 line (as used as input line of change table)
 *		 ----------------------------------------------------------
 *		 ^				  ^					^					 ^
 *			parsepntr	 parsepntr2			 len				LINEMAX
 *
 *  Also wpntr and wpntr2 are used in place of parsepntr and
 *  parsepntr2 in wedginline.
 *
 *
 *		 table (storage area for changes)
 *		 -----------------------------------------------------------
 *					  ^			 ^
 *				tloadpointer			tablemax
 *
 *		Also tblpnt is used as a temporary pointer into table
 *		during the performance of a change.
 *
 *		backbuf (output ring BUFFER for backup command)
 *		-----------------------------------------------------
 *		 ^						^							^
 *			backoutptr		  backinptr				 backbufmax
 *		 (place to get		 (place to put
 *		  next char)		  next char)
 *
 *     cngpointers (pointers into table)
 *		 --------------------------
 * 
 *
 *
 *		 match (data area in which matching of changes is done)
 *		 -------------------------------------------------------------------
 *                        ^                                          ^
 *                    matchpntr                                   MAXMATCH
 *
 *
 *  (22-Nov-82 I put storearea at top of table area instead of separate.)
 *  storearea of table (data area for store and out commands)
 *		-----------------------------------------------------------------
 *     ^ 
 *   tloadpointer
 *
 *  storebegin (pointers into storearea, to beginnings of storage spaces)
 *  also storend (pointers to ends of storage spaces)
 *		 --------------------------------
 *                                     ^
 *                                 NUMSTORES
 *
 *  CC part of program works as follows:
 *
 *  It is in two main parts.	The first is to input and compile a table of
 *  changes.  The second is to actually do the changes on an input file.
 *  During compiling processing is done line by line.  Within a line
 *  processing is done element by element.  An element is one logical
 *  piece, either a literal string, a keyword, or a special mark such as
 *  a WEDGE or comment character.  The parse routine finds the individual
 *  elements.	It uses parsepntr2 as its main pointer.  This pointer always
 *  points to the next position which is to be looked at.  It thus points
 *  to the position one beyond the last character of an element which has
 *  just been found.  The other pointer (parsepntr) is placed at the first
 *  character of an element which has just been found.  The parse routine
 *  identifies (or parses) elements and marks them by setting these two
 *  pointers.
 *
 *  The procedure storeelement stores a particular element by identifying
 *  its type and setting it up for storage.	Changes are stored into an
 *  area called table.	A pointer named tloadpointer is used for storing
 *  into the table.	It always points to the last place a character was
 *  stored.  The actual storing is always done by the procedure storechar,
 *  which stores one character into the table.
 *
 *  During the actual changes, processing is done on a character by
 *  character basis.  An input end of line is converted to a single
 *  carriage return, and end of file is converted to CTRL-Z.  These
 *  characters are then detected during processing by routines that need
 *  to identify them.
 *
 *  Characters are input into a match area until it is full.  Each position
 *  of this match area is then looked at to see if a search string can be
 *  matched at that position.  If there is no match, the character is
 *  output, and a test is made to see if there is enough left in the match
 *  area for further searching.	If not enough is left, the unused part is
 *  moved down to the bottom of the match area and the match area is filled
 *  in with new input.	If a match is found at any position, the
 *  replacement for the match is output, and the match pointer is moved
 *  over the part that matched the search string.
 *
 *  Switch commands are set, clear, if, ifn, else, and endif.	Switches are
 *  set and cleared in the array switches.
 *
 *  Storage commands are store, append, endstore, and out.	Storing works
 *  as follows:  At initialization, storebegin and storend are all
 *  initialized to tloadpointer, placing them just above the table.	When
 *  something is stored into a storage location, all stored material above
 *  it is moved up to make room as each character comes in.  However, once
 *  a storage area has had something stored in it, it always has that much
 *  room available in it, because the pointers in storebegin are not moved
 *  back down when a storage area is cleared.  This means that at the
 *  beginning of a program storage moves as each thing is stored.  But
 *  after that, the only time storage moves is when a particular storage
 *  area is used to store something that is longer than anything it ever
 *  had in it before.  This procedure results in a sort of dynamic
 *  dimensioning which is not quite as efficient in storage as full dynamic
 *  string handling, but is much faster in operation.  This is based on the
 *  assumption that the same types of data will be stored in particular
 *  storage areas over and over, and that the necessary space in each
 *  storage area will be relatively constant during the operation of a
 *  given table.
 *
 *  The backup command takes the n previous output characters or stored
 *  characters and places them in the match table.
 *
 * Change History:
 *  5-Dec-84  5.2E  GT	changed names: popen propen & popenstatus prntrstatus
 *								used global prntr variables eliminating tempvertspace
 *								& dotrowbytes change ht (MS01.H MS2C.C MS2E.C MS2G.C)
 *  3-Apr-85  5.2G  GT	Don't require USR NOSWAP
 *								test if handlers aren't loaded
 *  4-Apr-85  5.2H  GT	prntr don't buffer |u & |d
 *								prntr allow 8th bit sequences
 *								prntr change freeing propen
 *								prntr prevent \37 past start of graphic
 *								prntr change \10 to \36
 *								prntr don't simulate bold on bytepercol == 1
 *								prntr if printable after graphic, force graphic out
 *  18-May-85 5.3  AB	change nulls to 1025, version to 5.3 for release
 *  05-Dec-85 5.4X SEB	Adapt for Motorola 68000 UNIX
 *  25-Jul-86		 MT	Renamed module MSA.C to MS.C
 *								Changed comments at beginning of procedures
 *  15-Aug-86		 MT	Began commenting the entire set of modules
 *								to bring up to the coding standard.
 *  17-Oct-86		 MT	Added code (under #ifdef MONITOR) to do
 *								profiling using AZTEC C V3.4
 *   2-Nov-86 6.1A MT	Added support for 8-bit characters under CC
 *								Changed version
 *								Added display of copyright message
 *  17-Nov-86 6.1A MT	Made display of program name and copyright message
 *								unconditional under RT-11/TSX+
 *   8-Dec-86 6.1B MT	Fixed bug in outs
 *   9-Dec-86 6.2A MT	Began installing new font system
 *  23-Jan-87 7.?? MT	New font system
 *  22-May-87 7.00A MT	Starting to pre-release it, so need to track by
 *								version number
 *  28-May-87 7.00B MT	No longer accepting -c as compiled table under CC,
 *								Better error message in cmdlnparse
 *									(MS-DOS, CP/M, Unix)
 *  28-May-87 7.00C MT	Got rid of /m option for compiling, use /c for
 *								compiling all tables for CC or MS
 *   3-Jun-87 7.0D  MT	New version of PFLIB (1.5L),
 *								corrected "Usage:" statement,
 *								added DATE define for banner
 *   5-Jun-87 7.0E  MT	Fixed bug with line numbering
 *   8-Jun-87 7.0F  MT	New version of PFLIB (1.5N)
 *  15-Jun-87 7.0G  MT	Fixed problem with /s (skip pages) option and
 *								setting up printer correctly
 *  16-Jun-87 7.0H  MT	Fixed bug in CC to allow group(1) to not have to be
 *								the first group defined
 *								Deleted the define MYVERSION, since it is not used
 *  17-Jun-87 7.0I  MT	Fixed handling of <BACKSPACE>s in formatting
 *  18-Jun-87 7.0J  MT	New version of PFLIB (1.5Q), which fixes underlining
 *								of unit spaces
 *   1-Jul-87 7.0K  MT	Fixed handling of |<SPACE> and |<RETURN>.
 *   2-Jul-87 7.0L  MT	Reduced unnecessary readbuffer calls in formatline
 *								and lineoutput
 *  14-Jul-87 7.0M  MT	New version of PFLIB (1.5R)
 *  17-Jul-87 7.0N  MT	New version of PFLIB (1.5S)
 *  27-Jul-87 7.0P  MT	More informative error message if unable to allocate
 *								tables (tell them their font file is probably too
 *								big)
 *			Note: No version 7.0O because O looks too much like a zero.
 *  30-Jul-87 7.0Q  MT	Changed justify to use pointers instead of readbuffer
 *   6-Aug-87 7.0R  MT	Changed formatline to use pointers
 *								instead of readbuffer calls
 *   6-Aug-87 7.0S  MT	Changed lineoutput to use pointers
 *								instead of readbuffer calls
 *  10-Aug-87 7.0T  MT	Dropped support for <CTRL-A>, <CTRL-B> as fixed spaces
 *  13-Aug-87 7.0U  MT	Added MSONLY version (no CC front end)
 *  17-Aug-87 7.0V  MT	CC bug fix with wd() and prec()
 *  18-Aug-87 7.0W  MT	Fix in proportional TABbing
 *  20-Aug-87 7.0X  MT	Version 1.5T of pflib (8-bit chars mixed w/multi-hi)
 *  21-Aug-87 7.0Y  MT	Version 1.5U of pflib (overstrikes mixed w/multi-hi)
 *  25-Aug-87 7.0Z  MT	Checking for CTRLZ instead of EOF on CP/M
 *   2-Sep-87 7.1   MT	Skip over NULs on input, because they will hang the
 *								new font system, which deals with
 *								NUL-terminated strings
 *   2-Sep-87 7.1A  MT	New version of pflib
 *								(fix for graphics bolding on Toshiba printers)
 *   3-Sep-87 7.1B  MT	New version of pflib
 *								(further graphics fix for Toshiba printers)
 *  17-Sep-87 7.1C  MT	Fix for multi-line headers/footers
 *  25-Sep-87 7.1D  MT	Changed to a separate underline-on/underline-off
 *								sequence.  Also, new version of PFLIB to support
 *								the new sequence
 *  30-Sep-87 7.1E  MT	New version of pflib (1.6)
 *								(send initialization seq. before setting hmi & vmi
 *								to defaults, fix in graphic underlining)
 *   2-Oct-87 7.1F  MT	New version of pflib (1.6A)
 *								(further fix to underlining of multi-high graphics)
 *   7-Oct-87 7.1G  MT	Added .tc (tab char) and .db (discretionary break)
 *								commands
 *  14-Oct-87 7.1H  MT	On DEC, if output going to a terminal from CC
 *								use unbuffered output (.TTYOUT calls)
 *  15-Oct-87 7.1I  MT	New version of pflib (1.6B), which handles underlining
 *								of single-high chars which are mixed in with
 *								multi-high
 *  19-Oct-87 7.1J  MT	Corrected a side-effect from a previous change,
 *								which had resulted in double-column
 *								backing up too high
 *  21-Oct-87 7.1K  MT	pflib version 1.6C, which fixes underlining elongated
 *								graphics on the 8510, and turns elongated back on
 *								after a char set change so the Toshiba printers
 *								keep elongating
 *								(they turn elongation off after a char set change)
 *  22-Oct-87 7.1L  MT	pflib version 1.6D, which allows a vmi of less than
 *								the height of a font.  It will back up the proper
 *								amount at the end of the last pass in order to
 *								create the smaller effective vmi.
 *  27-Oct-87 7.1M  MT	Made "ms -c" (CC only, but under MS) use the
 *								new font system.
 *   3-Nov-87 7.1N  MT	A dot-pa at the end, with concatenation off, no longer
 *								causes one-too-many pages to be output.
 *								When a line to be centered is too long, and contains
 *								discretionary hyphens or discretionary breaks,
 *								the first character of the next line to be output
 *								will no longer be lost.
 *								If "ed.chr" was loaded as the font file, don't ask
 *								for an output file, since it can only go to
 *								the screen.
 *   (No version 7.1O)
 *
 *  10-Nov-87 7.1P  MT	Be sure the font system knows which font we're in
 *								by calling pflenrt() with the current font,
 *								in doleftmargin()
 *  19-Nov-87 7.2   MT	Version change for release
 *   9-Dec-87 7.2A  MT	Added new dot command--.rv (reverse) and added new
 *								procedure r2l_lineoutput, in ms2f.c
 *								Changed header to say "pre-release"
 *  26-Jan-88 7.2B  MT	Added symbolic names capability to CC
 *  27-Jan-88		  MT	Got rid of underlining the left margin when
 *								underlining of spaces is enabled
 *  28-Jan-88 7.2C  MT	Fixed problem with double-column making right column
 *								one line too long
 *								Got rid of the extra <NEWLINE> after the footer by
 *								writing 2 bar-u's before the formfeed
 *  29-Jan-88 7.2D  MT	Fixed bug with <TAB>s in a justified line
 *								giving a line one space too short in fixed-space
 *								Fixed bug with bar-m causing subsequent lines to
 *								shift left by the width of previous bar-m text
 *   1-Feb-88 7.2E  MT	Replaced <NEWLINE>-before-formfeed fix from 7.2C with
 *								a call to the new pflib routine pfff()
 *								(introduced in pflib 1.6G)
 *  12-Feb-88 7.2F  MT	New version of pflib (1.6H), which breaks a clump
 *								at a SPACE, even if it's in the font
 *								(which usually means a graphic SPACE),
 *								and doesn't load .PRT files for the CP/M version
 *								of MSONLY
 *  18-Feb-88 7.3   MT	Release version,	(or so I thought!!)
 *								updated copyright information in banner
 *   8-Mar-88 7.3A  MT	Fixed bug with underlining intermittently failing
 *								with multi-high
 *								Fixed bug with centering in .fo n generating an
 *								unnecessary blank line
 *  11-Mar-88 7.3B  MT	Fixed bug with font changes on single lines causing
 *								pflib to get confused and possibly print in the
 *								wrong font, fix was to print an explicit
 *								change to the current font in doleftmargin()
 *
 *		 7.3B released as a major release -- March 88
 *
 *  28-Mar-88 7.3C  MT	Fixed bug with ifgt(),ifeq(), and ifneq() in handling
 *								8-bit characters and returning the wrong answer
 *								(problem was due to high bit extending to sign bit)
 *  11-Apr-88 7.3D  MT	Added functions add(), sub(), mul(), div() to CC.
 *								Implemented as long int's, but maximum values
 *								limited to 1,999,999,999 (without commas) for ease
 *								of implementation.
 *								Added compile-time checking for commands other than
 *								cont() following the comparison operators
 *								ifeq(), ifgt(), ifneq() and following the arithmetic
 *								commands just added.
 *								Added compile-time checking for size of arithmetic
 *								constants used in add, sub, mul, div.
 *  13-Apr-88 7.3E  MT	Added functions incl() and excl() to CC, to
 *								include and exclude individual groups.
 *   4-May-88 7.3F  MT	New version of pflib (1.6J)
 *								fixed varying error message on font load failure
 *								fixed handling of bar-bar (||) when not going to
 *								a .PRT file
 *  17-Jan-89 7.3G  MT  Changed CC to try a numeric compare on
 *                      ifgt, ifeq, ifneq  (the way it used to)
 *  19-Jan-89 7.3H  MT  Changed CC to allow multiple fol()'s to look for
 *                      multiple characters following the match string
 *                      Changed CC to try appending ".CCT" to the table name
 *                      if there is no . (period) in the given name
 *    ( no 7.3I )
 *  24-Jan-89 7.3J  MT  Changed CC to allow multiple prec()'s to look for
 *                      multiple characters preceding the match string;
 *                      "stacked" prec()'s are interpreted left-to-right
 *                      (i.e.--'a' prec(1,2,3) will look for
 *                      something in 1, followed by something in 2,
 *                      followed by something in 3, followed by a)
 *  15-Nov-89 7.4   ACR	(beta test) New version of pflib (2.0). Also ported
 *	 (started)				to Microsoft C and large data model. 
 *	 28-Feb-90		  		Removed last vestiges of support for RT11 and CP/M.
 *	 (finished)				Also removed stuff related to Aztec profiler.
 *								Removed UNIX-specific stuff.
 *
 *  06-Mar-90 7B.4a ACR	Minor change to remove unnecessary code in MS1B.C.
 *								Also fix in pf.lib to detect if printer is off-line.
 *
 *  15-May-90 7.4   ACR	Version change for release. (The earlier 7.4 was
 *								a beta test version). Also fixed a handful of minor
 *								bugs. Removed the ancient '/w' option.
 *	23-Aug-90 7.4a	  ACR	Minor fixes to support versions for Sharp PC-5000
 *
 *	04-Sep-90 7.4b	  ACR	Fixed bug where long lines on EpsonLQ printers would
 *								not wrap properly. (See NOC 9.5.38) Also upgraded to
 *								PF.LIB version 2.0d to fix bug where mixtures of
 *								single and double-high did not always work properly.
 *	01-Nov-90 7.4c	  ACR	Fixed bug where "omit (1)" was erroneously treated
 *								like "omit(2)" because of the space preceding the
 *								opening parenthesis. Such constructs are now flagged
 *								as syntax errors. Also, made keywords case-insensitive
 *								so that "begin" is equivalent to "BEGIN" or "Begin".
 * 13-Feb-81 7.4d   DJE Produced CC only source file. All MS and SHARP code
 *                      has been removed. Name of this file changes to cc.c
 * 04-Mar-81 7.4e   DJE (Beta Test) Fixed bug in cc2d.c dealing with prec().
 *                      prec() was not matching correctly at the beginning
 *                      of the file.
 * 19-Apr-91 7.4f   BLR Fixed a bug which occurred when a group was included
 *								multiple times.
 *								Fixed a bug with prec() command.  This bug only
 *								happened at an odd boundary condition in backbuf.
 *			               Fixed bug with using nl after ifeq(). Was formerly
 *								flagged as an error because nl was not seen as a
 *								string.
 *	20-Sep-91 7.4g   BLR	Added wildcard handling for input and output file
 *								names.  Command line interface is now the only
 *								way to specify files.  No more "Twenty Questions"
 * 27-Sep-91 7.4h   BLR Added -q switch to command line, for quiet mode.
 *								When included on command line, -q suppresses the 
 *								"Output file exists.  OK to replace?" question.
 * 10-Oct-91 7.4i   DJE (Beta Test) Changed FILEWC.C to make output file
 *                      name behavior match RENAME when wildcard chars are
 *                      present.  Added -a (append) switch to append to
 *                      existing output files.
 * 25-Oct-91 7.4j   DJE Increased MAXCHANGES in CC01.H from 2500 to 5000
 *  5-Nov-91 7.4k   DJE Increased size of back buffer from 200 to 1000.
 *                      All msg output was changed from stderr to stdout.
 *                      That way cc output can be redirected to a file.
 *                      The -w switch was added to the command line.  This
 *                      allows the write and wrstore output to directed to
 *                      another file.
 * 7-Nov-91 7.4l    DJE Added ifsubset(store) and len(store) commands.
 *19-Feb-93 7.4n    GET remove show_sf_error & show_pf_error rtns
 *31-Mar-93 7.4o	DJE Move to MSVC v1.0. Fix bug dealing with reporting no input file
 * 1-Apr-93 7b.4p    DJE (No joke!) Put limited prompting for missing inputs back into program.
 *                      If -q switch is used there is no prompting for missing inputs.
 *						ALB says: Actually this apparently was a joke! DJE did not put any prompting back in.
 *							That was actually done in 7.4.09.
 * 24-Oct-94 7b.4r ALB Rolled back in SourceSafe to 7.4p from 7.4q3
 *					7b.4r0 ALB Remove two references to fntarg font stuff that prevented compile
 *									Fix copyright date from 91 to 94
 * 25-Oct-94 7b.4r1 ALB Move VERSION definition to directly under change history
 *									Change all printf to msg_printf in prep for DLL
 *									Change all stdout to msgfile in prep for DLL
 *									Remove day of month from DATE definition as per standard
 *					7b.4r2 ALB Fix CC_007 Incorrect information (about -i) in usage
 *									(Note changed again at 7.4.04.)
 *					7b.4r3 ALB Fix CC_??? New bug of repeat without begin corrupting output
 *									Repeat now stops at the wedge as well as at begin
 *					7b.4r4 ALB Fix CC_??? New bug of write of upper ascii char not correct
 *					7b.4r5 ALB (With Bryan Wussow) Add FUNC and DLL definitions for DLL
 *									Fix code that did illegal read and caused a Windows GPF
 *					7b.4r6 ALB Remove unnecessary header line that said "PC Compatible version"
 * 26 Oct 94 7.4.01 ALB Change version system to Windows standard of three numbers
 *                  7.4.02 ALB Eliminate "Processing file..." messages that had been added to the beta
 *					7.4.03 ALB Add Peter Mielke code to check for valid wildcard pairs
 *									Adds new message: Error: Wildcard mismatch between input \"%s\" and output \"%s\"
 *					7.4.04 ALB CC007 Further fix of usage message wording on -i
 *					7.4.05 ALB Display banner only if no -q
 *									Move comma in banner to after month and year (I also eliminated day earlier)
 *					7.4.06 ALB CC001 Fix concatenation of input files (Put in Peter's fix)
 *					7.4.07 ALB Change inparg and argcnt from char to int in case of lots of files from -i file
 *					7.4.08 ALB Put "Processing..." message back in, but for wildcard only
 *					7.4.09 ALB Add Peter Mielke code to put questions back in, tagged by ifdef NO_QUESTIONS
 *					7.4.10 ALB Fix some bugs in question code
 *					7.4.11 ALB Fix bug of falsely saying output file is full
 *					7.4.12 BDW Work toward implementation of FUNC and DLL versions
 *					7.4.13 ALB Fix bug of falsely saying Cannot open after <RETURN> if no more question
 *					7.4.14 ALB Fix bug in upper ASCII compare, installed Peter's fix    
 *					7.4.15 ALB Fix further bug in upper ASCII write command (in addition to 7b4r4 fix)
 * 25-Jan-95 7.4.16 BJY Fix bug in output from dup and out() in caseless mode         
 * 31-Jan-95 7.4.17 BJY CC020, CC024 Eliminate null pointer assignment error caused by use of
 *								uninitialized file pointers
 * 01-Feb-95 7.4.18 BJY Fix bug of overflowing input buffers with huge filenames
 *					7.4.19 BJY Fix bug of wrong output filename when whitespace entered before filename
 *					7.4.20 BJY Fix bug allowing user to specify output file ending with a backslash
 *					7.4.21 BJY Put "Processing..." message back in for all files  
 * 02-Feb-95 7.4.22 BJY Added beep to additional input file prompt and -n noiseless switch to disable it
 *								as well as disabling the banner and processing... messages
 *					7.4.23 BJY Changed wording in usage() for -i switch
 *					7.4.24 BJY Fix bug that occurs when wildcard in output spec and input spec is entered interactively
 *					7.4.25 BJY Made so * wildcards don't have be in the same position in input and output specs
 *					7.4.26 BJY Added code to reject filenames not separated from their switches by a space
 * 13-Feb-95 7.4.27 BJY Fixed problem with ? wildcard being rejected if any path specified. Also fixed problem
 *								with filespec like a:*.txt (no backslash) failing
 * 14-Feb-95 7.4.28 BJY Fixed numerous wildcard bugs and fixed message displayed when max input files exceeded
 * 16-Feb-95 7.4.29 BJY Fixed additional problem with * wildcard
 * 28-Feb-95 7.4.30 BJY Fixed problem with prec() seeing space before beginning of file and made so prec() works
 *						correctly when placed on the left side of the match string
 * 02-Mar-95 7.4.31 BJY Merged DLL stuff back into main project sources
 * 31-Jul-95 7.5.01 DAR Changed ENDFILE to a command. ENDFILE is no longer marked by a CTRL-Z in the data stream
 *                      but by the end of the last file being read and the match buffer being emptied out.
 * 21-Nov-95 7.6    DRC Added decr (decrement) command, doublebyte support
 * 23-Dec-95 7.6    DRC Added Windows DLL support
 *
 */

#if defined(_WinIO)
#include "windows.h"
#include "winio.h"
#else
#if defined(_WindowsExe)
#include "windows.h"
#endif
#if defined(_WINDLL)
#include <windows.h>
#include <setjmp.h>
#endif

#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>

/* CC Main Program */
#define MAIN       // MUST be defined before cc.h is loaded!
#if defined(UNIX)
#include "wtype.h"
#endif
#include "cc.h"
#include "keybrd.h"

#ifdef _WINDOWS
#include "cc08.h"
#endif

#ifdef HEAP_CHECK
void check_heap(void);
unsigned int heap_size(void);
#endif

/****************************************************************************/
void banner(void)                                 // 7.4.05 ALB
/****************************************************************************/
{
    static int banner_out = 0;

    if ( banner_out )
        return;
    banner_out = 1;
    msg_printf("%s %s%c%s, %s,", PROGRAM, VERSIONMAJOR, '.',
               VERSIONMINOR, DATE);
    msg_puts(" Copyright 1980-2007 SIL Inc.\n");
}

#ifdef _WINDLL

/****************************************************************************/
void  CallCCMainPart ()
/****************************************************************************/
/*                                                                          */
/* This is the main (and only) entry point for when _WINDLL is defined      */
/* This directs the overall flow for CC for when _WINDLL is defined.        */
/*                                                                          */
/*                                                                          */
/* This routine is where the CC DLL Interfaces call the main part of CC     */
/* here when they want to perform the CC functions.  This is called         */
/* here whether the user wants to treat CC (via the DLL) as a front end     */
/* process or a back end process, or whether they want to call it in a      */
/* "Visual Basic style" with just passing data in and out relatively        */
/* simply without callbacks for processing more data.                       */
/*                                                                          */
/*                                                                          */
/*                                                                          */
/* Inputs:              Many global variables                               */
/*                                                                          */
/* Outputs:             Many global variables                               */
/*                                                                          */
/* Results:             CC operations performed                             */
/*                                                                          */
/****************************************************************************/
{

    // this is where we do the DLL processing for the first time DLL is called

    if (bFirstDLLCall == TRUE)
    {
        bFirstDLLCall = FALSE;  // reset for the next time through
        begin_found = FALSE;    // we have not found begin statement yet
        nInSoFar = 0;           // have not processed any data yet
        bEndofInputData = FALSE;// indicate that we still have data to process
        bSavedDblChar = FALSE;  // denote do not have a saved output character
        bSavedInChar = FALSE;   // denote do not have a saved input character

        bAuxBufUsed = FALSE;    // denote aux buffer not allocated or used yet
        nCharsInAuxBuf = 0;     // mark that no space in buffer used as yet
        iAuxBuf= 0;
        hAuxBuf = NULL;         // mark special aux buffer handle as not used
        nAuxLength = 0;         // initialize special aux buffer length to zero
        bBeginExecuted= FALSE;
        eof_written = FALSE;
    }
    else
    {
        // if we one saved char from last time (second half of doublebyte)
        // then output that char first before we go on to any new data
        if ( bSavedDblChar)
        {
            out_char_buf(dblSavedChar);       // output character into buffer
            bSavedDblChar = FALSE;            // turn off flag for next time
        }

        // If we have saved off data before in special auxiliary area, then
        // now go through that and output that data first before getting more
        if (bAuxBufUsed)
        {
            while ((!bOutputBufferFull) && (iAuxBuf < nCharsInAuxBuf))
            {
                out_char_buf( *lpAuxNextToOutput++);
                iAuxBuf++;
            }
            // if we have exhausted this data, then set things to indicate that
            if (iAuxBuf == nCharsInAuxBuf)
            {
                lpAuxNextToOutput = lpAuxBufStart;
                lpAuxOutBuf = lpAuxBufStart;
                bAuxBufUsed = FALSE;
                nCharsInAuxBuf = 0;
                iAuxBuf= 0;
            }
        }
    }

    if (!bOutputBufferFull)
    {
        if(!bBeginExecuted)
            fillmatch();

        if(bBeginExecuted)
            execcc();
    }

    if (bFileErr)           // Return with error message if appropriate
    {
        Process_msg(errorFunction, 30, 0, 0);
        freeMem();
        return ;
    }
}  /* End--MAIN (for Windows DLL) */


#else
#ifdef _WINDOWS

void DoChangesToFile (char *pszCmdLine, char *pszErrBuf)

#else
int main(int argc, char *argv[]) /* Main program */
#endif
/****************************************************************************/
/* Below is the main program (unless _WINDLL is defined, then it is above). */
/****************************************************************************/
{
#ifdef _WINDOWS
    int argc;         /* form argc, argv for 'called as a function' version */
    char ** argv;
#endif

#ifdef _WINDOWS								// Set keyboard & msgfile
    store_area = NULL;   // this will be dynamically allocated
    msgfile = fdMessageFile ();

#else
/* Be sure keyboard is really the keyboard */
keyboard = stdin;
msgfile = stderr;	// DAR 8.1.8
#endif	


#ifdef _WINDOWS
    msg_puts("\
             **************************************************************\n\
             WARNING: This is an alpha test version for use ONLY by\n\
             JAARS ICS and David Bevan. DO NOT DISTRIBUTE!\n\
             **************************************************************\n\
             \n");
#endif

#ifdef ALPHA_TEST
    msg_puts("\
             **************************************************************\n\
             WARNING: This is an alpha test version for use ONLY within the\n\
             computer department at JAARS. DO NOT DISTRIBUTE! Please report\n\
             any bugs to Software Development as soon as possible.\n\
             **************************************************************\n\
             \n");
#endif

#ifdef _WINDOWS
    makeArgv ( pszCmdLine, &argv, &argc);
#endif

    begin_found = FALSE; // denote no begin statement found yet
    bBeginExecuted = FALSE;
    bNeedMoreInput = FALSE;
    utf8encoding = FALSE;

    sysinit(argc);			/* system-specific initialization */

    cmdlnparse(argc, argv);		/* parse command line */

    if ( !noiseless )		// 7.4.22 BJY  changed to -n
    {
        banner();						// 7.4.05 ALB Display banner only if no -q

#if defined(_WindowsExe)
        hWndMessages= GetActiveWindow();

        SetWindowText(hWndMessages, "Consistant Changes for Windows");
#endif
    }

#ifdef _WINDOWS														//7.4.15
    if ( !noiseless )
        msg_printf("command line: %s\n",pszCmdLine);
#endif

#ifdef HEAP_CHECK
    check_heap();
#endif
    tblcreate();      /* allocate memory for table */

    compilecc();      /* Get CC table if there is one */
    /*  and compile it */

    if (errors)						// Return or exit if errors in table
#ifdef _WINDOWS
    {
        strcpy (pszErrBuf, "error");
        CloseMessageFile(msgfile);
        freeMem();
        return 1;
    }
#else			
bailout(BADEXIT, FALSE);
#endif		

#ifdef _WINDOWS
    bFileErr = FALSE;										// BDW 7.4.15
    openfilepair( TRUE );	/* Open input & output file */
    // note: DOS EXE often calls exit() if file err
    if (!bFileErr)
#else
if (openfilepair(TRUE))	// if no err in opening initial file pair
#endif
    {
        do
        {

            startcc();				/* start up consistent change */

            fillmatch();         /* fill up match area, execute begin section */

            execcc();				/* Do CC */

#ifdef _WINDOWS
            if (bFileErr) break;	//err in opening concat input file 7.4.15
#endif
        } while ( openfilepair( FALSE ) );		//next input,output pair
    }

    syscleanup();					/* System dependent cleanup */

#ifdef _WINDOWS
    if (bFileErr)					// Return or exit if errors occurred 7.4.15
    {
        Process_msg(errorFunction, 30, 0, 0);
        strcpy (pszErrBuf, "error");
        CloseMessageFile(msgfile);
        freeMem();
        return 1;
    }
#endif

#ifdef _WINDOWS								//Done - successful! 7.4.15
    if ( !noiseless )
        msg_printf("Changes completed.\n");
    *pszErrBuf = '\0';
    CloseMessageFile(msgfile);
    freeMem();
    return 0;
#else	
if ( !noiseless )
    msg_printf("Changes completed.\n");
bailout(GOODEXIT, FALSE);
	return 0;
#endif
}	/* End--MAIN */
#endif	

#if defined(_WINDOWS) || defined(UNIXSHAREDLIB)
/************************************************************************/
void freeMem()			/* frees memory allocated during execution */
/************************************************************************/
/* This function is used to free up allocated memory after DoChangesToFile()
   (or the DLL routine CallCCMainPart) has completed. While it cleans
   things up well, it may not be absolutely required, since subsequent
   runs of DoChangesToFile (or DLL routine CallCCMainPart) would use the
   already allocated memory.
   This is not an issue for the DOS EXE version, since the exit to DOS 
	automatically frees all allocated memory.
   note: For DoChangesToString(), which will be called repeatedly, we should
	 probably not free the memory each time. However, if strange errors occur
    on subsequent calls to DoChangesToString(), try freeing the mem each time.
*/    
{
    tblfree();
    storefree();
}
#endif

#ifdef HEAP_CHECK
/************************************************************************/
void check_heap()
/************************************************************************/
{
    unsigned s;
    unsigned long total;
    int i;
    void *p[20];

    total = 0L;
    i = 0;
    while ( (s = heap_size()) > 10 )
    {
        total += s;
        p[i++] = malloc(s);
    }

    msg_printf("Available heap = %lu bytes in %d segments\n", total, i);

    for ( ; i; )
        free(p[--i]);
}

/************************************************************************/
unsigned int heap_size()		/* Compute size of remaining heap space */
/************************************************************************/
{
    char *buffer;				/* buffer pointer */
    unsigned int high, low, t;	/* loop controls */

    high = ~0;					/* Highest integer (1's complement of 0) */
    low = 0;					/* Lowest integer */
    while ( low < (high-1) )	/* Loop until only 1-byte difference */
    {
        t = low + (high-low)/2; /* Compute trial buffer size */
        if (buffer = (char *)malloc(t)) /* Attempt to allocate that size */
        {					/* If successful... */
            free(buffer);		/* ...free the buffer and... */
            low = t;			/* ...raise the 'low' value */
        }
        else					/* else... */
            high = t;			/* ...lower the 'high' value */
    }

    /* When finished, the 'low' value is the largest malloc-able buffer */
    return(low);
}
#endif

/* END */

#ifdef OLDCODE
#include <malloc.h>

void check_heap( char *s )
{
    struct _heapinfo hinfo;
    int heapstatus;

    msg_printf("%s:\n", s );
    hinfo._pentry = NULL;
    while ( (heapstatus = _heapwalk( &hinfo ) ) == _HEAPOK )
    {
        /*
        msg_printf("%6s block at %p of size %4.4x\n",
        	(hinfo._useflag == _USEDENTRY ? "USED" : "FREE"),
        	hinfo._pentry, hinfo._size);
        */
        ;
    }
    switch( heapstatus )
    {
    case _HEAPEMPTY:
        msg_printf("OK - heap is empty\n\n");
        break;
    case _HEAPEND:
        msg_printf("OK - end of heap\n\n");
        break;
    case _HEAPBADPTR:
        msg_printf("ERROR - bad pointer to heap\n\n");
        break;
    case _HEAPBADBEGIN:
        msg_printf("ERROR - bad start of heap\n\n");
        break;
    case _HEAPBADNODE:
        msg_printf("ERROR - bad node in heap\n\n");
        break;
    }
}
#endif
