
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
 *  File:          /src/rfcptool_file.c//
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


#define _CRT_SECURE_NO_WARNINGS

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rfcptool_def.h"
#include "rfcptool_util.h"
#include "rfcptool_file.h"


/*****************************************************************************************
 *
 *  I N T E R N A L   F U N C T I O N S
 *
 *****************************************************************************************/


static int L_IsSuffix(const char *sPath, const char *sSuffix)
{
	size_t pathLen;
	size_t suffixLen;
	
	if ((sPath == NULL) || (sSuffix == NULL))
	{
		return 0;
	}
	
	pathLen = strlen(sPath);
	suffixLen = strlen(sSuffix);
	
	if (pathLen < suffixLen + 1)
	{
		return 0;
	}
	
	return RFCPTOOL_IsStr(&sPath[pathLen - suffixLen], sSuffix, RFCPTOOL_IS_STR_FLAG_IGNORE_CASE);
}


static int L_VerifyVersion(RFCPTOOL_FILE_TEXT *pFile, int offset, int reqMajor, int reqMinor)
{
	int actMajor;
	int actMinor;
	
	if ((RFCPTOOL_FILE_PEEK_CHAR(pFile, offset) >= '1') && (RFCPTOOL_FILE_PEEK_CHAR(pFile, offset) <= '9'))
	{
		actMajor = RFCPTOOL_FILE_READ_CHAR(pFile, offset) - '0';
		
		if ((RFCPTOOL_FILE_PEEK_CHAR(pFile, offset) >= '0') && (RFCPTOOL_FILE_PEEK_CHAR(pFile, offset) <= '9'))
		{
			actMajor = (actMajor * 10) + (RFCPTOOL_FILE_READ_CHAR(pFile, offset) - '0');
		}
		
		if ((actMajor <= 32) && (RFCPTOOL_FILE_READ_CHAR(pFile, offset) == '.'))
		{
			if ((RFCPTOOL_FILE_PEEK_CHAR(pFile, offset) >= '0') && (RFCPTOOL_FILE_PEEK_CHAR(pFile, offset) <= '9'))
			{
				actMinor = RFCPTOOL_FILE_READ_CHAR(pFile, offset) - '0';
				
				if ((RFCPTOOL_FILE_PEEK_CHAR(pFile, offset) >= '0') && (RFCPTOOL_FILE_PEEK_CHAR(pFile, offset) <= '9'))
				{
					actMinor = (actMinor * 10) + (RFCPTOOL_FILE_READ_CHAR(pFile, offset) - '0');
				}
				
				if ((actMinor <= 31) && ((RFCPTOOL_FILE_PEEK_CHAR(pFile, offset) == ':') || (RFCPTOOL_FILE_PEEK_CHAR(pFile, offset) == '?') || (RFCPTOOL_FILE_PEEK_CHAR(pFile, offset) == '\n')))
				{
					if (actMajor > reqMajor)
					{
						RFCPTOOL_Error("Format version", " of input file ", pFile->ref.sPath, " not supported", 0);
						return 0;
					}
					
					if (actMinor > reqMinor)
					{
						RFCPTOOL_Warning("Not all format features", " of input file ", pFile->ref.sPath, " supported", 0);
					}
					
					return offset;
				}
			}
		}
	}
	
	RFCPTOOL_Error("Invalid format version", " of input file ", pFile->ref.sPath, NULL, 0);
	
	return 0;
}


static int L_SkipHeaderElements(RFCPTOOL_FILE_TEXT *pFile, int offset, int fTerminal, char sHeader[], size_t headerLenMax)
{
	char c1, c2;
	int fEscape;
	size_t pos;
	
	pos = 0;
	
	c1 = RFCPTOOL_FILE_READ_CHAR(pFile, offset);
	
	if (c1 == ':')
	{
		/* skip header elements: */
		
		fEscape = 0;
		
		do
		{
			if (c1 == '\n')
			{
				++pFile->headerLines;
			}
			
			if (offset >= RFCPTOOL_FILE_BUF_SIZE - 1)  /* ensure that at least two characters are in the buffer */
			{
				if (!RFCPTOOL_FileReadText(pFile, offset))
				{
					return 0;
				}
				
				offset = 0;
			}
			
			c2 = c1;
			c1 = RFCPTOOL_FILE_READ_CHAR(pFile, offset);
			
			if (c1 == '\0')
			{
				RFCPTOOL_Error("Unexpected end of format header", " in input file ", pFile->ref.sPath, NULL, 0);
				return 0;
			}
			
			fEscape ^= (c2 != '^') ? fEscape : 1;
			
			if (sHeader != NULL)
			{
				if ((c1 == '\n') || (((c1 == ':') || (c1 == '?')) && !fEscape))
				{
					if (pos <= headerLenMax)
					{
						sHeader[pos] = '\0';  /* caller must check whether sHeader is terminated (overflow otherwise) */
					}
					
					sHeader = NULL;  /* ignore sHeader on subsequent header elements */
				}
				else if ((pos <= headerLenMax) && !fEscape)
				{
					sHeader[pos++] = c1;
				}
			}
		}
		while (((c1 != '\n') || (c2 == ':')) && ((c1 != '?') || fEscape));  /*!! this seems not quite correct as it would allow a line break after an escpated colon (e.g. "^:\n") but maybe we accept it !!*/
	}
	
	switch (c1)
	{
	case '\n':
		
		++pFile->headerLines;
		
		if (fTerminal)
		{
			return RFCPTOOL_FileReadText(pFile, offset);
		}
		break;
		
	case '?':
		
		if (!fTerminal)
		{
			if (RFCPTOOL_FILE_PEEK_CHAR(pFile, offset) == '\n')
			{
				++offset;
			}
			
			return RFCPTOOL_FileReadText(pFile, offset);
		}
		
		if (RFCPTOOL_FILE_READ_CHAR(pFile, offset) == '?')
		{
			return RFCPTOOL_FileReadText(pFile, offset);
		}
	}
	
	RFCPTOOL_Error("Invalid format header", " of input file ", pFile->ref.sPath, NULL, 0);
	
	return 0;
}


static int L_ParseHeader(RFCPTOOL_FILE_TEXT *pFile, const char *sFileType, char sHeader[], size_t headerLenMax)
{
	int offset;
	
	offset = L_VerifyVersion(pFile, 5, 1, 0);  /* start at position 5 as "RFFF/" has been matched already */
	
	if (!offset)
	{
		return 0;
	}
	
	if (!L_SkipHeaderElements(pFile, offset, 0, NULL, 0))
	{
		return 0;
	}
	
	offset = 0;
	
	while (RFCPTOOL_FILE_READ_CHAR(pFile, offset) == *sFileType)
	{
		if (*++sFileType == '\0')
		{
			if (RFCPTOOL_FILE_READ_CHAR(pFile, offset) == '/')
			{
				offset = L_VerifyVersion(pFile, offset, 1, 0);
				
				if (!offset)
				{
					return 0;
				}
				
				return L_SkipHeaderElements(pFile, offset, 1, sHeader, headerLenMax);
			}
		}
	}
	
	return -1;
}


static RFCPTOOL_FILE_TEXT* L_OpenFileText(RFCPTOOL_FILE_TEXT *pFile, const char *sPath, const char *sFileType, const char *sSuffix, char sHeader[], size_t headerLenMax, int *pfSkip)
{
	int fRewind;
	
	assert(sFileType != NULL);
	
	if (sHeader != NULL)
	{
		sHeader[0] = '\0';
	}
	
	if (pfSkip != NULL)
	{
		*pfSkip = 0;
	}
	
	if (pFile != NULL)
	{
		fRewind = 1;
	}
	else
	{
		fRewind = 0;
		
		pFile = RFCPTOOL_MemAlloc(sizeof(RFCPTOOL_FILE_TEXT));
		
		if (pFile == NULL)
		{
			return NULL;
		}
		
		if (sPath == NULL)
		{
			pFile->ref.pHandle = stdin;  /* will never be used as neither CPCODE nor CPSPEC may be read from stdin but anyway... */
		}
		else
		{
			pFile->ref.pHandle = fopen(sPath, "r");
			
			if (pFile->ref.pHandle == NULL)
			{
				if (pfSkip != NULL)
				{
					*pfSkip = 1;
				}
				else
				{
					RFCPTOOL_Error("Cannot open", " input file ", sPath, NULL, 0);
				}
				
				RFCPTOOL_MemFree(pFile);
				return NULL;
			}
		}
	}
	
	pFile->ref.sPath = sPath;
	pFile->ref.fInput = 1;
	pFile->sFileType = sFileType;
	pFile->fHeader = 0;
	pFile->headerLines = 0;
	pFile->fCR = 0;
	pFile->base = 0;
	pFile->tBuf[0] = '\n';  /* prevent EOF detection */
	
	if (!RFCPTOOL_FileReadText(pFile, RFCPTOOL_FILE_BUF_SIZE))
	{
		if (!fRewind)
		{
			RFCPTOOL_FileDestroy(&pFile->ref);
		}
		
		return 0;
	}
	
	if ((pFile->tBuf[0] == 'R') && (pFile->tBuf[1] == 'F') && (pFile->tBuf[2] == 'F') && (pFile->tBuf[3] == 'F') && (pFile->tBuf[4] == '/'))
	{
		/* if the magix prefix "RFFF/" is present, a valid header is expected: */
		
		pFile->fHeader = 1;
		
		switch (L_ParseHeader(pFile, sFileType, sHeader, headerLenMax))
		{
		case 0:
			
			if (!fRewind)
			{
				RFCPTOOL_FileDestroy(&pFile->ref);
			}
			return NULL;
			
		case 1:
			
			return pFile;
		}
		
		if (pfSkip != NULL)
		{
			*pfSkip = 1;
			
			if (!fRewind)
			{
				RFCPTOOL_FileDestroy(&pFile->ref);
			}
			return NULL;
		}
	}
	else if (L_IsSuffix(sPath, sSuffix))
	{
		return pFile;
	}
	
	RFCPTOOL_Error("Invalid file type", " of input file ", sPath, NULL, 0);
	
	if (!fRewind)
	{
		RFCPTOOL_FileDestroy(&pFile->ref);
	}
	
	return NULL;
}


static RFCPTOOL_FILE_REF* L_OpenFileRef(const char *sPath, int fInput, const char *sMode, int fOverwrite)
{
	RFCPTOOL_FILE_REF *pFile;
	
	assert(sMode != NULL);
	
	pFile = RFCPTOOL_MemAlloc(sizeof (RFCPTOOL_FILE_REF));
	
	if (pFile == NULL)
	{
		return NULL;
	}
	
	if (!fOverwrite && (sPath != NULL))
	{
		pFile->pHandle = fopen(sPath, "rb");
		
		if (pFile->pHandle != NULL)
		{
			fclose(pFile->pHandle);
			RFCPTOOL_Error("Output file", " ", sPath, " already exists", 0);
			RFCPTOOL_MemFree(pFile);
			return NULL;
		}
	}
	
	if (sPath == NULL)
	{
		pFile->pHandle = fInput ? stdin : stdout;
	}
	else
	{
		pFile->pHandle = fopen(sPath, sMode);
		
		if (pFile->pHandle == NULL)
		{
			RFCPTOOL_Error("Cannot open", fInput ? " input file " : " output file ", sPath, NULL, 0);
			RFCPTOOL_MemFree(pFile);
			return NULL;
		}
	}
	
	pFile->sPath = sPath;
	pFile->fInput = fInput;
	
	return pFile;
}


/*****************************************************************************************
 *
 *  E X T E R N A L   F U N C T I O N S
 *
 *****************************************************************************************/


void RFCPTOOL_FileDestroy(RFCPTOOL_FILE_REF *pFile)
{
	if (pFile != NULL)
	{
		if ((pFile->pHandle != NULL) && (pFile->sPath != NULL))
		{
			fclose(pFile->pHandle);
		}
		
		RFCPTOOL_MemFree(pFile);
	}
}


int RFCPTOOL_FileClose(RFCPTOOL_FILE_REF *pFile)
{
	assert(pFile != NULL);
	
	if ((pFile->pHandle == NULL) || (pFile->sPath == NULL))
	{
		return 1;
	}
	
	if (fclose(pFile->pHandle))
	{
		if (pFile->fInput)
		{
			RFCPTOOL_Warning("Closing input", " file ", pFile->sPath, " failed", 0);
		}
		else
		{
			RFCPTOOL_Error("Closing output", " file ", pFile->sPath, " failed", 0);
		}
		
		return 0;
	}
	
	pFile->pHandle = NULL;
	pFile->sPath = NULL;
	
	return 1;
}


RFCPTOOL_FILE_TEXT* RFCPTOOL_FileOpenInputCPCODE(const char *sPath, char sTarget[RFCPTOOL_TARGET_LEN_MAX + 1])
{
	return L_OpenFileText(NULL, sPath, "CP-CODE", ".CPC", sTarget, RFCPTOOL_TARGET_LEN_MAX, NULL);
}


RFCPTOOL_FILE_TEXT* RFCPTOOL_FileOpenInputCPSPEC(const char *sPath, char sHeader[RFCPTOOL_DOMAIN_NAME_LEN_MAX + 1], int *pfSkip)
{
	return L_OpenFileText(NULL, sPath, "CP-SPEC", NULL, sHeader, RFCPTOOL_DOMAIN_NAME_LEN_MAX, pfSkip);  /* no implicit format detection by suffix here */
}


RFCPTOOL_FILE_REF* RFCPTOOL_FileOpenOutputCPCODE(const char *sPath, char *sTarget, int fOverwrite)
{
	RFCPTOOL_FILE_REF *pFile;
	
	pFile = L_OpenFileRef(sPath, 0, "w", fOverwrite);
	
	if (pFile == NULL)
	{
		return NULL;
	}
	
	if (!RFCPTOOL_FileWrite(pFile, fprintf(pFile->pHandle, "RFFF/1.0?CP-CODE/1.0"), -1))
	{
		RFCPTOOL_FileDestroy(pFile);
		return NULL;
	}
	
	if (sTarget != NULL)
	{
		if (!RFCPTOOL_FileWrite(pFile, fprintf(pFile->pHandle, ":%s", sTarget), -1))
		{
			RFCPTOOL_FileDestroy(pFile);
			return NULL;
		}
	}
	
	if (!RFCPTOOL_FileWrite(pFile, fprintf(pFile->pHandle, "\n\n; FILE GENERATED BY RETRO-C RFCPTOOL\n"), -1))
	{
		RFCPTOOL_FileDestroy(pFile);
		return NULL;
	}
	
	return pFile;
}


RFCPTOOL_FILE_REF* RFCPTOOL_FileOpenInputCP(const char *sPath)
{
	return L_OpenFileRef(sPath, 1, "rb", 1);
}


RFCPTOOL_FILE_REF* RFCPTOOL_FileOpenOutputCP(const char *sPath, int fOverwrite)
{
	return L_OpenFileRef(sPath, 0, "wb", fOverwrite);
}


int RFCPTOOL_FileWrite(RFCPTOOL_FILE_REF *pFile, int result, int valid)
{
	if (((valid >= 0) && (result != valid)) || ((valid < 0) && (result < 0)))
	{
		RFCPTOOL_Error("Writing", " to output file ", pFile->sPath, " failed", 0);
		return 0;
	}
	
	return 1;
}


int RFCPTOOL_FileWriteSpace(RFCPTOOL_FILE_REF *pFile, int count)
{
	int result;
	
	if (count <= 0)
	{
		return -1;
	}
	
	do
	{
		result = fprintf(pFile->pHandle, " ");
	}
	while (--count && (result >= 0));
	
	return RFCPTOOL_FileWrite(pFile, result, -1);
}


int RFCPTOOL_FileWriteCodepoint(RFCPTOOL_FILE_REF *pFile, RFCPTOOL_UINT32 codepoint)
{
	if (codepoint <= 0xFFFF)
	{
		if (!RFCPTOOL_FileWrite(pFile, fprintf(pFile->pHandle, "%04X", (int)codepoint), -1))
		{
			return 0;
		}
		
		return 4;
	}
	
	if (!RFCPTOOL_FileWrite(pFile, fprintf(pFile->pHandle, "%06X", (int)codepoint), -1))
	{
		return 0;
	}
	
	return 6;
}


int RFCPTOOL_FileReadText(RFCPTOOL_FILE_TEXT *pFile, int count)
{
	int fCRLF;
	int index;
	int c;
	
	assert(pFile != NULL);
	assert((count >= 0) && (count <= RFCPTOOL_FILE_BUF_SIZE));
	
	if (count > 0)
	{
		index = pFile->base;
		
		pFile->base = (index + count) % RFCPTOOL_FILE_BUF_SIZE;
		
		if (pFile->tBuf[pFile->base] == '\0')
		{
			return 1;  /* EOF */
		}
		
		while (count--)
		{
			do
			{
				do
				{
					fCRLF = 0;
					
					c = fgetc(pFile->ref.pHandle);
					
					switch (c)
					{
					case EOF:
						
						pFile->fCR = 0;
						pFile->tBuf[index] = '\0';  /* use NUL as EOF indicator */
						
						if (ferror(pFile->ref.pHandle))
						{
							RFCPTOOL_Error("Read", " from input file ", pFile->ref.sPath, " failed", 0);
							return 0;
						}
						return 1;
						
					case '\r':
						
						pFile->fCR = 1;
						c = '\n';
						break;
						
					case '\n':
						
						if (pFile->fCR)
						{
							fCRLF = 1;
						}
						/* no break */
						
					default:
						
						pFile->fCR = 0;
					}
				}
				while (fCRLF);
				
				pFile->tBuf[index] = (char)c;
			}
			while (c == '\0');  /* ignore NUL characters */
			
			index = (index + 1) % RFCPTOOL_FILE_BUF_SIZE;
		}
	}
	
	return 1;
}


int RFCPTOOL_FileRewindText(RFCPTOOL_FILE_TEXT *pFile)
{
	assert(pFile != NULL);
	assert(pFile->ref.pHandle != NULL);
	
	rewind(pFile->ref.pHandle);
	
	if (!pFile->fHeader)
	{
		return 1;
	}
	
	return (L_OpenFileText(pFile, pFile->ref.sPath, pFile->sFileType, NULL, NULL, 0, NULL) != NULL) ? 1 : 0;
}


int RFCPTOOL_FileReadBin(RFCPTOOL_FILE_REF *pFile, RFCPTOOL_UINT8 *pBuf, size_t count, int fAllowEOF)
{
	if (fread(pBuf, 1, count, pFile->pHandle) == count)
	{
		return 1;
	}
	
	if (ferror(pFile->pHandle))
	{
		RFCPTOOL_Error("Read", " from input file ", pFile->sPath, " failed", 0);
		return 0;
	}
	
	if (!fAllowEOF)
	{
		RFCPTOOL_Error("Unexpected end of input", " file ", pFile->sPath, NULL, 0);
		return 0;
	}
	
	return -1;
}
