
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
 *  File:          /src/rfcptool_decode.c//
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rfcptool_def.h"
#include "rfcptool_util.h"
#include "rfcptool_file.h"
#include "rfcptool_decode.h"


#define RFCPTOOL_DECODE_BUF_SIZE            4


typedef struct {
	RFCPTOOL_FILE_REF *pSrcFile;
	RFCPTOOL_FILE_REF *pDestFile;
	int tableIndexMax;
} RFCPTOOL_DECODE_CTX;


static RFCPTOOL_FILE_REF* L_OpenSrcFile(const char *sSrcPath, char sTarget[RFCPTOOL_TARGET_LEN_MAX + 1])
{
	RFCPTOOL_UINT8 tBuf[8];
	RFCPTOOL_FILE_REF *pSrcFile;
	
	pSrcFile = RFCPTOOL_FileOpenInputCP(sSrcPath);
	
	if (pSrcFile == NULL)
	{
		return 0;
	}
	
	if (!RFCPTOOL_FileReadBin(pSrcFile, tBuf, 8, 0))
	{
		RFCPTOOL_FileDestroy(pSrcFile);
		return 0;
	}
	
	if ((tBuf[0] != 0x52) || (tBuf[1] != 0x46) || (tBuf[2] != 0x46) || (tBuf[3] != 0x46) || (tBuf[4] != 0x43) || (tBuf[5] != 0x50))
	{
		RFCPTOOL_Error("Invalid file format header", " of input file ", sSrcPath, NULL, 0);
		RFCPTOOL_FileDestroy(pSrcFile);
		return 0;
	}
	
	if (tBuf[6] > 0x34)
	{
		RFCPTOOL_Error("Format version", " of input file ", sSrcPath, " not supported", 0);
		RFCPTOOL_FileDestroy(pSrcFile);
		return 0;
	}
	
	sTarget[0] = 'C';
	sTarget[1] = 'P';
	sTarget[2] = '/';
	sTarget[3] = (char)(tBuf[6] - 0x30) + '0';
	sTarget[4] = '.';
	sTarget[5] = (char)(tBuf[7] - 0x30) + '0';
	sTarget[6] = '\0';
	
	return pSrcFile;
}


static RFCPTOOL_UINT32 L_ReadPCS(RFCPTOOL_DECODE_CTX *pCtx, int fStartOfLine, int fAllowEscape)
{
	RFCPTOOL_UINT8 tBuf[3];
	RFCPTOOL_UINT32 c;
	
	switch (RFCPTOOL_FileReadBin(pCtx->pSrcFile, tBuf, 1, fStartOfLine))
	{
	case 0:
		
		return RFCPTOOL_CODEPOINT_ERROR;
		
	case -1:
		
		return RFCPTOOL_CODEPOINT_EOF;
	}
	
	if (tBuf[0] < 0xC0)
	{
		return (RFCPTOOL_UINT32)tBuf[0];
	}
	
	if (tBuf[0] >= 0xFE)
	{
		if ((tBuf[0] == 0xFE) && fAllowEscape)
		{
			return RFCPTOOL_TAG_TYPE_UNSPECIFIED;  /* 0xFE escape prefix */
		}
		
		if ((tBuf[0] == 0xFF) && fStartOfLine)
		{
			return RFCPTOOL_TAG_TYPE_INVALID;  /* 0xFF range prefix */
		}
		
		RFCPTOOL_Error("Character expected", " in input file ", pCtx->pSrcFile->sPath, NULL, 0);
		
		return RFCPTOOL_CODEPOINT_ERROR;
	}
	
	if (!RFCPTOOL_FileReadBin(pCtx->pSrcFile, &tBuf[1], 1, 0))
	{
		return RFCPTOOL_CODEPOINT_ERROR;
	}
	
	c = ((RFCPTOOL_UINT32)tBuf[0] << 8) | (RFCPTOOL_UINT32)tBuf[1];
	
	if (c < 0xEBC0)
	{
		return c - 0xBF40;
	}
	
	if (!RFCPTOOL_FileReadBin(pCtx->pSrcFile, &tBuf[2], 1, 0))
	{
		return RFCPTOOL_CODEPOINT_ERROR;
	}
	
	c = (c << 8) | (RFCPTOOL_UINT32)tBuf[2];
	
	if (c < 0xEC8E50)
	{
		if (c < 0xEC7080)
		{
			c -= 0xEB9380;
		}
		else
		{
			c -= 0xEB9080;
		}
	}
	else
	{
		if (c < 0xFC903E)
		{
			c -= 0xEB9060;
			c += (c / 0x00FFFE) << 1;
		}
		else
		{
			c -= 0xEB903E;
		}
	}
	
	return c;
}


int L_DoDecode(RFCPTOOL_DECODE_CTX *pCtx)
{
	RFCPTOOL_UINT8 tBuf[RFCPTOOL_DECODE_BUF_SIZE];
	RFCPTOOL_UINT32 codepoint;
	const char *sStr;
	int spaceCount;
	int tableCount;
	int tableIndex;
	int code;
	int range;
	int fNewLine;
	
	spaceCount = 0;
	tableCount = 0;
	code = 0;
	range = 0;
	fNewLine = 1;
	
	codepoint = L_ReadPCS(pCtx, 1, 1);
	
	while (codepoint != RFCPTOOL_CODEPOINT_EOF)
	{
		if (codepoint == RFCPTOOL_CODEPOINT_ERROR)
		{
			return 0;
		}
		
		code += range;
		
		if (code >= 0x100)
		{
			if (++tableCount >= 0x140)
			{
				RFCPTOOL_Error("Too many tables", " in input file ", pCtx->pSrcFile->sPath, NULL, 0);
				return 0;
			}
			
			if (!RFCPTOOL_FileWrite(pCtx->pDestFile, fprintf(pCtx->pDestFile->pHandle, "\n\n:TAB%03X\n", tableCount), -1))
			{
				return 0;
			}
			
			code = 0;
			fNewLine = 1;
		}
		
		range = 1;
		
		if (codepoint == RFCPTOOL_TAG_TYPE_INVALID)
		{
			/* 0xFF range prefix: */
			
			if (!RFCPTOOL_FileReadBin(pCtx->pSrcFile, tBuf, 1, 0))
			{
				return 0;
			}
			
			if (tBuf[0] != 0xFF)
			{
				range = tBuf[0] + 2;
				
				if (range > 0x100 - code)
				{
					RFCPTOOL_Error("Range exceeds table", " in input file ", pCtx->pSrcFile->sPath, NULL, 0);
					return 0;
				}
				
				codepoint = L_ReadPCS(pCtx, 0, 1);  /* codepoint will be != RFCPTOOL_TAG_TYPE_INVALID */
				
				if (codepoint == RFCPTOOL_CODEPOINT_ERROR)
				{
					RFCPTOOL_Error("Unexpected range specifier", " in input file ", pCtx->pSrcFile->sPath, NULL, 0);
					return 0;
				}
				
				fNewLine = 1;
			}
		}
		
		if (codepoint == RFCPTOOL_TAG_TYPE_INVALID)
		{
			range = 0x100 - code;  /* new table in next iteration */
		}
		else
		{
			if (codepoint != RFCPTOOL_TAG_TYPE_UNSPECIFIED)
			{
				tBuf[0] = 0x00;
			}
			else if (!RFCPTOOL_FileReadBin(pCtx->pSrcFile, tBuf, 1, 0))
			{
				return 0;
			}
			
			if (fNewLine || ((tBuf[0] >= 0x08) && (tBuf[0] <= 0x1F)) || (tBuf[0] >= 0x40) || !(code & 0x0F))
			{
				if (!RFCPTOOL_FileWrite(pCtx->pDestFile, fprintf(pCtx->pDestFile->pHandle, "\n%02X", code), -1))
				{
					return 0;
				}
				
				spaceCount = 6;
				
				if (range > 1)
				{
					if (!RFCPTOOL_FileWrite(pCtx->pDestFile, fprintf(pCtx->pDestFile->pHandle, "..%02X", code + range - 1), -1))
					{
						return 0;
					}
					
					spaceCount -= 4;
				}
				
				if (range == 1)
				{
					fNewLine = 0;
				}
			}
			else  if ((tBuf[0] >= 0x20) && (tBuf[0] <= 0x3F))
			{
				spaceCount = 2;
			}
			
			if (!RFCPTOOL_FileWriteSpace(pCtx->pDestFile, spaceCount))
			{
				return 0;
			}
			
			if (codepoint != RFCPTOOL_TAG_TYPE_UNSPECIFIED)
			{
				spaceCount = 2;
				
				if (!RFCPTOOL_FileWriteCodepoint(pCtx->pDestFile, codepoint))
				{
					return 0;
				}
			}
			else if ((tBuf[0] <= 0x17) || (tBuf[0] >= 0x40))
			{
				if (tBuf[0] >= 0xC0)
				{
					RFCPTOOL_Error("Invalid escape sequence", " in input file ", pCtx->pSrcFile->sPath, NULL, 0);
					return 0;
				}
				
				if (tBuf[0] >= 0x08)
				{
					sStr = (((tBuf[0] & 0xF0) == 0x10) || (tBuf[0] >= 0x80)) ? "MULTIBYTE " : "> ";
					fNewLine = 1;
				}
				else
				{
					sStr = "";
					spaceCount = 5;
				}
				
				if ((tBuf[0] & 0xC7) <= 0x01)
				{
					if (!RFCPTOOL_FileWrite(pCtx->pDestFile, fprintf(pCtx->pDestFile->pHandle, "%s-", sStr), -1))
					{
						return 0;
					}
				}
				else if ((tBuf[0] & 0xC7) <= 0x03)
				{
					if (!RFCPTOOL_FileWrite(pCtx->pDestFile, fprintf(pCtx->pDestFile->pHandle, "%s.", sStr), -1))
					{
						return 0;
					}
				}
				else if ((tBuf[0] & 0xC7) <= 0x05)
				{
					if (!RFCPTOOL_FileWrite(pCtx->pDestFile, fprintf(pCtx->pDestFile->pHandle, "%s/", sStr), -1))
					{
						return 0;
					}
				}
				else if (tBuf[0] <= 0x07)
				{
					if (!RFCPTOOL_FileWrite(pCtx->pDestFile, fprintf(pCtx->pDestFile->pHandle, "<<"), -1))
					{
						return 0;
					}
					
					--spaceCount;
				}
				else
				{
					if (tBuf[0] >= 0x40)
					{
						tableIndex = tBuf[0] & 0x3F;
					}
					else if (RFCPTOOL_FileReadBin(pCtx->pSrcFile, tBuf, 1, 0))
					{
						tableIndex = tBuf[0] + 0x40;
					}
					else
					{
						return 0;
					}
					
					if (pCtx->tableIndexMax < tableIndex)
					{
						pCtx->tableIndexMax = tableIndex;
					}
					
					if (!RFCPTOOL_FileWrite(pCtx->pDestFile, fprintf(pCtx->pDestFile->pHandle, "%s:TAB%03X", sStr, tableIndex), -1))
					{
						return 0;
					}
				}
			}
			else if (tBuf[0] <= 0x1F)
			{
				fNewLine = 1;
				
				codepoint = L_ReadPCS(pCtx, 0, 0);
				
				if (codepoint == RFCPTOOL_CODEPOINT_ERROR)
				{
					return 0;
				}
				
				if (tBuf[0] <= 0x1B)
				{
					sStr = (tBuf[0] <= 0x19) ? "ITERATE" : "ITERATE-LE";
				}
				else
				{
					sStr = (tBuf[0] <= 0x1D) ? "ITERATE-LE-32" : "ITERATE-LE-16";
				}
				
				if (!RFCPTOOL_FileWrite(pCtx->pDestFile, fprintf(pCtx->pDestFile->pHandle, "%s ", sStr), -1))
				{
					return 0;
				}
				
				if (!RFCPTOOL_FileWriteCodepoint(pCtx->pDestFile, codepoint))
				{
					return 0;
				}
			}
			else
			{
				sStr = (tBuf[0] & 0x10) ? "(+" : "(";
				
				spaceCount = 2;
				tBuf[0] &= 0x0F;
				
				do
				{
					if (!RFCPTOOL_FileWrite(pCtx->pDestFile, fprintf(pCtx->pDestFile->pHandle, sStr), -1))
					{
						return 0;
					}
					
					sStr = " ";
					
					codepoint = L_ReadPCS(pCtx, 0, 0);
					
					if (codepoint == RFCPTOOL_CODEPOINT_ERROR)
					{
						return 0;
					}
					
					if (!RFCPTOOL_FileWriteCodepoint(pCtx->pDestFile, codepoint))
					{
						return 0;
					}
				}
				while (tBuf[0]--);
				
				if (!RFCPTOOL_FileWrite(pCtx->pDestFile, fprintf(pCtx->pDestFile->pHandle, ")"), -1))
				{
					return 0;
				}
			}
		}
		
		codepoint = L_ReadPCS(pCtx, 1, 1);
	}
	
	while (tableCount < pCtx->tableIndexMax)
	{
		if (!RFCPTOOL_FileWrite(pCtx->pDestFile, fprintf(pCtx->pDestFile->pHandle, "\n\n:TAB%03X\n", ++tableCount), -1))
		{
			return 0;
		}
	}
	
	return RFCPTOOL_FileWrite(pCtx->pDestFile, fprintf(pCtx->pDestFile->pHandle, "\n"), -1);
}


int RFCPTOOL_Decode(const char *sSrcPath, const char *sDestPath, int fOverwrite)
{
	char sTarget[RFCPTOOL_TARGET_LEN_MAX + 1];
	RFCPTOOL_DECODE_CTX ctx;
	
	ctx.pSrcFile = L_OpenSrcFile(sSrcPath, sTarget);
	
	if (ctx.pSrcFile == NULL)
	{
		return 0;
	}
	
	ctx.pDestFile = RFCPTOOL_FileOpenOutputCPCODE(sDestPath, sTarget, fOverwrite);
	
	if (ctx.pDestFile == NULL)
	{
		return 0;
	}
	
	ctx.tableIndexMax = 0;
	
	if (!L_DoDecode(&ctx))
	{
		RFCPTOOL_FileDestroy(ctx.pDestFile);
		RFCPTOOL_FileDestroy(ctx.pSrcFile);
		return 0;
	}
	
	RFCPTOOL_FileDestroy(ctx.pSrcFile);
	
	if (!RFCPTOOL_FileClose(ctx.pDestFile))
	{
		RFCPTOOL_FileDestroy(ctx.pDestFile);
		return 0;
	}
	
	RFCPTOOL_FileDestroy(ctx.pDestFile);
	
	return 1;
}
