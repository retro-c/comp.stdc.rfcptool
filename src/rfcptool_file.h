
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
 *  Header File
 *
 *  Repository:    <http://source.retro-c.net/comp.stdc.rfcptool>
 *  File:          /src/rfcptool_file.h//
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


#ifndef RFCPTOOL_FILE_H
#define RFCPTOOL_FILE_H


#include <stdio.h>
#include "rfcptool_def.h"


#define RFCPTOOL_FILE_BUF_SIZE            64  /* must be >= maximum length of file type string + 8 */

#define RFCPTOOL_FILE_PEEK_CHAR(FILE_PTR, OFFSET)  (FILE_PTR)->tBuf[((FILE_PTR)->base + (OFFSET)) % RFCPTOOL_FILE_BUF_SIZE]
#define RFCPTOOL_FILE_READ_CHAR(FILE_PTR, OFFSET)  (FILE_PTR)->tBuf[((FILE_PTR)->base + (OFFSET)++) % RFCPTOOL_FILE_BUF_SIZE]


typedef struct RFCPTOOL_FILE_REF {
	FILE *pHandle;
	const char *sPath;
	int fInput;
} RFCPTOOL_FILE_REF;


typedef struct RFCPTOOL_FILE_TEXT {
	RFCPTOOL_FILE_REF ref;
	const char *sFileType;
	int fHeader;
	int headerLines;
	int fCR;
	int base;
	char tBuf[RFCPTOOL_FILE_BUF_SIZE];  /* ring buffer */
} RFCPTOOL_FILE_TEXT;


void RFCPTOOL_FileDestroy(RFCPTOOL_FILE_REF *pFile);  /* may implicitly close the file */
int RFCPTOOL_FileClose(RFCPTOOL_FILE_REF *pFile);

RFCPTOOL_FILE_TEXT* RFCPTOOL_FileOpenInputCPCODE(const char *sPath, char sTarget[RFCPTOOL_TARGET_LEN_MAX + 1]);  /* sTarget may be NULL */
RFCPTOOL_FILE_TEXT* RFCPTOOL_FileOpenInputCPSPEC(const char *sPath, char sDomain[RFCPTOOL_DOMAIN_NAME_LEN_MAX + 1], int *pfSkip);  /* sDomain and pfSkip may be NULL (if pfSkip is not NULL, open and invalid header errors are supressed) */
RFCPTOOL_FILE_REF* RFCPTOOL_FileOpenOutputCPCODE(const char *sPath, char *sTarget, int fOverwrite);  /* sTarget may be NULL */
RFCPTOOL_FILE_REF* RFCPTOOL_FileOpenInputCP(const char *sPath);
RFCPTOOL_FILE_REF* RFCPTOOL_FileOpenOutputCP(const char *sPath, int fOverwrite);

int RFCPTOOL_FileWrite(RFCPTOOL_FILE_REF *pFile, int result, int valid);  /* this function only performs error handling if result != valid (if valid >= 0) or result < 0 (if valid < 0) */

int RFCPTOOL_FileWriteSpace(RFCPTOOL_FILE_REF *pFile, int count);
int RFCPTOOL_FileWriteCodepoint(RFCPTOOL_FILE_REF *pFile, RFCPTOOL_UINT32 codepoint);

int RFCPTOOL_FileReadText(RFCPTOOL_FILE_TEXT *pFile, int count);
int RFCPTOOL_FileRewindText(RFCPTOOL_FILE_TEXT *pFile);

int RFCPTOOL_FileReadBin(RFCPTOOL_FILE_REF *pFile, RFCPTOOL_UINT8 *pBuf, size_t count, int fAllowEOF);


#endif
