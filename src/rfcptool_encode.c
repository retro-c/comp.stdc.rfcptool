
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
 *  Repository:    <http://source.retro-c.net/util.stdc.rfcptool>
 *  File:          /src/rfcptool_encode.c//
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
#include "rfcptool_encode.h"


#define RFCPTOOL_ENCODE_BUF_SIZE          (2 + 16 * 3)  /* must be sufficient to hold 0xFE 0x10 and 16 packed characters of 3 bytes maximum each */

#define RFCPTOOL_ENCODE_STR_TAB_COUNT_MAX 0x0140
#define RFCPTOOL_ENCODE_TOKEN_LEN_MAX     31

#define RFCPTOOL_ENCODE_TOKEN_VALUE_MAX   0x00FFFFFF
#define RFCPTOOL_ENCODE_TOKEN_LINE_BREAK  0xFFFFFFEE
#define RFCPTOOL_ENCODE_TOKEN_RANGE       0xFFFFFFEF  /* ".." */
#define RFCPTOOL_ENCODE_TOKEN_COLON       0xFFFFFFF0  /* ":" */
#define RFCPTOOL_ENCODE_TOKEN_L_PAREN     0xFFFFFFF1  /* "(" */
#define RFCPTOOL_ENCODE_TOKEN_R_PAREN     0xFFFFFFF2  /* ")" */
#define RFCPTOOL_ENCODE_TOKEN_PLUS_SIGN   0xFFFFFFF3  /* "+" */
#define RFCPTOOL_ENCODE_TOKEN_SOLIDUS     0xFFFFFFF4  /* "/" */
#define RFCPTOOL_ENCODE_TOKEN_HYPHEN      0xFFFFFFF5  /* "-" */
#define RFCPTOOL_ENCODE_TOKEN_FULL_STOP   0xFFFFFFF6  /* "." */
#define RFCPTOOL_ENCODE_TOKEN_SHIFT_IN    0xFFFFFFF7  /* "<<" */
#define RFCPTOOL_ENCODE_TOKEN_SHIFT_OUT   0xFFFFFFF8  /* ">" */
#define RFCPTOOL_ENCODE_TOKEN_MULTIBYTE   0xFFFFFFF9  /* "MULTIBYTE" */
#define RFCPTOOL_ENCODE_TOKEN_BE          0xFFFFFFFA  /* "ITERATE" */
#define RFCPTOOL_ENCODE_TOKEN_LE          0xFFFFFFFB  /* "ITERATE-LE" */
#define RFCPTOOL_ENCODE_TOKEN_LE32        0xFFFFFFFC  /* "ITERATE-LE-32" */
#define RFCPTOOL_ENCODE_TOKEN_LE16        0xFFFFFFFD  /* "ITERATE-LE-16" */
#define RFCPTOOL_ENCODE_TOKEN_IDENTIFIER  0xFFFFFFFE
#define RFCPTOOL_ENCODE_TOKEN_EOF         0xFFFFFFFF


typedef struct {
	RFCPTOOL_FILE_TEXT *pSrcFile;
	RFCPTOOL_FILE_REF *pDestFile;
	void *hStrTab;
	RFCPTOOL_UINT32 token;
	char sIdentifier[RFCPTOOL_ENCODE_TOKEN_LEN_MAX + 1];  /* valid if token == RFCPTOOL_ENCODE_TOKEN_IDENTIFIER */
	int offset;
	int line;
	char tC[2];
	RFCPTOOL_UINT8 tTargetVersion[2];
} RFCPTOOL_ENCODE_CTX;


static RFCPTOOL_FILE_TEXT* L_OpenSrcFile(const char *sSrcPath, RFCPTOOL_UINT8 tTargetVersion[2])
{
	RFCPTOOL_FILE_TEXT *pSrcFile;
	char sTarget[RFCPTOOL_TARGET_LEN_MAX + 1];
	
	tTargetVersion[0] = 0;
	
	pSrcFile = RFCPTOOL_FileOpenInputCPCODE(sSrcPath, sTarget);
	
	if (pSrcFile == NULL)
	{
		return NULL;
	}
	
	if (sTarget[0] != '\0')
	{
		if ((sTarget[0] != 'C') || (sTarget[1] != 'P') || (sTarget[2] != '/') || (sTarget[3] < '1') || (sTarget[3] > '9') || (sTarget[4] != '.') || (sTarget[5] < '0') || (sTarget[5] > '9') || (sTarget[6] != '\0'))
		{
			RFCPTOOL_Error("Invalid target header", " in input file ", sSrcPath, NULL, 0);
			RFCPTOOL_FileDestroy(&pSrcFile->ref);
			return NULL;
		}
		
		tTargetVersion[0] = (RFCPTOOL_UINT8)(sTarget[3] - '0') + 0x30;
		tTargetVersion[1] = (RFCPTOOL_UINT8)(sTarget[5] - '0') + 0x30;
	}
	
	return pSrcFile;
}


static int L_ValidateTargetVersion(RFCPTOOL_ENCODE_CTX *pCtx, RFCPTOOL_UINT8 tTargetVersion[2])
{
	char sVersion[4];
	
	if (tTargetVersion[0])
	{
		sVersion[0] = (char)(pCtx->tTargetVersion[0] - 0x30) + '0';
		sVersion[1] = '.';
		sVersion[2] = (char)(pCtx->tTargetVersion[1] - 0x30) + '0';
		sVersion[3] = '\0';
		
		if ((pCtx->tTargetVersion[0] > tTargetVersion[0]) || ((pCtx->tTargetVersion[0] == tTargetVersion[0]) && (pCtx->tTargetVersion[1] > tTargetVersion[1])))
		{
			RFCPTOOL_Error("Target version lacks required features", " (required version is ", sVersion, ")", 0);
			return 0;
		}
		
		if ((pCtx->tTargetVersion[0] < tTargetVersion[0]) || ((pCtx->tTargetVersion[0] == tTargetVersion[0]) && (pCtx->tTargetVersion[1] < tTargetVersion[1])))
		{
			RFCPTOOL_Warning("Target version higher as required", " (required version is ", sVersion, ")", 0);
		}
	}
	
	return 1;
}


static int L_VersionRequired(RFCPTOOL_ENCODE_CTX *pCtx, RFCPTOOL_UINT8 major, RFCPTOOL_UINT8 minor)
{
	if ((pCtx->tTargetVersion[0] < major) || ((pCtx->tTargetVersion[0] == major) && (pCtx->tTargetVersion[1] < minor)))
	{
		if (pCtx->pDestFile != NULL)
		{
			RFCPTOOL_Error("Version mismatch on 2nd pass", NULL, NULL, NULL, 0);
			return 0;
		}
		
		pCtx->tTargetVersion[0] = major;
		pCtx->tTargetVersion[1] = minor;
	}
	
	return 1;
}


static size_t L_EncodePCS(RFCPTOOL_ENCODE_CTX *pCtx, RFCPTOOL_UINT8 tBuf[])
{
	size_t count;
	
	if (pCtx->token > RFCPTOOL_ENCODE_TOKEN_VALUE_MAX)
	{
		RFCPTOOL_Error("Codepoint expected", " in input file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
		return 0;
	}
	
	count = RFCPTOOL_EncodePCS(pCtx->token, tBuf);
	
	if (!count)
	{
		RFCPTOOL_Error("Invalid codepoint", " in input file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
		return 0;
	}
	
	return count;
}


static int L_WriteBuf(RFCPTOOL_ENCODE_CTX *pCtx, RFCPTOOL_UINT8 tBuf[], size_t count)
{
	if (pCtx->pDestFile != NULL)
	{
		if (!RFCPTOOL_FileWrite(pCtx->pDestFile, (int)fwrite(tBuf, 1, count, pCtx->pDestFile->pHandle), (int)count))
		{
			return 0;
		}
	}
	
	return 1;
}


static int L_ReadChar(RFCPTOOL_ENCODE_CTX *pCtx)
{
	int fComment;
	
	pCtx->tC[0] = pCtx->tC[1];
	
	if (pCtx->tC[0] == '\n')
	{
		pCtx->line++;
	}
	
	fComment = 0;
	
	do
	{
		if (pCtx->offset >= RFCPTOOL_FILE_BUF_SIZE)
		{
			if (!RFCPTOOL_FileReadText(pCtx->pSrcFile, pCtx->offset))
			{
				return 0;
			}
			
			pCtx->offset = 0;
		}
		
		pCtx->tC[1] = RFCPTOOL_ToUpper(RFCPTOOL_FILE_PEEK_CHAR(pCtx->pSrcFile, pCtx->offset));
		
		if (pCtx->tC[1] == '\0')
		{
			return 1;
		}
		
		pCtx->offset++;
		
		if (pCtx->tC[1] == '\n')
		{
			return 1;
		}
		
		if (pCtx->tC[1] == ';')
		{
			fComment = 1;
		}
	}
	while (fComment);
	
	return 1;
}


static int L_StrCmp(RFCPTOOL_ENCODE_CTX *pCtx, const char *sStr)
{
	while (pCtx->tC[0] == *sStr)
	{
		if (!L_ReadChar(pCtx))
		{
			return 0;
		}
		
		if (*++sStr == '\0')
		{
			return RFCPTOOL_IsAlNum(pCtx->tC[0]) ? -1 : 1;
		}
	}
	
	return -1;
}


static int L_ReadValue(RFCPTOOL_ENCODE_CTX *pCtx)
{
	int len;
	
	pCtx->token = 0;
	len = 0;
	
	do
	{
		if (++len > RFCPTOOL_ENCODE_TOKEN_LEN_MAX)
		{
			return -1;
		}
		
		if ((pCtx->tC[0] >= '0') && (pCtx->tC[0] <= '9'))
		{
			pCtx->token = (pCtx->token << 4) | (pCtx->tC[0] - '0');
		}
		else if ((pCtx->tC[0] >= 'A') && (pCtx->tC[0] <= 'F'))
		{
			pCtx->token = (pCtx->token << 4) | (pCtx->tC[0] - 'A' + 10);
		}
		else
		{
			return -1;
		}
		
		if (pCtx->token > RFCPTOOL_ENCODE_TOKEN_VALUE_MAX)
		{
			RFCPTOOL_Error("Value overflow", " in input file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
			return 0;
		}
		
		if (!L_ReadChar(pCtx))
		{
			return 0;
		}
	}
	while (RFCPTOOL_IsAlNum(pCtx->tC[0]));
	
	return 1;
}


static int L_ReadIdentifier(RFCPTOOL_ENCODE_CTX *pCtx)
{
	int len;
	
	if (!RFCPTOOL_IsAlNum(pCtx->tC[0]) && (pCtx->tC[0] != '-'))
	{
		return -1;
	}
	
	pCtx->token = RFCPTOOL_ENCODE_TOKEN_IDENTIFIER;
	len = 0;
	
	do
	{
		if (len >= RFCPTOOL_ENCODE_TOKEN_LEN_MAX)
		{
			return -1;
		}
		
		pCtx->sIdentifier[len++] = pCtx->tC[0];
		
		if (!L_ReadChar(pCtx))
		{
			return 0;
		}
	}
	while (RFCPTOOL_IsAlNum(pCtx->tC[0]) || (pCtx->tC[0] == '-'));
	
	pCtx->sIdentifier[len] = '\0';
	
	return 1;
}


static int L_ReadToken(RFCPTOOL_ENCODE_CTX *pCtx, int fIdentifier)
{
	while ((pCtx->tC[0] == ' ') || (pCtx->tC[0] == '\t'))
	{
		if (!L_ReadChar(pCtx))
		{
			return 0;
		}
	}
	
	switch (pCtx->tC[0])
	{
	case '\0':
		
		pCtx->token = RFCPTOOL_ENCODE_TOKEN_EOF;
		return L_ReadChar(pCtx);
		
	case '\n':
		
		pCtx->token = RFCPTOOL_ENCODE_TOKEN_LINE_BREAK;
		return L_ReadChar(pCtx);
		
	case '/':
		
		pCtx->token = RFCPTOOL_ENCODE_TOKEN_SOLIDUS;
		return L_ReadChar(pCtx);
		
	case '-':
		
		pCtx->token = RFCPTOOL_ENCODE_TOKEN_HYPHEN;
		return L_ReadChar(pCtx);
		
	case '<':
		
		if (!L_ReadChar(pCtx))
		{
			return 0;
		}
		
		if (pCtx->tC[0] == '<')
		{
			pCtx->token = RFCPTOOL_ENCODE_TOKEN_SHIFT_IN;
			return L_ReadChar(pCtx);
		}
		break;
		
	case '>':
		
		pCtx->token = RFCPTOOL_ENCODE_TOKEN_SHIFT_OUT;
		return L_ReadChar(pCtx);
		
	case '(':
		
		pCtx->token = RFCPTOOL_ENCODE_TOKEN_L_PAREN;
		return L_ReadChar(pCtx);
		
	case ')':
		
		pCtx->token = RFCPTOOL_ENCODE_TOKEN_R_PAREN;
		return L_ReadChar(pCtx);
		
	case '+':
		
		pCtx->token = RFCPTOOL_ENCODE_TOKEN_PLUS_SIGN;
		return L_ReadChar(pCtx);
		
	case ':':
		
		pCtx->token = RFCPTOOL_ENCODE_TOKEN_COLON;
		return L_ReadChar(pCtx);
		
	case '.':
		
		if (pCtx->tC[1] != '.')
		{
			pCtx->token = RFCPTOOL_ENCODE_TOKEN_FULL_STOP;
		}
		else if (L_ReadChar(pCtx))
		{
			pCtx->token = RFCPTOOL_ENCODE_TOKEN_RANGE;
		}
		else
		{
			return 0;
		}
		
		return L_ReadChar(pCtx);
		
	default:
		
		if (fIdentifier)
		{
			switch (L_ReadIdentifier(pCtx))
			{
			case 0:
				
				return 0;
				
			case 1:
				
				return 1;
			}
		}
		else if (pCtx->tC[0] == 'M')
		{
			switch (L_StrCmp(pCtx, "MULTIBYTE"))
			{
			case 0:
				
				return 0;
				
			case 1:
				
				pCtx->token = RFCPTOOL_ENCODE_TOKEN_MULTIBYTE;
				return 1;
			}
		}
		else if (pCtx->tC[0] == 'I')
		{
			switch (L_StrCmp(pCtx, "ITERATE"))
			{
			case 0:
				
				return 0;
				
			case 1:
				
				if ((pCtx->tC[0] != '-') || (pCtx->tC[1] != 'L'))
				{
					pCtx->token = RFCPTOOL_ENCODE_TOKEN_BE;
					return 1;
				}
				
				switch (L_StrCmp(pCtx, "-LE"))
				{
				case 0:
					
					return 0;
					
				case 1:
					
					if ((pCtx->tC[0] == '-') && (pCtx->tC[1] == '3'))
					{
						switch (L_StrCmp(pCtx, "-32"))
						{
						case 0:
							
							return 0;
							
						case 1:
							
							pCtx->token = RFCPTOOL_ENCODE_TOKEN_LE32;
							return 1;
						}
					}
					else if ((pCtx->tC[0] == '-') && (pCtx->tC[1] == '1'))
					{
						switch (L_StrCmp(pCtx, "-16"))
						{
						case 0:
							
							return 0;
							
						case 1:
							
							pCtx->token = RFCPTOOL_ENCODE_TOKEN_LE16;
							return 1;
						}
					}
					else
					{
						pCtx->token = RFCPTOOL_ENCODE_TOKEN_LE;
						return 1;
					}
				}
			}
		}
		else switch (L_ReadValue(pCtx))
		{
		case 0:
			
			return 0;
			
		case 1:
			
			return 1;
		}
	}
	
	RFCPTOOL_Error("Invalid token", " in input file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
	
	return 0;
}


static int L_DoEncode(RFCPTOOL_ENCODE_CTX *pCtx)
{
	RFCPTOOL_UINT32 tRange[2];
	RFCPTOOL_UINT8 tBuf[RFCPTOOL_ENCODE_BUF_SIZE];
	char sMsg[32];
	size_t index;
	size_t count;
	int ref;
	RFCPTOOL_UINT8 code;
	
	if (!L_ReadToken(pCtx, 0))
	{
		return 0;
	}
	
	tRange[0] = 0;
	tRange[1] = 0;
	
	tBuf[0] = 0x52;
	tBuf[1] = 0x46;
	tBuf[2] = 0x46;
	tBuf[3] = 0x46;
	tBuf[4] = 0x43;
	tBuf[5] = 0x50;
	tBuf[6] = pCtx->tTargetVersion[0];  /* will always set 0x31 on 1st pass */
	tBuf[7] = pCtx->tTargetVersion[1];  /* will always set 0x30 on 1st pass */
	
	if (!L_WriteBuf(pCtx, tBuf, 8))
	{
		return 0;
	}
	
	while (pCtx->token != RFCPTOOL_ENCODE_TOKEN_EOF)
	{
		if (pCtx->token != RFCPTOOL_ENCODE_TOKEN_LINE_BREAK)
		{
			if (pCtx->token == RFCPTOOL_ENCODE_TOKEN_COLON)
			{
				if (!L_ReadToken(pCtx, 1))
				{
					return 0;
				}
				
				if (pCtx->token != RFCPTOOL_ENCODE_TOKEN_IDENTIFIER)
				{
					RFCPTOOL_Error("Identifier expected", " in input file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
					return 0;
				}
				
				ref = RFCPTOOL_GetStrTabCount(pCtx->hStrTab);
				
				if (ref >= RFCPTOOL_ENCODE_STR_TAB_COUNT_MAX)
				{
					RFCPTOOL_Error("Too many identifiers defined", " in input file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
					return 0;
				}
				
				if (!L_VersionRequired(pCtx, (ref == 0) ? 0x32 : 0x33, 0x30))
				{
					return 0;
				}
				
				if (pCtx->pDestFile == NULL)
				{
					/* insert identifier on first pass only: */
					
					switch (RFCPTOOL_InsertStrTab(pCtx->hStrTab, pCtx->sIdentifier))
					{
					case 1:
						
						break;
						
					case 0:
						
						return 0;
						
					default:
						
						RFCPTOOL_Error("Duplicate identifier defined", " in input file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
						return 0;
					}
				}
				
				if (tRange[0] <= 0xFF)
				{
					/* close current block: */
					
					tBuf[0] = 0xFF;
					tBuf[1] = 0xFF;
					
					if (!L_WriteBuf(pCtx, tBuf, 2))
					{
						return 0;
					}
				}
				
				tRange[0] = 0;
				tRange[1] = 0;
			}
			else if (tRange[0] > 0xFF)
			{
				RFCPTOOL_Error("\'[\' or end of file expected", " in input file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
				return 0;
			}
			else if (pCtx->token != tRange[0])
			{
				sprintf(sMsg, "Value %02X expected", (int)tRange[0]);
				RFCPTOOL_Error(sMsg, " in input file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
				return 0;
			}
			else
			{
				if (!L_ReadToken(pCtx, 0))
				{
					return 0;
				}
				
				if (pCtx->token == RFCPTOOL_ENCODE_TOKEN_RANGE)
				{
					if (!L_ReadToken(pCtx, 0))
					{
						return 0;
					}
					
					if ((pCtx->token <= tRange[0]) || (pCtx->token > 0xFF))
					{
						sprintf(sMsg, "Value %02X..FF expected", (int)(tRange[0] + 1));
						RFCPTOOL_Error(sMsg, " in input file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
						return 0;
					}
					
					tRange[1] = pCtx->token;
					
					tBuf[0] = 0xFF;
					tBuf[1] = (RFCPTOOL_UINT8)(tRange[1] - tRange[0] - 1);
					
					if (!L_WriteBuf(pCtx, tBuf, 2))
					{
						return 0;
					}
					
					if (!L_ReadToken(pCtx, 0))
					{
						return 0;
					}
				}
				
				tBuf[0] = 0xFE;
				tBuf[1] = 0x00;
				
				switch (pCtx->token)
				{
				case RFCPTOOL_ENCODE_TOKEN_MULTIBYTE:
					
					if (!L_VersionRequired(pCtx, 0x33, 0x30))
					{
						return 0;
					}
					
					tBuf[1] = 0x08;
					/* no break */
					
				case RFCPTOOL_ENCODE_TOKEN_SHIFT_OUT:
					
					if (!L_VersionRequired(pCtx, 0x32, 0x30))  /* limitation for version 32:30 regarding maximum table index of 1 is checked by ensuring no more than 2 tables (see above) */
					{
						return 0;
					}
					
					/* <cpcode-ref>: */
					
					count = 2;
					
					if (!L_ReadToken(pCtx, 1))
					{
						return 0;
					}
					
					switch (pCtx->token)
					{
					case RFCPTOOL_ENCODE_TOKEN_SOLIDUS:
						
						tBuf[1] += 0x02;
						/* no break */
						
					case RFCPTOOL_ENCODE_TOKEN_FULL_STOP:
						
						tBuf[1] += 0x02;
						/* no break */
						
					case RFCPTOOL_ENCODE_TOKEN_HYPHEN:
						
						tBuf[1] += 0x08;
						break;
						
					case RFCPTOOL_ENCODE_TOKEN_COLON:
						
						if (!L_ReadToken(pCtx, 1))
						{
							return 0;
						}
						
						ref = 0;
						
						if (pCtx->token == RFCPTOOL_ENCODE_TOKEN_IDENTIFIER)
						{
							if (pCtx->pDestFile != NULL)
							{
								/* lookup identifier on second pass only: */
								
								ref = RFCPTOOL_LookupStrTab(pCtx->hStrTab, pCtx->sIdentifier);
								
								if (ref <= 0)
								{
									RFCPTOOL_Error("Undefined identifier", " in input file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
									return 0;
								}
							}
						}
						else if (pCtx->token != RFCPTOOL_ENCODE_TOKEN_LINE_BREAK)
						{
							RFCPTOOL_Error("Unexpected token", " in input file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
							return 0;
						}
						
						if (ref < 0x0040)
						{
							tBuf[1] = (RFCPTOOL_UINT8)(0x40 + (tBuf[1] << 3) + ref);
						}
						else
						{
							tBuf[1] += 0x0E;
							tBuf[2] = (RFCPTOOL_UINT8)(ref - 0x0040);
							count = 3;
						}
						break;
						
					default:
						
						RFCPTOOL_Error("\'%%\', \'/\', \'.\' or \'-\' expected", " in input file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
						return 0;
					}
					
					if (!L_WriteBuf(pCtx, tBuf, count))
					{
						return 0;
					}
					
					if (pCtx->token != RFCPTOOL_ENCODE_TOKEN_LINE_BREAK)
					{
						if (!L_ReadToken(pCtx, 0))
						{
							return 0;
						}
					}
					
					tRange[1]++;
					break;
					
				case RFCPTOOL_ENCODE_TOKEN_LE16:
					
					tBuf[1] += 0x02;
					/* no break */
					
				case RFCPTOOL_ENCODE_TOKEN_LE32:
					
					tBuf[1] += 0x02;
					/* no break */
					
				case RFCPTOOL_ENCODE_TOKEN_LE:
					
					tBuf[1] += 0x02;
					
					if (!L_VersionRequired(pCtx, 0x33, 0x30))
					{
						return 0;
					}
					/* no break */
					
				case RFCPTOOL_ENCODE_TOKEN_BE:
					
					/* <cpcode-range-mapping>: */
					
					tBuf[1] += 0x18;
					
					if (!L_ReadToken(pCtx, 0))
					{
						return 0;
					}
					
					count = L_EncodePCS(pCtx, &tBuf[2]);
					
					if (!count)
					{
						return 0;
					}
					
					if (!L_WriteBuf(pCtx, tBuf, count + 2))
					{
						return 0;
					}
					
					if (!L_ReadToken(pCtx, 0))
					{
						return 0;
					}
					
					tRange[1]++;
					break;
					
				default:
					
					do
					{
						tBuf[0] = 0xFE;
						tBuf[1] = 0x00;
						
						switch (pCtx->token)
						{
						case RFCPTOOL_ENCODE_TOKEN_SHIFT_IN:
							
							if (!L_VersionRequired(pCtx, 0x32, 0x30))
							{
								return 0;
							}
							
							tBuf[1] += 0x02;
							/* no break */
							
						case RFCPTOOL_ENCODE_TOKEN_SOLIDUS:
							
							tBuf[1] += 0x02;
							/* no break */
							
						case RFCPTOOL_ENCODE_TOKEN_FULL_STOP:
							
							tBuf[1] += 0x02;
							/* no break */
							
						case RFCPTOOL_ENCODE_TOKEN_HYPHEN:
							
							count = 2;
							break;
							
						case RFCPTOOL_ENCODE_TOKEN_L_PAREN:
							
							if (!L_VersionRequired(pCtx, 0x34, 0x30))
							{
								return 0;
							}
							
							if (!L_ReadToken(pCtx, 0))
							{
								return 0;
							}
							
							if (pCtx->token == RFCPTOOL_ENCODE_TOKEN_PLUS_SIGN)
							{
								if (!L_ReadToken(pCtx, 0))
								{
									return 0;
								}
								
								if (!L_VersionRequired(pCtx, 0x34, 0x31))
								{
									return 0;
								}
								
								code = 0x2F;  /* results in escape sequence FE 30..3F */
							}
							else
							{
								code = 0x1F;  /* results in escape sequence FE 20..2F */
							}
							
							index = 2;
							
							while (pCtx->token != RFCPTOOL_ENCODE_TOKEN_R_PAREN)
							{
								if (++tBuf[1] > RFCPTOOL_SEQUENCE_LEN_MAX)
								{
									sprintf(sMsg, "More than %d sequence values", (int)RFCPTOOL_SEQUENCE_LEN_MAX);
									RFCPTOOL_Error(sMsg, " in input file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
									return 0;
								}
								
								count = L_EncodePCS(pCtx, &tBuf[index]);
								
								if (!count)
								{
									return 0;
								}
								
								index += count;
								
								if (!L_ReadToken(pCtx, 0))
								{
									return 0;
								}
							}
							
							if (tBuf[1] < RFCPTOOL_SEQUENCE_LEN_MIN)
							{
								sprintf(sMsg, "Less than %d sequence values", (int)RFCPTOOL_SEQUENCE_LEN_MIN);
								RFCPTOOL_Error(sMsg, " in input file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
								return 0;
							}
							
							tBuf[1] += code;
							
							count = index;
							break;
							
						default:
							
							count = L_EncodePCS(pCtx, tBuf);
							
							if (!count)
							{
								return 0;
							}
						}
						
						if (!L_WriteBuf(pCtx, tBuf, count))
						{
							return 0;
						}
						
						if (!L_ReadToken(pCtx, 0))
						{
							return 0;
						}
						
						tRange[0]++;
						tRange[1]++;
					}
					while ((tRange[0] == tRange[1]) && (pCtx->token != RFCPTOOL_ENCODE_TOKEN_LINE_BREAK));
				}
				
				if (pCtx->token != RFCPTOOL_ENCODE_TOKEN_LINE_BREAK)
				{
					RFCPTOOL_Error("End of line expected", " in input file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
					return 0;
				}
				
				tRange[0] = tRange[1];
			}
		}
		
		if (!L_ReadToken(pCtx, 0))
		{
			return 0;
		}
	}
	
	return 1;
}


int RFCPTOOL_Encode(const char *sSrcPath, const char *sDestPath, int fOverwrite)
{
	RFCPTOOL_UINT8 tTargetVersion[2];
	RFCPTOOL_ENCODE_CTX ctx;
	
	ctx.pDestFile = NULL;
	
	ctx.hStrTab = RFCPTOOL_CreateStrTab(RFCPTOOL_ENCODE_STR_TAB_COUNT_MAX);
	
	if (ctx.hStrTab == NULL)
	{
		return 0;
	}
	
	ctx.pSrcFile = L_OpenSrcFile(sSrcPath, tTargetVersion);
	
	if (ctx.pSrcFile == NULL)
	{
		RFCPTOOL_DestroyStrTab(ctx.hStrTab);
		return 0;
	}
	
	ctx.offset = 0;
	ctx.line = ctx.pSrcFile->headerLines + 1;
	ctx.tC[0] = ' ';
	ctx.tC[1] = ' ';
	ctx.tTargetVersion[0] = 0x31;
	ctx.tTargetVersion[1] = 0x30;
	
	if (!L_DoEncode(&ctx))
	{
		RFCPTOOL_FileDestroy(&ctx.pSrcFile->ref);
		RFCPTOOL_DestroyStrTab(ctx.hStrTab);
		return 0;
	}
	
	if (!L_ValidateTargetVersion(&ctx, tTargetVersion))
	{
		RFCPTOOL_FileDestroy(&ctx.pSrcFile->ref);
		RFCPTOOL_DestroyStrTab(ctx.hStrTab);
		return 0;
	}
	
	if (!RFCPTOOL_FileRewindText(ctx.pSrcFile))
	{
		RFCPTOOL_FileDestroy(&ctx.pSrcFile->ref);
		RFCPTOOL_DestroyStrTab(ctx.hStrTab);
		return 0;
	}
	
	ctx.pDestFile = RFCPTOOL_FileOpenOutputCP(sDestPath, fOverwrite);
	
	if (ctx.pDestFile == NULL)
	{
		RFCPTOOL_FileDestroy(&ctx.pSrcFile->ref);
		RFCPTOOL_DestroyStrTab(ctx.hStrTab);
		return 0;
	}
	
	ctx.offset = 0;
	ctx.line = ctx.pSrcFile->headerLines + 1;
	ctx.tC[0] = ' ';
	ctx.tC[1] = ' ';
	
	if (!L_DoEncode(&ctx))
	{
		RFCPTOOL_FileDestroy(ctx.pDestFile);
		RFCPTOOL_FileDestroy(&ctx.pSrcFile->ref);
		RFCPTOOL_DestroyStrTab(ctx.hStrTab);
		return 0;
	}
	
	RFCPTOOL_FileDestroy(&ctx.pSrcFile->ref);
	RFCPTOOL_DestroyStrTab(ctx.hStrTab);
	
	if (!RFCPTOOL_FileClose(ctx.pDestFile))
	{
		RFCPTOOL_FileDestroy(ctx.pDestFile);
		return 0;
	}
	
	RFCPTOOL_FileDestroy(ctx.pDestFile);
	
	return 1;
}
