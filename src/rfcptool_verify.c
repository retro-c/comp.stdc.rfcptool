
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
 *  File:          /src/rfcptool_verify.c//
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


#define _CRT_SECURE_NO_WARNINGS  /* be careful */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "rfcptool_def.h"
#include "rfcptool_util.h"
#include "rfcptool_verify.h"


/*****************************************************************************************
 *
 *  M A C R O   D E F I N I T I O N S
 *
 *****************************************************************************************/


#define FILE_SIZE_MAX         4096

#define MULTIBYTE_DEPTH_MAX   8

#define TAG_INVALID           0xFFFFFFFF
#define TAG_IGNORE            0xFFFFFFFE
#define TAG_SHIFT_IN          0xFFFFFFFD
#define TAG_IDENTITY          0xFFFFFFFC
#define TAG_MULIBYTE          0xFFFFFFFB
#define TAG_ITERATE_MASK      0x0F000000
#define TAG_ITERATE           0x08000000
#define TAG_ITERATE_LE        0x09000000
#define TAG_ITERATE_LE_32     0x0A000000
#define TAG_ITERATE_LE_16     0x0B000000
#define TAG_VALUE_MASK        0x00FFFFFF


/*****************************************************************************************
 *
 *  T Y P E   D E F I N I T I O N S
 *
 *****************************************************************************************/


struct RFCPTOOL_VERIFY_CP_TAB;


typedef struct RFCPTOOL_VERIFY_CP_ENTRY {
	int codeMax;
	RFCPTOOL_UINT32 tag;
	struct RFCPTOOL_VERIFY_CP_TAB *pTab;
} RFCPTOOL_VERIFY_CP_ENTRY;


typedef struct RFCPTOOL_VERIFY_CP_TAB {
	RFCPTOOL_VERIFY_CP_ENTRY tEntry[256];
	int count;
} RFCPTOOL_VERIFY_CP_TAB;


typedef struct RFCPTOOL_VERIFY_CTX {
	RFCPTOOL_VERIFY_CP_TAB *tpTab[0x143];  /* 0x140 is invalid, 0x141 is ignore and 0x102 is identity */
	int count;
} RFCPTOOL_VERIFY_CTX;


typedef int VALIDATE_FUNC(RFCPTOOL_VERIFY_CTX*);


/*****************************************************************************************
 *
 *  I N T E R N A L   S U P P O R T I N G   F U N C T I O N S
 *
 *****************************************************************************************/


static int L_CodeMin(RFCPTOOL_VERIFY_CP_TAB *pTab, int index)
{
	if (index <= 0)
	{
		return 0;
	}
	
	if (index >= pTab->count)
	{
		return 0xFF;
	}
	
	return pTab->tEntry[index - 1].codeMax + 1;
}


static int L_LookupEntry(RFCPTOOL_VERIFY_CP_TAB *pTab, int code)
{
	int l, r, m;
	
	l = 0;
	r = pTab->count - 1;
	
	while (l <= r)
	{
		m = (l + r) / 2;
		
		if (code > pTab->tEntry[m].codeMax)
		{
			l = m + 1;
		}
		else if (code < L_CodeMin(pTab, m))
		{
			r = m - 1;
		}
		else
		{
			return m;
		}
	}
	
	RFCPTOOL_Error("Internal error", NULL, NULL, NULL, 0);  /* should never happen */
	
	return 0;
}


static void L_Swap(RFCPTOOL_UINT32 *tValue, size_t l, size_t r)
{
	RFCPTOOL_UINT32 temp;
	
	temp = tValue[l];
	tValue[l] = tValue[r];
	tValue[r] = temp;
}


static int L_VerifyCodepoint(RFCPTOOL_VERIFY_CTX *pCtx, RFCPTOOL_UINT8 *tBuf, size_t count, RFCPTOOL_UINT32 codepoint)
{
	RFCPTOOL_VERIFY_CP_TAB *tpTab[MULTIBYTE_DEPTH_MAX];
	RFCPTOOL_VERIFY_CP_ENTRY *pEntry;
	int tIndex[MULTIBYTE_DEPTH_MAX];
	RFCPTOOL_UINT32 tCount[MULTIBYTE_DEPTH_MAX];
	RFCPTOOL_UINT32 tValue[MULTIBYTE_DEPTH_MAX];
	RFCPTOOL_UINT32 temp;
	size_t index;
	
	if ((count < 1) || (count > 8))
	{
		return 0;
	}
	
	tpTab[0] = pCtx->tpTab[0];
	
	index = 0;
	
	while (index < count - 1)
	{
		tIndex[index] = L_LookupEntry(tpTab[index], tBuf[index]);
		
		pEntry = &tpTab[index]->tEntry[tIndex[index]];
		
		temp = (RFCPTOOL_UINT32)L_CodeMin(tpTab[index], tIndex[index]);
		
		tCount[index] = (RFCPTOOL_UINT32)pEntry->codeMax - temp + 1;
		tValue[index] = (RFCPTOOL_UINT32)tBuf[index] - temp;
		
		if (pEntry->tag != TAG_MULIBYTE)
		{
			return 0;
		}
		
		tpTab[++index] = pEntry->pTab;
	}
	
	tIndex[index] = L_LookupEntry(tpTab[index], tBuf[index]);
	
	pEntry = &tpTab[index]->tEntry[tIndex[index]];
	
	temp = (RFCPTOOL_UINT32)L_CodeMin(tpTab[index], tIndex[index]);
	
	tCount[index] = (RFCPTOOL_UINT32)pEntry->codeMax - temp + 1;
	tValue[index] = (RFCPTOOL_UINT32)tBuf[index] - temp;
	
	if (pEntry->tag == TAG_MULIBYTE)
	{
		return 0;
	}
	
	if ((pEntry->tag <= TAG_VALUE_MASK) || (pEntry->tag >= TAG_SHIFT_IN))
	{
		if (pEntry->tag == codepoint)
		{
			return 1;
		}
	}
	else if (pEntry->tag == TAG_IDENTITY)
	{
		if ((RFCPTOOL_UINT32)(tBuf[index]) == codepoint)
		{
			return 1;
		}
	}
	else
	{
		switch (pEntry->tag & TAG_ITERATE_MASK)
		{
		case TAG_ITERATE_LE:
			
			for (index = 0; index < count / 2; ++index)
			{
				L_Swap(tCount, index, count - index - 1);
				L_Swap(tValue, index, count - index - 1);
			}
			break;
			
		case TAG_ITERATE_LE_32:
			
			index = 0;
			
			while (index + 3 < count)
			{
				L_Swap(tCount, index, index + 3);
				L_Swap(tCount, index + 1, index + 2);
				L_Swap(tValue, index, index + 3);
				L_Swap(tValue, index + 1, index + 2);
				index += 4;
			}
			
			if (index + 2 < count)
			{
				L_Swap(tCount, index, index + 2);
				L_Swap(tValue, index, index + 2);
			}
			else if (index + 1 < count)
			{
				L_Swap(tCount, index, index + 1);
				L_Swap(tValue, index, index + 1);
			}
			break;
			
		case TAG_ITERATE_LE_16:
			
			index = 0;
			
			while (++index < count)
			{
				L_Swap(tCount, index - 1, index);
				L_Swap(tValue, index - 1, index);
				++index;
			}
			break;
		}
		
		temp = tValue[0];
		
		for (index = 1; index < count; ++index)
		{
			temp *= tCount[index];
			
			if (temp > TAG_VALUE_MASK)
			{
				return 0;
			}
			
			temp += tValue[index];
		}
		
		temp += pEntry->tag & TAG_VALUE_MASK;
		
		if (temp == codepoint)
		{
			return 1;
		}
	}
	
	return 0;
}


static int L_ValidateInvalid(RFCPTOOL_VERIFY_CP_TAB *pTab, RFCPTOOL_UINT8 codeStart, RFCPTOOL_UINT8 codeEnd)
{
	int index;
	int count;
	
	if (codeStart <= 0)
	{
		index = 0;
	}
	else
	{
		index = L_LookupEntry(pTab, codeStart);
	}
	
	if (codeEnd >= 0xFF)
	{
		count = pTab->count;
	}
	else
	{
		count = L_LookupEntry(pTab, codeEnd) + 1;
	}
	
	while (index < count)
	{
		if (pTab->tEntry[index].tag != TAG_INVALID)
		{
			if (pTab->tEntry[index].tag != TAG_MULIBYTE)
			{
				return 0;
			}
			
			if (!L_ValidateInvalid(pTab->tEntry[index].pTab, 0x00, 0xFF))
			{
				return 0;
			}
		}
		
		++index;
	}
	
	return 1;
}


static int L_ValidateRange(RFCPTOOL_VERIFY_CP_TAB *pTab, RFCPTOOL_UINT8 ttRange[][2], int countMax)
{
	int index;
	int count;
	
	index = 0;
	count = pTab->count;
	
	if (ttRange[0][0] > 0x00)
	{
		if (!L_ValidateInvalid(pTab, 0x00, ttRange[0][0] - 1))
		{
			return 0;
		}
		
		if (countMax > 1)
		{
			index = L_LookupEntry(pTab, ttRange[0][0]);
		}
	}
	
	if (ttRange[0][1] < 0xFF)
	{
		if (!L_ValidateInvalid(pTab, ttRange[0][1] + 1, 0xFF))
		{
			return 0;
		}
		
		if (countMax > 1)
		{
			count = L_LookupEntry(pTab, ttRange[0][1]) + 1;
		}
	}
	
	if (countMax > 1)
	{
		while (index < count)
		{
			if (pTab->tEntry[index].tag == TAG_MULIBYTE)
			{
				if (!L_ValidateRange(pTab->tEntry[index].pTab, &ttRange[1], countMax - 1))
				{
					return 0;
				}
			}
			
			++index;
		}
	}
	
	return 1;
}


static void L_ClearCtx(RFCPTOOL_VERIFY_CTX *pCtx)
{
	int j;
	
	for (j = 0; j < pCtx->count; ++j)
	{
		free(pCtx->tpTab[j]);
		pCtx->tpTab[j] = NULL;
	}
}


static int L_BuildCtx(RFCPTOOL_VERIFY_CTX *pCtx)
{
	RFCPTOOL_VERIFY_CP_TAB *pTab;
	int i, j;
	
	pCtx->count = 0;
	
	for (j = 0; j < 0x142; ++j)
	{
		pTab = RFCPTOOL_MemAlloc(sizeof (RFCPTOOL_VERIFY_CP_TAB));
		
		if (pTab == NULL)
		{
			L_ClearCtx(pCtx);
			return 0;
		}
		
		if (j <= 0x140)
		{
			for (i = 0; i < 256; ++i)
			{
				pTab->tEntry[i].codeMax = 0xFF;
				pTab->tEntry[i].tag = TAG_INVALID;
				pTab->tEntry[i].pTab = NULL;
			}
		}
		else if (j <= 0x141)
		{
			for (i = 0; i < 256; ++i)
			{
				pTab->tEntry[i].codeMax = 0xFF;
				pTab->tEntry[i].tag = TAG_IGNORE;
				pTab->tEntry[i].pTab = NULL;
			}
		}
		else
		{
			for (i = 0; i < 256; ++i)
			{
				pTab->tEntry[i].codeMax = 0xFF;
				pTab->tEntry[i].tag = (RFCPTOOL_UINT32)i;
				pTab->tEntry[i].pTab = NULL;
			}
		}
		
		pTab->count = 1;
		
		pCtx->tpTab[pCtx->count++] = pTab;
	}
	
	return 1;
}


static size_t L_LoadFile(const char *sPath, RFCPTOOL_UINT8 tBuf[FILE_SIZE_MAX])
{
	FILE *pFile;
	size_t count;
	
	if (sPath != NULL)
	{
		pFile = fopen(sPath, "rb");
		
		if (pFile != NULL)
		{
			count = fread(tBuf, 1, FILE_SIZE_MAX, pFile);
			
			fclose(pFile);
			
			if ((count > 0) && (count < FILE_SIZE_MAX))
			{
				return count;
			}
		}
	}
	
	RFCPTOOL_Error("Failed loading", " CP file ", sPath, NULL, 0);
	
	return 0;
}


static size_t L_ReadPCS(RFCPTOOL_UINT8 *tBuf, size_t count, RFCPTOOL_UINT32 *pC)
{
	RFCPTOOL_UINT32 c;
	
	if (!count)
	{
		return 0;
	}
	
	if ((tBuf[0] <= 0xBF) || (tBuf[0] >= 0xFE))
	{
		if (tBuf[0] == 0xFF)
		{
			*pC = TAG_INVALID;
		}
		else if (tBuf[0] == 0xFE)
		{
			*pC = TAG_IGNORE;
		}
		else
		{
			*pC = tBuf[0];
		}
		
		return 1;
	}
	
	if (count < 2)
	{
		return 0;
	}
	
	c = (tBuf[0] << 8) | tBuf[1];
	
	if (c < 0xEBC0)
	{
		*pC = c - 0xBF40;
		
		return 2;
	}
	
	if (count < 3)
	{
		return 0;
	}
	
	c = (c << 8) | tBuf[2];
	
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
	
	*pC = c;
	
	return 3;
}


static int L_Read(RFCPTOOL_VERIFY_CTX *pCtx, RFCPTOOL_UINT8 tBuf[FILE_SIZE_MAX], size_t count)
{
	RFCPTOOL_VERIFY_CP_ENTRY *pEntry;
	RFCPTOOL_UINT32 c;
	int codeMin;
	int codeMax;
	size_t index;
	size_t len;
	int j;
	
	if ((count < 8) || (tBuf[0] != 0x52) || (tBuf[1] != 0x46) || (tBuf[2] != 0x46) || (tBuf[3] != 0x46) || (tBuf[4] != 0x43) || (tBuf[5] != 0x50) || (tBuf[6] < 0x31) || (tBuf[6] > 0x34) || (tBuf[7] < 0x30) || (tBuf[7] > 0x39))
	{
		return 0;
	}
	
	codeMin = 0;
	codeMax = 0;
	index = 8;
	j = 0;
	
	while (index < count)
	{
		if (tBuf[index] != 0xFF)
		{
			codeMax = codeMin;
		}
		else
		{
			if (++index >= count)
			{
				return 0;  /* unexpected end of file */
			}
			
			if (tBuf[index] == 0xFF)
			{
				++index;
				codeMax = -1;
			}
			else
			{
				codeMax = codeMin + tBuf[index++] + 1;
				
				if (codeMax > 0xFF)
				{
					return 0;  /* range exceeds table */
				}
			}
		}
		
		if (codeMax >= 0)
		{
			pEntry = &pCtx->tpTab[j]->tEntry[pCtx->tpTab[j]->count - 1];
			
			pEntry->codeMax = codeMax;
			
			len = L_ReadPCS(&tBuf[index], count - index, &c);
			
			if (!len || (c > TAG_IGNORE))
			{
				return 0;  /* invalid encoding or unexpected end of file */
			}
			
			index += len;
			
			if (c < TAG_IGNORE)
			{
				pEntry->tag = c;  /* simple codepoint */
			}
			else
			{
				/* escape code:*/
				
				if (index >= count)
				{
					return 0;  /* unexpected end of file */
				}
				
				if (tBuf[index] <= 0x17)
				{
					if (tBuf[index] <= 0x07)
					{
						/* predefined: */
						
						if (tBuf[index] <= 0x01)
						{
							pEntry->tag = TAG_INVALID;
						}
						else if (tBuf[index] <= 0x03)
						{
							pEntry->tag = TAG_IGNORE;
						}
						else if (tBuf[index] <= 0x05)
						{
							pEntry->tag = TAG_IDENTITY;
						}
						else
						{
							pEntry->tag = TAG_SHIFT_IN;
						}
					}
					else if(tBuf[index] <= 0x0F)
					{
						return 0;  /* unexpected shift-out */
					}
					else
					{
						/* multibyte: */
						
						if (tBuf[index] <= 0x11)
						{
							pEntry->tag = TAG_MULIBYTE;
							pEntry->pTab = pCtx->tpTab[0x140];
						}
						else if (tBuf[index] <= 0x13)
						{
							pEntry->tag = TAG_MULIBYTE;
							pEntry->pTab = pCtx->tpTab[0x141];
						}
						else if (tBuf[index] <= 0x15)
						{
							pEntry->tag = TAG_MULIBYTE;
							pEntry->pTab = pCtx->tpTab[0x142];
						}
						else
						{
							if (++index >= count)
							{
								return 0;  /* unexpected end of file */
							}
							
							pEntry->tag = TAG_MULIBYTE;
							pEntry->pTab = pCtx->tpTab[tBuf[index] + 0x40];
						}
					}
					
					++index;
				}
				else if (tBuf[index] <= 0x1F)
				{
					if (tBuf[index] <= 0x1B)
					{
						if (tBuf[index] <= 0x19)
						{
							pEntry->tag = TAG_ITERATE;
						}
						else
						{
							pEntry->tag = TAG_ITERATE_LE;
						}
					}
					else
					{
						if (tBuf[index] <= 0x1D)
						{
							pEntry->tag = TAG_ITERATE_LE_32;
						}
						else
						{
							pEntry->tag = TAG_ITERATE_LE_16;
						}
					}
					
					++index;
					
					len = L_ReadPCS(&tBuf[index], count - index, &c);
					
					if (!len || (c >= TAG_IGNORE))
					{
						return 0;  /* invalid encoding or unexpected end of file */
					}
					
					index += len;
					
					pEntry->tag |= c;
				}
				else if (tBuf[index] <= 0x3F)
				{
					return 0;  /* unexpected sequence */
				}
				else if (tBuf[index] <= 0x7F)
				{
					return 0;  /* unexpected shift-out */
				}
				else if (tBuf[index] <= 0xBF)
				{
					pEntry->tag = TAG_MULIBYTE;
					pEntry->pTab = pCtx->tpTab[tBuf[index++] & 0x3F];
				}
				else
				{
					return 0;  /* unrecognized escape code */
				}
			}
			
			if (codeMax < 0xFF)
			{
				pCtx->tpTab[j]->count++;
				codeMin = codeMax + 1;
			}
			else
			{
				++j;
				codeMin = 0;
			}
		}
		else
		{
			++j;
			codeMin = 0;
		}
	}
	
	return 1;
}


static int L_ValidateASCII(RFCPTOOL_VERIFY_CTX *pCtx)
{
	RFCPTOOL_UINT8 ttRange[MULTIBYTE_DEPTH_MAX][2];
	RFCPTOOL_UINT32 i;
	RFCPTOOL_UINT8 code;
	
	for (i = 0x00; i <= 0x7F; ++i)
	{
		code = (RFCPTOOL_UINT8)i;
		
		if (!L_VerifyCodepoint(pCtx, &code, 1, i))
		{
			return 0;
		}
	}
	
	ttRange[0][0] = 0x00;
	ttRange[0][1] = 0x7F;
	
	return L_ValidateRange(pCtx->tpTab[0], ttRange, 1);
}


static int L_ValidateLatin1(RFCPTOOL_VERIFY_CTX *pCtx)
{
	RFCPTOOL_UINT32 i;
	RFCPTOOL_UINT8 code;
	
	for (i = 0x00; i <= 0xFF; ++i)
	{
		code = (RFCPTOOL_UINT8)i;
		
		if (!L_VerifyCodepoint(pCtx, &code, 1, i))
		{
			return 0;
		}
	}
	
	return 1;
}


static int L_ValidatePCS(RFCPTOOL_VERIFY_CTX *pCtx)
{
	RFCPTOOL_UINT8 ttRange[MULTIBYTE_DEPTH_MAX][2];
	RFCPTOOL_UINT8 tCode[3];
	RFCPTOOL_UINT32 i, j;
	RFCPTOOL_UINT32 codepoint;
	
	for (i = 0x00; i <= 0xBF; ++i)
	{
		tCode[0] = (RFCPTOOL_UINT8)i;
		
		if (!L_VerifyCodepoint(pCtx, tCode, 1, i))
		{
			return 0;
		}
	}
	
	codepoint = 0xC0;
	
	for (i = 0xC000; i <= 0xEBBF; ++i)
	{
		tCode[0] = (RFCPTOOL_UINT8)(i >> 8);
		tCode[1] = (RFCPTOOL_UINT8)(i & 0xFF);
		
		if (!L_VerifyCodepoint(pCtx, tCode, 2, codepoint++))
		{
			return 0;
		}
	}
	
	for (i = 0xEBC000; i <= 0xEC707F; ++i)
	{
		tCode[0] = (RFCPTOOL_UINT8)(i >> 16);
		tCode[1] = (RFCPTOOL_UINT8)((i >> 8) &0xFF);
		tCode[2] = (RFCPTOOL_UINT8)(i & 0xFF);
		
		if (!L_VerifyCodepoint(pCtx, tCode, 3, codepoint++))
		{
			return 0;
		}
	}
	
	codepoint = 0xE000;
	
	for (i = 0xEC7080; i <= 0xEC8E4F; ++i)
	{
		tCode[0] = (RFCPTOOL_UINT8)(i >> 16);
		tCode[1] = (RFCPTOOL_UINT8)((i >> 8) & 0xFF);
		tCode[2] = (RFCPTOOL_UINT8)(i & 0xFF);
		
		if (!L_VerifyCodepoint(pCtx, tCode, 3, codepoint++))
		{
			return 0;
		}
	}
	
	codepoint = 0xFDF0;
	
	for (i = 0xEC8E50; i <= 0xEC905D; ++i)
	{
		tCode[0] = (RFCPTOOL_UINT8)(i >> 16);
		tCode[1] = (RFCPTOOL_UINT8)((i >> 8) & 0xFF);
		tCode[2] = (RFCPTOOL_UINT8)(i & 0xFF);
		
		if (!L_VerifyCodepoint(pCtx, tCode, 3, codepoint++))
		{
			return 0;
		}
	}
	
	i = 0xEC905E;
	
	for (j = 0x010000; j <= 0x100000; j += 0x010000)
	{
		for (codepoint = 0; codepoint < 0xFFFE; ++codepoint)
		{
			tCode[0] = (RFCPTOOL_UINT8)(i >> 16);
			tCode[1] = (RFCPTOOL_UINT8)((i >> 8) & 0xFF);
			tCode[2] = (RFCPTOOL_UINT8)(i & 0xFF);
			
			if (!L_VerifyCodepoint(pCtx, tCode, 3, j + codepoint))
			{
				return 0;
			}
			
			++i;
		}
	}
	
	codepoint = 0x110000;
	
	for (i = 0xFC903E; i <= 0xFDFFFF; ++i)
	{
		tCode[0] = (RFCPTOOL_UINT8)(i >> 16);
		tCode[1] = (RFCPTOOL_UINT8)((i >> 8) & 0xFF);
		tCode[2] = (RFCPTOOL_UINT8)(i & 0xFF);
		
		if (!L_VerifyCodepoint(pCtx, tCode, 3, codepoint++))
		{
			return 0;
		}
	}
	
	ttRange[0][0] = 0x00;
	ttRange[0][1] = 0xFD;
	
	return L_ValidateRange(pCtx->tpTab[0], ttRange, 1);
}


static int L_ValidateUTF16(RFCPTOOL_VERIFY_CTX *pCtx, int fLittleEndian, int fUCS)
{
	RFCPTOOL_UINT8 ttRange[MULTIBYTE_DEPTH_MAX][2];
	RFCPTOOL_UINT8 tCode[4];
	RFCPTOOL_UINT32 i, j;
	int index0;
	int index1;
	
	if (!fLittleEndian)
	{
		index0 = 0;
		index1 = 1;
	}
	else
	{
		index0 = 1;
		index1 = 0;
	}
	
	for (i = 0x0000; i <= 0xFFFF; ++i)
	{
		tCode[index0] = (RFCPTOOL_UINT8)(i >> 8);
		tCode[index1] = (RFCPTOOL_UINT8)(i & 0xFF);
		
		if ((i >= 0xD800) && (i <= 0xDBFF) && !fUCS)
		{
			for (j = 0xDC00; j <= 0xDFFF; ++j)
			{
				tCode[2 + index0] = (RFCPTOOL_UINT8)(j >> 8);
				tCode[2 + index1] = (RFCPTOOL_UINT8)(j & 0xFF);
				
				if (!L_VerifyCodepoint(pCtx, tCode, 4, ((j >= 0xDC00) && (j <= 0xDFFF)) ? (((i - 0xD800) << 10) | (j - 0xDC00)) + 0x010000 : TAG_INVALID))
				{
					return 0;
				}
			}
		}
		else if (!L_VerifyCodepoint(pCtx, tCode, 2, ((i < 0xD800) || (i > 0xDFFF)) ? i : TAG_INVALID))
		{
			return 0;
		}
	}
	
	if (!fUCS)
	{
		if (!fLittleEndian)
		{
			ttRange[0][0] = 0x00;
			ttRange[0][1] = 0xFF;
			ttRange[1][0] = 0x00;
			ttRange[1][1] = 0xFF;
			ttRange[2][0] = 0xDC;
			ttRange[2][1] = 0xDF;
			ttRange[3][0] = 0x00;
			ttRange[3][1] = 0xFF;
		}
		else
		{
			ttRange[0][0] = 0x00;
			ttRange[0][1] = 0xFF;
			ttRange[1][0] = 0x00;
			ttRange[1][1] = 0xFF;
			ttRange[2][0] = 0x00;
			ttRange[2][1] = 0xFF;
			ttRange[3][0] = 0xDC;
			ttRange[3][1] = 0xDF;
		}
		
		if (!L_ValidateRange(pCtx->tpTab[0], ttRange, 4))
		{
			return 0;
		}
	}
	
	return 1;
}


static int L_ValidateUCS2BE(RFCPTOOL_VERIFY_CTX *pCtx)
{
	return L_ValidateUTF16(pCtx, 0, 1);
}


static int L_ValidateUCS2LE(RFCPTOOL_VERIFY_CTX *pCtx)
{
	return L_ValidateUTF16(pCtx, 1, 1);
}


static int L_ValidateUTF16BE(RFCPTOOL_VERIFY_CTX *pCtx)
{
	return L_ValidateUTF16(pCtx, 0, 0);
}


static int L_ValidateUTF16LE(RFCPTOOL_VERIFY_CTX *pCtx)
{
	return L_ValidateUTF16(pCtx, 1, 0);
}


static int L_ValidateUTF32(RFCPTOOL_VERIFY_CTX *pCtx, int fLittleEndian, int fUCS)
{
	RFCPTOOL_UINT8 ttRange[MULTIBYTE_DEPTH_MAX][2];
	RFCPTOOL_UINT8 tCode[8];
	RFCPTOOL_UINT32 i, j;
	int index0;
	int index1;
	int index2;
	int index3;
	
	if (!fLittleEndian)
	{
		index0 = 0;
		index1 = 1;
		index2 = 2;
		index3 = 3;
	}
	else
	{
		index0 = 3;
		index1 = 2;
		index2 = 1;
		index3 = 0;
	}
	
	for (i = 0x0000; i <= 0x10FFFF; ++i)
	{
		tCode[index0] = 0;
		tCode[index1] = (RFCPTOOL_UINT8)((i >> 16) & 0xFF);
		tCode[index2] = (RFCPTOOL_UINT8)((i >> 8) & 0xFF);
		tCode[index3] = (RFCPTOOL_UINT8)(i & 0xFF);
		
		if ((i >= 0xD800) && (i <= 0xDBFF) && !fUCS)
		{
			for (j = 0xDC00; j <= 0xDFFF; ++j)
			{
				tCode[4 + index0] = 0;
				tCode[4 + index1] = (RFCPTOOL_UINT8)((j >> 16) & 0xFF);
				tCode[4 + index2] = (RFCPTOOL_UINT8)((j >> 8) & 0xFF);
				tCode[4 + index3] = (RFCPTOOL_UINT8)(j & 0xFF);
				
				if (!L_VerifyCodepoint(pCtx, tCode, 8, ((j >= 0xDC00) && (j <= 0xDFFF)) ? (((i - 0xD800) << 10) | (j - 0xDC00)) + 0x010000 : TAG_INVALID))
				{
					return 0;
				}
			}
		}
		else if (!L_VerifyCodepoint(pCtx, tCode, 4, ((i < 0xD800) || (i > 0xDFFF)) ? i : TAG_INVALID))
		{
			return 0;
		}
	}
	
	if (!fUCS)
	{
		if (!fLittleEndian)
		{
			ttRange[0][0] = 0x00;
			ttRange[0][1] = 0x00;
			ttRange[1][0] = 0x00;
			ttRange[1][1] = 0x10;
			ttRange[2][0] = 0x00;
			ttRange[2][1] = 0xFF;
			ttRange[3][0] = 0x00;
			ttRange[3][1] = 0xFF;
			ttRange[4][0] = 0x00;
			ttRange[4][1] = 0x00;
			ttRange[5][0] = 0x00;
			ttRange[5][1] = 0x00;
			ttRange[6][0] = 0xDC;
			ttRange[6][1] = 0xDF;
			ttRange[7][0] = 0x00;
			ttRange[7][1] = 0xFF;
		}
		else
		{
			ttRange[0][0] = 0x00;
			ttRange[0][1] = 0xFF;
			ttRange[1][0] = 0x00;
			ttRange[1][1] = 0xFF;
			ttRange[2][0] = 0x00;
			ttRange[2][1] = 0x10;
			ttRange[3][0] = 0x00;
			ttRange[3][1] = 0x00;
			ttRange[4][0] = 0x00;
			ttRange[4][1] = 0xFF;
			ttRange[5][0] = 0xDC;
			ttRange[5][1] = 0xDF;
			ttRange[6][0] = 0x00;
			ttRange[6][1] = 0x00;
			ttRange[7][0] = 0x00;
			ttRange[7][1] = 0x00;
		}
		
		if (!L_ValidateRange(pCtx->tpTab[0], ttRange, 8))
		{
			return 0;
		}
	}
	
	if (!fLittleEndian)
	{
		ttRange[0][0] = 0x00;
		ttRange[0][1] = 0x00;
		ttRange[1][0] = 0x00;
		ttRange[1][1] = 0x10;
		ttRange[2][0] = 0x00;
		ttRange[2][1] = 0xFF;
		ttRange[3][0] = 0x00;
		ttRange[3][1] = 0xFF;
	}
	else
	{
		ttRange[0][0] = 0x00;
		ttRange[0][1] = 0xFF;
		ttRange[1][0] = 0x00;
		ttRange[1][1] = 0xFF;
		ttRange[2][0] = 0x00;
		ttRange[2][1] = 0x10;
		ttRange[3][0] = 0x00;
		ttRange[3][1] = 0x00;
	}
	
	return L_ValidateRange(pCtx->tpTab[0], ttRange, 4);
}


static int L_ValidateUCS4BE(RFCPTOOL_VERIFY_CTX *pCtx)
{
	return L_ValidateUTF32(pCtx, 0, 1);
}


static int L_ValidateUCS4LE(RFCPTOOL_VERIFY_CTX *pCtx)
{
	return L_ValidateUTF32(pCtx, 1, 1);
}


static int L_ValidateUTF32BE(RFCPTOOL_VERIFY_CTX *pCtx)
{
	return L_ValidateUTF32(pCtx, 0, 0);
}


static int L_ValidateUTF32LE(RFCPTOOL_VERIFY_CTX *pCtx)
{
	return L_ValidateUTF32(pCtx, 1, 0);
}


static int L_ValidateUTF8(RFCPTOOL_VERIFY_CTX *pCtx, int fRelaxed, int fCESU)
{
	RFCPTOOL_UINT8 ttRange[MULTIBYTE_DEPTH_MAX][2];
	RFCPTOOL_UINT8 tCode[8];
	RFCPTOOL_UINT32 i, j;
	int index;
	int count;
	
	/* 1-byte sequences: */
	
	for (i = 0; i <= 0x7F; ++i)
	{
		tCode[0] = (RFCPTOOL_UINT8)i;
		
		if (!L_VerifyCodepoint(pCtx, tCode, 1, i))
		{
			return 0;
		}
	}
	
	if (!L_ValidateInvalid(pCtx->tpTab[0], 0x80, 0xBF))
	{
		return 0;
	}
	
	/* 2-byte sequences: */
	
	for (i = 0; i <= 0x7FF; ++i)
	{
		tCode[0] = 0xC0 | (RFCPTOOL_UINT8)(i >> 6);
		tCode[1] = 0x80 | (RFCPTOOL_UINT8)(i & 0x3F);
		
		if (!L_VerifyCodepoint(pCtx, tCode, 2, (fRelaxed || (i > 0x7F)) ? i : TAG_INVALID))
		{
			return 0;
		}
	}
	
	ttRange[0][0] = 0x80;
	ttRange[0][1] = 0xBF;
	
	index = L_LookupEntry(pCtx->tpTab[0], 0xC0);
	count = L_LookupEntry(pCtx->tpTab[0], 0xDF) + 1;
	
	while (index < count)
	{
		if (!L_ValidateRange(pCtx->tpTab[0]->tEntry[index].pTab, ttRange, 1))
		{
			return 0;
		}
		
		++index;
	}
	
	/* 3-byte sequences: */
	
	for (i = 0; i <= 0xFFFF; ++i)
	{
		tCode[0] = 0xE0 | (RFCPTOOL_UINT8)(i >> 12);
		tCode[1] = 0x80 | (RFCPTOOL_UINT8)((i >> 6) & 0x3F);
		tCode[2] = 0x80 | (RFCPTOOL_UINT8)(i & 0x3F);
		
		if ((i >= 0xD800) && (i <= 0xDBFF) && fCESU)
		{
			for (j = 0xDC00; j <= 0xDFFF; ++j)
			{
				tCode[3] = 0xE0 | (RFCPTOOL_UINT8)(j >> 12);
				tCode[4] = 0x80 | (RFCPTOOL_UINT8)((j >> 6) & 0x3F);
				tCode[5] = 0x80 | (RFCPTOOL_UINT8)(j & 0x3F);
				
				if (!L_VerifyCodepoint(pCtx, tCode, 6, (((i - 0xD800) << 10) | (j - 0xDC00)) + 0x010000))
				{
					return 0;
				}
			}
			
			if (fRelaxed)
			{
				for (j = 0xDC00; j <= 0xDFFF; ++j)
				{
					tCode[3] = 0xF0 | (RFCPTOOL_UINT8)(j >> 18);
					tCode[4] = 0x80 | (RFCPTOOL_UINT8)((j >> 12) & 0x3F);
					tCode[5] = 0x80 | (RFCPTOOL_UINT8)((j >> 6) & 0x3F);
					tCode[6] = 0x80 | (RFCPTOOL_UINT8)(j & 0x3F);
					
					if (!L_VerifyCodepoint(pCtx, tCode, 7, (((i - 0xD800) << 10) | (j - 0xDC00)) + 0x010000))
					{
						return 0;
					}
				}
			}
		}
		else if (!L_VerifyCodepoint(pCtx, tCode, 3, ((fRelaxed || (i > 0x7FF)) && ((i < 0xD800) || i > 0xDFFF)) ? i : TAG_INVALID))
		{
			return 0;
		}
	}
	
	/*!! to do: low-surrogate range if fCESU !!!*/
	
	ttRange[1][0] = 0x80;
	ttRange[1][1] = 0xBF;
	
	index = L_LookupEntry(pCtx->tpTab[0], 0xE0);
	count = L_LookupEntry(pCtx->tpTab[0], 0xEF) + 1;
	
	while (index < count)
	{
		if (!L_ValidateRange(pCtx->tpTab[0]->tEntry[index].pTab, ttRange, 2))
		{
			return 0;
		}
		
		++index;
	}
	
	/* 4-byte sequences: */
	
	for (i = 0; i <= 0x10FFFF; ++i)
	{
		tCode[0] = 0xF0 | (RFCPTOOL_UINT8)(i >> 18);
		tCode[1] = 0x80 | (RFCPTOOL_UINT8)((i >> 12) & 0x3F);
		tCode[2] = 0x80 | (RFCPTOOL_UINT8)((i >> 6) & 0x3F);
		tCode[3] = 0x80 | (RFCPTOOL_UINT8)(i & 0x3F);
		
		if ((i >= 0xD800) && (i <= 0xDBFF) && fCESU && fRelaxed)
		{
			for (j = 0xDC00; j <= 0xDFFF; ++j)
			{
				tCode[4] = 0xE0 | (RFCPTOOL_UINT8)(j >> 12);
				tCode[5] = 0x80 | (RFCPTOOL_UINT8)((j >> 6) & 0x3F);
				tCode[6] = 0x80 | (RFCPTOOL_UINT8)(j & 0x3F);
				
				if (!L_VerifyCodepoint(pCtx, tCode, 7, (((i - 0xD800) << 10) | (j - 0xDC00)) + 0x010000))
				{
					return 0;
				}
			}
			
			for (j = 0xDC00; j <= 0xDFFF; ++j)
			{
				tCode[4] = 0xF0 | (RFCPTOOL_UINT8)(j >> 18);
				tCode[5] = 0x80 | (RFCPTOOL_UINT8)((j >> 12) & 0x3F);
				tCode[6] = 0x80 | (RFCPTOOL_UINT8)((j >> 6) & 0x3F);
				tCode[7] = 0x80 | (RFCPTOOL_UINT8)(j & 0x3F);
				
				if (!L_VerifyCodepoint(pCtx, tCode, 8, (((i - 0xD800) << 10) | (j - 0xDC00)) + 0x010000))
				{
					return 0;
				}
			}
		}
		else if (!L_VerifyCodepoint(pCtx, tCode, 4, ((fRelaxed || (i > 0xFFFF)) && ((i < 0xD800) || i > 0xDFFF)) ? i : TAG_INVALID))
		{
			return 0;
		}
	}
	
	/*!! to do: low-surrogate range if fCESU !!!*/
	
	ttRange[2][0] = 0x80;
	ttRange[2][1] = 0xBF;
	
	index = L_LookupEntry(pCtx->tpTab[0], 0xF0);
	count = L_LookupEntry(pCtx->tpTab[0], 0xF3) + 1;
	
	while (index < count)
	{
		if (!L_ValidateRange(pCtx->tpTab[0]->tEntry[index].pTab, ttRange, 3))
		{
			return 0;
		}
		
		++index;
	}
	
	ttRange[0][1] = 0x8F;
	
	index = L_LookupEntry(pCtx->tpTab[0], 0xF4);
	
	if (!L_ValidateRange(pCtx->tpTab[0]->tEntry[index].pTab, ttRange, 3))
	{
		return 0;
	}
	
	return L_ValidateInvalid(pCtx->tpTab[0], 0xF5, 0xFF);
}


static int L_ValidateUTF8Strict(RFCPTOOL_VERIFY_CTX *pCtx)
{
	return L_ValidateUTF8(pCtx, 0, 0);
}


static int L_ValidateUTF8Relaxed(RFCPTOOL_VERIFY_CTX *pCtx)
{
	return L_ValidateUTF8(pCtx, 1, 0);
}


static int L_ValidateCESU8Strict(RFCPTOOL_VERIFY_CTX *pCtx)
{
	return L_ValidateUTF8(pCtx, 0, 1);
}


static int L_ValidateCESU8Relaxed(RFCPTOOL_VERIFY_CTX *pCtx)
{
	return L_ValidateUTF8(pCtx, 1, 1);
}


static VALIDATE_FUNC* L_GetValidateFunc(const char *sEncoding)
{
	if (!RFCPTOOL_IsStr(sEncoding, "ASCII", RFCPTOOL_IS_STR_FLAG_IGNORE_CASE))
	{
		return &L_ValidateASCII;
	}
	
	if (!RFCPTOOL_IsStr(sEncoding, "LATIN-1", RFCPTOOL_IS_STR_FLAG_IGNORE_CASE))
	{
		return &L_ValidateLatin1;
	}
	
	if (!RFCPTOOL_IsStr(sEncoding, "PCS", RFCPTOOL_IS_STR_FLAG_IGNORE_CASE))
	{
		return &L_ValidatePCS;
	}
	
	if (!RFCPTOOL_IsStr(sEncoding, "UCS-2BE", RFCPTOOL_IS_STR_FLAG_IGNORE_CASE))
	{
		return &L_ValidateUCS2BE;
	}
	
	if (!RFCPTOOL_IsStr(sEncoding, "UCS-2LE", RFCPTOOL_IS_STR_FLAG_IGNORE_CASE))
	{
		return &L_ValidateUCS2LE;
	}
	
	if (!RFCPTOOL_IsStr(sEncoding, "UTF-16BE", RFCPTOOL_IS_STR_FLAG_IGNORE_CASE))
	{
		return &L_ValidateUTF16BE;
	}
	
	if (!RFCPTOOL_IsStr(sEncoding, "UTF-16LE", RFCPTOOL_IS_STR_FLAG_IGNORE_CASE))
	{
		return &L_ValidateUTF16LE;
	}
	
	if (!RFCPTOOL_IsStr(sEncoding, "UCS-4BE", RFCPTOOL_IS_STR_FLAG_IGNORE_CASE))
	{
		return &L_ValidateUCS4BE;
	}
	
	if (!RFCPTOOL_IsStr(sEncoding, "UCS-4LE", RFCPTOOL_IS_STR_FLAG_IGNORE_CASE))
	{
		return &L_ValidateUCS4LE;
	}
	
	if (!RFCPTOOL_IsStr(sEncoding, "UTF-32BE", RFCPTOOL_IS_STR_FLAG_IGNORE_CASE))
	{
		return &L_ValidateUTF32BE;
	}
	
	if (!RFCPTOOL_IsStr(sEncoding, "UTF-32LE", RFCPTOOL_IS_STR_FLAG_IGNORE_CASE))
	{
		return &L_ValidateUTF32LE;
	}
	
	if (!RFCPTOOL_IsStr(sEncoding, "UTF-8", RFCPTOOL_IS_STR_FLAG_IGNORE_CASE))
	{
		return &L_ValidateUTF8Strict;
	}
	
	if (!RFCPTOOL_IsStr(sEncoding, "UTF-8X", RFCPTOOL_IS_STR_FLAG_IGNORE_CASE))
	{
		return &L_ValidateUTF8Relaxed;
	}
	
	if (!RFCPTOOL_IsStr(sEncoding, "CESU-8", RFCPTOOL_IS_STR_FLAG_IGNORE_CASE))
	{
		return &L_ValidateCESU8Strict;
	}
	
	if (!RFCPTOOL_IsStr(sEncoding, "CESU-8X", RFCPTOOL_IS_STR_FLAG_IGNORE_CASE))
	{
		return &L_ValidateCESU8Relaxed;
	}
	
	return NULL;
}


/*****************************************************************************************
 *
 *  E X T E R N A L   F U N C T I O N S
 *
 *****************************************************************************************/


int RFCPTOOL_Verify(const char *sEncoding, const char *sSrcPath)
{
	RFCPTOOL_UINT8 tBuf[FILE_SIZE_MAX];
	VALIDATE_FUNC *pValidateFunc;
	RFCPTOOL_VERIFY_CTX ctx;
	size_t count;
	
	assert(sEncoding != NULL);
	
	pValidateFunc = L_GetValidateFunc(sEncoding);
	
	if (pValidateFunc == NULL)
	{
		RFCPTOOL_Error("Invalid character encoding", " ", sEncoding, NULL, 0);
		return 0;
	}
	
	count = L_LoadFile(sSrcPath, tBuf);
	
	if (!count)
	{
		return 0;
	}
	
	if (!L_BuildCtx(&ctx))
	{
		return 0;
	}
	
	if (!L_Read(&ctx, tBuf, count))
	{
		RFCPTOOL_Error("Invalid format", " of CP file ", sSrcPath, NULL, 0);
		L_ClearCtx(&ctx);
		return 0;
	}
	
	if (!(*pValidateFunc)(&ctx))
	{
		RFCPTOOL_Error(sEncoding, " does not match specification of CP file ", sSrcPath, NULL, 0);
		L_ClearCtx(&ctx);
		return 0;
	}
	
	printf("\nSUCCESS: CP file \'%s\' represents %s character encoding.\n", sSrcPath, sEncoding);
	
	return 1;
}
