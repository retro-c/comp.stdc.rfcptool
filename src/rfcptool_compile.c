
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
 *  File:          /src/rfcptool_compile.c//
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
#include "rfcptool_compile.h"



/*****************************************************************************************
 *
 *  L O C A L   V A R I A B L E S
 *
 *****************************************************************************************/


const char *ltsSymbolSimple[3] = { "<<", "-", "." };
const char *ltsSymbolShiftOut[3] = { "> -", "> .", "> /" };
const char *ltsSymbolMultibyte[3] = { "MULTIBYTE -", "MULTIBYTE .", "MULTIBYTE /" };


/*****************************************************************************************
 *
 *  I N T E R N A L   F U N C T I O N S
 *
 *****************************************************************************************/


static int L_IsTag(const RFCPTOOL_UINT32 tTag[256], int codeStart, RFCPTOOL_UINT32 tag)
{
	int codeEnd;
	
	if ((codeStart > 0xFF) || (tTag[codeStart] != tag))
	{
		return 0;
	}
	
	codeEnd = codeStart;
	
	while (++codeEnd <= 0xFF)
	{
		if (tTag[codeEnd] != tag)
		{
			return codeEnd - 1;
		}
	}
	
	return 0xFF;
}


static int L_IsConsecutive(const RFCPTOOL_UINT32 tTag[256], int codeStart)
{
	RFCPTOOL_UINT32 codepoint;
	
	if ((codeStart > 0xFF) || ((tTag[codeStart] & RFCPTOOL_TAG_MASK_TYPE) != RFCPTOOL_TAG_TYPE_CODEPOINT))
	{
		return 0;
	}
	
	codepoint = tTag[codeStart];
	
	while (++codeStart <= 0xFF)
	{
		if (tTag[codeStart] != (RFCPTOOL_UINT32)++codepoint)
		{
			return codeStart - 1;
		}
	}
	
	return 0xFF;
}


static int L_WritePrepare(RFCPTOOL_FILE_REF *pFile, int codeStart, int codeEnd, int spaceCount)
{
	RFCPTOOL_UINT8 tBuf[2];
	
	assert(pFile != NULL);
	
	if (codeEnd > codeStart)
	{
		if (pFile->sPath == NULL)
		{
			return RFCPTOOL_FileWrite(pFile, fprintf(pFile->pHandle, "\n%02X..%02X  ", codeStart, codeEnd), -1);
		}
		
		tBuf[0] = 0xFF;
		tBuf[1] = (RFCPTOOL_UINT8)(codeEnd - codeStart - 1);
		
		return RFCPTOOL_FileWrite(pFile, (int)fwrite(tBuf, 1, 2, pFile->pHandle), 2);
	}
	
	if (pFile->sPath != NULL)
	{
		return 1;
	}
	
	if (!spaceCount || !(codeStart % 16))
	{
		return RFCPTOOL_FileWrite(pFile, fprintf(pFile->pHandle, "\n%02X      ", codeStart), -1);
	}
	
	return RFCPTOOL_FileWriteSpace(pFile, spaceCount);
}


static int L_WriteCodepoint(RFCPTOOL_FILE_REF *pFile, RFCPTOOL_UINT32 codepoint)
{
	RFCPTOOL_UINT8 tBuf[3];
	size_t count;
	
	assert(pFile != NULL);
	
	if (pFile->sPath == NULL)
	{
		return RFCPTOOL_FileWriteCodepoint(pFile, codepoint);
	}
	
	count = RFCPTOOL_EncodePCS(codepoint, tBuf);
	
	assert(count > 0);
	
	return RFCPTOOL_FileWrite(pFile, (int)fwrite(tBuf, 1, count, pFile->pHandle), (int)count);
}


static int L_WriteSequence(RFCPTOOL_FILE_REF *pFile, const RFCPTOOL_UINT32 *sSequence, RFCPTOOL_UINT8 escapeCode)
{
	RFCPTOOL_UINT8 tBuf[3];
	const char *sPrefix;
	size_t count;
	int index;
	
	assert(pFile != NULL);
	assert(sSequence != NULL);
	assert((escapeCode == 0x00) || (escapeCode == 0x10));
	
	if (pFile->sPath == NULL)
	{
		sPrefix = escapeCode ? "(+" : "(";
		index = 0;
		
		while (sSequence[index] <= RFCPTOOL_CODEPOINT_MAX)
		{
			if (!RFCPTOOL_FileWrite(pFile, fprintf(pFile->pHandle, "%s", sPrefix), -1))
			{
				return 0;
			}
			
			if (!RFCPTOOL_FileWriteCodepoint(pFile, sSequence[index]))
			{
				return 0;
			}
			
			sPrefix = " ";
			++index;
		}
		
		return RFCPTOOL_FileWrite(pFile, fprintf(pFile->pHandle, ")"), -1);
	}
	
	index = 0;
	
	while (sSequence[index] <= RFCPTOOL_CODEPOINT_MAX)
	{
		++index;
	}
	
	tBuf[0] = 0xFE;
	tBuf[1] = (RFCPTOOL_UINT8)((escapeCode + 0x20) | (index - 1));
	
	index = 0;
	count = 2;
	
	while (RFCPTOOL_FileWrite(pFile, (int)fwrite(tBuf, 1, count, pFile->pHandle), (int)count))
	{
		count = RFCPTOOL_EncodePCS(sSequence[index], tBuf);
		
		if ((sSequence[index] > RFCPTOOL_CODEPOINT_MAX) || !count)
		{
			return 1;
		}
		
		++index;
	}
	
	return 0;
}


static int L_WriteRef(RFCPTOOL_FILE_REF *pFile, RFCPTOOL_UINT32 tag, RFCPTOOL_UINT8 escapeCode)
{
	RFCPTOOL_UINT8 tBuf[3];
	size_t count;
	int index;
	
	assert(pFile != NULL);
	assert(((tag & RFCPTOOL_TAG_MASK_TYPE) == RFCPTOOL_TAG_TYPE_MULTIBYTE) || ((tag & RFCPTOOL_TAG_MASK_TYPE) == RFCPTOOL_TAG_TYPE_SHIFT_OUT));
	assert((escapeCode == 0x00) || (escapeCode == 0x40));
	
	index = (int)(tag & RFCPTOOL_TAG_MASK_VALUE);
	
	if (pFile->sPath == NULL)
	{
		return RFCPTOOL_FileWrite(pFile, fprintf(pFile->pHandle, "%s :TAB%03X", escapeCode ? "MULTIBYTE" : ">", (int)(tag & RFCPTOOL_TAG_MASK_VALUE)), -1);
	}
	
	tBuf[0] = 0xFE;
	count = 1;
	
	if (index <= 0x3F)
	{
		tBuf[count++] = (RFCPTOOL_UINT8)((escapeCode + 0x40) | index);
	}
	else
	{
		tBuf[count++] = (RFCPTOOL_UINT8)((escapeCode >> 3) + 0x0E);
		tBuf[count++] = (RFCPTOOL_UINT8)index;
	}
	
	return RFCPTOOL_FileWrite(pFile, (int)fwrite(tBuf, 1, count, pFile->pHandle), (int)count);
}


static int L_WritePredefined(RFCPTOOL_FILE_REF *pFile, int *pSpaceCount, const char *sSymbol, RFCPTOOL_UINT8 escapeCode)
{
	RFCPTOOL_UINT8 tBuf[2];
	
	assert(pFile != NULL);
	assert(sSymbol != NULL);
	
	if (pFile->sPath == NULL)
	{
		if (pSpaceCount != NULL)
		{
			*pSpaceCount = 6 - (int)strlen(sSymbol);
		}
		
		return RFCPTOOL_FileWrite(pFile, fprintf(pFile->pHandle, "%s", sSymbol), -1);
	}
	
	tBuf[0] = 0xFE;
	tBuf[1] = escapeCode;
	
	return RFCPTOOL_FileWrite(pFile, (int)fwrite(tBuf, 1, 2, pFile->pHandle), 2);
}


static int L_WriteTab(RFCPTOOL_FILE_REF *pFile, const RFCPTOOL_UINT32 tTag[256], const void *hSequenceTab)
{
	const RFCPTOOL_UINT32 *sSequence;
	int *pSpaceCount;
	int spaceCount;  /* if set to 0, a new line is enforced */
	int codeCount;
	int codeStart;
	int codeEnd;
	int index;
	
	codeCount = 256;
	
	while ((codeCount > 0) && (tTag[codeCount - 1] == (RFCPTOOL_TAG_TYPE_PREDEFINED | RFCPTOOL_TAG_INVALID)))
	{
		--codeCount;
	}
	
	codeStart = 0;
	spaceCount = 0;
	
	while (codeStart < codeCount)
	{
		codeEnd = 0;
		pSpaceCount = &spaceCount;
		
		switch (tTag[codeStart] & RFCPTOOL_TAG_MASK_TYPE)
		{
		case RFCPTOOL_TAG_TYPE_CODEPOINT:
			
			codeEnd = L_IsConsecutive(tTag, codeStart);
			
			if ((tTag[codeStart] == (RFCPTOOL_UINT32)codeStart) && ((codeEnd - codeStart >= 3) || ((codeEnd - codeStart >= 2) && (tTag[codeEnd] >= 0xC0)) || (codeEnd && (tTag[codeStart] >= 0xC0))))
			{
				/* write identity symbol "/" without loss of efficiency: */
				
				if (codeEnd > codeStart + 1)
				{
					if (!L_WritePrepare(pFile, codeStart, codeEnd, spaceCount))
					{
						return -1;
					}
					
					codeStart = codeEnd;
					spaceCount = 0;
					pSpaceCount = NULL;
				}
				else if (!L_WritePrepare(pFile, codeStart, 0, spaceCount))
				{
					return -1;
				}
				
				if (!L_WritePredefined(pFile, pSpaceCount, "/", 0x04))
				{
					return -1;
				}
			}
			else if ((codeEnd - codeStart >= 3) || ((codeEnd - codeStart >= 2) && (tTag[codeEnd] >= 0xC0)))
			{
				/* write range without loss of efficiency (minimum of 3 consecutive values): */
				
				if (!L_WritePrepare(pFile, codeStart, codeEnd, 0))
				{
					return -1;
				}
				
				if (!L_WritePredefined(pFile, NULL, "ITERATE ", 0x18))
				{
					return -1;
				}
				
				if (!L_WriteCodepoint(pFile, tTag[codeStart]))
				{
					return -1;
				}
				
				codeStart = codeEnd;
				spaceCount = 0;
			}
			else
			{
				/* write codepoint: */
				
				codeEnd = L_IsTag(tTag, codeStart, tTag[codeStart]);
				
				assert(codeEnd >= codeStart);
				
				if ((codeEnd > codeStart + 1) || ((codeEnd > codeStart) && (tTag[codeStart] >= 0x2C80)))
				{
					if (!L_WritePrepare(pFile, codeStart, codeEnd, spaceCount))
					{
						return -1;
					}
					
					codeStart = codeEnd;
					spaceCount = 0;
				}
				else
				{
					if (!L_WritePrepare(pFile, codeStart, 0, spaceCount))
					{
						return -1;
					}
					
					spaceCount = 2;
				}
				
				if (!L_WriteCodepoint(pFile, tTag[codeStart]))
				{
					return -1;
				}
				break;
			}
			break;
			
		case RFCPTOOL_TAG_TYPE_SEQUENCE_INV:
			
			codeEnd = 0x10;
			
			/* no break */
			
		case RFCPTOOL_TAG_TYPE_SEQUENCE:
			
			sSequence = RFCPTOOL_LookupSequenceTab(hSequenceTab, tTag[codeStart] & RFCPTOOL_TAG_MASK_VALUE);
			
			assert(sSequence != NULL);
			
			if (spaceCount > 0)
			{
				spaceCount = 2;
			}
			
			if (!L_WritePrepare(pFile, codeStart, 0, spaceCount))
			{
				return -1;
			}
			
			spaceCount = 2;
			
			if (!L_WriteSequence(pFile, sSequence, (RFCPTOOL_UINT8)codeEnd))
			{
				return -1;
			}
			break;
			
		case RFCPTOOL_TAG_TYPE_MULTIBYTE:
			
			codeEnd = 0x40;
			/* no break */
			
		case RFCPTOOL_TAG_TYPE_SHIFT_OUT:
			
			if (!L_WritePrepare(pFile, codeStart, 0, 0))
			{
				return -1;
			}
			
			spaceCount = 0;
			
			if (!L_WriteRef(pFile, tTag[codeStart], (RFCPTOOL_UINT8)codeEnd))
			{
				return -1;
			}
			break;
			
		case RFCPTOOL_TAG_TYPE_PREDEFINED:
			
			index = 0;
			
			switch (tTag[codeStart] & RFCPTOOL_TAG_MASK_VALUE)
			{
			case RFCPTOOL_TAG_IGNORE:
				
				++index;
				/* no break */
				
			case RFCPTOOL_TAG_INVALID:
				
				++index;
				/* no break */
				
			case RFCPTOOL_TAG_SHIFT_IN:
				
				codeEnd = L_IsTag(tTag, codeStart, tTag[codeStart]);
				
				assert(codeEnd >= codeStart);
				
				if (codeEnd > codeStart + 1)
				{
					if (!L_WritePrepare(pFile, codeStart, codeEnd, spaceCount))
					{
						return -1;
					}
					
					codeStart = codeEnd;
					spaceCount = 0;
					pSpaceCount = NULL;
				}
				else if (!L_WritePrepare(pFile, codeStart, 0, spaceCount))
				{
					return -1;
				}
				
				if (!L_WritePredefined(pFile, pSpaceCount, ltsSymbolSimple[index], (RFCPTOOL_UINT8)(((index << 1) + 0x06) & 0x07)))
				{
					return -1;
				}
				break;
				
			case RFCPTOOL_TAG_MULTIBYTE_IDENTITY:
				
				++index;
				/* no break */
				
			case RFCPTOOL_TAG_MULTIBYTE_IGNORE:
				
				++index;
				/* no break */
				
			case RFCPTOOL_TAG_MULTIBYTE_INVALID:
				
				if (!L_WritePrepare(pFile, codeStart, 0, spaceCount))
				{
					return -1;
				}
				
				spaceCount = 0;
				
				if (!L_WritePredefined(pFile, NULL, ltsSymbolMultibyte[index], (RFCPTOOL_UINT8)((index << 1) + 0x10)))
				{
					return -1;
				}
				break;
				
			case RFCPTOOL_TAG_SHIFT_OUT_IDENTITY:
				
				++index;
				/* no break */
				
			case RFCPTOOL_TAG_SHIFT_OUT_IGNORE:
				
				++index;
				/* no break */
				
			case RFCPTOOL_TAG_SHIFT_OUT_INVALID:
				
				if (!L_WritePrepare(pFile, codeStart, 0, spaceCount))
				{
					return -1;
				}
				
				spaceCount = 0;
				
				if (!L_WritePredefined(pFile, NULL, ltsSymbolShiftOut[index], (RFCPTOOL_UINT8)((index << 1) + 0x08)))
				{
					return -1;
				}
			}
		}
		
		++codeStart;
	}
	
	if (pFile->sPath == NULL)
	{
		if (!RFCPTOOL_FileWrite(pFile, fprintf(pFile->pHandle, "\n"), -1))
		{
			return -1;
		}
	}
	
	return codeCount;
}


static void L_SetMinVer(char sTarget[7], char major, char minor)
{
	if (sTarget[3] <= major)
	{
		if (sTarget[3] < major)
		{
			sTarget[3] = major;
			sTarget[5] = minor;
		}
		else if (sTarget[5] < minor)
		{
			sTarget[5] = minor;
		}
	}
}


static void L_DetermineTarget(RFCPTOOL_CP_TAB *tpTab[], int count, char sTarget[7])
{
	const RFCPTOOL_UINT32 *pTag;
	int index;
	int code;
	
	strcpy(sTarget, "CP/1.0");
	
	assert(sTarget[3] == '1');
	assert(sTarget[5] == '0');
	
	for (index = 0; index < count; ++index)
	{
		pTag = tpTab[index]->tTag;
		
		for (code = 0; code < 256; ++code)
		{
			switch (pTag[code] & RFCPTOOL_TAG_MASK_TYPE)
			{
			case RFCPTOOL_TAG_TYPE_SEQUENCE:
				
				L_SetMinVer(sTarget, '4', '0');
				break;
				
			case RFCPTOOL_TAG_TYPE_SEQUENCE_INV:
				
				L_SetMinVer(sTarget, '4', '1');
				break;
				
			case RFCPTOOL_TAG_TYPE_MULTIBYTE:
				
				L_SetMinVer(sTarget, '3', '0');
				break;
				
			case RFCPTOOL_TAG_TYPE_SHIFT_OUT:
				
				L_SetMinVer(sTarget, ((pTag[code] & RFCPTOOL_TAG_MASK_VALUE) <= 1) ? '2' : '3', '0');
				break;
				
			case RFCPTOOL_TAG_TYPE_PREDEFINED:
				
				switch (pTag[code] & RFCPTOOL_TAG_MASK_VALUE)
				{
				case RFCPTOOL_TAG_SHIFT_IN:
					
					L_SetMinVer(sTarget, '2', '0');
					break;
					
				case RFCPTOOL_TAG_SHIFT_OUT_INVALID:
				case RFCPTOOL_TAG_SHIFT_OUT_IGNORE:
				case RFCPTOOL_TAG_SHIFT_OUT_IDENTITY:
				case RFCPTOOL_TAG_MULTIBYTE_INVALID:
				case RFCPTOOL_TAG_MULTIBYTE_IGNORE:
				case RFCPTOOL_TAG_MULTIBYTE_IDENTITY:
					
					L_SetMinVer(sTarget, '3', '0');
				}
			}
		}
	}
}


/*****************************************************************************************
 *
 *  E X T E R N A L   F U N C T I O N S
 *
 *****************************************************************************************/


int RFCPTOOL_Compile(const char *sDestPath, int fOverwrite, RFCPTOOL_CP_TAB *tpTab[], int count, void *hSequenceTab)
{
	char sTarget[7];
	RFCPTOOL_UINT8 tBuf[8];
	RFCPTOOL_FILE_REF *pFile;
	int index;
	int result;
	
	assert(tpTab != NULL);
	assert(count >= 1);
	
	L_DetermineTarget(tpTab, count, sTarget);
	
	if (sDestPath == NULL)
	{
		pFile = RFCPTOOL_FileOpenOutputCPCODE(sDestPath, sTarget, 0);  /* write CPCODE file to stdout */
	}
	else
	{
		pFile = RFCPTOOL_FileOpenOutputCP(sDestPath, fOverwrite);  /* write binary CP file */
	}
	
	if (pFile == NULL)
	{
		return 0;
	}
	
	if (sDestPath != NULL)
	{
		/* write binary file header: */
		
		tBuf[0] = 0x52;
		tBuf[1] = 0x46;
		tBuf[2] = 0x46;
		tBuf[3] = 0x46;
		tBuf[4] = 0x43;
		tBuf[5] = 0x50;
		tBuf[6] = (sTarget[3] - '0') + 0x30;
		tBuf[7] = (sTarget[5] - '0') + 0x30;
		
		if (!RFCPTOOL_FileWrite(pFile, (int)fwrite(tBuf, 1, 8, pFile->pHandle), 8))
		{
			RFCPTOOL_FileDestroy(pFile);
			return 0;
		}
	}
	
	result = L_WriteTab(pFile, tpTab[0]->tTag, hSequenceTab);
	
	if (result < 0)
	{
		RFCPTOOL_FileDestroy(pFile);
		return 0;
	}
	
	tBuf[0] = 0xFF;
	tBuf[1] = 0xFF;
	
	for (index = 1; index < count; ++index)
	{
		if (sDestPath == NULL)
		{
			if (!RFCPTOOL_FileWrite(pFile, fprintf(pFile->pHandle, "\n:TAB%03X\n", index), -1))
			{
				RFCPTOOL_FileDestroy(pFile);
				return 0;
			}
		}
		else if (result < 256)
		{
			if (!RFCPTOOL_FileWrite(pFile, (int)fwrite(tBuf, 1, 2, pFile->pHandle), 2))
			{
				RFCPTOOL_FileDestroy(pFile);
				return 0;
			}
		}
		
		result = L_WriteTab(pFile, tpTab[index]->tTag, hSequenceTab);
		
		if (result < 0)
		{
			RFCPTOOL_FileDestroy(pFile);
			return 0;
		}
	}
	
	if (!RFCPTOOL_FileClose(pFile))
	{
		RFCPTOOL_FileDestroy(pFile);
		return 0;
	}
	
	RFCPTOOL_FileDestroy(pFile);
	
	return 1;
}
