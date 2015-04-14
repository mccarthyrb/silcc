/* ccstate.c 12/95  Global Variables saving and restoring functions
 *                  and Windows CC DLL routines.       DRC (Doug Case)
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
 * This module is ONLY for the Windows environment, not for DOS.
 *
 *
 * Global routines in this module include:
 *
 *   HANDLE SaveState( HANDLE hInHandle ) -- This saves he global variables
 *                                 (helps make code reentrant for DLL)
 *
 *   void RestoreState( HANDLE hGlobin) -- Restores all our global variables
 *                                  (helps make code reentrant for DLL)
 *
 *   int EXPDECL CCLoadTable(char *lpszCCTableFile,
 *                                          HANDLE* hpLoadHandle,
 *                                          HINSTANCE hParent) --
 *                                  This is DLL interface code used for the 
 *                                  user to pass the name of the file with
 *                                  the changes table.  This routine has
 *                                  CC do appropriate initialization.  This
 *                                  must be the first DLL routine called.
 *
 *   int EXPDECL CCReinitializeTable(HANDLE hReHandle) --
 *                                  This DLL interface reinitializes the CC
 *                                  table, e.g. when you want the same CC  
 *                                  table to apply to multiple input files.
 *                                  This is a DLL interface routine.
 *
 *   int EXPDECL CCUnloadTable(HANDLE hUnlHandle) --
 *                                  This deallocates all buffers, memory
 *                                  allocated to store, match areas etc, and
 *                                  deallocates global variable structure.
 *                                  This must be the last routine called.
 *                                  This is a DLL interface routine.
 *
 *   int EXPDECL CCSetErrorCallBack(HANDLE hErrHandle, 
 *       int (*lpUserFunc) (short, short unsigned, long unsigned, long *)) --
 *                                  This initialization routine is called
 *                                  by the user to set the error routine
 *                                  of the user that the user wants called
 *                                  in case of errors.  This is optional,
 *                                  and if not called (or if called with
 *                                  NULL) then CC does it own messages.
 *                                  This is a DLL interface routine.
 *
 *   int EXPDECL CCSetUpInputFilter (HANDLE hSetUpHandle,
 *                                         CCInputProc * lpInCBFunct,
 *                                         long lUserInputCBData) --
 *                                  This routine stores the address of the 
 *                                  "input" callback function, etc.
 *                                  This is a DLL interface routine.
 *
 *   int EXPDECL CCGetBuffer (HANDLE hGetHandle,
 *                                char *lpOutputBuffer, int *npOutBufLen) --
 *                                  This routine is the core DLL routine for
 *                                  these interfaces used for when CC is a
 *                                  front end for the routine calling the DLL.
 *                                  It does most of the work, by calling the
 *                                  "main" or "regular" parts of CC.
 *
 *   int EXPDECL CCProcessBuffer (HANDLE hProHandle,
 *                                char *lpInputBuffer, int nInBufLen,
 *                                char *lpOutputBuffer, int *npOutBufLen) --
 *                                  This routine is the core DLL routine for
 *                                  these interfaces when CC is called in
 *                                  "Visual Basic" mode, that is without
 *                                  callbacks, and there is just one buffer of
 *                                  data as input, and CC processes all that
 *                                  and returns the output.  This is called by
 *                                  the user with an input buffer and length
 *                                  and an output buffer and length.  This
 *                                  has CC process one buffer of input, and
 *                                  just return one buffer of output.
 *
 *   int EXPDECL CCMultiProcessBuffer (HANDLE hMultiHandle,
 *                                char *lpInputBuffer, int nInBufLen,
 *                                BOOL bLastCallWithInput,
 *                                char *lpOutputBuffer, int *npOutBufLen) --
 *                                  This routine is the core DLL routine for
 *                                  these interfaces when CC is called in
 *                                  "Visual Basic" mode, that is without
 *                                  callbacks, and there can be multiple
 *                                  calls to this routine.  The user must   
 *                                  keep calling this and sending in data,  
 *                                  and getting data back again, until the 
 *                                  input BOOL flag tells CC that this was 
 *                                  the final call with input data.  
 *
 *   int EXPDECL CCSetUpOutputFilter (HANDLE hOutHandle,
 *                                    CCOutputProc * lpOutCBFunct,
 *                                    long lUserOutputCBData) --
 *                                  This routine stores the address of the 
 *                                  "output" callback function, etc.  This
 *                                  is a CC back end DLL interface routine.
 *
 *   int EXPDECL CCPutBuffer (HANDLE hPutHandle,
 *                                    char *lpPutBuffer, int nInBufLen,
 *                                    BOOL bLastBuffer) --
 *                                  This routine is the core DLL routine for
 *                                  these interfaces used for when CC is a
 *                                  back end for the routine calling the DLL.
 *                                  It does most of the work, by calling the
 *                                  "main" or "regular" parts of CC.
 *
 *   int EXPDECL CCFlush (HANDLE hFlushHandle) --
 *                                  This routine works in conjunction with  
 *                                  the CCPutBuffer routine.  Calling this
 *                                  DLL routine is equivalent to calling the
 *                                  CCPutBuffer with a zero length and the  
 *                                  boolean last buffer indicator set on.
 *
 *   int EXPDECL CCProcessFile (HANDLE hProFileHandle,
 *                                    char *lpInputFile, char *lpOutputFile,
 *                                    BOOL bAppendOutput) --
 *                                  This routine is the core DLL routine for
 *                                  being passed in the names of the input
 *                                  and output files, and using them with CC.
 *
 *
 *
 *
 *
 *                      MODES FOR USING THE CC DLL
 *
 *  There are different modes in which to use the CC DLL and its variety 
 *  of interfaces.  A user must first call the CCLoadTable interface.  The
 *  user can optionally call the CCReinitializeTable and CCSetErrorCallBack
 *  interfaces.  The user would then call a DLL interface mentioned below,
 *  either once or in a loop as appropriate for the interface and usage.
 *  Lastly the user would call the CCUnloadTable interface.
 *
 *
 *                      FILE MODE
 *
 *  This mode does not require the use of callback functions, and so it
 *  works with Visual Basic and Word Basic which do not support callbacks.
 *  The CC DLL interface CCProcessFile is the way to call the CC DLL and
 *  pass in an input file as the CC input, and pass in an output file to
 *  get the CC output.  This is somewhat similar to the operation of the
 *  DOS CC.EXE program operation.
 *
 *
 *                      SINGLE BUFFER MODE
 *
 *  With this mode the user passes the CC DLL a buffer of data for CC
 *  to operate on, and an output buffer.  This can be especially useful if
 *  there are small amounts of data to be operated upon.  This does not
 *  require callbacks. There are two different DLL interfaces for this mode.
 *
 *  The CCProcessBuffer DLL Interface is passed one buffer of input data,
 *  and returns one buffer of output data that CC has completed processing.
 *
 *  The CCMultiProcessBuffer DLL Interface is meant to be called in a loop,
 *  passing one buffer of data in at a time.  This interface is to be called
 *  repeatedly until there is no more output data from CC.
 *
 *
 *                      BUFFERED STREAM MODE
 *
 *  With this mode the CC DLL processes changes on a continual stream of
 *  input data.  This uses callback functions to do this, so it not useful
 *  when using things like Visual Basic and Word Basic that do not support
 *  the use of callback functions.  There are two pairs of CC DLL interfaces
 *  that can be used in this mode.
 *
 *  One way is to use the CC DLL as a front end.  That is the callback
 *  function passes the input data to CC, and the "main" CC DLL interface
 *  is passed the output data.  The CC DLL interface CCSetUpInputFilter
 *  is called to set up the input callback function that the user provides,
 *  and the CC DLL interface CCGetBuffer is called by the user to receive
 *  the output data.
 * 
 *  The other way is to use the CC DLL as a back end.  That is the "main"
 *  DLL interface passes the input data to CC, and the callback function
 *  that is provided by the user accepts that output data from CC.  The
 *  CC DLL interface CCSetUpOutputFilter is called to set up the output
 *  callback function that accepts output data from CC, and the CC DLL
 *  interface CCPutBuffer is the "main" DLL interface to pass input data
 *  to CC.  CCFlush works in conjunction with CCPutBuffer.
 *
 *
 *
 *
 *
 *
 * Change History:
 *  04-Dec-95      DRC Original version, with SaveState(), RestoreState(),
 *                     CCLoadTable(), CCReinitializeTable(), CCUnloadTable(),
 *                     CCSetErrorCallBack(), CCSetUpInputFilter, and
 *                     CCGetBuffer (and CCProcessBuffer added in Jan 1996).
 *                     (CCSetUpOutputFilter and CCPutBuffer added Jan 1996).
 *                     (CCMultiProcessBuffer added in February 1996).
 *                     (CCProcessFile added in March 1996).
 *                     (CCFlush added in April 1996).
 *
 *  29-Feb-96      DRC added many routines related to multi-tasking for the
 *                     DLL.  These are derived from MULTINST DLL sample
 *                     programs with many changes made by Bob Hallissy of SIL.
 *
 */
#if defined(UNIX)
#include <setjmp.h>
#include <ctype.h>
#endif

#ifdef _WinIO
#include "windows.h"
#include "winio.h"
#else
#ifdef _WindowsExe
#include "windows.h"
#endif
#ifdef _WINDLL
#include "windows.h"
#include "setjmp.h"
#endif
#endif

#include <stdio.h>
#include <stdlib.h>

#include <stdarg.h>
#include <assert.h>
#include <string.h>
#if defined(_WINDLL) && !defined(WIN32) && !defined(STATICLINK)
#include "ccmalloc.h"
#elif !defined(UNIX)
#include <malloc.h>
#endif
#if defined(__GNUC__) && __GNUC__ >= 4
#define HAVE_GCCVISIBILITYPATCH 1
#endif

#if defined(WIN32)
	#if defined(__BORLANDC__)
		#define DLLExport
		#define EXPDECL __stdcall _export
	#elif defined(_MAC)
		// Not using dll on Mac
		// Neil Mayhew - 6 Nov 98
		#define __far
		#define _far
		#define _export
		#define DLLExport
		//on Mac, WINAPI is #defined as _cdecl (ref ccdll.h)  8 Apr 99 BDW
		#define EXPDECL _cdecl
	#else
		#define __far
		#define _far
		#define _export
		#define DLLExport __declspec( dllexport )
		#define EXPDECL __stdcall
	#endif
#elif defined(UNIX)
		#define __far
		#define _far
		#define EXPDECL
	#if HAVE_GCCVISIBILITYPATCH
		#define DLLExport __attribute__ ((visibility("default")))
		//#define DLLLocal __attribute__ ((visibility("hidden")))
	#else
		#define DLLExport
		//#define DLLLocal
	#endif
#else
	#define DLLExport
	#define EXPDECL WINAPI _export
#endif

#if defined(UNIX)
#include "wtype.h"
#include "cctio.h"
#endif
#include "ccdll.h"
#include "cc.h"

#if !defined(WIN32) && !defined(STATICLINK) && !defined(UNIX)
WORD LoadInstanceData (void);
static WORD GetNewDS (void);
static WORD LoadDS(WORD wDS);
static void FreeDS(WORD wDS);
#endif

DLLExport int EXPDECL CCUnloadTable(HANDLE hUnlHandle);
static int CleanUp(void);

int WINAPI FillFunction(char *lpCCInputBuffer,
                        int nCCInputBufferLen,
                        long *lpUserInputData);

// define the global variables

#if !defined(WIN32) && !defined(STATICLINK) && !defined(UNIX)
static LPSTR  lpInitialDS = NULL;// pointer to initial values of static variables
static WORD   cbInitialHeapSize;// Size of initial heap
static DWORD  cbInitialDSSize;  // Size of initial Data Segment
static int    nTasks = 0;       // number of tasks currently linked to the DLL
#endif

void Cleanup(void);
int CCLoadTableCore(char *lpszCCTableFile,
                    HANDLE FAR *hpLoadHandle);

#define SaveVariable(x) Global_Vars->x = x
#define SaveArray(x, size) memcpy(Global_Vars->x, x, (size) * sizeof(*x))
/************************************************************************/
HANDLE SaveState(HANDLE hInHandle)    /* save state of global variables */
/************************************************************************/
/*
 * Description:
 *                This routine saves the state of many of CC's global 
 *                variables.  It saves the state of the variables that
 *                are relevent when CC is called from a DLL, it does not
 *                save variables related to input and output files.
 *
 *
 *
 * Input values:
 *                hInHandle == NULL means to allocate/use a new area/handle.
 *
 *                hInHandle == non-NULL means to use that handle.
 *
 *
 *
 * Return values:
 *                0    If an error occurred either allocating or locking
 *                     the space where we wanted to save global variables.
 *
 *                A valid handle for the area with the variables is the
 *                normal successful non-error return value.
 *
 *
 */
{
    int  i;
    HANDLE hGlobal;                  // handle for our alloc'ed space
    void *lpvSpace;                  // ptr to our alloc'ed space
    GLOBAL_VARS_STRUCT *Global_Vars; // globals mapped to alloc'ed space
#if !defined(WIN32) && !defined(STATICLINK) && !defined(UNIX)

    WORD wDS;
    _asm {
        mov ax, ds
        mov wDS, ax
    }
#endif


    if ( hInHandle == NULL )
    {
        hGlobal = GlobalAlloc(GPTR, sizeof(GLOBAL_VARS_STRUCT));  // alloc space

        if ( hGlobal == NULL )
        {
            Process_msg(errorFunction, 63, 0, 0);
            return (NULL);
        }
    }
    else
        hGlobal = hInHandle;

    lpvSpace = (void *)GlobalLock(hGlobal);  // lock space for relevent globals

    if ( lpvSpace == NULL )
    {
        Process_msg(errorFunction, 64, 0, 0);
        return (NULL);
    }
    Global_Vars = (GLOBAL_VARS_STRUCT *) lpvSpace;  // map globals to space

#if !defined(WIN32) && !defined(STATICLINK) && !defined(UNIX)

    SaveVariable(wDS);
#endif


    SaveVariable(sym_table[0]);
    SaveVariable(sym_table[1]);
    SaveVariable(sym_table[2]);
    SaveVariable(sym_table[3]);
    SaveArray(cctpath, PATH_LEN + 1);
    SaveVariable(tblarg);
    SaveVariable(tablefile);
    SaveVariable(bEndofInputData);
    SaveVariable(nextstatusupdate);
    SaveVariable(nullname[0]);

#ifndef NO_QUESTIONS

    SaveVariable(prompt_for_input);
    SaveArray(outnamebuff, 80 + 1);
    SaveArray(innamebuff, 80 + 1);
    SaveArray(ccnamebuff, 80 + 1);
#endif

    // We assume that parsepntr and parse2pntr pt to alloc'ed areas
    SaveVariable(parsepntr);
    SaveVariable(parse2pntr);
    SaveVariable(fontsection);
    SaveVariable(notable);
    SaveVariable(was_math);
    SaveVariable(was_string);
    SaveVariable(table);
    SaveVariable(tablelimit);
    SaveVariable(tloadpointer);
    SaveVariable(maintablend);
    SaveVariable(storelimit);
    SaveArray(switches, MAXARG + 1);

    for ( i = 0; i < NUMSTORES + 1; i++ )
    {
        SaveVariable(storebegin[i]);
        SaveVariable(storend[i]);
        SaveVariable(storeact[i]);
        SaveVariable(storepre[i]);
    }

    SaveVariable(curstor);
    SaveVariable(iStoreStackIndex);
    SaveVariable(storeoverflow);
    SaveVariable(doublebyte1st);
    SaveVariable(doublebyte2nd);

    for ( i = 0; i < MAXGROUPS + 1; i++ )
    {
        SaveVariable(groupbeg[i]);
        SaveVariable(groupxeq[i]);
        SaveVariable(groupend[i]);
    }

    SaveArray(curgroups,  GROUPSATONCE + 1);

    SaveVariable(cgroup);
    SaveVariable(numgroupson);

    SaveArray(letterset, 256);

    SaveVariable(setcurrent);

    SaveArray(defarray, MAXARG + 1);
    SaveArray(stack, STACKMAX + 1);

    SaveVariable(stacklevel);
    SaveVariable(backbuf);
    SaveVariable(backinptr);
    SaveVariable(backoutptr);
    SaveVariable(dupptr);
    SaveVariable(cngpointers);
    SaveVariable(cngpend);
    SaveVariable(match);
    SaveVariable(matchpntr);
    SaveVariable(maxsrch);
    SaveVariable(errors);
    SaveVariable(bFileErr);
    SaveVariable(tblfull);
    SaveVariable(eof_written);
    SaveVariable(debug);
    SaveVariable(mandisplay);
    SaveVariable(mydebug);
    SaveVariable(single_step);
    SaveVariable(caseless);
    SaveVariable(uppercase);
    SaveVariable(unsorted);
    SaveVariable(binary_mode);
    SaveVariable(utf8encoding);
    SaveVariable(doublebyte_mode);
    SaveVariable(doublebyte_recursion);
    SaveVariable(quiet_flag);
    SaveVariable(noiseless);
    SaveVariable(hWndMessages);

    SaveArray(StoreStack, STORESTACKMAX);
    SaveVariable(mchlen[0]);
    SaveVariable(mchlen[1]);
    SaveVariable(matchlength);
    SaveVariable(tblxeq[0]);
    SaveVariable(tblxeq[1]);
    SaveVariable(tblptr);
    SaveVariable(mchptr);
    SaveVariable(matchpntrend);
    SaveVariable(executetableptr);
    SaveVariable(firstletter);
    SaveVariable(cngletter);
    SaveVariable(cnglen);
    SaveVariable(endoffile_in_mch[0]);
    SaveVariable(endoffile_in_mch[1]);
    SaveVariable(endoffile_in_match);
    SaveVariable(store_area);
    SaveArray(keyword, 20 + 1);
    SaveVariable(begin_found);
    SaveVariable(precparse);
    SaveVariable(errorFunction);
    SaveVariable(nInBuf);
    SaveVariable(nInSoFar);
    SaveVariable(bPassedAllData);
    SaveVariable(nMaxOutBuf);
    SaveVariable(nUsedOutBuf);
    SaveVariable(lpInProc);
    SaveVariable(lpOutProc);
    SaveVariable(bMoreNextTime);
    SaveVariable(bNeedMoreInput);
    SaveVariable(bOutputBufferFull);
    SaveVariable(bFirstDLLCall);
    SaveVariable(bBeginExecuted);
    SaveVariable(hInputBuffer);
    SaveVariable(hOutputBuffer);
    SaveVariable(bSavedDblChar);
    SaveVariable(dblSavedChar);
    SaveVariable(bSavedInChar);
    SaveVariable(inSavedChar);
    SaveVariable(bAuxBufUsed);
    SaveVariable(bProcessingDone);
    SaveVariable(bPassNoData);
    SaveVariable(nCharsInAuxBuf);
    SaveVariable(iAuxBuf);
    SaveVariable(nAuxLength);
    SaveVariable(hAuxBuf);
    SaveVariable(lDLLUserInputCBData);
    SaveVariable(lDLLUserOutputCBData);
    SaveVariable(lDLLUserErrorCBData);
    SaveVariable(hTableLineIndexes);
    SaveVariable(iCurrentLine);
    SaveVariable(lpMatchLineCallback);
    SaveVariable(lpExecuteLineCallback);
    SaveVariable(lpInBufStart);
    SaveVariable(lpInBuf);
    SaveVariable(lpOutBuf);
    SaveVariable(lpOutBufStart);

#if !defined(WIN32) && !defined(UNIX)

    if (hTableLineIndexes != 0)
    {
        i = GlobalUnlock(hTableLineIndexes);
        if ( i != 0 )
        {
            char * t= "Table Line Indexes";
            Process_msg(errorFunction, 65, 0, (unsigned long) t);
            return(NULL);
        }
    }
    if ( hInputBuffer != 0 )    // unlock input buffer space if allocated yet
    {
        i = GlobalUnlock(hInputBuffer);
        if ( i != 0 )
        {
            char * t= "Input Buffer";
            Process_msg(errorFunction, 65, 0, (unsigned long) t);
            return(NULL);
        }
    }

    if ( hOutputBuffer != 0 )   // unlock output buffer space if allocated yet
    {
        i = GlobalUnlock(hOutputBuffer);
        if ( i != 0 )
        {
            char * t= "Output Buffer";
            Process_msg(errorFunction, 65, 0,  (unsigned long) t);
            return(NULL);
        }
    }

    if ( hAuxBuf != 0 )       // unlock auxiliary buffer space if allocated
    {
        i = GlobalUnlock(hAuxBuf);
        if ( i != 0 )
        {
            char * t= "Auxilary Buffer";
            Process_msg(errorFunction, 65, 0,  (unsigned long) t);
            return(NULL);
        }
    }


    i = GlobalUnlock(hGlobal);  // unlock space we just stored globals into
    if ( i != 0 )
    {
        char * t= "CC Table Handle";
        Process_msg(errorFunction, 65, 0,  (unsigned long) t);
        return(NULL);
    }
#endif

    return(hGlobal);   // return handle to the area we saved stuff to
}  // End - SaveState

#define RestoreVariable(x) x = Global_Vars->x
#define RestoreArray(x, size) memcpy(x, Global_Vars->x, (size) * sizeof(*x))

/**********************************************************************/
void RestoreState(HANDLE hGlobin)    /* restore our global variables  */
/**********************************************************************/
/*
 * Description:
 *                This routine restores most of CC's global variables.
 *                (It does not store ones that the DLL does not care about,
 *                ones related to input and output files for example).
 *                This routines unlocks and frees the area that the handle
 *                is for after restoring the global variables.
 *
 *
 *
 * Input Value:
 *                hGlobin is a handle for the old global variables that have
 *                been saved there.
 *
 *
 * Output Value:
 *                This routines unlocks the area that the handle
 *                is for after restoring the global variables.
 *
 *
 */
{
    int  i;
    void *lpvSpace;                  // ptr to our space for globals
    GLOBAL_VARS_STRUCT *Global_Vars; // globals mapped to alloc'ed space

    if ( hGlobin == NULL )
    {
        Process_msg(errorFunction, 66, 0, 0);
        return;
    }

    lpvSpace = (void *)GlobalLock(hGlobin);      // lock space for globals

    if ( lpvSpace == NULL )
    {
        Process_msg(errorFunction, 67, 0, 0);
        return;
    }

    Global_Vars = (GLOBAL_VARS_STRUCT *) lpvSpace;  // map globals to space
#if !defined(WIN32) && !defined(STATICLINK) && !defined(UNIX)

    LoadDS(Global_Vars->wDS);
#endif

    RestoreVariable(sym_table[0]);
    RestoreVariable(sym_table[1]);
    RestoreVariable(sym_table[2]);
    RestoreVariable(sym_table[3]);

    RestoreArray(cctpath,  PATH_LEN + 1);
    tblarg = &cctpath[0];
    RestoreVariable(tablefile);
    RestoreVariable(bEndofInputData);
    RestoreVariable(nextstatusupdate);
    RestoreVariable(nullname[0]);

#if TESTNOTNEEDED

    for ( i = 0; i < LINEMAX + 1; i++ )
        line[i]  RestoreVariable(line[i]);
#endif

#ifndef NO_QUESTIONS

    RestoreVariable(prompt_for_input);
    RestoreArray(outnamebuff, 80 + 1);
    RestoreArray(innamebuff, 80 + 1);
    RestoreArray(ccnamebuff, 80 + 1);
#endif

    RestoreVariable(parsepntr);
    RestoreVariable(parse2pntr);
    RestoreVariable(fontsection);
    RestoreVariable(notable);
    RestoreVariable(was_math);
    RestoreVariable(was_string);
    RestoreVariable(table);
    RestoreVariable(tablelimit);
    RestoreVariable(tloadpointer);
    RestoreVariable(maintablend);
    RestoreVariable(storelimit);
    RestoreArray(switches, MAXARG + 1);

    for ( i = 0; i < NUMSTORES + 1; i++ )
    {
        RestoreVariable(storebegin[i]);
        RestoreVariable(storend[i]);
        RestoreVariable(storeact[i]);
        RestoreVariable(storepre[i]);
    }

    RestoreVariable(curstor);
    RestoreVariable(iStoreStackIndex);
    RestoreVariable(storeoverflow);
    RestoreVariable(doublebyte1st);
    RestoreVariable(doublebyte2nd);

    for ( i = 0; i < MAXGROUPS + 1; i++ )
    {
        RestoreVariable(groupbeg[i]);
        RestoreVariable(groupxeq[i]);
        RestoreVariable(groupend[i]);
    }

    RestoreArray(curgroups, GROUPSATONCE + 1);
    RestoreVariable(cgroup);
    RestoreVariable(numgroupson);
    RestoreArray(letterset, 256);
    RestoreVariable(setcurrent);
    RestoreArray(defarray, MAXARG + 1);
    RestoreArray(stack, STACKMAX + 1);
    RestoreVariable(stacklevel);
    RestoreVariable(backbuf);
    RestoreVariable(backinptr);
    RestoreVariable(backoutptr);
    RestoreVariable(dupptr);
    RestoreVariable(cngpointers);
    RestoreVariable(cngpend);
    RestoreVariable(match);
    RestoreVariable(matchpntr);
    RestoreVariable(maxsrch);
    RestoreVariable(errors);
    RestoreVariable(bFileErr);
    RestoreVariable(tblfull);
    RestoreVariable(eof_written);
    RestoreVariable(debug);
    RestoreVariable(mandisplay);
    RestoreVariable(mydebug);
    RestoreVariable(single_step);
    RestoreVariable(caseless);
    RestoreVariable(uppercase);
    RestoreVariable(unsorted);
    RestoreVariable(binary_mode);
    RestoreVariable(utf8encoding);
    RestoreVariable(doublebyte_mode);
    RestoreVariable(doublebyte_recursion);
    RestoreVariable(quiet_flag);
    RestoreVariable(noiseless);
    RestoreVariable(hWndMessages);

    RestoreArray(StoreStack, STORESTACKMAX);
    RestoreVariable(mchlen[0]);
    RestoreVariable(mchlen[1]);
    RestoreVariable(matchlength);
    RestoreVariable(tblxeq[0]);
    RestoreVariable(tblxeq[1]);
    RestoreVariable(tblptr);
    RestoreVariable(mchptr);
    RestoreVariable(matchpntrend);
    RestoreVariable(executetableptr);
    RestoreVariable(firstletter);
    RestoreVariable(cngletter);
    RestoreVariable(cnglen);
    RestoreVariable(endoffile_in_mch[0]);
    RestoreVariable(endoffile_in_mch[1]);
    RestoreVariable(endoffile_in_match);
    RestoreVariable(store_area);
    RestoreArray(keyword, 20 + 1);
    RestoreVariable(begin_found);
    RestoreVariable(precparse);
    RestoreVariable(errorFunction);
    RestoreVariable(nInBuf);
    RestoreVariable(nInSoFar);
    RestoreVariable(bPassedAllData);
    RestoreVariable(nMaxOutBuf);
    RestoreVariable(nUsedOutBuf);
    RestoreVariable(lpInProc);
    RestoreVariable(lpOutProc);
    RestoreVariable(bMoreNextTime);
    RestoreVariable(bNeedMoreInput);
    RestoreVariable(bOutputBufferFull);
    RestoreVariable(bFirstDLLCall);
    RestoreVariable(bBeginExecuted);
    RestoreVariable(hInputBuffer);
    RestoreVariable(hOutputBuffer);
    RestoreVariable(bSavedDblChar);
    RestoreVariable(dblSavedChar);
    RestoreVariable(bSavedInChar);
    RestoreVariable(inSavedChar);
    RestoreVariable(bAuxBufUsed);
    RestoreVariable(bProcessingDone);
    RestoreVariable(bPassNoData);
    RestoreVariable(nCharsInAuxBuf);
    RestoreVariable(iAuxBuf);
    RestoreVariable(nAuxLength);
    RestoreVariable(hAuxBuf);
    RestoreVariable(lDLLUserInputCBData);
    RestoreVariable(lDLLUserOutputCBData);
    RestoreVariable(lDLLUserErrorCBData);
    RestoreVariable(hTableLineIndexes);
    RestoreVariable(iCurrentLine);
    RestoreVariable(lpMatchLineCallback);
    RestoreVariable(lpExecuteLineCallback);

    if ( hTableLineIndexes != 0 )    // lock Table Line Indexes space if allocated yet
    {
        TableLineIndexes = (unsigned *)GlobalLock(hTableLineIndexes); // lock input buffer

        if ( TableLineIndexes == NULL )
        {
            Process_msg(errorFunction, 67, 0, 0);
            return;
        }
    }

    if ( hInputBuffer == 0 )    // lock input buffer space if allocated yet
    {
        RestoreVariable(lpInBufStart);
        RestoreVariable(lpInBuf);
    }
    else
    {
        lpInBufStart = (char*)GlobalLock(hInputBuffer); // lock input buffer

        if ( lpInBufStart == NULL )
        {
            Process_msg(errorFunction, 67, 0, 0);
            return;
        }
        lpInBuf = lpInBufStart + nInSoFar;
    }

    if ( hOutputBuffer == 0 )   // lock output buffer space if allocated yet
    {
        RestoreVariable(lpOutBuf);
        RestoreVariable(lpOutBufStart);
    }
    else
    {
        lpOutBufStart = (char *)GlobalLock(hOutputBuffer);  // lock space for output buffer
        if ( lpOutBufStart == NULL )
        {
            Process_msg(errorFunction, 67, 0, 0);
            return;
        }
        lpOutBuf= lpOutBufStart + nUsedOutBuf;
    }

    if ( hAuxBuf != 0 )       // lock auxiliary buffer space if allocated
    {
        lpAuxBufStart = (char *)GlobalLock(hAuxBuf);  // lock space for aux buffer area
        if ( lpAuxBufStart == NULL )
        {
            Process_msg(errorFunction, 67, 0, 0);
            return;
        }

        lpAuxNextToOutput= lpAuxBufStart + iAuxBuf;
        lpAuxOutBuf= lpAuxBufStart + nCharsInAuxBuf;
    }
    hActiveCCTable= hGlobin;

#if !defined(WIN32) && !defined(UNIX)

    i = GlobalUnlock(hGlobin);  // unlock the space we had stored globals in

    if ( i != 0 )
        Process_msg(errorFunction, 68, 0, 0);
#endif
}  // End - RestoreState

int CCLoadTableFromBufferA(char *lpszBuffer,
                           HANDLE FAR *hpLoadHandle)
{
    int rc;
#if !defined(WIN32) && !defined(STATICLINK)&& !defined(UNIX)

    WORD wDS;
#endif

    HANDLE hCCTableBuffer;

    if ( hpLoadHandle == NULL )
    {
        Process_msg(errorFunction, 98, 0, 0);
        return(-1);
    }

#if !defined(WIN32) && !defined(STATICLINK) && !defined(UNIX)
    if ((wDS= GetNewDS()) == 0)
    {
        Process_msg(errorFunction, 114, 0, 0);
        return(-1);
    }

    if (!LoadDS(wDS))
    {
        Process_msg(errorFunction, 134, 0, 0);
        return(-1);
    }
#endif

    hCCTableBuffer= GlobalAlloc(GPTR, strlen(lpszBuffer) + 1);

    lpszCCTableBuffer= (char *)GlobalLock(hCCTableBuffer);

    strcpy(lpszCCTableBuffer, lpszBuffer);


    rc= CCLoadTableCore(NULL, hpLoadHandle);
#if !defined(WIN32) && !defined(UNIX)

    GlobalUnlock(hCCTableBuffer);
#endif

    GlobalFree(hCCTableBuffer);

#if !defined(WIN32) && !defined(STATICLINK) && !defined(UNIX)

    if (rc != CC_SUCCESS && rc != CC_SUCCESS_BINARY)
        FreeDS(wDS);
#endif

    return rc;
}

DLLExport int EXPDECL CCLoadTableFromBufferWithErrorCallback(char *lpszBuffer,
        HANDLE FAR *hpLoadHandle,
        CCCompileErrorCallback * lpCCCompileErrorCallback)
{
    lpCompileErrorCallback= lpCCCompileErrorCallback;
    return CCLoadTableFromBufferA(lpszBuffer, hpLoadHandle);
}

DLLExport int EXPDECL CCLoadTableFromBuffer(char *lpszBuffer,
        HANDLE FAR *hpLoadHandle)
{
    lpCompileErrorCallback= NULL;
    return CCLoadTableFromBufferA(lpszBuffer, hpLoadHandle);
}


/**************************************************************************/
DLLExport int EXPDECL CCLoadTable(char *lpszCCTableFile,
                                  HANDLE FAR *hpLoadHandle,
                                  HINSTANCE hParent)
{
    int rc;
#if !defined(WIN32) && !defined(STATICLINK) && !defined(UNIX)

    WORD wDS;
#endif

    if ( hpLoadHandle == NULL )
    {
        Process_msg(errorFunction, 98, 0, 0);
        return(-1);
    }

    if ( lpszCCTableFile == NULL)
    {
        Process_msg(errorFunction, 69, 0, 0);
        return(-1);
    }

#if !defined(WIN32) && !defined(STATICLINK) && !defined(UNIX)
    if ((wDS= GetNewDS()) == 0)
    {
        Process_msg(errorFunction, 114, 0, 0);
        return(-1);
    }

    if (!LoadDS(wDS))
    {
        Process_msg(errorFunction, 134, 0, 0);
        return(-1);
    }
#endif
    lpCompileErrorCallback= NULL;
    rc= CCLoadTableCore(lpszCCTableFile, hpLoadHandle);

#if !defined(WIN32) && !defined(STATICLINK) && !defined(UNIX)

    if (rc != CC_SUCCESS && rc != CC_SUCCESS_BINARY)
        FreeDS(wDS);
#endif

    return rc;
}

DLLExport int EXPDECL CCLoadTable2(char *lpszCCTableFile,
                                  HANDLE FAR *hpLoadHandle)
{
	return CCLoadTable(lpszCCTableFile, hpLoadHandle, NULL);
}


int CCLoadTableCore(char *lpszCCTableFile,
                    HANDLE FAR *hpLoadHandle)
/**************************************************************************/
/*
 * Description:
 *                This DLL interface is passed a string with the name of 
 *                the CC table to be used.  It performs some CC processing
 *                to start things up with that CC table, and then it returns
 *                a handle for the area with the global variables after that.
 *                This needs to be the first CC DLL interface called.  This
 *                routine also registers the calling task.
 *
 *
 * Input values:
 *                *lpszCCTableFile has the name of the CC Table to be used.
 *
 *                hParent has the hinstance (handle) of the invoking task
 *
 *
 * Output values:
 *                *hpLoadHandle points to the handle for CC variables.
 *
 *
 * Return values:
 *                CC_SUCCESS          If everything is fine, files are to be
 *                                    opened in the usual text mode.
 *
 *                CC_SUCCESS_BINARY   If everything is fine, files are to be
 *                                    opened in binary mode.
 *
 *                non-zero            Indicates an error occurred.
 *
 *
 */
{

    if (setjmp(abort_jmp_buf) != 0)
    {
        if (tablefile)
        {
            wfclose(tablefile);						/* recover file BUFFER */
            tablefile= NULL;
        }

        CleanUp();
        *hpLoadHandle= NULL;
        return (-1);
    }

    if (lpszCCTableFile)
    {
        lpszCCTableBuffer= NULL;

        if ( *lpszCCTableFile == '\0' )
        {
            Process_msg(errorFunction, 69, 0, 0);
            return(-1);
        }

        // make sure that path is not too long, then copy it into our
        // space so we are not dependent on the user keeping it around
        if ( (strlen(lpszCCTableFile) + 1) > sizeof(cctpath) )
        {
            Process_msg(errorFunction, 135, 0, (long unsigned) lpszCCTableFile);
            return(-1);
        }
        else
            strcpy(cctpath, lpszCCTableFile);
    }

    // initialize variables to NULL here that are initialized to NULL
    // in cc.h for the non-DLL case

    tablefile = NULL;
    parsepntr = NULL;
    parse2pntr = NULL;
    tablelimit = NULL;
    tloadpointer = NULL;
    maintablend = NULL;
    storelimit = NULL;
    backbuf = NULL;
    backinptr = NULL;
    backoutptr = NULL;
    dupptr = NULL;
    cngpointers = NULL;
    cngpend = NULL;
    match = NULL;
    matchpntr = NULL;
    matchpntrend = NULL;
    executetableptr= NULL;
    lpOutBuf = NULL;
    lpOutBufStart = NULL;
    lpInBuf = NULL;
    lpInBufStart = NULL;
    lDLLUserInputCBData = 0;
    lDLLUserOutputCBData = 0;
    lDLLUserErrorCBData = 0;
    lpInProc= NULL;
    lpOutProc= NULL;
    lpMatchLineCallback= NULL;
    lpExecuteLineCallback= NULL;
    table = NULL;
    store_area = NULL;
    binary_mode = FALSE;
    utf8encoding = FALSE;
    doublebyte_mode = FALSE;
    errorFunction = NULL;      // initialize, CCSetErrorCallBack may reset

    errors = FALSE;            // no CC errors yet
    bProcessingDone = FALSE;   // have not passed user completion retcode
    bPassNoData = FALSE;       // do not want user to pass no data in
    bPassedAllData = FALSE;    // have not yet received all input data
    hInputBuffer = 0;          // have not yet allocated input buffer area
    hOutputBuffer = 0;         // have not yet allocated output buffer
    hAuxBuf = NULL;            // have not yet allocated aux buffer area
    nInBuf = 0;                  // set input buffer length
    nInSoFar= 0;
    nullname[0] = '\0';        // initialize some variables here
    noiseless = TRUE;
    begin_found = FALSE;
#ifndef NO_QUESTIONS

    prompt_for_input = false;
#endif

    bMoreNextTime = FALSE;     // denote we have no data waiting to be processed
    bFirstDLLCall = TRUE;      // initialize to the first call
    bBeginExecuted = FALSE;

    tblarg = &cctpath[0];      // set up input for next calls

    hTableLineIndexes= GlobalAlloc(GPTR, MAXTABLELINES * sizeof(unsigned));

    if (hTableLineIndexes == NULL)
    {
        Process_msg(errorFunction, 78, 0, 0);
        return(-1);
    }

    TableLineIndexes= (unsigned *)GlobalLock(hTableLineIndexes);

    iCurrentLine= 0;

    tblcreate();               // allocate memory for CC table

    compilecc();               // get cc table and compile it

#if(!defined(WIN32)) && !defined(UNIX)

    GlobalUnlock(hTableLineIndexes);
#endif

    hTableLineIndexes= GlobalReAlloc((void *)hTableLineIndexes, (iCurrentLine + 1) * sizeof(unsigned), GPTR);

    if (hTableLineIndexes == NULL)
    {
        Process_msg(errorFunction, 78, 0, 0);
        return(-1);
    }

    TableLineIndexes= (unsigned *)GlobalLock(hTableLineIndexes);

    if (!errors)
        startcc();

    if ( !errors )
        *hpLoadHandle = SaveState(NULL);  // alloc space and save global variables
    else
    {
        CleanUp();
        *hpLoadHandle= NULL;
        Process_msg(errorFunction, 70, 0, 0);
        return(CC_SYNTAX_ERROR);
    }

    if ( binary_mode )
        return(CC_SUCCESS_BINARY);     // return success, binary mode
    else
        return(CC_SUCCESS);            // return success, text mode

}  // End - CCLoadTable

/**************************************************************************/
DLLExport int EXPDECL CCReinitializeTable(HANDLE hReHandle)
/**************************************************************************/
/*
 * Description:
 *               This DLL routine is called to reinitialize CC without   
 *               starting over totally from scratch.  E.g. use this with
 *               new data, but using same change table.  It uses the global
 *               variables that are in the area pointed to by the handle.
 *
 *
 *
 * Input values:
 *               hReHandle is the handle pointing to the global variables.
 *
 *
 *
 * Output values:
 *               The global variables referenced by hReHandle are updated.
 *
 *
 *
 *
 * Return values:
 *                0        If everything is fine
 *
 *                non-zero indicates an error occurred
 *
 *
 *
 */
{
    register int i;     // Loop index for initializing storage pointers
    unsigned storemax;


    if ( hReHandle == NULL )
    {
        Process_msg(errorFunction, 71, 0, 0);
        return(-1);
    }

    RestoreState(hReHandle);  // this restores our global variables

    if (setjmp(abort_jmp_buf) != 0)
    {
        SaveState(hReHandle);
        return (-1);
    }

    storemax = max_heap() / sizeof(SSINT);

    hAuxBuf = NULL;           // denote have not yet allocated aux buffer area

    bMoreNextTime = FALSE;    // denote have no data waiting to be processed
    bFirstDLLCall = TRUE;     // initialize to the first call
    bBeginExecuted = FALSE;
    bProcessingDone = FALSE;  // denote have not passed user completion rc
    bPassNoData = FALSE;      // denote do not want user to pass no data in
    bSavedInChar = FALSE;     // denote we do not have saved input character
    bSavedDblChar = FALSE;    // denote we do not have saved output character
    lpOutBuf = lpOutBufStart; // set to NULL or start of buffer as appropriate
    bOutputBufferFull= FALSE;
    nInBuf = 0;                  // set input buffer length
    nInSoFar= 0;

    // This starts a section that is basically a subset of the startcc()
    // routine that is in ccexec.c.  The difference is that we do not free
    // and then just re-malloc the storage area again, we just keep it.

    bytset(switches, FALSE, MAXARG+1);     // Clear all switches

    storemax = max_heap() / sizeof(SSINT);
    if ( store_area != NULL )
        free(store_area);
    store_area = (SSINT *) tblalloc(storemax, (sizeof(SSINT)));
    storelimit = store_area + storemax;

    for ( i = 0; i <= NUMSTORES; i++ )     // Initialize storage pointers
    {
        storebegin[i] = storend[i] = store_area;
        storeact[i] = FALSE;
        storepre[i] = 0;
    }

    curstor = 0;                           // Clear storing and overflow
    storeoverflow = FALSE;
    iStoreStackIndex = 0;
    setcurrent = FALSE;                    // Letterset not current
    eof_written = FALSE;                   // We haven't written EOF yet
    numgroupson = 0;

    if ( groupbeg[1] != 0 )
        groupinclude(1);                    // Start in group 1, if it exist

    backinptr = backoutptr = backbuf;      // Initialize backup buffer
    ssbytset( backbuf, (SSINT)' ',
              BACKBUFMAX);                 // make spaces so debug looks OK

    SaveState(hReHandle);     // save global variables into saved area

    if ( errors )
    {
        Process_msg(errorFunction, 72, 0, 0);
        return(-1);
    }

    return(0);

}  // End - CCReinitializeTable

static int CleanUp(void)
{
    int retcode;
#if !defined(UNIX)
    int rc;
#endif
    retcode = 0;

    freeMem();                 // free all allocated memory other than globals

#if !defined(WIN32) && !defined(UNIX)

    if (hTableLineIndexes != 0)
    {
        rc = GlobalUnlock(hTableLineIndexes);
        if ( rc != 0 )
        {
            Process_msg(errorFunction, 74, 0, 0);
            retcode = -1;
        }
    }
    if ( hInputBuffer != 0 )   // unlock input buffer space if allocated yet
    {
        rc = GlobalUnlock(hInputBuffer);
        if ( rc != 0 )
        {
            Process_msg(errorFunction, 74, 0, 0);
            retcode = -1;
        }
    }

    if ( hOutputBuffer != 0 )  // unlock output buffer space if allocated yet
    {
        rc = GlobalUnlock(hOutputBuffer);
        if ( rc != 0 )
        {
            Process_msg(errorFunction, 74, 0, 0);
            retcode = -1;
        }
    }

    if ( hAuxBuf != 0 )        // unlock auxiliary buffer space if allocated
    {
        rc = GlobalUnlock(hAuxBuf);
        if ( rc != 0 )
        {
            Process_msg(errorFunction, 74, 0, 0);
            retcode = -1;
        }
    }
#endif

    if (hTableLineIndexes != 0)
    {
#if defined(UNIX)
        GlobalFree(hTableLineIndexes);
#else
        rc = (int)GlobalFree(hTableLineIndexes);
        if ( rc != 0 )
        {
            Process_msg(errorFunction, 75, 0, 0);
            retcode = -1;
        }
#endif
    }

    if ( hInputBuffer != 0 )
    {
#if defined(UNIX)
        GlobalFree(hInputBuffer);    // free space now that we are done with it
#else
        rc = (int)GlobalFree(hInputBuffer);    // free space now that we are done with it
        if ( rc != 0 )
        {
            Process_msg(errorFunction, 75, 0, 0);
            retcode = -1;
        }
#endif
	}

    if ( hOutputBuffer != 0 )
    {
#if defined(UNIX)
        GlobalFree(hOutputBuffer);    // free space now that we are done with it
#else
        rc = (int)GlobalFree(hOutputBuffer);    // free space now that we are done with it
        if ( rc != 0 )
        {
            Process_msg(errorFunction, 75, 0, 0);
            retcode = -1;
        }
#endif
	}

    if ( hAuxBuf != 0 )
    {
#if defined(UNIX)
        GlobalFree(hAuxBuf);    // free space now that we are done with it
#else
        rc = (int)GlobalFree(hAuxBuf);    // free space now that we are done with it
        if ( rc != 0 )
        {
            Process_msg(errorFunction, 75, 0, 0);
            retcode = -1;             // return a non-zero return code
        }
#endif
	}

    return(retcode);
}

/**************************************************************************/
DLLExport int EXPDECL CCUnloadTable(HANDLE hUnlHandle)
/**************************************************************************/
/*
 * Description:
 *                This DLL routine is called when the user is done with CC
 *                to free the global variable structure area and other stuff.
 *                This needs to be the last CC DLL interface called.  This
 *                routine also unregisters the calling task.
 *
 *
 *
 * Input Value:
 *                hUnlHandle is the handle for the area to be freed.
 *
 *
 *
 * Output Value:
 *                The area for hUnlHandle etc are unlocked and freed.
 *
 *
 *
 * Return values:
 *                0          Successful completion.
 *                          
 *                non-zero   An error occurred.
 *
 *
 */
{
#if !defined(UNIX)
    int rc; 	
#endif
    int retcode;
#if !defined(WIN32) && !defined(STATICLINK) && !defined(UNIX)

    WORD wDS;
    WORD wIDS;
#endif

    if ( hUnlHandle == NULL )
    {
        Process_msg(errorFunction, 73, 0, 0);
        return(-1);
    }

#if !defined(WIN32) && !defined(STATICLINK) && !defined(UNIX)
    // this assembler code added by Doug Rintoul to get the correct segment
    _asm{
        mov ax, DS
        mov wDS, ax
    }
#endif

    RestoreState(hUnlHandle);  // this restores the global variable structure

    retcode= CleanUp();

#if !defined(WIN32) && !defined(STATICLINK) && !defined(UNIX)

    _asm{
        mov ax, ds
        mov wIDS, ax
        mov ax, wDS
        mov DS, ax
    }

    FreeDS(wIDS);
#endif
#if defined(UNIX)
    GlobalFree(hUnlHandle);    // free space now that we are done with it
#else
    rc = (int)GlobalFree(hUnlHandle);    // free space now that we are done with it

    if ( rc != 0 )
    {
        Process_msg(errorFunction, 75, 0, 0);
        retcode = -1;                // return a non-zero return code
    }
#endif
    return(retcode);

}   // End - CCUnloadTable

DLLExport int EXPDECL CCSetDebugCallbacks(HANDLE hCCTHandle,
        CCMatchLineCallback * lpCCMatchLineCallback,
        CCExecuteLineCallback * lpCCExecuteLineCallback)
{
    if ( hCCTHandle == NULL )
    {
        Process_msg(errorFunction, 76, 0, 0);
        return(-1);
    }

    RestoreState(hCCTHandle);  // this restores our global variables

    lpMatchLineCallback= lpCCMatchLineCallback;
    lpExecuteLineCallback= lpCCExecuteLineCallback;

    SaveState(hCCTHandle);     // save global variables into saved area

    return(0);

}  // End - CCSetDebugCallbacks

DLLExport BOOL EXPDECL CCQuerySwitch(HANDLE hCCTable, char * pszSwitchName)
{
    int iSwitch;
    BOOL fReturnValue= FALSE;

#if !defined(DEBUGLIB)

    RestoreState(hCCTable);  // this restores our global variables
#endif

    iSwitch= get_symbol_number(pszSwitchName, SWITCH_HEAD);

    if (iSwitch != -1 && switches[iSwitch])
        fReturnValue= TRUE;

#if !defined(DEBUGLIB)

    SaveState(hCCTable);
#endif

    return fReturnValue;
}


DLLExport int EXPDECL CCQueryStore(HANDLE hCCTable, char * pszStoreName, char * pszValue, unsigned nLenValue)
{
    int iStore;
    unsigned lenStore;
    unsigned iStoreBuffer;
    unsigned iValue = 0;
    int return_code = 0;
    SSINT tempch;
    SSINT cha;

#if !defined(DEBUGLIB)

    RestoreState(hCCTable);  // this restores our global variables
#endif

    iStore= get_symbol_number(pszStoreName, STORE_HEAD);

    if (iStore == -1)
    {
        *pszValue= '\0';
        SaveState(hCCTable);
        return FALSE;
    }
    else
    {
        lenStore = storend[iStore] - storebegin[iStore];		/* Get # of chars in store */

        // if nothing in the store and predefined flag is non-zero then
        // we are to output one of the predefined output values
        if (( lenStore == 0 ) && ( storepre[iStore] != 0 ))
        {
            resolve_predefined(iStore);

            lenStore= strlen(predefinedBuffer);

            if (lenStore > nLenValue - 1)
                lenStore= nLenValue - 1;

            for ( iStoreBuffer = 0; iStoreBuffer < lenStore; iStoreBuffer++)
                pszValue[iStoreBuffer]= predefinedBuffer[iStoreBuffer];
        }
        else
        {
            iValue= 0;

            for ( iStoreBuffer = 0; iStoreBuffer < lenStore; iStoreBuffer++ )
            {
                tempch = *(storebegin[iStore] + iStoreBuffer);	// 7.4.16

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
                    if (iValue == nLenValue - 1)
                    {
                        return_code= -1;
                        break;
                    }

                    cha = (tempch >> 8) & 0x00ff;  /* get hi order half of doublebyte char */
                    tempch = tempch & 0x00ff; /* keep low order half here  */

                    pszValue[iValue]= (char) (cha & 0xFF);              /* output high order half first */
                    iValue++;

                    if ((cha < doublebyte1st) ||
                            ((cha == doublebyte1st) && (tempch < doublebyte2nd)))
                        Process_msg(errorFunction, 17, 0, 0);
                }

                if (iValue == nLenValue - 1)
                {
                    return_code= -1;
                    break;
                }

                pszValue[iValue]= (char)(tempch & 0xFF);
                iValue++;
            }
        }
        pszValue[iValue]= '\0';
    }
#if !defined(DEBUGLIB)
    SaveState(hCCTable);
#endif

    if (return_code != -1)
        return strlen(pszValue);
    else
        return -1;
}

DLLExport int EXPDECL CCGetActiveStore(HANDLE hCCTable, char * pszActiveStore, unsigned nLenActiveStore)
{
    if (curstor == 0)
        strcpy(pszActiveStore, "none");
    else
        strcpy(pszActiveStore, sym_name(curstor, STORE_HEAD));

    return strlen(pszActiveStore);
}

DLLExport int EXPDECL CCGetActiveGroups(HANDLE hCCTable, char * pszActiveGroups, unsigned nLenActiveGroups)
{
    int iGroup;
    char * pszGroup;
    int return_code= 0;

#if !defined(DEBUGLIB)

    RestoreState(hCCTable);  // this restores our global variables
#endif

    *pszActiveGroups= '\0';

    if (!bBeginExecuted)
        strcpy(pszActiveGroups, "begin statement");
    else
    {
        for (iGroup = 1; iGroup <= numgroupson; iGroup++)
        {
            pszGroup = sym_name(curgroups[iGroup], GROUP_HEAD);  /* Get symbolic name */

            if (strlen(pszGroup) + strlen(pszActiveGroups) < nLenActiveGroups - 1)
            {
                strcat(pszActiveGroups, pszGroup);
                strcat(pszActiveGroups, " ");
            }
            else
                return_code= -1;

        }
    }

#if !defined(DEBUGLIB)
    SaveState(hCCTable);
#endif

    if (return_code != -1)
        return strlen(pszActiveGroups);
    else
        return -1;

}

int buffcpy(char * dest, SSINT * source, int nChars, int nDestinationBufferSize)
{
    int nBytesCopied;

    for (nBytesCopied = 0;
            nChars > 0 && nBytesCopied < nDestinationBufferSize;
            source++, nChars--)
    {

        if (doublebyte_mode && ((*source & 0xff00) != 0))
        {
            *dest++= ((*source) >> 8) & 0x00ff;

            nBytesCopied++;

            if (nBytesCopied == nDestinationBufferSize)
                break;

        }


        *dest++= *source & 0x00ff;

        nBytesCopied++;

        if (nBytesCopied == nDestinationBufferSize)
            break;

        if (*source == '\r')
        {
            *dest++= '\n';
            nBytesCopied++;
        }

    }

    *dest= '\0';

    return nBytesCopied;
}

DLLExport int EXPDECL CCQueryInput(HANDLE hCCTable, char * pszInputBuffer, unsigned nLenBuffer)
{
    int MatchStringLength;

#if !defined(DEBUGLIB)

    RestoreState(hCCTable);  // this restores our global variables
#endif

    if (dupptr)
        MatchStringLength= buffcpy(pszInputBuffer, dupptr, matchlength, nLenBuffer);
    else
        MatchStringLength= buffcpy(pszInputBuffer, matchpntr - matchlength, matchlength, nLenBuffer);

    buffcpy(pszInputBuffer + MatchStringLength, matchpntr, matchpntrend - matchpntr, nLenBuffer - MatchStringLength);

#if !defined(DEBUGLIB)

    SaveState(hCCTable);
#endif

    return MatchStringLength;
}

DLLExport int EXPDECL CCQueryOutput(HANDLE hCCTable, char * pszOutputBuffer, unsigned nLenBuffer)
{
    SSINT * pBuff;
    char *pStart;
    unsigned nBytesInBackBuffer;
    pStart= pszOutputBuffer;

#if !defined(DEBUGLIB)

    RestoreState(hCCTable);  // this restores our global variables
#endif

    *pszOutputBuffer= '\0';

    // first determine the number of bytes in the backbuffer taking into account the double byte mode

    for (nBytesInBackBuffer= 0, pBuff= backoutptr;
            pBuff != backinptr;)
    {
        if ((doublebyte_mode && ((*pBuff & 0xff00) != 0)) ||
                (*pBuff == '\r'))
            nBytesInBackBuffer+= 2;
        else
            nBytesInBackBuffer++;

        pBuff++;


        if (pBuff >= backbuf+BACKBUFMAX)
            pBuff= backbuf;
    }

    for (pBuff= backoutptr; nBytesInBackBuffer > 0;)
    {
        if (doublebyte_mode && ((*pBuff & 0xff00) != 0))
        {
            if (nBytesInBackBuffer <= nLenBuffer)
                *pszOutputBuffer++= ((*pBuff) >> 8) & 0x00ff;
            nBytesInBackBuffer--;
        }

        if (nBytesInBackBuffer <= nLenBuffer)
            *pszOutputBuffer++= (*pBuff) & 0x00ff;

        nBytesInBackBuffer--;

        if (*pBuff == '\r')
        {
            if (nBytesInBackBuffer <= nLenBuffer)
                *pszOutputBuffer++= '\n';

            nBytesInBackBuffer--;
        }

        pBuff++;

        if (pBuff >= backbuf+BACKBUFMAX)
            pBuff= backbuf;
    }

#if !defined(DEBUGLIB)
    SaveState(hCCTable);
#endif

    *pszOutputBuffer= '\0';

    return strlen(pStart);

}
/**************************************************************************/
DLLExport int EXPDECL CCSetUTF8Encoding(HANDLE hCCTableHandle,
                                        bool lutf8encoding)
/**************************************************************************/
/*
 * Description:
 *                This DLL routine is called by the user to enable or disable
 *                utf-8 character handling.
 *
 *
 * Input values:
 *                hCCTableHandle has our input handle for global variables.
 *
 *                lutf8encoding is set to TRUE to enable UTF-8 character
 *                processing and set to FALSE to disable UTF-8 character.
 *                note the the "u" encoding directive will still expand 
 *                a USC4 character to UTF-8 within the cc table even if
 *                UTF-8 processing is disabled.
 *
 * Output values: The global variable utf8encoding is updated,
 *
 * Return values:
 *                0           Success.
 *                
 *                non-zero    Error occurred.
 *
 *
 */
{

    if ( hCCTableHandle == NULL )
    {
        Process_msg(errorFunction, 138, 0, 0);
        return(-1);
    }

    RestoreState(hCCTableHandle);  // this restores our global variables

    utf8encoding= lutf8encoding;

    SaveState(hCCTableHandle);     // save global variables into saved area

    return(0);

}  // End - CCSetUTF8Encoding

/**************************************************************************/
DLLExport int EXPDECL CCSetErrorCallBack(HANDLE hErrHandle,
        CCErrorCallback * lpUserFunc)
/**************************************************************************/
/*
 * Description:
 *                This DLL routine is called by the user to pass to CC the
 *                address of an error callback routine that the user wants
 *                CC to call if CC has an error.  This routine stores the
 *                address of that routine as a CC global variable so that
 *                it can be used during the execution of CC.  If the input
 *                parm is NULL (or if this optional routine is not called) 
 *                then we use the default CC error checking routine.
 *
 *                NOTE: file ccerror.h needs to be included by a user who
 *                      writes thei own error handler.  ccerror.h has many
 *                      comments in it describing to the user how they
 *                      can write their own error handler.
 *
 *
 * Input values:
 *                hErrHandle has our input handle for global variables.
 *
 *                lpUserFunc is a pointer to the user's error routine.  If
 *                this is NULL we use the default CC error routine
 *                (which is an int function with a short, short unsigned, 
 *                and long unsigned parameters as input, and a long pointer
 *                which may contain user-defined data).
 *
 *
 *
 *
 * Output values: The global variables referenced by hErrHandle are updated,
 *                CC variable errorFunction is set to either NULL, or to 
 *                point to the user's error processing routine.  If passed 
 *                in NULL here (or defaults to NULL if this is never called)
 *                then we use the CC default error handler Process_msg().
 * 
 *
 *
 * Return values:
 *                0           Success.
 *                
 *                non-zero    Error occurred.
 *
 *
 */
{

    if ( hErrHandle == NULL )
    {
        Process_msg(errorFunction, 76, 0, 0);
        return(-1);
    }

    RestoreState(hErrHandle);  // this restores our global variables

    errorFunction = lpUserFunc;

    SaveState(hErrHandle);     // save global variables into saved area

    return(0);

}  // End - CCSetErrorCallBack

/**************************************************************************/
DLLExport int EXPDECL CCSetUpInputFilter (HANDLE hSetUpHandle,
        CCInputProc * lpInCBFunct,
        long lUserInputCBData)
/**************************************************************************/
/*
 * Description:
 *                This DLL routine is called by the user to set up the input
 *                filter.  The user's input filter routine is then
 *                called iteratively as needed by CC's
 *                CCGetBuffer DLL routine to pass input data to CC.
 *                This also allocates the input area that CCGetBuffer
 *                tells the input filter to fill up with data.
 *
 *
 * Input values:
 *                hSetUpHandle  This points to our global variable struct.
 *
 *
 *                lpInCBFunct   Points to callback routine used to get input.
 *                              This is called from the CCGetBuffer routine.
 *
 *                lUserInputCBData  This is input from the user to be saved
 *                                  by CC and passed to the input callback
 *                                  function everytime that it is called.
 *                                  The usage of this is up to the user.
 *                                  (One example of how to use it would be
 *                                   to pass it to this routine as zero, and
 *                                   then test it in the callback routine and
 *                                   then know it is the first call the   
 *                                   appropriate file can be opened).
 *
 *    Callback function:        The routine this points to has two input
 *                              parameters, one is char *, the other
 *                              is int.
 *                              One input value is the char *, which
 *                              points to a CC buffer.  That is where the
 *                              callback function is to put the data.
 *                              The other input value is the int, which tells
 *                              the callback function the maximum number of
 *                              bytes to place in the buffer.
 *
 *                              For output the callback function puts data
 *                              into the passed buffer.
 *                  
 *                              The int return value from the callback is
 *                              the number of bytes placed into the buffer.
 *                      
 *                              Note that CC assumes that if the number of
 *                              bytes returned is less than the passed in
 *                              maximum number of bytes to place in the
 *                              buffer then that means there is no more data
 *                              to get ("end of file" so to speak).
 *
 *                              Note that if an error occurs in the input
 *                              filter callback routine it is the user's
 *                              responsibility to handle that error.  If
 *                              the user routine encopunters an error then
 *                              the user should return zero bytes to CC so it
 *                              looks to CC like the input is exhausted.
 *
 *
 *
 *
 *
 * Output values: The global variables referenced by hSetUpHandle are updated
 *                to reflect the callback routine that gets CC input data.
 *
 *
 *
 * Return values:
 *                0           Success.
 *                
 *                non-zero    Error occurred.
 *
 *
 */
{

    if ( hSetUpHandle == NULL )
    {
        Process_msg(errorFunction, 77, 0, 0);
        return(-1);
    }

    if ( lpInCBFunct == NULL )
    {
        Process_msg(errorFunction, 100, 0, 0);
        return(-1);
    }

    RestoreState(hSetUpHandle);  // this restores our global variables

    if (setjmp(abort_jmp_buf) != 0)
    {
        SaveState(hSetUpHandle);
        return (-1);
    }

    lDLLUserInputCBData = lUserInputCBData;   // pass this to callback function

    lpInProc = lpInCBFunct;

    if ( hInputBuffer == 0 )
    {
        hInputBuffer = GlobalAlloc(GPTR, INPUTBUFFERSIZE);  // allocate space

        if ( hInputBuffer == 0 )
        {

            SaveState(hSetUpHandle);
            Process_msg(errorFunction, 78, 0, 0);
            return(-1);
        }
    }

    lpInBuf = (char*) GlobalLock(hInputBuffer);  // lock space for input buffer area

    if ( lpInBuf == NULL )
    {
        SaveState(hSetUpHandle);
        Process_msg(errorFunction, 79, 0, 0);
        return(-1);
    }

    lpInBufStart = lpInBuf;
    nInBuf = 0;
    nInSoFar= 0;

    SaveState(hSetUpHandle);         // save global variables into saved area

    return(0);

}  // End - CCSetUpInputFilter

/**************************************************************************/
DLLExport int EXPDECL CCGetBuffer (HANDLE hGetHandle,
                                   char *lpOutputBuffer,
                                   int *npOutBufLen)
/**************************************************************************/
/*
 * Description:
 *                This DLL routine is called (usually repetitively) by the
 *                user.  This uses the callback routine that is passed to
 *                CCSetUpInputFilter to "read" the input data, it calls the
 *                main part of CC then using this data as input data.  This
 *                routine passes back as output the output data from CC.
 *    
 *
 * Input values:
 *                hGetHandle is the handle with input global data.
 *
 *                *lpOutputBuffer points to a buffer area for our output.
 *
 *                *npOutBufLen is the max size of the buffer CC outputs to.
 *
 *
 *
 * Output values: The global variables referenced by hGetHandle are updated.
 *                
 *                *lpOutputBuffer points to buffer with output data from CC.
 *
 *                *npOutBufLen is the used amount of the output buffer.
 *
 *
 *
 * Return values:
 *                CC_GOT_FULL_BUFFER        Success, buffer is full.
 *
 *                CC_GOT_END_OF_DATA        Success and "end of data".
 *                                          (This means done,
 *                                           do not call this again).
 *
 *                Other                     Error occurred.
 *
 */
{
    int  rc;

    // NOTE: This routine has many similarities to CCMultiProcessBuffer.
    //       Changes made here might apply to that routine as well!

    if ( hGetHandle == NULL )
    {
        Process_msg(errorFunction, 80, 0, 0);
        return(-1);
    }

    if ( lpOutputBuffer == NULL )
    {
        Process_msg(errorFunction, 101, 0, 0);
        return(-1);
    }

    if ( npOutBufLen == NULL )  // if user asked for no data just return
    {
        Process_msg(errorFunction, 93, 0, 0);
        return(-1);
    }

    if ( *npOutBufLen <= 0 )    // if user asked for no data just return
    {
        Process_msg(errorFunction, 93, 0, 0);
        return(-1);
    }

    RestoreState(hGetHandle);

    if (setjmp(abort_jmp_buf) != 0)
    {
        SaveState(hGetHandle);
        return (-1);
    }

    if ( lpInProc == NULL )
    {
        SaveState(hGetHandle);   // save global variables
        Process_msg(errorFunction, 85, 0, 0);
        *npOutBufLen = 0;        // pass no data back if error
        return(-1);
    }

    // if passed back return value of CC_GOT_END_OF_DATA last time saying done
    // and the user again calls us without first Reinitializing then error
    if ( bProcessingDone )
    {
        SaveState(hGetHandle);   // save global variables
        Process_msg(errorFunction, 112, 0, 0);
        *npOutBufLen = 0;        // pass no data back if error
        return(-1);
    }

    lpOutBuf = lpOutputBuffer;  // point to beginning of CC'c output buffer
    // (which is the caller's input area)
    rc = CC_GOT_FULL_BUFFER;    // denote have not yet run out of input data
    // store parms regarding cc output buffer as global variables
    nMaxOutBuf = *npOutBufLen;  // set the max size of our output buffer
    nUsedOutBuf = 0;            // denote that no data has been output yet
    bOutputBufferFull= FALSE;
    bNeedMoreInput= FALSE;

    // call CC in a loop until either we run out of input data, or we run out
    // of space in our output buffer for output data to be output from CC
    while (( rc == CC_GOT_FULL_BUFFER ) && ( nUsedOutBuf < nMaxOutBuf ))
    {

        CallCCMainPart();                         // call "main" part of CC

        if ( errors )
        {
            SaveState(hGetHandle);                 // save global variables
            Process_msg(errorFunction, 81, 0, 0);
            *npOutBufLen = 0;                      // pass no data back if error
            return(-1);
        }

        // if we have really hit end of input, and have no more data to pass
        // back, then set the "correct" rc to indicate that we are now done.
        if (( eof_written ) &&
                ( backoutptr == backinptr ) &&
                ( !bAuxBufUsed ) &&
                ( !bSavedDblChar ))
        {
            bProcessingDone = TRUE;       // we have now passed completion rc
            rc = CC_GOT_END_OF_DATA;
        }
    }

    *npOutBufLen = nUsedOutBuf;         // return amount of data sent back
    SaveState(hGetHandle);              // save global variables

    return(rc);

}  // End - CCGetBuffer

/**************************************************************************/
DLLExport int EXPDECL CCProcessBuffer (HANDLE hProHandle,
                                       char *lpInputBuffer, int nInBufLen,
                                       char *lpOutputBuffer, int *npOutBufLen)
/**************************************************************************/
/*
 * Description:
 *                This DLL routine is called by the user to have CC
 *                operations performed on one user input buffer, and have
 *                the results placed into one user output buffer.
 *                This interface does not use any callbacks at all.
 *                This does not save any data across calls, it processes
 *                all of the data that was passed into it.  This calls
 *                CCReinitializeTable near the start of its processing,
 *                so a user that calls this repeatedly with different data
 *                does not have to bother with that.
 *
 *
 *
 * Input values:
 *                hProHandle is the handle with input global data.
 *
 *                *lpInputBuffer points to a user buffer area with user input.
 *
 *                nInBufLen is the size of the user input buffer.  Note that
 *                          if the user wants a null character at the end of
 *                          this to be considered part of the input this
 *                          length should include the null character.  If the
 *                          user does not want a null character included this
 *                          should not include that, but note that the output
 *                          will then not contain a null character at the
 *                          end of the data, the user must then totally rely
 *                          upon the *npOutBufLen output buffer length value.
 *
 *                *lpOutputBuffer points to a user buffer area for CC output.
 *
 *                *npOutBufLen is the max size of the buffer CC outputs to.
 *                             Note that this will have to be longer than
 *                             nInBufLen above if CC adds to the size of
 *                             the user output at all (which it often does).
 *                             If it is not big enough, CC terminates with
 *                             an error message.
 *
 *
 *
 * Output values: 
 *                The global variables referenced by hProHandle are updated.
 *                
 *                *lpOutputBuffer points to buffer with output data from CC.
 *
 *                *npOutBufLen points to the used size of the output buffer.
 *
 *
 *
 * Return values:
 *                0           Success
 *
 *                Other       Error occurred.
 *
 */
{
    // NOTE: This routine has many similarities to CCMultiProcessBuffer.
    //       Changes made here might apply to those routines as well!

    if ( hProHandle == NULL )
    {
        Process_msg(errorFunction, 82, 0, 0);
        return(-1);
    }

    if ( npOutBufLen == NULL )
    {
        Process_msg(errorFunction, 102, 0, 0);
        return(-1);
    }

    if ( *npOutBufLen == 0 )      // if passed in no data then just return
    {
        Process_msg(errorFunction, 137, 0, 0);
        *npOutBufLen = 0;
        return(-1);
    }

    if ( lpInputBuffer == NULL )
    {
        Process_msg(errorFunction, 103, 0, 0);
        *npOutBufLen = 0;
        return(-1);
    }

    if ( lpOutputBuffer == NULL )
    {
        Process_msg(errorFunction, 104, 0, 0);
        *npOutBufLen = 0;
        return(-1);
    }

    if ( nInBufLen <= 0 )      // if passed in no data then just return
    {
        Process_msg(errorFunction, 94, 0, 0);
        *npOutBufLen = 0;
        return(-1);
    }

    if ( CCReinitializeTable(hProHandle) != 0 )
    {
        Process_msg(errorFunction, 97, 0, 0);
        return(-1);
    }

    RestoreState(hProHandle);            // restore our global variables

    if (setjmp(abort_jmp_buf) != 0)
    {
        SaveState(hProHandle);
        return (-1);
    }

    nMaxOutBuf = *npOutBufLen;           // set maximum size of output buffer
    bPassedAllData = TRUE;               // have all our input data already
    bNeedMoreInput= FALSE;
    nInBuf = nInBufLen;                  // set input buffer length
    nInSoFar= 0;
    lpInBufStart = lpInputBuffer;        // point to start of input buffer
    lpInBuf = lpInBufStart;              // point to start of input buffer
    lpOutBuf = lpOutputBuffer;           // point to output buffer area
    bOutputBufferFull= FALSE;
    nUsedOutBuf = 0;                     // denote no data has been output yet

    CallCCMainPart();                    // call the "main" part of CC

    // If the size of the output buffer we tried to pass all of the
    // output data to was not big enough, then we have one of these
    // flags on.  If so, then put out appropriate error message.
    if (( backoutptr != backinptr )
            || ( bSavedDblChar )
            || ( bAuxBufUsed ))
    {
        SaveState(hProHandle);            // save global variables
        Process_msg(errorFunction, 83, 0,
                    (long unsigned) *npOutBufLen);
        *npOutBufLen = 0;                 // pass no data back if error
        return(-1);
    }

    if ( errors )
    {
        SaveState(hProHandle);            // save global variables
        Process_msg(errorFunction, 84, 0, 0);
        *npOutBufLen = 0;                 // pass no data back if error
        return(-1);
    }

    *npOutBufLen = nUsedOutBuf;          // return amount of data sent back

    SaveState(hProHandle);               // save global variables

    return(0);

}  // End - CCProcessBuffer

/**************************************************************************/
DLLExport int EXPDECL CCMultiProcessBuffer (HANDLE hMultiHandle,
        char *lpInputBuffer, int nInBufLen,
        BOOL bLastCallWithInput,
        char *lpOutputBuffer, int *npOutBufLen)
/**************************************************************************/
/*
 * Description:
 *                This DLL routine is called by the user to have CC
 *                operations performed on input data, and have
 *                the results placed into an output buffer.  This saves
 *                data across calls.  This is intended to generally be
 *                called multiple times.  The last time the user passes
 *                input in the user turns on the bLastCallWithInput flag.
 *                Note the user might have to keep calling back (with no more
 *                input data) until return value of CC_GOT_END_OF_DATA given.
 *                Note that the size of the output may vary (it may even
 *                be zero in some cases).  This does not utilize callbacks.
 *                Note also that if CC wants to pass back more output data
 *                than there is output buffer space supplied to CC that a
 *                return code of CC_CALL_AGAIN_FOR_MORE_DATA is returned.
 *                The user then must call again passing no input data until
 *                return code of CC_SUCCESS or CC_GOT_END_OF_DATA received.
 *
 *
 * Input values:
 *                hMultiHandle is the handle with input global data.
 *
 *                *lpInputBuffer points to a user buffer area with user input.
 *
 *                nInBufLen is the size of the user input buffer.  Note that
 *                          if the user wants a null character at the end of
 *                          this to be considered part of the input this
 *                          length should include the null character.  If the
 *                          user does not want a null character included this
 *                          should not include that, but note that the output
 *                          will then not contain a null character at the
 *                          end of the data, the user must then totally rely
 *                          upon the *npOutBufLen output buffer length value.
 *                          This buffer does not have to be filled each time.
 *
 *                bLastCallWithInput  is TRUE (1) if this is the last call
 *                                    from the user that supplies input data.
 *                                    it is FALSE (0) otherwise.
 *                                    Note that CC has a typedef defining
 *                                    BOOL to char.
 *
 *                *lpOutputBuffer points to user buffer area for CC output.
 *
 *                *npOutBufLen is the max size of the buffer CC outputs to.
 *                            This buffer should be larger than the input
 *                            buffer, to allow for differing amounts of
 *                            data to be returned than was passed in, and
 *                            to allow for any expansion of data if any.
 *
 *
 *
 *
 * Output values: The global variables referenced by hMultiHandle are updated.
 *                
 *                *lpOutputBuffer points to buffer with output data from CC.
 *
 *                *npOutBufLen points to the used size of the output buffer.
 *
 *    
 *
 * Return values:
 *                CC_SUCCESS                   Success (output buffer
 *                                              may not be full though)
 *
 *                CC_GOT_END_OF_DATA           Success and "end of data".
 *                                             (This means done,
 *                                              do not call this again).
 *
 *               CC_CALL_AGAIN_FOR_MORE_DATA   The output buffer was not big
 *                                             enough to hold all the output
 *                                             data.  The user must call back
 *                                             again passing no input data
 *                                             until all the "old" output
 *                                             data is returned to the user
 *                                             (with return value CC_SUCCESS
 *                                             or CC_GOT_END_OF_DATA).  The
 *                                             user can then call again
 *                                             passing input data in again.
 *
 *                Other       Error occurred.
 *
 */
{
    int rc;


    // NOTE: This routine has many similarities to CCGetBuffer
    //       and to the CCProcessBuffer routine as well.
    //       Changes made here might apply to those routines as well!

    if ( hMultiHandle == NULL )
    {
        Process_msg(errorFunction, 105, 0, 0);
        return(-1);
    }

    if ( nInBufLen > 0 )
    {
        if ( lpInputBuffer == NULL )
        {
            Process_msg(errorFunction, 106, 0, 0);
            *npOutBufLen = 0;
            return(-1);
        }
    }

    if ( npOutBufLen == NULL )
    {
        Process_msg(errorFunction, 107, 0, 0);
        return(-1);
    }

    if ( *npOutBufLen <= 0 )      // if user asked for no data just return
    {
        Process_msg(errorFunction, 108, 0, 0);
        return(-1);
    }
    else
    {
        if ( lpOutputBuffer == NULL )
        {
            Process_msg(errorFunction, 109, 0, 0);
            *npOutBufLen = 0;
            return(-1);
        }
    }

    RestoreState(hMultiHandle);          // restore our global variables

    if (setjmp(abort_jmp_buf) != 0)
    {
        SaveState(hMultiHandle);
        return (-1);
    }

    // if we passed back return value of 1 last time saying we are done
    // and the user again calls us without first Reinitializing then error
    if ( bProcessingDone )
    {
        SaveState(hMultiHandle);          // save global variables
        Process_msg(errorFunction, 111, 0, 0);
        *npOutBufLen = 0;                 // pass no data back if error
        return(-1);
    }

    // if we passed back return value of CC_CALL_AGAIN_FOR_MORE_DATA last time
    // saying we did not have a big enough output buffer passed to us, then
    // they better pass us zero bytes of input data this time or give error
    if (( bPassNoData ) && ( nInBufLen != 0 ))
    {
        SaveState(hMultiHandle);          // save global variables
        Process_msg(errorFunction, 113, 0, 0);
        *npOutBufLen = 0;                 // pass no data back if error
        return(-1);
    }

    if ( bLastCallWithInput )    // no more data in after this call?
        bPassedAllData = TRUE;            // we have received all input data
    else
        bPassedAllData = FALSE;            // we have not received all input data

    bNeedMoreInput= FALSE;
    nInBuf = nInBufLen;                  // set input buffer length
    nInSoFar= 0;
    lpInBufStart = lpInputBuffer;        // point to start of input buffer
    lpInBuf = lpInputBuffer;             // point to start of input buffer
    lpOutBuf = lpOutputBuffer;           // point to output buffer area
    bOutputBufferFull= FALSE;
    nUsedOutBuf = 0;                     // denote no data has been output yet
    nMaxOutBuf = *npOutBufLen;           // set maximum size of output buffer
    rc = CC_SUCCESS;                     // default return to the usual case

    CallCCMainPart();                    // call the "main" part of CC

    if ( errors )
    {
        SaveState(hMultiHandle);          // save global variables
        Process_msg(errorFunction, 110, 0, 0);
        *npOutBufLen = 0;                 // pass no data back if error
        return(-1);
    }

    *npOutBufLen = nUsedOutBuf;          // return amount of data sent back

    // if we have received all our input data and processed it and we have
    // no more output data to return after this, then tell user we are done
    if (bPassedAllData)
    {
        if ((backoutptr == backinptr) &&
                (!bAuxBufUsed) &&
                (!bSavedDblChar))
        {
            bProcessingDone = TRUE;           // we have now passed completion rc
            rc = CC_GOT_END_OF_DATA;
        }
        else
        {
            bPassNoData = TRUE;               // user should pass no data next time
            rc = CC_CALL_AGAIN_FOR_MORE_DATA;
        }
    }
    else
        bPassNoData = FALSE;              // user can do whatever next time

    SaveState(hMultiHandle);             // save global variables

    return(rc);

}  // End - CCMultiProcessBuffer

/**************************************************************************/
DLLExport int EXPDECL CCSetUpOutputFilter (HANDLE hOutHandle,
        CCOutputProc * lpOutCBFunct,
        long lUserOutputCBData)
/**************************************************************************/
/*
 * Description:
 *                This DLL routine is called by the user to set up the output
 *                filter.  The user's output filter routine is then
 *                called iteratively as needed by CC's
 *                CCPutBuffer DLL routine to take output data from CC.
 *                The output filter routine would then presumably write
 *                the output data to a file, or do something like that.
 *                This also allocates the output area that CCPutBuffer
 *                tells CC to fill up with CC output data.
 *
 *
 * Input values:
 *                hOutHandle     This points to our global variable struct.
 *
 *
 *                lpOutCBFunct  Points to callback routine that takes output.
 *                              This is called from the CCPutBuffer routine.
 *
 *
 *                lUserOutputCBData  This is input from the user to be saved
 *                                   by CC and passed to the output callback
 *                                   function everytime that it is called.
 *                                   The usage of this is up to the user.
 *                                   (One example of how to use it would be
 *                                    to pass it to this routine as zero and
 *                                    then test it in the callback routine
 *                                    and then know it is the first call the   
 *                                    appropriate file can be opened).
 *
 *
 *    Callback function:        The routine this points to has two input
 *                              parameters, one is char *, the other
 *                              is int.
 *                              One input value is the char *, which
 *                              points to a CC output buffer.  That is where
 *                              the callback function is to get the data.
 *                              The other input value is the int, which tells
 *                              the callback function the number of bytes
 *                              that CC has put into the buffer.
 *
 *                              For output the callback function gets data
 *                              from the passed buffer, and handles is as
 *                              appropriate (e.g. writes it to a file).
 *                  
 *                              The int return value from the callback is
 *                              0 for success, non-zero if an error occurred.
 *
 *
 *
 * Output values: The global variables referenced by hOutHandle are updated
 *                to reflect the callback routine that takes CC output data.
 *
 *
 *
 * Return values:
 *                0           Success.
 *                
 *                non-zero    Error occurred.
 *
 *
 */
{

    if ( hOutHandle == NULL )
    {
        Process_msg(errorFunction, 87, 0, 0);
        return(-1);
    }

    RestoreState(hOutHandle);        // this restores our global variables

    if (setjmp(abort_jmp_buf) != 0)
    {
        SaveState(hOutHandle);
        return (-1);
    }

    lDLLUserOutputCBData = lUserOutputCBData;  // pass this to output callback

    lpOutProc = lpOutCBFunct;        // save address of user ouput function

    nMaxOutBuf = OUTPUTBUFFERSIZE;   // set size of output buffer

    nUsedOutBuf = 0;                 // denote no data has been output yet

    if ( hOutputBuffer == 0 )        // if no output buffer yet then allocate
    {
        hOutputBuffer = GlobalAlloc(GPTR, OUTPUTBUFFERSIZE);
        if ( hOutputBuffer == 0 )
        {
            SaveState(hOutHandle);
            Process_msg(errorFunction, 88, 0, 0);
            return(-1);
        }
        else
        {
            lpOutBufStart = (char*)GlobalLock(hOutputBuffer);  // lock output buffer
            lpOutBuf = lpOutBufStart;           // start at beginning of buffer
            if ( lpOutBufStart == NULL )
            {
                SaveState(hOutHandle);
                Process_msg(errorFunction, 89, 0, 0);
                return(-1);
            }
        }
    }

    SaveState(hOutHandle);           // save global variables into saved area

    return(0);

}  // End - CCSetUpOutputFilter

int CallOutputCallback(HANDLE hActiveCCTable)
{
    int rc;
    char * lpTempOutBuf;


    lpTempOutBuf= malloc(nUsedOutBuf);

    memcpy(lpTempOutBuf, lpOutBufStart, nUsedOutBuf);

    SaveState(hActiveCCTable);

    rc= (*lpOutProc)(lpTempOutBuf, nUsedOutBuf, &lDLLUserOutputCBData);

    RestoreState(hActiveCCTable);

    free(lpTempOutBuf);

    return rc;
}

/**************************************************************************/
DLLExport int EXPDECL CCPutBuffer (HANDLE hPutHandle,
                                   char *lpPutBuffer, int nInBufLen, BOOL bLastBuffer)
/**************************************************************************/
/*
 * Description:
 *                This is the main DLL routine when the user wants CC to be
 *                a back end to take output from another program in as CC 
 *                input.  This routine is called (usually repetitively) by
 *                the user.  This uses the callback routine that is passed to
 *                CCSetUpOutputFilter to "take" CC input data, it calls the
 *                main part of CC then using this data as input data. 
 *    
 *
 * Input values:
 *                hPutHandle is the handle with input global data.
 *
 *                *lpPutBuffer points to a buffer area for input data to CC.
 *
 *                nInBufLen is the size of the input data for this call.
 *
 *                bLastBuffer is TRUE if this is the last buffer of data
 *                            passed in with this interface, FALSE otherwise
 *
 *
 *
 * Output values: The global variables referenced by hPutHandle are updated.
 *
 *                CC processes the data passed in, and passes output data
 *                to the callback function passed to CCSetUpOutputFilter.
 *
 *
 *
 * Return values:
 *                0           Success.
 *
 *                non-zero    Error occurred.
 *
 */
{
    int  rc;


    if ( hPutHandle == NULL )
    {
        Process_msg(errorFunction, 90, 0, 0);
        return(-1);
    }

    RestoreState(hPutHandle);          // restore our global variables

    if ( lpOutProc == NULL )
    {
        Process_msg(errorFunction, 86, 0, 0);
        SaveState(hPutHandle);
        return(-1);
    }

    if (setjmp(abort_jmp_buf) != 0)
    {
        SaveState(hPutHandle);
        return (-1);
    }

    bPassedAllData = bLastBuffer;      // note if this is last buffer or not

    nInBuf = nInBufLen;                // save how much data we are passed in

    nInSoFar= 0;

    lpInBuf = lpPutBuffer;             // input buffer we are passed in

    bNeedMoreInput= FALSE;

    if (( nInBuf > 0 ) && ( lpInBuf == NULL ))
    {
        Process_msg(errorFunction, 96, 0, 0);
        SaveState(hPutHandle);
        return(-1);
    }

    CallCCMainPart();               // call "main" part of CC

    if ( errors )
    {
        SaveState(hPutHandle);       // save global variables
        Process_msg(errorFunction, 91, 0, 0);
        return(-1);
    }

    // if this is last time and we have some data left to output then do so
    if (( nUsedOutBuf > 0 ) && ( bLastBuffer ))
    {
        rc = CallOutputCallback(hPutHandle);

        nUsedOutBuf = 0;                // note we emptied buffer
        bOutputBufferFull= FALSE;
        lpOutBuf = lpOutBufStart;       // point to start of buffer again
        if ( rc != 0 )                  // verify callback function worked
        {
            SaveState(hPutHandle);
            Process_msg(errorFunction, 92, 0, 0);
            return(-1);
        }
    }

    SaveState(hPutHandle);

    return(0);

}  // End - CCPutBuffer

/**************************************************************************/
DLLExport int EXPDECL CCFlush (HANDLE hFlushHandle)
/**************************************************************************/
/*
 * Description:
 *                This DLL routine just calls the CCPutBuffer routine with 
 *                a zero length and the last buffer boolean set to TRUE. 
 *                Calling this is just a "shorthand" way of calling 
 *                CCPutBuffer with the appropriate parameters to flush the  
 *                remaining data out after all of the input data has been  
 *                passed in. 
 *    
 *
 * Input values:
 *                hFlushHandle is the handle with input global data.
 *
 *
 *
 * Output values: The global variables referenced by hFlushHandle are updated.
 *
 *
 *
 *
 * Return values:
 *                0           Success.
 *
 *                non-zero    Error occurred.
 *
 */
{
    int  rc;

    if ( hFlushHandle == NULL )
    {
        Process_msg(errorFunction, 133, 0, 0);
        return(-1);
    }
    else
    {
        rc = CCPutBuffer(hFlushHandle, NULL, 0, TRUE);
        return(rc);
    }

}  // End - CCFlush

/**************************************************************************/
DLLExport int EXPDECL CCProcessFile (HANDLE hProFileHandle,
                                     char *lpInputFile, char *lpOutputFile, BOOL bAppendOutput)
/**************************************************************************/
/*
 * Description:
 *                This is the main DLL routine when the user wants to just
 *                pass in the name of an input file and an output file to 
 *                CC via the DLL, and have CC operate on them.  The user
 *                uses no callback routines at all with this interface.  The
 *                user calls this DLL interface just once (after calling
 *                CCLoadTable, and optionally CCSetErrorCallBack, and before
 *                calling CCUnloadTable).
 *
 *                NOTE: It is the responsibility of the caller to make sure
 *                      that the output file specified either does not
 *                      exist, or if it does that the user wants CC to
 *                      overwrite it (or append if bAppendOutput is on).
 *    
 *                NOTE: This does not support wildcards in the input/output
 *                      filenames, or multiple input/output files.
 *
 *                NOTE: If there is a lot of data it may take a long
 *                      time to return from calling this interface. 
 *
 *
 *
 *
 * Input values:
 *                *lpInputFile is the name of the user input file.
 *
 *                *lpOutputFile is the name of the user output file.
 *
 *                bAppendOutput is set to TRUE (1) if the user wants to
 *                              append to the output file if it already
 *                              exists.  Otherwise if the output file
 *                              specified exists it will be overwritten.
 *
 *
 *
 *
 * Output values: CC operates on the input file data, and writes output file.
 *                If there were no errors, then the output file name passed
 *                in by the user is filled with the CC output data.
 *
 *
 *
 * Return values:
 *                0           Success.
 *
 *                non-zero    Error occurred.
 *
 */
{
    int rc;
    char OutputArea[OUTPUTBUFFERSIZE];  // CC output area (user output buffer)
    int nOutBufferLen;
    char *lpOutBuffer;             // pointer to output data from CC DLL
    int n;

    if ( hProFileHandle == NULL )
    {
        Process_msg(errorFunction, 125, 0, 0);
        return(-1);
    }

    if ( lpInputFile == NULL )
    {
        Process_msg(errorFunction, 126, 0, 0);
        return(-1);
    }

    if ( lpOutputFile == NULL )
    {
        Process_msg(errorFunction, 127, 0, 0);
        return(-1);
    }

    RestoreState(hProFileHandle);         // restore our global variables

    if (setjmp(abort_jmp_buf) != 0)
    {
        SaveState(hProFileHandle);
        return (-1);
    }

    // set up the internal CC input filter (callback routine)
    rc = CCSetUpInputFilter(hProFileHandle, FillFunction, 0);

    if ( rc != 0 )
    {
        Process_msg(errorFunction, 128, 0, 0);
        SaveState(hProFileHandle);
        return(rc);
    }


    // open input file in read mode or read/binary mode, as appropriate
    if ( binary_mode )
        fUserInFile = wfopen(lpInputFile, "rb");
    else
        fUserInFile = wfopen(lpInputFile, "r");
    if ( fUserInFile == NULL )
    {
        Process_msg(errorFunction, 129, 0, (long unsigned) lpInputFile);
        SaveState(hProFileHandle);
        return(-1);
    }

    // open output file with append and/or binary mode, as appropriate
    if ( binary_mode )
        if ( bAppendOutput )
            fUserOutFile = wfopen(lpOutputFile, "ab");
        else
            fUserOutFile = wfopen(lpOutputFile, "wb");
    else
        if ( bAppendOutput )
            fUserOutFile = wfopen(lpOutputFile, "a");
        else
            fUserOutFile = wfopen(lpOutputFile, "w");
    if ( fUserOutFile == NULL )
    {
        Process_msg(errorFunction, 130, 0, (long unsigned) lpOutputFile);
        wfclose(fUserInFile);        // close the input file we just opened
        SaveState(hProFileHandle);
        return (-1);
    }


    rc = 0;
    while ( rc == 0 )
    {
        lpOutBuffer = &OutputArea[0];     // point to area for CC to output to
        nOutBufferLen = OUTPUTBUFFERSIZE;  // set length of users output area
        // call the CC DLL to process one buffer full of data
        rc = CCGetBuffer(hProFileHandle, lpOutBuffer, &nOutBufferLen);
        if (( rc < 0 ) || ( rc > 1 ))
        {
            msg_printf("Return code %ld", rc);

            Process_msg(errorFunction, 131, 0, 0);
            errors = TRUE;
        }
        else
        {
            // write data passed back from the CC DLL to user output file
            n = 0;
            while ( n < nOutBufferLen )
            {
                n++;
                wfputc(*lpOutBuffer++, fUserOutFile);
            }
        }
    }

    wfclose(fUserInFile);           // close the user input file
    wfclose(fUserOutFile);          // close the user output file

    SaveState(hProFileHandle);

    if ( !errors )
        return(0);
    else
        return(-1);

}  // End - CCProcessFile

/**************************************************************************/
int WINAPI FillFunction(char *lpCCInputBuffer,
                        int nCCInputBufferLen,
                        long *lpUserInputData)
/**************************************************************************/
/*
 * Description:
 *                This is a callback function, whose address is passed to 
 *                the CCGetBuffer DLL routine by CCProcessFile.  This is the
 *                "users" input routine, and it passes input data to CC DLL.
 *                This is used with the CCProcessFile DLL routine above.
 *
 *
 * Input values:
 *                *lpCCInputBuffer points to the start of the buffer.
 *
 *                nCCInputBufferLen has the maximum size of the buffer.
 *
 *                *lpUserInputData has user defined data (zero first time).
 *
 *
 * Output values:
 *                *lpCCInputBuffer points to the start of the buffer,
 *                 which this routine fills with data.
 *
 *                *lpUserInputData is incremented by one.
 *
 *
 *
 * Return values:
 *                0    If "EOF" condition (data buffer is therefore filled).
 *                     (If error condition we also do not return data, but
 *                      error condition is denoted by error message).
 *
 *                n    Where n is the number of bytes returned.  This must be
 *                     less than or equal to input parm nCCInputBufferLen.
 *
 *
 */
{
    short signed int ch;   // use int instead of char so that for binary
    // mode we are able to differentiate between
    // x'255' in the data and EOF
    char *chp;
    int len;
    BOOL bEof;
#if defined(_WINDOWS)
    MSG msg;
#endif
    chp = lpCCInputBuffer;
    bEof = FALSE;

    (*lpUserInputData)++;     // increment just for debugging purposes
    // (in case of failure we can use this to see
    //  if we had trouble 1st, 2nd, or nth time)

    // read in amount of data indicated to us (unless we hit EOF first)
    len = 0;
    while (( len < nCCInputBufferLen ) && ( !bEof  ))
    {
        ch = wgetc(fUserInFile);
        if ( ch != EOF )
        {
            *chp++ = (char)ch;
            len++;
        }
        else
            bEof = TRUE;
    }
#if defined(_WINDOWS)
    // Without this loop when the user called the CCProcessFile interface
    // the resulting processing would never yield control to other tasks
    // running.  So if the user had a large job that took a minute then they
    // could not even get their computer to respond to mouse movements or
    // clicks in the meantime.  This loop prevents that nasty behavior.
    while ( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) )
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
#endif
    return(len);

}  // End - FillFunction


#if !defined(WIN32) && !defined(STATICLINK) && !defined(UNIX)
static WORD GetNewDS (void)
{
    HANDLE hDS;
    WORD wDS;
    LPSTR lpDS;

    // get a global memory block for this instance
    if ( !(hDS = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, cbInitialDSSize)) )
        return (0);

    // Copy initial global values
    lpDS = GlobalLock(hDS);
    hmemcpy(lpDS, lpInitialDS, cbInitialDSSize - cbInitialHeapSize);
    wDS = HIWORD((LONG)lpDS);
    LocalInit(wDS, 0, cbInitialHeapSize);

    return (wDS);
}

static WORD LoadDS (WORD wDS)
{
    if (wDS)
    {
        // Change DS to point to callers area
        _asm{
            mov     ax,wDS
            mov     DS,ax
        }
    }

    return (wDS);
}

static void FreeDS(WORD wDS)
{
    HANDLE hDS;

    hDS = LOWORD(GlobalHandle(wDS));
    GlobalUnlock(hDS);                   // Doug Rintoul added Feb 1996
    GlobalFree(hDS);

}


// **************************************************************************
//
// Cleanup()
//
// Purpose: Clean up my allocated memory "atexit". This routine was added to
// make sure there are no resource leaks!
//
//   Parameters:
//
//      None
//
//   Return Value:
//
//      None
//
//   History:   Date            Author          Reason
//              11/18/93        bobh            Created
//
// **************************************************************************

// Make sure this is in the same text segment as the WEP!
#pragma alloc_text(WEP_TEXT, Cleanup)

void Cleanup(void)
{
    if ( lpInitialDS )
    {
        // Need to free global memory copy of initialized data.

        HGLOBAL h;

        // Get original handle back.
        h = LOWORD (GlobalHandle (SELECTOROF (lpInitialDS)));
        if ( h )
        {
            // Now unlock & free it
            GlobalUnlock (h);
            GlobalFree (h);
        }
    }
}


// **************************************************************************
//   InitInstanceData()
//
//   Purpose:
//
//   Called by LibMain when DLL starts up.  Allocates a block of memory and
//   saves initial values of static variables in that block.  Also initializes
//   the list of tasks
//
//   Parameters:
//
//       wDataSeg  - the data segment of the DLL.
//       cbHeapSize - initial heap size for each instance
//       Enable - whether to allow more than one task
//       Debug - whether to display debugging messages
//
//   Return Value:
//
//       int   - 1 if the initial instance data was saved correctly.
//               0 if could not allocate memory to save initial instance data
//
//   History:   Date            Author          Reason
//              10/19/91        briansc         Created
//               8/30/93        bobh            Removed extra code that
//                                              cleard new pTaskEnt elements
//                                              Added cbHeapSize
//               9/02/93        bobh            Added Enable & Debug
//
// ***************************************************************************
int InitInstanceData (WORD wDataSeg, WORD cbHeapSize, WORD Debug, WORD Enable)
{
    HANDLE  hDS;        // Handle to Data Segment
    HGLOBAL hInitialDS; // block that holds the initial DS

    // Save initial heap size.
    cbInitialHeapSize = cbHeapSize;

    // Save initial static data

    hDS = LOWORD (GlobalHandle(wDataSeg));
    cbInitialDSSize = GlobalSize(hDS);

    // Before allocating memory, get into "at exit" list:
    if ( _fatexit (Cleanup) )
        return (0);

    // Allocate global memory to hold a copy of our initialized data
    if ( !(hInitialDS = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE,
                                    cbInitialDSSize - cbInitialHeapSize)) )
        return (0);

    // Copy initialized data into new block
    lpInitialDS = GlobalLock(hInitialDS);
    hmemcpy(lpInitialDS, (LPSTR)(MAKELONG(0,wDataSeg)), cbInitialDSSize -
            cbInitialHeapSize);

    return (1);
}

static char INIFileName[] = "demodll.ini";
static char INISection[] = "Debug";
static char INIKeyLevel[] = "Level";
static char INIKeyMultitask[] = "MultiTask";

static int x, y;

DLLExport int EXPDECL LibMain (HINSTANCE hInstance, WORD wDataSeg,
                               WORD wHeapSize, LPSTR lpCmdLine)
{

    /*  if ( wHeapSize > 0 )
         UnlockData(0);        */

    if ( !InitInstanceData (wDataSeg, wHeapSize,
                            x= GetPrivateProfileInt (INISection, INIKeyLevel, 0, INIFileName),
                            y= GetPrivateProfileInt (INISection, INIKeyMultitask, 1, INIFileName)) )
    {
        return (0);
    }
    ;
    return (1);
}
#endif
/* END */
