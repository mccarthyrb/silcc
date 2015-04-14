#include <stdio.h>

#include "ccdll.h"

int main(int argc, char ** argv)
{
	HANDLE hCCT;
	int rc;

	if (argc != 4)
	{
		fprintf(stderr, "Usage: filetest in cct out\n");
		return 1;
	}
	
	rc = CCLoadTable2(argv[2], &hCCT);
	rc = CCProcessFile (hCCT, argv[1], argv[3], 0);
	rc = CCUnloadTable(hCCT);
	return 0;
}
