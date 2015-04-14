/**************************************************************************/
/* filewc.h  Doug Case 3/96  This has the Windows-specify variables that are
 *                           in filewc.c.  Filewc.c is used in the old DOS
 *                           command line CC using just variables in cc.h.
 *                           The windows version of CC also uses filewc.c
 *                           for its wildcard processing, and this .h file
 *                           has the windows-specific variable definitions.
 *                           These variables are (and need to be) globals.
 *
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
 * Change History:
 *
 *  15-Mar-96      DRC Original version
 *
 *
 *
/**************************************************************************/

#ifndef GLOBALF
#define GLOBALF
#endif


/* keep these synced up with cc.h unless there is good reason to diverge  */
#define NUMBER_FILES 600
#define PATH_LEN 250             /* allows for a path and file name       */
#if defined(WIN32)
#define PRIMARY_LEN 250
#define EXT_LEN 3
#else
#define PRIMARY_LEN 8
#define EXT_LEN 3
#endif
#define ARRAY_SIZE (PRIMARY_LEN+EXT_LEN+2)
#define ARRAY_SIZE_2 (ARRAY_SIZE-1)


/* filewc.c formerly static variables                                     */
GLOBALF char (*fn_arr_pnt)[ARRAY_SIZE];      /* array of input file names */
GLOBALF char (*pad_arr_pnt)[ARRAY_SIZE_2];   /* used to sort the above    */
GLOBALF char inpath[PATH_LEN];   /* path specification from input file    */
GLOBALF char *inpath_end;        /* pointer to end of input file path     */
GLOBALF char outpath[PATH_LEN];  /* path specification from output file   */
GLOBALF char *outpath_end;       /* pointer to end of output file path    */
GLOBALF int files_read;          /* number of input files to be processed */
GLOBALF int *index;              /* used as a general purpose index       */
GLOBALF int ind_incr;            /* index to next input file              */


/* END */

