
/**********************************************************************************
 *   ____      _                    ____
 *  |  _ \ ___| |_ _ __ ___        / ___|
 *  | |_) / _ \ __| '__/ _ \ _____| |
 *  |  _ <  __/ |_| | | (_) |_____| |___
 *  |_| \_\___|\__|_|  \___/       \____|
 *
 *
 *  RFCPTOOL - STDC Retro-Frame Codepage Tool
 *
 *  Source File
 *
 *  Repository:    <http://source.retro-c.net/comp.stdc.rfcptool>
 *  File:          /src/rfcptool_main.c//
 *  Version:       01.00!00
 *  Environments:  C90
 *  Compliance:    Retro-Frame 1.0
 *  License:       MIT
 *
 *  Copyright (c) 2026 Ingo Boehmer <ingo@retro-leisure.net>
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *
 **********************************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include "rfcptool_def.h"
#include "rfcptool_util.h"
#include "rfcptool_encode.h"
#include "rfcptool_decode.h"
#include "rfcptool_cpspec.h"
#include "rfcptool_native.h"
#include "rfcptool_verify.h"


#define RFCPTOOL_COPYRIGHT                "Copyright (c) 2026 Ingo Boehmer <ingo@retro-leisure.net>"
#define RFCPTOOL_VERSION                  "1.0!0 (alpha)"

#ifndef RFCPTOOL_OPTION_PREFIX
#define RFCPTOOL_OPTION_PREFIX            '-'
#endif


int main(int argc, char *argv[])
{
	RFCPTOOL_DIR *pDirList;
	RFCPTOOL_DIR **ppDirTail;
	int fOverwrite;
	int argi;
	
	fprintf(stderr, "RFCPTOOL - Retro-Frame Codepage Tool v%s\n", RFCPTOOL_VERSION);
	
	if (argc < 2)
	{
		fprintf(stderr, "%s\n\n", RFCPTOOL_COPYRIGHT);
		
		printf("Usage:\n\n");
		printf("  rfcptool encode [ <option> ]* <src-file> <dest-file>\n");
		printf("  rfcptool decode [ <option> ]* <src-file> [ <dest-file> ]\n");
		printf("  rfcptool cpspec [ <option> ]* <codepage> [ <dest-file> ]\n");
		printf("  rfcptool native [ <option> ]* [ <dest-file> ]\n");
		printf("  rfcptool verify <encoding> <src-file>\n");
		printf("  rfcptool license\n");
		printf("\nArguments (encode):\n\n");
		printf("  <src-file>        Path to codepage code file (*.CPC)\n");
		printf("  <dest-file>       Path to binary codepage file (*.CP)\n");
		printf("\nArguments (decode):\n\n");
		printf("  <src-file>        Path to binary codepage file (*.CP)\n");
		printf("  <dest-file>       Path to codepage code file (*.CPC)\n");
		printf("\nArguments (cpspec):\n\n");
		printf("  <codepage>        Codepage reference in the format <domain>:<identifier>\n");
		printf("  <dest-file>       Path to binary code file (*.CP) - if omitted, a codepage\n");
		printf("                    code file format is generated and written to stdout\n");
		printf("\nArgument (native):\n\n");
		printf("  <dest-file>       Path to binary code file (*.CP) - if omitted, a codepage\n");
		printf("                    code file format is generated and written to stdout\n");
		printf("\nArguments (verify):\n\n");
		printf("  <encoding>        Character encoding to be verified (ASCII, LATIN-1, PCS,\n");
		printf("                    UTF-8, UTF-8X, CESU-8, CESU-8X, UTF-16BE, UTF-16LE,\n");
		printf("                    UCS-2BE, UCS-2LE, UTF-32BE, UTF-32LE, UCS-4BE or UCS4-LE)\n");
		printf("  <src-file>        Path to binary codepage file (*.CP)\n");
		printf("\nOptions:\n\n");
		printf("  %cO                Overwrite existing <dest-file>\n", RFCPTOOL_OPTION_PREFIX);
		printf("  %cCPSPECDIR <dir>  Include <dir> in codepage specification file (*.CPS) search\n", RFCPTOOL_OPTION_PREFIX);
		printf("                    (use trailing path separator if necessary)\n");
		
		return EXIT_SUCCESS;
	}
	
	if (RFCPTOOL_IsStr(argv[1], "license", RFCPTOOL_IS_STR_FLAG_IGNORE_CASE))
	{
		fprintf(stderr, "\n");
		
		printf("MIT License\n\n");
		printf("%s\n\n", RFCPTOOL_COPYRIGHT);
		printf("Permission is hereby granted, free of charge, to any person obtaining a copy\n");
		printf("of this software and associated documentation files (the \"Software\"), to deal\n");
		printf("in the Software without restriction, including without limitation the rights\n");
		printf("to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n");
		printf("copies of the Software, and to permit persons to whom the Software is\n");
		printf("furnished to do so, subject to the following conditions:\n\n");
		printf("The above copyright notice and this permission notice shall be included in all\n");
		printf("copies or substantial portions of the Software.\n\n");
		printf("THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n");
		printf("IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n");
		printf("FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n");
		printf("AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n");
		printf("LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n");
		printf("OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\n");
		printf("SOFTWARE.\n\n");
		
		fprintf(stderr, "Source: <http://source.retro-c.net/comp.stdc.rfcptool>\n");
		
		if (argc == 2)
		{
			return EXIT_SUCCESS;
		}
	}
	else
	{
		fprintf(stderr, "%s\n", RFCPTOOL_COPYRIGHT);
		
		pDirList = RFCPTOOL_CreateDefaultDir();
		ppDirTail = (pDirList != NULL) ? &pDirList->pNext : &pDirList;
		
		argi = 2;
		fOverwrite = 0;
		
		while ((argi < argc) && (argv[argi][0] == RFCPTOOL_OPTION_PREFIX))
		{
			if (RFCPTOOL_IsStr(&argv[argi][1], "O", RFCPTOOL_IS_STR_FLAG_IGNORE_CASE))
			{
				fOverwrite = 1;
			}
			else if (RFCPTOOL_IsStr(&argv[argi][1], "CPSPECDIR", RFCPTOOL_IS_STR_FLAG_IGNORE_CASE))
			{
				if (argi + 1 < argc)
				{
					*ppDirTail = RFCPTOOL_CreateDir(argv[++argi]);
					
					if (*ppDirTail == NULL)
					{
						RFCPTOOL_DestroyDirList(&pDirList);
						return EXIT_FAILURE;
					}
				}
				else
				{
					argc = 0;
				}
			}
			else
			{
				argc = 0;
			}
			
			++argi;
		}
		
		if (argi <= argc)
		{
			if (RFCPTOOL_IsStr(argv[1], "ENCODE", RFCPTOOL_IS_STR_FLAG_IGNORE_CASE))
			{
				RFCPTOOL_DestroyDirList(&pDirList);
				
				switch (argc - argi)
				{
				case 2:
					
					return RFCPTOOL_Encode(argv[argi], argv[argi + 1], fOverwrite) ? EXIT_SUCCESS : EXIT_FAILURE;
				}
			}
			else if (RFCPTOOL_IsStr(argv[1], "DECODE", RFCPTOOL_IS_STR_FLAG_IGNORE_CASE))
			{
				RFCPTOOL_DestroyDirList(&pDirList);
				
				switch (argc - argi)
				{
				case 1:
					
					return RFCPTOOL_Decode(argv[argi], NULL, fOverwrite) ? EXIT_SUCCESS : EXIT_FAILURE;
					
				case 2:
					
					return RFCPTOOL_Decode(argv[argi], argv[argi + 1], fOverwrite) ? EXIT_SUCCESS : EXIT_FAILURE;
				}
			}
			else if (RFCPTOOL_IsStr(argv[1], "CPSPEC", RFCPTOOL_IS_STR_FLAG_IGNORE_CASE))
			{
				switch (argc - argi)
				{
				case 1:
					
					if (!RFCPTOOL_CPSpec(argv[argi], NULL, pDirList, fOverwrite))
					{
						RFCPTOOL_DestroyDirList(&pDirList);
						return EXIT_FAILURE;
					}
					
					RFCPTOOL_DestroyDirList(&pDirList);
					return EXIT_SUCCESS;
					
				case 2:
					
					if (!RFCPTOOL_CPSpec(argv[argi], argv[argi + 1], pDirList, fOverwrite))
					{
						RFCPTOOL_DestroyDirList(&pDirList);
						return EXIT_FAILURE;
					}
					
					RFCPTOOL_DestroyDirList(&pDirList);
					return EXIT_SUCCESS;
				}
			}
			else if (RFCPTOOL_IsStr(argv[1], "NATIVE", RFCPTOOL_IS_STR_FLAG_IGNORE_CASE))
			{
				RFCPTOOL_DestroyDirList(&pDirList);
				
				switch (argc - argi)
				{
				case 0:
					
					return RFCPTOOL_Native(NULL, fOverwrite) ? EXIT_SUCCESS : EXIT_FAILURE;
					
				case 1:
					
					return RFCPTOOL_Native(argv[argi], fOverwrite) ? EXIT_SUCCESS : EXIT_FAILURE;
				}
			}
			else if (RFCPTOOL_IsStr(argv[1], "VERIFY", RFCPTOOL_IS_STR_FLAG_IGNORE_CASE))
			{
				RFCPTOOL_DestroyDirList(&pDirList);
				
				switch (argc - argi)
				{
				case 2:
					
					return RFCPTOOL_Verify(argv[argi], argv[argi + 1]) ? EXIT_SUCCESS : EXIT_FAILURE;
				}
			}
		}
		
		RFCPTOOL_DestroyDirList(&pDirList);
	}
	
	RFCPTOOL_Error("Invalid argument(s)", NULL, NULL, NULL, 0);
	
	return EXIT_FAILURE;
}
