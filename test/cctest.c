#if defined(_WINDOWS)
#include <windows.h>
#endif
#include <stdio.h>
#include <memory.h>
#include <string.h>
#if defined(WIN32)
#include <winver.h>
#elif !defined(UNIX)
#include <ver.h>
#endif
#if defined(UNIX)
#include <unistd.h>
#include "wtype.h"
#define MessageBox(w,x,y,z) fprintf(stderr, "%s\n", x);
#endif
#undef _WINDLL
#include "ccdll.h"
#if defined(WIN32)
#define _export
#endif

int iInputState;

int WINAPI _export CCMatchLineCallbackProc(HANDLE hCCTable, unsigned iLineNumber)
{
	char szTestStore[100];
	char szActiveGroups[100];
	BOOL fTestSwitch1;
	BOOL fTestSwitch2;
	BOOL fTestSwitch3;
	
	
	if (iLineNumber == 6)
	{
		MessageBox(NULL, "Match on CC Table Line 6", "CC DLL Test", MB_ICONHAND);
		
		CCQueryStore(hCCTable, "teststore", szTestStore, sizeof(szTestStore));
		CCGetActiveGroups(hCCTable, szActiveGroups, sizeof(szActiveGroups));
		fTestSwitch1= CCQuerySwitch(hCCTable, "testswitch1");
		fTestSwitch2= CCQuerySwitch(hCCTable, "testswitch2");
		fTestSwitch3= CCQuerySwitch(hCCTable, "testswitch3");
		fprintf(stderr, "teststore: %s\nactive groups: %s\ntestswitch1 %d\ntestswitch2 %d\ntestswitch2 %d\n",
			szTestStore, szActiveGroups, fTestSwitch1, fTestSwitch2, fTestSwitch3);
	} 

	return 1;		
}

int WINAPI _export CCExecuteLineCallbackProc(HANDLE hCCTable, unsigned iLineNumber)
{
	if (iLineNumber == 4)
		MessageBox(NULL, "Executing CC Table Line 4", "CC DLL Test", MB_ICONHAND);
		
	return 1;
}

int WINAPI _export CCInputCallbackProc(char * pInputBuffer, int nInputBufferSize, long * dwUserData)
{
	iInputState++;

	switch (iInputState)
	{
	case 1:
	case 2:
	case 3:
		strcpy(pInputBuffer, "aaaaa");
		return 5;
	case 4:
		strcpy(pInputBuffer, "cccccddddd");
		return 10;
	default:
		return 0;
	}


	return 0;
}

int WINAPI _export CCOutputCallback(char * pOutputBuffer, int nBytesInBuffer, long * dwUserData)
{
	FILE * fOutputFile;

	fOutputFile= (FILE *) *dwUserData;
	fwrite(pOutputBuffer, nBytesInBuffer, 1, fOutputFile);

	return 0;
}

int g_nCCMsgIndex;
int g_lCCParam;

int WINAPI _export localCCErrorCallback(short nMsgIndex,
								   unsigned short wParam,
								   unsigned long lParam,
								   long *lpUserData)
{
         g_nCCMsgIndex = nMsgIndex;
         g_lCCParam = lParam;

         // GETS TO HERE OK

         return 1;
}
#if defined(_WINDOWS)
void GetModuleVersion(char * lpszFullPath, char * lpszVersion)
{
   DWORD   dwVerInfoSize;          // Size of version information block
   DWORD   dwVerHnd=0;                     // An 'ignored' parameter, always '0'
   char    szGetName[256];
   WORD    wRootLen;
   BOOL    bRetCode;
   LPSTR   lpVersion;                      // String pointer to 'version' text
   UINT    uVersionLen;

   // Now lets dive in and pull out the version information:
	dwVerInfoSize = GetFileVersionInfoSize(lpszFullPath, &dwVerHnd);

	if (!dwVerInfoSize) 
		*lpszVersion= '\0';
	else
		{
		LPSTR   lpstrVffInfo;
		HANDLE  hMem;
		hMem = GlobalAlloc(GMEM_MOVEABLE, dwVerInfoSize);
		lpstrVffInfo  = GlobalLock(hMem);
		GetFileVersionInfo(lpszFullPath, dwVerHnd, dwVerInfoSize, lpstrVffInfo);
		lstrcpy(szGetName, "\\StringFileInfo\\040904E4\\");      
		wRootLen = lstrlen(szGetName); // Save this position
		                        
		// Set the title of the dialog:
		lstrcat (szGetName, "FileVersion");
		bRetCode = VerQueryValue((LPVOID)lpstrVffInfo,
		       szGetName,
		       (LPVOID)&lpVersion,
		       (UINT *)&uVersionLen);
		       
		strcpy(lpszVersion, lpVersion);
     	GlobalUnlock(hMem);
      GlobalFree(hMem);
		}
}
#endif

#if defined(UNIX)
int main(int argc, char ** argv)
#else
int PASCAL WinMain(
   HINSTANCE hinstCurrent,   /* handle of current instance    */
   HINSTANCE hinstPrevious,  /* handle of previous instance   */
   LPSTR lpszCmdLine,        /* address of command line       */
   int nCmdShow)/* show-window type (open/icon)  */
#endif
{
	HANDLE hCCT;
	char * pszCCTable;
	char * pszInputBuffer;
	char szOutputBuffer[1000];
	char szGetBuffer[4];

	char * pChar;
	int ch;
	int nSizeOutputBuffer;
	int nSizeGetBuffer;
	int rc;
	FILE * fTestFile;
	FILE * fOutputFile;
	BOOL bFailure= 0;
	char * utf8expected;
#if defined(_WINDOWS)
   char szModuleName[260];
	char szModuleVersion[20];   
	HINSTANCE hinstCCDLL;
	char szMsgBuffer[1000];

#if defined(WIN32)
	hinstCCDLL= LoadLibrary("CC32.DLL");
#else
	hinstCCDLL= LoadLibrary("CC.DLL");
#endif
	GetModuleFileName(hinstCCDLL, szModuleName, sizeof(szModuleName));
	GetModuleVersion(szModuleName, szModuleVersion);   	 
	
	sprintf(szMsgBuffer, "CC DLL: %s, Version: %s", szModuleName, szModuleVersion);
	MessageBox(NULL, szMsgBuffer, "CC DLL Test", MB_ICONHAND);
#endif	

	/********************/
	/* Double byte test */
	/********************/
	pszCCTable= 
        "begin > doublebyte(x78,d22) store(teststore) 'This is a test' endstore\n" \
        "        set(testswitch1)\n"    \
        "        clear(testswitch2)\n"  \
        "        use(main, group2)\n"   \
        "group(main)\n"                 \
        "'xa' > 'xz'\n"                 \
        "'xb' > dup back(2) store(x) fwd(2) endstore '{' out(x) '}'\n" \
        "group(group2)\n"               \
        "'a' > 'b'\n";
	
	rc= CCLoadTableFromBuffer(pszCCTable, &hCCT);
	CCSetDebugCallbacks(hCCT, CCMatchLineCallbackProc, CCExecuteLineCallbackProc);
	
	nSizeOutputBuffer= sizeof(szOutputBuffer);
	memset(szOutputBuffer, 0, sizeof(szOutputBuffer));
	pszInputBuffer= "xaxbxcxdxexf";
	
	CCSetErrorCallBack(hCCT, localCCErrorCallback);

	rc= CCProcessBuffer(hCCT, pszInputBuffer, strlen(pszInputBuffer),
		szOutputBuffer, &nSizeOutputBuffer);
    
	
	if (strcmp(szOutputBuffer, "{xzxb}xcxdxexf") != 0 ||
		 nSizeOutputBuffer != 14)
	{
		bFailure= 1;
		MessageBox(NULL, "Error in Double Byte test. Output incorrect", "CC DLL Test", MB_ICONHAND);
		fprintf(stderr, "expected {xzxb}xcxdxexf got %s\n", szOutputBuffer);
	}
	else
	{
		fprintf(stderr, "Double Byte test: passed\n");
	}
	
	rc= CCUnloadTable(hCCT);

	/********************/
	/* UTF-8 test */
	/********************/
	pszCCTable= 
        "begin > utf8\n" \
        "        use(main, group2)\n"   \
        "group(main)\n"                 \
        "'ɱ' > 'ɗ' back(1)\n"                 \
        "'ɗɐ' > dup back(1) store(x) fwd(1) endstore '{' out(x) '}'\n" \
        "group(group2)\n"               \
        "'ɐ' > 'z'\n";
	
	rc= CCLoadTableFromBuffer(pszCCTable, &hCCT);
	CCSetDebugCallbacks(hCCT, CCMatchLineCallbackProc, CCExecuteLineCallbackProc);
	
	nSizeOutputBuffer= sizeof(szOutputBuffer);
	memset(szOutputBuffer, 0, sizeof(szOutputBuffer));
	pszInputBuffer= "ɐɱɐɱɐɱ";
	
	CCSetErrorCallBack(hCCT, localCCErrorCallback);

	rc= CCProcessBuffer(hCCT, pszInputBuffer, strlen(pszInputBuffer),
		szOutputBuffer, &nSizeOutputBuffer);
    
	utf8expected= "zɗ{ɐ}ɗ{ɐ}ɗ";
	if (strcmp(szOutputBuffer, utf8expected) != 0 ||
		 nSizeOutputBuffer != strlen(utf8expected))
	{
		bFailure= 1;
		MessageBox(NULL, "Error in UTF-8 test. Output incorrect", "CC DLL Test", MB_ICONHAND);
		fprintf(stderr, "expected %s got %s\n", utf8expected, szOutputBuffer);
	}
	else
	{
		fprintf(stderr, "UTF-8 test: passed\n");
	}
	
	rc= CCUnloadTable(hCCT);

    /********************************************/
	/* Single Buffer test with 8 bit characters */
    /********************************************/
	pszCCTable= "begin > use(main)\ngroup(main)\n'a' > 'b'\nd147 > d169\nd148 > d170\n";
	rc= CCLoadTableFromBuffer(pszCCTable, &hCCT);
	CCSetDebugCallbacks(hCCT, CCMatchLineCallbackProc, CCExecuteLineCallbackProc);
	nSizeOutputBuffer= sizeof(szOutputBuffer);
	pszInputBuffer= "\x93" "aaaaaaa" "\x96" "\x94";
	memset(szOutputBuffer, 0, sizeof(szOutputBuffer));
	rc= CCProcessBuffer(hCCT, pszInputBuffer, strlen(pszInputBuffer),
		szOutputBuffer, &nSizeOutputBuffer);

	if (strcmp(szOutputBuffer, "\xA9" "bbbbbbb" "\x96" "\xAA") != 0 ||
		 nSizeOutputBuffer != 10)
	{
		bFailure= 1;
		MessageBox(NULL, "Error in CCProcessBuffer test. Output incorrect", "CC DLL Test", MB_ICONHAND);
	}
	else
	{
		fprintf(stderr, "Single Buffer test with 8 bit characters: passed\n");
	}
	
    /**********************************************/
	/* Multiple Buffer test with 7 bit characters */
	/* output buffer large enough to hold all the */
	/* final data                                 */
    /**********************************************/
	CCReinitializeTable(hCCT);
	memset(szOutputBuffer, 0, sizeof(szOutputBuffer));
	nSizeOutputBuffer= sizeof(szOutputBuffer);
	pszInputBuffer= "aaaaaaaaaa";
	rc= CCMultiProcessBuffer(hCCT, pszInputBuffer, strlen(pszInputBuffer), 0,
		szOutputBuffer, &nSizeOutputBuffer);

	nSizeOutputBuffer= sizeof(szOutputBuffer);
	memset(szOutputBuffer, 0, sizeof(szOutputBuffer));
	pszInputBuffer= "aaaaaaaaaa";
	rc= CCMultiProcessBuffer(hCCT, pszInputBuffer, strlen(pszInputBuffer), 1,
		szOutputBuffer, &nSizeOutputBuffer);

	if (strcmp(szOutputBuffer, "bbbbbbbbbbbbbbbbbbbb") != 0 ||
		 nSizeOutputBuffer != 20 ||
		 rc != CC_GOT_END_OF_DATA)
	{
		bFailure= 1;
		MessageBox(NULL, "Error in CCMultiProcessBuffer test. Output incorrect", "CC DLL Test", MB_ICONHAND);
	}
	else
	{
		fprintf(stderr, "Multi Buffer test 1 with 7 bit characters: passed\n");
	}
	
	CCReinitializeTable(hCCT);

    /**********************************************/
	/* Multiple Buffer test with 7 bit characters */
	/* output buffer not large enough to hold all */
	/* the final data, multiple calls required    */
    /* after  input data has been exhausted       */
    /**********************************************/
	nSizeOutputBuffer= sizeof(szOutputBuffer);
	memset(szOutputBuffer, 0, sizeof(szOutputBuffer));
	pszInputBuffer= "aaaaaaaaaa";
	rc= CCMultiProcessBuffer(hCCT, pszInputBuffer, strlen(pszInputBuffer), 0,
		szOutputBuffer, &nSizeOutputBuffer);

	pszInputBuffer= "cccccddddd";
	nSizeOutputBuffer= 10;
	memset(szOutputBuffer, 0, sizeof(szOutputBuffer));
	rc= CCMultiProcessBuffer(hCCT, pszInputBuffer, strlen(pszInputBuffer), 1,
		szOutputBuffer, &nSizeOutputBuffer);

	nSizeOutputBuffer= 5;
	memset(szOutputBuffer, 0, sizeof(szOutputBuffer));
	rc= CCMultiProcessBuffer(hCCT, NULL, 0, 1,
		szOutputBuffer, &nSizeOutputBuffer);

	if (strcmp(szOutputBuffer, "ccccc") != 0 ||
		 nSizeOutputBuffer != 5 ||
		 rc != CC_CALL_AGAIN_FOR_MORE_DATA)
	{
		bFailure= 1;
		MessageBox(NULL, "Error in CCMultiProcessBuffer test. Output incorrect", "CC DLL Test", MB_ICONHAND);
    }
	else
	{
		fprintf(stderr, "Multi Buffer test 2a with 7 bit characters: passed\n");
	}
    
	nSizeOutputBuffer= 5;
	memset(szOutputBuffer, 0, sizeof(szOutputBuffer));
	rc= CCMultiProcessBuffer(hCCT, NULL, 0, 1,
		szOutputBuffer, &nSizeOutputBuffer);

	if (strcmp(szOutputBuffer, "ddddd") != 0 ||
		 nSizeOutputBuffer != 5 ||
		 rc != CC_GOT_END_OF_DATA)
	{
		bFailure= 1;
		MessageBox(NULL, "Error in CCMultiProcessBuffer test. Output incorrect", "CC DLL Test", MB_ICONHAND);
    }
	else
	{
		fprintf(stderr, "Multi Buffer test 2b with 7 bit characters: passed\n");
	}
    
    /*********************/
    /* Process File test */
    /*********************/
    
	CCReinitializeTable(hCCT);

	fTestFile= fopen("procfile.in", "w");
	
	unlink("procfile.out");
	
	fprintf(fTestFile, "aaaaaaaaaacccccddddd\n");

	fclose(fTestFile);

	rc=  CCProcessFile (hCCT, "procfile.in", "procfile.out", 0);

	rc= CCUnloadTable(hCCT);

	fTestFile= fopen("procfile.out","r");
	
	if (!fTestFile) {
		bFailure= 1;
	}
	else
	{
		pChar= szOutputBuffer;
	
		while((ch= fgetc(fTestFile)) != EOF)
			*pChar++= ch;
	
		pChar= '\0';
	
		fclose(fTestFile);
	
		if (strcmp(szOutputBuffer, "bbbbbbbbbbcccccddddd\n") != 0)
		{
			bFailure= 1;
			MessageBox(NULL, "Error in CCProcessFile test. Output incorrect", "CC DLL Test", MB_ICONHAND);
		}
		else
		{
			fprintf(stderr, "Process File test: passed\n");
		}
	}    
    /**********************************************************/
	/* Multiple Buffer test with a fwd in the begin statement */
	/* In the first call to CCMultipleProcessBuffer, pass     */
	/* less then the required data to execute the begin       */
	/* statement. The DLL should wait until enough data is in */
	/* input buffer before it executes the begin statement    */
	/**********************************************************/
	
	pszCCTable= "begin > fwd(10) use(main)\ngroup(main)\n'a' > 'b'\n";
	
	rc= CCLoadTableFromBuffer(pszCCTable, &hCCT);
	CCSetDebugCallbacks(hCCT, CCMatchLineCallbackProc, CCExecuteLineCallbackProc);

	nSizeOutputBuffer= sizeof(szOutputBuffer);
	memset(szOutputBuffer, 0, sizeof(szOutputBuffer));
	pszInputBuffer= "aaaaa";
	rc= CCMultiProcessBuffer(hCCT, pszInputBuffer, strlen(pszInputBuffer), 0,
		szOutputBuffer, &nSizeOutputBuffer);

	nSizeOutputBuffer= sizeof(szOutputBuffer);
	memset(szOutputBuffer, 0, sizeof(szOutputBuffer));
	pszInputBuffer= "aaaaa";
	rc= CCMultiProcessBuffer(hCCT, pszInputBuffer, strlen(pszInputBuffer), 0,
		szOutputBuffer, &nSizeOutputBuffer);

	nSizeOutputBuffer= sizeof(szOutputBuffer);
	memset(szOutputBuffer, 0, sizeof(szOutputBuffer));
	pszInputBuffer= "aaaaa";
	rc= CCMultiProcessBuffer(hCCT, pszInputBuffer, strlen(pszInputBuffer), 0,
		szOutputBuffer, &nSizeOutputBuffer);

	pszInputBuffer= "cccccddddd";
	nSizeOutputBuffer= 15;
	memset(szOutputBuffer, 0, sizeof(szOutputBuffer));
	rc= CCMultiProcessBuffer(hCCT, pszInputBuffer, strlen(pszInputBuffer), 1,
		szOutputBuffer, &nSizeOutputBuffer);

	if (strcmp(szOutputBuffer, "aaaaaaaaaabbbbb") != 0 ||
		 nSizeOutputBuffer != 15 ||
		 rc != CC_CALL_AGAIN_FOR_MORE_DATA)
	{
		bFailure= 1;
		MessageBox(NULL, "Error in CCMultiProcessBuffer test. Output incorrect", "CC DLL Test", MB_ICONHAND);
    }
	else
	{
		fprintf(stderr, "Multi Buffer test 3a with 7 bit characters: passed\n");
	}
    
	nSizeOutputBuffer= 5;
	memset(szOutputBuffer, 0, sizeof(szOutputBuffer));
	rc= CCMultiProcessBuffer(hCCT, NULL, 0, 1,
		szOutputBuffer, &nSizeOutputBuffer);

	if (strcmp(szOutputBuffer, "ccccc") != 0 ||
		 nSizeOutputBuffer != 5 ||
		 rc != CC_CALL_AGAIN_FOR_MORE_DATA)
	{
		bFailure= 1;
		MessageBox(NULL, "Error in CCMultiProcessBuffer test. Output incorrect", "CC DLL Test", MB_ICONHAND);
    }
    else
	{
		fprintf(stderr, "Multi Buffer test 3b with 7 bit characters: passed\n");
	}

	nSizeOutputBuffer= 5;
	memset(szOutputBuffer, 0, sizeof(szOutputBuffer));
	rc= CCMultiProcessBuffer(hCCT, NULL, 0, 1,
		szOutputBuffer, &nSizeOutputBuffer);

	if (strcmp(szOutputBuffer, "ddddd") != 0 ||
		 nSizeOutputBuffer != 5 ||
		 rc != CC_GOT_END_OF_DATA)
	{
		bFailure= 1;
		MessageBox(NULL, "Error in CCMultiProcessBuffer test. Output incorrect", "CC DLL Test", MB_ICONHAND);
    }
	else
	{
		fprintf(stderr, "Multi Buffer test 3c with 7 bit characters: passed\n");
	}

	/************************/
	/* Output callback test */
	/************************/
	
	CCReinitializeTable(hCCT);
	
	fOutputFile= fopen("ccputbuf.out", "w");
    
	CCSetUpOutputFilter(hCCT, CCOutputCallback, (long) fOutputFile);

	pszInputBuffer= "aaaaa";
	rc= CCPutBuffer(hCCT, pszInputBuffer, strlen(pszInputBuffer), 0)	;

	pszInputBuffer= "aaaaa";
	rc= CCPutBuffer(hCCT, pszInputBuffer, strlen(pszInputBuffer), 0);

	pszInputBuffer= "aaaaa";
	rc= CCPutBuffer(hCCT, pszInputBuffer, strlen(pszInputBuffer), 0);

	pszInputBuffer= "cccccddddd";	
	rc= CCPutBuffer(hCCT, pszInputBuffer, strlen(pszInputBuffer), 1);

	fclose(fOutputFile);

	rc= CCUnloadTable(hCCT);
	
	fTestFile= fopen("ccputbuf.out","r");
	
	pChar= szOutputBuffer;

	while((ch= fgetc(fTestFile)) != EOF)
		*pChar++= ch;

	pChar= '\0';

	fclose(fTestFile);

	if (strcmp(szOutputBuffer, "aaaaaaaaaabbbbbcccccddddd") != 0)
	{
		bFailure= 1;
		MessageBox(NULL, "Error in CCPutBuffer test. Output incorrect", "CC DLL Test", MB_ICONHAND);
    }
	else
	{
		fprintf(stderr, "CCPutBuffer test: passed\n");
	}
    
	/***********************/
	/* Input callback test */
	/***********************/

	pszCCTable= "begin > fwd(10) use(main)\ngroup(main)\n'a' > 'b'\n";
	
	rc= CCLoadTableFromBuffer(pszCCTable, &hCCT);
	CCSetDebugCallbacks(hCCT, CCMatchLineCallbackProc, CCExecuteLineCallbackProc);
	CCSetUpInputFilter(hCCT, CCInputCallbackProc, (long) NULL);

	iInputState= 0;

	memset(szOutputBuffer, 0 , sizeof(szOutputBuffer));

	do
	{
		nSizeGetBuffer= sizeof(szGetBuffer);
		rc= CCGetBuffer(hCCT, szGetBuffer, &nSizeGetBuffer);
		strncat(szOutputBuffer, szGetBuffer, nSizeGetBuffer);
	}
	while (rc != CC_GOT_END_OF_DATA);

	rc= CCUnloadTable(hCCT);

	if (strcmp(szOutputBuffer, "aaaaaaaaaabbbbbcccccddddd") != 0)
	{
		bFailure= 1;
		MessageBox(NULL, "Error in CCGetBuffer test. Output incorrect", "CC DLL Test", MB_ICONHAND);
    }
	else
	{
		fprintf(stderr, "CCGetBuffer test: passed\n");
	}
    
    if (!bFailure)
		MessageBox(NULL, "Passed all test.", "CC DLL Test", MB_ICONHAND);
    	
	return (0);
}
