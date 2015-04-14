/* ccerror.h   MS Windows/CC Error Table for DLL    23-Dec-95    DRC
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
 * Edit history:
 *  23-Dec-95   DRC  Original Version      
 */

/**************************************************************************/
/*
 * Description: ccerror.h is the .h file that is used within the CC
 *              product to produce messages, and it can also be used by 
 *              Windows CC DLL callers if they want to write their own
 *              error processing routine.
 *
 *              Within CC, this file is used by the Process_msg() routine
 *              to produce messages, either on the screen for DOS, or in
 *              windowed messages for the Windows DLL version.  Note that
 *              not all messages are contained in this module, there are
 *              some that are specific to DOS, or that are produced only
 *              when certain debug flags are on that are not included
 *              here.
 *
 *              In Windows DLL mode, if there is message to be printed
 *              CC calls its Process_msg routine.  It checks if CC
 *              is supposed to handle the message itself, or if CC is
 *              to call a custom user error handler.  If CC is to call a
 *              user error handler then it does so, passing three parms.
 *              The first parm is a short, and it is the number of the
 *              message in the structure array errorstruct below.
 *              The second parm (called here wParam) is a short unsigned 
 *              and the third parm (called here lParam) is a long unsigned.
 *
 *              Again, the first parm is the number of the message, 
 *              that is the index into the table errorstruct below.
 *              Each table entry has three parts to it as defined in the
 *              definition for errorstruct.  The first is errmsg, which
 *              is the text of the message itself.  The second, errtype,
 *              actually describes the format of any additional parms
 *              to the message (that is wParam and lParam).  The purpose
 *              of which is described below in the comments for the defines.
 *              The third is msgtype, which denotes if the message is a
 *              warning or error message.
 *
 *              The error processing routine is passed (among other things)
 *              a short unsigned parm (called here wParam) and a long
 *              unsigned parm (called here lParam).  These are parameters
 *              to the messages below, and the comments for the defines
 *              below describe how they are to be used.
 *
 *
*/

#define MSG_S_S      0   // MSG_S_S means the message has two char * (%s)
// parameters.  For this ignore the wParam, and the
// lParam is a pointer to a structure containing
// both strings.  it is of the format:
//   typedef struct msg_S_S_struct
//         {
//         char *string1;
//         char *string2;
//         } MSG_STRUCT_S_S;

#define MSG_S_C_S    1   // MSG_S_C_S means the message has two char * (%s)
// parameters, and also one char (%c) parameter
// as well.  For this ignore the wParam, and the
// lParam is a pointer to a structure containing
// the two strings and the char. The format is:
//   typedef struct msg_S_C_S_struct
//         {
//         char *string1;
//         char char1;
//         char *string2;
//         } MSG_STRUCT_S_C_S;

#define MSG_S        2   // MSG_S means the message has one char * (%s)
// parameter.  For this ignore the wParam, and
// the lParam contains this char * argument

#define MSG_noparms  3   // This means the message has no parameters,
// both the wParam and lParam can be ignored.

#define MSG_LD       4   // MSG_LD means the message has one long int 
// parameter.  For this ignore the wParam, and
// the lParam contains this long int argument




#define WARNING_MESSAGE   0      // message is warning or informational
#define ERROR_MESSAGE     1      // message means nasty error problem


struct errorstruct
{
    char *errmsg;    // this is the error/warning message to be put out
    int   errtype;   // this denotes the type or format of the message
    int   msgtype;   // denotes if the message is warning or error message
    //    (this only applies to the Windows environment)
};


struct errorstruct errortable[] =
    {
        /*   0 */  { "?CC-E-Backed up too far\n", MSG_noparms, ERROR_MESSAGE},
        /*   1 */  { "CC-Warning caseless ignored, incompatible with doublebyte\n\n",
                     MSG_noparms, WARNING_MESSAGE },
        /*   2 */  { "Warning, group %s excluded but not active\n",
                     MSG_S, WARNING_MESSAGE },
        /*   3 */  { "Value %ld in 'back val(store)' inappropriate.\n",
                     MSG_LD, WARNING_MESSAGE},
        /*   4 */  { "Storage overflow of store %s\n", MSG_S, WARNING_MESSAGE },
        /*   5 */  { "?CC-E-Use of more than %ld groups\n", MSG_LD, ERROR_MESSAGE },
        /*   6 */  { "contents of store %s unchanged\n", MSG_S, WARNING_MESSAGE },
        /*   7 */  { "100%% complete\n", MSG_noparms, WARNING_MESSAGE },
        /*   8 */  { "Warning, store() content stack overflow.  Current storage context not saved.\n",
                     MSG_noparms, WARNING_MESSAGE },
        /*   9 */  { "Warning, store() content stack underflow.  Current storage context not restored.\n",
                     MSG_noparms, WARNING_MESSAGE },
        /*  10 */  { "Warning, value %ld in 'back val(store)' construct larger than the size of the back buffer. Executing a 'back(1)' command\n",
                     MSG_LD, WARNING_MESSAGE },
        /*  11 */  { "?CC-E-Backed too far storing\n", MSG_noparms,
                     ERROR_MESSAGE },
        /*  12 */  { "Warning, cannot decr new variable, it is not decremented\n",
                     MSG_noparms, WARNING_MESSAGE },
        /*  13 */  { "Warning, cannot decr variable that is zero, it is not decremented\n",
                     MSG_noparms, WARNING_MESSAGE },
        /*  14 */  { "Warning, doublebyte settings inconsistent.  Verify it is specified correctly in begin section\n",
                     MSG_noparms, WARNING_MESSAGE },
        /*  15 */  { "Output file full, processing aborted\n", MSG_noparms,
                     ERROR_MESSAGE },
        /*  16 */  { "Disk with output file full.  File is only partly complete!\n", MSG_noparms,
                     WARNING_MESSAGE },
        /*  17 */  { "Warning, doublebyte output no longer matches doublebyte criteria\n",
                     MSG_noparms, WARNING_MESSAGE },
        /*  18 */  { "Warning, doublebyte command has more than two arguments, others ignored\n",
                     MSG_noparms, WARNING_MESSAGE },
        /*  19 */  { "Arithmetic: %s in group %s\n", MSG_S_S, ERROR_MESSAGE },
        /*  20 */  { "%s %c %s\n", MSG_S_C_S, WARNING_MESSAGE },
        /*  21 */  { "Input and output files must be different!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  22 */  { "Wildcard mismatch between input \"%s\" and output \"%s\"\n",
                     MSG_S_S, ERROR_MESSAGE },
        /*  23 */  { "?CC-WARNING: defaulting to empty group %s\n",
                     MSG_S, WARNING_MESSAGE },
        /*  24 */  { "Input filename \"%s\" too long for available characters in output filename \"%s\". No processing occurred\n",
                     MSG_S_S, ERROR_MESSAGE },
        /*  25 */  { "?CC-F-Defaulting to non-existent group 1\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  26 */  { "?CC-E-No definition for do(%s)\n", MSG_S,ERROR_MESSAGE },
        /*  27 */  { "%s does not exist in specified path.\n",
                     MSG_S, ERROR_MESSAGE },
        /*  28 */  { "Total number of files must not exceed %ld.\n",
                     MSG_LD, ERROR_MESSAGE },
        /*  29 */  { "Memory allocation error. Exiting program.\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  30 */  { "Unable to do changes.\n", MSG_noparms, ERROR_MESSAGE },
        /*  31 */  { "CC fatal error; aborting host application\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  32 */  { "WARNING: Store %s used but never stored into\n",
                     MSG_S, WARNING_MESSAGE },
        /*  33 */  { "WARNING: Switch %s tested but never set or cleared\n",
                     MSG_S, WARNING_MESSAGE },
        /*  34 */  { "WARNING: use(%s) encountered, but group never defined\n",
                     MSG_S, WARNING_MESSAGE },
        /*  35 */  { "WARNING: do(%s) used but never defined\n",
                     MSG_S, WARNING_MESSAGE },
        /*  36 */  { "Not enough memory\n  (your font file is probably too big)\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  37 */  { "Only %ld bytes available to be allocated.  More is needed!\n",
                     MSG_LD, WARNING_MESSAGE },
        /*  38 */  { "?CC-F-Too many changes, limit is %ld\n", MSG_LD, ERROR_MESSAGE },
        /*  39 */  { "?CC-F-Group %s multiply defined\n", MSG_S,ERROR_MESSAGE },
        /*  40 */  { "?CC-W-Font section of table is ignored in CC\n",
                     MSG_noparms, WARNING_MESSAGE },
        /*  41 */  { "?CC-E-Group command not in front of change\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  42 */  { "?CC-E-Do nested deeper than %ld\n", MSG_LD, ERROR_MESSAGE },
        /*  43 */  { "?CC-E-Fwd too many\n", MSG_noparms, ERROR_MESSAGE },
        /*  44 */  { "?CC-E-Omit too many\n", MSG_noparms, ERROR_MESSAGE },
        /*  45 */  { "?CC-E-\'caseless\' command not in begin section\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  46 */  { "?CC-E-\'binary\' command not in begin section\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  47 */  { "?CC-E-\'doublebyte\' command not in begin section\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  48 */  { "?CC-E-\'unsorted\' command not in begin section\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  49 */  { "?CC-E-Illegal use of command \'%s\' after >\n",
                     MSG_S, ERROR_MESSAGE },
        /*  50 */  { "FATAL ERROR!\nexcl command in group %s removes all active groups\n",
                     MSG_S, ERROR_MESSAGE },
        /*  51 */  { "Too many prec()'s in succession, using only the first %ld",
                     MSG_LD, WARNING_MESSAGE },
        /*  52 */  { "?CC-E-Illegal use of command \'%s\' before >\n",
                     MSG_S, ERROR_MESSAGE },
        /*  53 */  { "%s\n?CC-F-%s\n", MSG_S_S, ERROR_MESSAGE },
        /*  54 */  { "Unrecognized option %s will be ignored.\n",
                     MSG_S, WARNING_MESSAGE },
        /*  55 */  { "Change table %s not found.\n",
                     MSG_S, ERROR_MESSAGE },
        /*  56 */  { "?CC-E-Table too large.\n", MSG_noparms, ERROR_MESSAGE },
        /*  57 */  { "There were errors in the change table.\nCorrect the errors and rerun.\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  58 */  { "Unable to open %s\n",
                     MSG_S, ERROR_MESSAGE },
        /*  59 */  { "Table file overflow.\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  60 */  { "Could not reallocate aux output buffer.  Need more memory!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  61 */  { "Aux output buffer allocation failed.  Need more memory!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  62 */  { "Auxiliary output buffer GlobalLock Failed!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  63 */  { "CC save state GlobalAlloc failed.  Probably need more memory!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  64 */  { "CC save state GlobalLock failed!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  65 */  { "CC save state GlobalUnlock failed in unlocking %s!\n",
                     MSG_S, ERROR_MESSAGE },
        /*  66 */  { "CC restore state was passed NULL handle.  Call CC with valid handle!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  67 */  { "CC restore state GlobalLock failed!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  68 */  { "CC restore state GlobalUnlock failed!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  69 */  { "CCLoadTable was passed invalid name of CC change table.  Pass in valid name!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  70 */  { "CCLoadTable encountered CC error.  Fix problem and rerun!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  71 */  { "CCReinitializeTable was passed NULL handle.  Call CC with valid handle!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  72 */  { "CCReinitializeTable encountered CC error.  Fix problem and rerun!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  73 */  { "CCUnloadTable was passed NULL handle.  Call CC with valid handle!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  74 */  { "CCUnloadTable GlobalUnlock failed!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  75 */  { "CCUnloadTable GlobalFree failed!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  76 */  { "CCSetErrorCallBack was passed NULL handle.  Call CC with valid handle!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  77 */  { "CCSetUpInputFilter was passed NULL handle.  Call CC with valid handle!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  78 */  { "CCSetUpInputFilter GlobalAlloc failed.  Need more memory!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  79 */  { "CCSetUpInputFilter GlobalLock failed!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  80 */  { "CCGetBuffer was passed NULL handle.  Call CC with valid handle!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  81 */  { "CCGetBuffer encountered CC error.  Fix problem and rerun!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  82 */  { "CCProcessBuffer was passed NULL handle.  Call CC with valid handle!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  83 */  { "CCProcessBuffer was passed output buffer of only %ld bytes, more needed!\nRerun with larger output buffer!\n",
                     MSG_LD, ERROR_MESSAGE },
        /*  84 */  { "CCProcessBuffer encountered CC error.  Fix problem and rerun!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  85 */  { "CCGetBuffer has NULL callback address!\nMust have successful call to CCSetUpInputFilter first!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  86 */  { "CCPutBuffer has NULL callback address!\nMust have successful call to CCSetUpOutputFilter first!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  87 */  { "CCSetUpOutputFilter was passed NULL handle.  Call CC with valid handle!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  88 */  { "CCSetUpOutputFilter GlobalAlloc failed.  Need more memory!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  89 */  { "CCSetUpOutputFilter GlobalLock failed!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  90 */  { "CCPutBuffer was passed NULL handle.  Call CC with valid handle!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  91 */  { "CCPutBuffer encountered CC error.  Fix problem and rerun!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  92 */  { "CCPutBuffer encountered error in output callback function.\nFix problem and rerun!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  93 */  { "CCGetBuffer called with no output data requested.\nReturning to caller after doing nothing!\n",
                     MSG_noparms, WARNING_MESSAGE },
        /*  94 */  { "CCProcessBuffer called with no input data.\nReturning to caller after doing nothing!\n",
                     MSG_noparms, WARNING_MESSAGE },
        /*  95 */  { "CCProcessFile input routine FillFunction had DLL failure getting segment address to data for calling task!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  96 */  { "CCPutBuffer was passed NULL input buffer pointer, but input data length greater than zero!.\nPlease fix problem and rerun!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  97 */  { "CCProcessBuffer call to CCReinitializeTable unsuccessful!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  98 */  { "CCLoadTable was passed NULL pointer to handle.  Pass in valid value!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /*  99 */  { "CC-Warning binary keyword ignored, incompatible with doublebyte\n\n",
                     MSG_noparms, WARNING_MESSAGE },
        /* 100 */  { "CCSetUpInputFilter was passed NULL function pointer.  Call CC with valid value!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /* 101 */  { "CCGetBuffer was passed NULL output buffer pointer.  Call CC with valid value!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /* 102 */  { "CCProcessBuffer was passed NULL output buffer length pointer.  Call CC with valid value!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /* 103 */  { "CCProcessBuffer was passed NULL input buffer pointer.  Call CC with valid value!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /* 104 */  { "CCProcessBuffer was passed NULL output buffer pointer.  Call CC with valid value!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /* 105 */  { "CCMultiProcessBuffer was passed NULL handle.  Call CC with valid handle!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /* 106 */  { "CCMultiProcessBuffer was passed NULL input buffer pointer.  Call CC with valid value!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /* 107 */  { "CCMultiProcessBuffer was passed NULL output buffer length pointer.  Call CC with valid value!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /* 108 */  { "CCMultiProcessBuffer called asking for no output data.\nReturning to caller after doing nothing!\n",
                     MSG_noparms, WARNING_MESSAGE },
        /* 109 */  { "CCMultiProcessBuffer was passed NULL output buffer pointer.  Call CC with valid value!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /* 110 */  { "CCMultiProcessBuffer encountered CC error.  Fix problem and rerun!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /* 111 */  { "CCMultiProcessBuffer called after it passed back completion return code!\n  Must first call CCReinitializeTable before calling again!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /* 112 */  { "CCGetBuffer called after it passed back completion return code!\n  Must first call CCReinitializeTable before calling again!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /* 113 */  { "CCMultiProcessBuffer passed input data.\nReturn value from last call indicates it must be passed no input data this time!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /* 114 */  { "CCLoadTable could not register the DLL task!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /* 115 */  { "CCReinitializeTable had DLL failure getting segment address to data for calling task!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /* 116 */  { "CCUnloadTable had DLL failure getting segment address to data for calling task!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /* 117 */  { "CCSetErrorCallBack had DLL failure getting segment address to data for calling task!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /* 118 */  { "CCSetUpInputFilter had DLL failure getting segment address to data for calling task!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /* 119 */  { "CCGetBuffer had DLL failure getting segment address to data for calling task!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /* 120 */  { "CCProcessBuffer had DLL failure getting segment address to data for calling task!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /* 121 */  { "CCMulti{rocessBuffer had DLL failure getting segment address to data for calling task!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /* 122 */  { "CCSetUpOutputFilter had DLL failure getting segment address to data for calling task!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /* 123 */  { "CCPutBuffer had DLL failure getting segment address to data for calling task!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /* 124 */  { "CCLoadTable called with NULL handle (hinstance) for calling task!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /* 125 */  { "CCProcessFile was passed NULL handle.  Call CC with valid handle!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /* 126 */  { "CCProcessFile was passed NULL input file pointer! Fix problem and rerun!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /* 127 */  { "CCProcessFile was passed NULL output file pointer! Fix problem and rerun!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /* 128 */  { "CCProcessFile call to CCSetUpInputFilter failed!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /* 129 */  { "CCProcessFile could not open input file it was passed!\nInput file: %s\nFix problem and rerun!\n",
                     MSG_S, ERROR_MESSAGE },
        /* 130 */  { "CCProcessFile could not open output file it was passed!\nOutput file: %s\nFix problem and rerun!\n",
                     MSG_S, ERROR_MESSAGE },
        /* 131 */  { "CCProcessFile encountered CC error.  Fix problem and rerun!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /* 132 */  { "CCProcessFile had DLL failure getting segment address to data for the calling task!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /* 133 */  { "CCFlush was passed NULL handle.  Call CC with valid handle!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /* 134 */  { "CCLoadTable had DLL failure getting segment address to data for calling task!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /* 135 */  { "CCLoadTable was passed a change table filename that is too long!\nPlease call it with a shorter change table file path name.\n",
                     MSG_S, ERROR_MESSAGE },
        /* 136 */  { "?CC-W-Change Table is empty: No changes will be made to file.\n",
                     MSG_noparms, WARNING_MESSAGE },
        /* 137 */  { "CCProcessBuffer was passed an output buffer length of 0.\nSet the output buffer length to the length of the output buffer before calling CCProcessBuffer!\n",
                     MSG_noparms, ERROR_MESSAGE },
        /* 138 */  { "CCSetUTF8Encoding was passed NULL handle.  Call CC with valid handle!\n",
                     MSG_noparms, ERROR_MESSAGE },
    };



/* END */
