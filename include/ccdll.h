#if !defined(_CCDLL_H)
#define _CCDLL_H
/**************************************************************************/
/* ccdll.h  Doug Case 2/96  This is the .h file for users of CC.DLL,
 *                          the Consistent Changes DLL.  This contains
 *                          the definitions of the DLL interfaces.       
 *                          This contains defines that allow this to
 *                          work with Visual C++ in 16 bit or 32 bit mode.
 *
 *                          NOTE: It is assumed that callers will use the
 *                          large memory model, and that having "FAR" in
 *                          the function definitions is redundant, so "FAR"
 *                          is not included in the definitions.
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
 *  06-Feb-96      DRC Original version
 *  01-Sep-97      DAR Corrected interface for Office 97
 *                 Added interface for Debugging
 *  31-10-01       DAR Added UTF-8 support
 *  
 *
 *
 *
 **************************************************************************/
#if defined(WIN32)
#if defined(__BORLANDC__)
#undef WINAPI
#define WINAPI _stdcall
#endif
#endif

#if defined(UNIX)
#define WINAPI
#include "wtype.h"
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    // define return codes from the CC DLL routines
#define CC_SUCCESS                   0
#define CC_GOT_FULL_BUFFER           0
#define CC_GOT_END_OF_DATA           1
#define CC_SUCCESS_BINARY            1
#define CC_CALL_AGAIN_FOR_MORE_DATA  2
#define CC_SYNTAX_ERROR             -2

#if (!defined(_WINDLL) && !defined(UNIXSHAREDLIB)) || defined(_WindowsExe)
    // The following typedefs define the callback routines that
    // are used by some of the CC DLL interfaces.
    typedef int WINAPI CCInputProc(char * lpInputBuffer, int nInputBufferSize, long * lpUserData);

    typedef int WINAPI CCOutputProc(char *lpOutputBuffer, int nOutputBufferSize, long *lpUserData);

    typedef int WINAPI CCMatchLineCallback(HANDLE hCCTable, unsigned iLine);

    typedef int WINAPI CCExecuteLineCallback(HANDLE hCCTable, unsigned iLine);

    typedef int WINAPI CCErrorCallback(short nMsgIndex, short unsigned wParam, long unsigned lParam, long *lpUserData);

    typedef int WINAPI CCCompileErrorCallback(char * lpszMessage, unsigned iLine, unsigned iCharacter);

    // the following are the CC DLL interfaces...

    int WINAPI CCLoadTable(char *lpszCCTableFile,
                           HANDLE *hpLoadHandle,
                           HINSTANCE hinstCurrent);

    int WINAPI CCLoadTable2(char *lpszCCTableFile,
                           HANDLE *hpLoadHandle);

	int WINAPI CCLoadTableFromBufferWithErrorCallback(const char *lpszBuffer,
            HANDLE FAR *hpLoadHandle,
            CCCompileErrorCallback * lpCCCompileErrorCallback);

    int WINAPI CCLoadTableFromBuffer(char *lpszBuffer,
                                     HANDLE FAR *hpLoadHandle);

    int WINAPI CCReinitializeTable(HANDLE hReHandle);

    int WINAPI CCUnloadTable(HANDLE hUnlHandle);

    int WINAPI CCSetDebugCallbacks(HANDLE hCCTHandle,
                                   CCMatchLineCallback * lpCCMatchLineCallback,
                                   CCExecuteLineCallback * lpCCExecuteLineCallback);

    int WINAPI CCSetUTF8Encoding(HANDLE hCCTableHandle,
                                 BOOL lutf8encoding);

    int WINAPI CCSetErrorCallBack(HANDLE hErrHandle,
                                  CCErrorCallback * lpCCErrorCallback);

    int WINAPI CCSetUpInputFilter(HANDLE hSetUpHandle,
                                  CCInputProc *lpInCBFunct, long lUserInputCBData);

    BOOL WINAPI CCQueryStore(HANDLE hCCTable, const char * pszStoreName, char * pszValue, unsigned nLenValue);

    BOOL WINAPI CCQuerySwitch(HANDLE hCCTable, const char * pszSwitchName);

    int WINAPI CCQueryInput(HANDLE hCCTable, char * pszInputBuffer, unsigned nLenBuffer);

    int WINAPI CCQueryOutput(HANDLE hCCTable, char * pszOutputBuffer, unsigned nLenBuffer);

    int WINAPI CCGetActiveGroups(HANDLE hCCTable, char * pszActiveGroups, unsigned nLenActiveGroups);

    int WINAPI CCGetActiveStore(HANDLE hCCTable, char * pszActiveStore, unsigned nLenActiveStore);

    int WINAPI CCFlush(HANDLE hFlushHandle);

    int WINAPI CCGetBuffer(HANDLE hGetHandle,
                           char *lpOutputBuffer, int *npOutBufLen);

    int WINAPI CCProcessBuffer(HANDLE hProHandle,
                               char *lpInputBuffer, int nInBufLen,
                               char *lpOutputBuffer, int *npOutBufLen);

    int WINAPI CCMultiProcessBuffer(HANDLE hMultiHandle,
                                    char *lpInputBuffer, int nInBufLen,
                                    BOOL bLastCallWithInput, char *lpOutputBuffer,
                                    int *npOutBufLen);

    int WINAPI CCSetUpOutputFilter (HANDLE hOutHandle,
                                    CCOutputProc *lpOutCBFunct,
                                    long lUserOutputCBData);

    int WINAPI CCPutBuffer (HANDLE hPutHandle,
                            char *lpPutBuffer, int nInBufLen,
                            BOOL bLastBuffer);

    int WINAPI CCProcessFile (HANDLE hProFileHandle,
                              char *lpInputFile, char *lpOutputFile,
                              BOOL bAppendOutput);
#ifdef __cplusplus
};   // extern "C"
#endif
#endif
#endif
