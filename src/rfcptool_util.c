
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
 *  File:          /src/rfcptool_util.c//
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
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "rfcptool_def.h"
#include "rfcptool_util.h"


/*****************************************************************************************
 *
 *  T Y P E   D E F I N I T I O N S
 *
 *****************************************************************************************/


typedef struct RFCPTOOL_STR_TAB_ELEMENT {
	int index;
	char *sStr;
} RFCPTOOL_STR_TAB_ELEMENT;


typedef struct RFCPTOOL_STR_TAB {
	int count;
	int countMax;
	RFCPTOOL_STR_TAB_ELEMENT *tElement;
} RFCPTOOL_STR_TAB;


typedef struct RFCPTOOL_SEQUENCE_NODE {
	struct RFCPTOOL_SEQUENCE_NODE *tpChildNode[2];
	RFCPTOOL_UINT32 *sSequence;
} RFCPTOOL_SEQUENCE_NODE;


typedef struct RFCPTOOL_SEQUENCE_TAB {
	struct RFCPTOOL_SEQUENCE_NODE *pTree;
	RFCPTOOL_UINT32 count;
} RFCPTOOL_SEQUENCE_TAB;


/*****************************************************************************************
 *
 *  I N T E R N A L   F U N C T I O N S
 *
 *****************************************************************************************/


static void L_Msg(const char *sType, const char *sMsgPrefix, const char *sPathPrefix, const char *sPath, const char *sMsgSuffix, int line)
{
	if (sMsgSuffix == NULL)
	{
		sMsgSuffix = "";
	}
	
	if (sPath == NULL)
	{
		fprintf(stderr, "\n%s: %s%s", sType, sMsgPrefix, sMsgSuffix);
	}
	else
	{
		fprintf(stderr, "\n%s: %s%s\'%s\'%s", sType, sMsgPrefix, sPathPrefix, sPath, sMsgSuffix);
	}
	
	if (line > 0)
	{
		fprintf(stderr, " at line %d.\n", line);
	}
	else
	{
		fprintf(stderr, ".\n");
	}
}


static void L_DestroySequenceTree(RFCPTOOL_SEQUENCE_NODE *pSequenceTree)
{
	if (pSequenceTree != NULL)
	{
		L_DestroySequenceTree(pSequenceTree->tpChildNode[0]);
		L_DestroySequenceTree(pSequenceTree->tpChildNode[1]);
		RFCPTOOL_MemFree(pSequenceTree);
	}
}


static RFCPTOOL_DIR* L_CreateDir(const char *sDir)
{
	RFCPTOOL_DIR *pDir;
	char *sPath;
	size_t size;
	size_t len;
	
	if (sDir == NULL)
	{
		return NULL;
	}
	
	/* determine length of sDir string: */
	
	len = 0;
	
	while (sDir[len] != '\0')
	{
		++len;
	}
	
	/* calculate total size of object: */
	
	size = sizeof (RFCPTOOL_DIR) + len + RFCPTOOL_DOMAIN_NAME_LEN_MAX + 5;
	
	if (size < len)
	{
		return RFCPTOOL_MemAlloc(0);  /* overflow */
	}
	
	/* allocate and initilize object: */
	
	pDir = RFCPTOOL_MemAlloc(size);
	
	if (pDir == NULL)
	{
		return NULL;
	}
	
	sPath = (char*)&pDir[1];  /* path is located behind the TFC_CP_DIR struct */
	
	pDir->pNext = NULL;
	pDir->sPath = sPath;
	pDir->sFileNameBuf = &sPath[len];
	
	/* copy sDir to object: */
	
	RFCPTOOL_StrCopy(sPath, sDir, len + 1);
	
	return pDir;
}


/*****************************************************************************************
 *
 *  E X T E R N A L   F U N C T I O N S
 *
 *****************************************************************************************/


void RFCPTOOL_Error(const char *sMsgPrefix, const char *sPathPrefix, const char *sPath, const char *sMsgSuffix, int line)
{
	L_Msg("ERROR", sMsgPrefix, sPathPrefix, sPath, sMsgSuffix, line);
}


void RFCPTOOL_Warning(const char *sMsgPrefix, const char *sPathPrefix, const char *sPath, const char *sMsgSuffix, int line)
{
	L_Msg("WARNING", sMsgPrefix, sPathPrefix, sPath, sMsgSuffix, line);
}


void* RFCPTOOL_MemAlloc(size_t size)
{
	void *pPtr;
	
	if (size > 0)
	{
		pPtr = malloc(size);
		
		if (pPtr != NULL)
		{
			return pPtr;
		}
	}
	
	RFCPTOOL_Error("Out of memory", NULL, NULL, NULL, 0);
	
	return NULL;
}


void RFCPTOOL_MemFree(void *pPtr)
{
	if (pPtr != NULL)
	{
		free(pPtr);
	}
}


size_t RFCPTOOL_EncodePCS(RFCPTOOL_UINT32 codepoint, RFCPTOOL_UINT8 tBuf[])
{
	if ((codepoint > 0x126FC1) || ((codepoint >= 0xDD00) && (codepoint <= 0xDFFF)) || ((codepoint >= 0xFDD0) && (codepoint <= 0xFDEF)) || ((codepoint <= 0x10FFFF) && ((codepoint & 0xFFFE) == 0xFFFE)))
	{
		return 0;
	}
	
	if (codepoint <= 0xBF)
	{
		tBuf[0] = (RFCPTOOL_UINT8)codepoint;
		
		return 1;
	}
	
	if (codepoint <= 0x2C7F)
	{
		codepoint += 0xBF40;
		
		tBuf[0] = (RFCPTOOL_UINT8)((codepoint >> 8) & 0xFF);
		tBuf[1] = (RFCPTOOL_UINT8)(codepoint & 0xFF);
		
		return 2;
	}
	
	if (codepoint < 0x00FDF0)
	{
		if (codepoint < 0x00E000)
		{
			codepoint += 0xEB9380;
		}
		else
		{
			codepoint += 0xEB9080;
		}
	}
	else
	{
		if (codepoint < 0x120000)
		{
			codepoint += 0xEB9060 - ((codepoint >> 15) & 0x3E);
		}
		else
		{
			codepoint += 0xEB903E;
		}
	}
	
	tBuf[0] = (RFCPTOOL_UINT8)((codepoint >> 16) & 0xFF);
	tBuf[1] = (RFCPTOOL_UINT8)((codepoint >> 8) & 0xFF);
	tBuf[2] = (RFCPTOOL_UINT8)(codepoint & 0xFF);
	
	return 3;
}


char RFCPTOOL_ToUpper(int c)
{
	return (char)toupper(c);
}


int RFCPTOOL_IsUpper(int c)
{
	return isupper(c);
}


int RFCPTOOL_IsAlpha(int c)
{
	return isalpha(c);
}


int RFCPTOOL_IsDigit(int c)
{
	return isdigit(c);
}


int RFCPTOOL_IsAlNum(int c)
{
	return isalnum(c);
}


int RFCPTOOL_IsStr(const char *sStr1, const char *sStr2, RFCPTOOL_UINT16 flags)
{
	assert(sStr1 != NULL);
	assert(sStr2 != NULL);
	
	if (flags & RFCPTOOL_IS_STR_FLAG_IGNORE_CASE)
	{
		while (*sStr2 != '\0')
		{
			if (toupper(*sStr1++) != toupper(*sStr2++))
			{
				return 0;
			}
		}
	}
	else
	{
		while (*sStr2 != '\0')
		{
			if (*sStr1++ != *sStr2++)
			{
				return 0;
			}
		}
	}
	
	if ((*sStr1 != '\0') && !(flags & RFCPTOOL_IS_STR_FLAG_PREFIX))
	{
		return 0;
	}
	
	return 1;
}


int RFCPTOOL_StrCmp(const char *sStr1, const char *sStr2, size_t lenMax)
{
	return strncmp(sStr1, sStr2, lenMax);
}


size_t RFCPTOOL_StrCopy(char *sDest, const char *sSrc, size_t countMax)
{
	size_t pos;
	
	assert(sDest != NULL);
	assert(sSrc != NULL);
	
	pos = 0;
	
	while ((pos < countMax) && (sSrc[pos] != '\0'))
	{
		sDest[pos] = toupper(sSrc[pos]);
		++pos;
	}
	
	if (pos < countMax)
	{
		sDest[pos] = '\0';
	}
	
	return pos;
}


char* RFCPTOOL_StrDup(const char *sStr)
{
	char *sDest;
	size_t len;
	
	len = strlen(sStr);
	
	sDest = RFCPTOOL_MemAlloc(len + 1);
	
	if (sDest == NULL)
	{
		return NULL;
	}
	
	strcpy(sDest, sStr);
	
	return sDest;
}


void* RFCPTOOL_CreateStrTab(int countMax)
{
	RFCPTOOL_STR_TAB *pStrTab;
	
	assert(countMax > 0);
	
	pStrTab = RFCPTOOL_MemAlloc(sizeof (RFCPTOOL_STR_TAB));
	
	if (pStrTab == NULL)
	{
		return NULL;
	}
	
	pStrTab->count = 0;
	pStrTab->countMax = countMax;
	pStrTab->tElement = RFCPTOOL_MemAlloc((size_t)countMax * sizeof (RFCPTOOL_STR_TAB_ELEMENT));
	
	if (pStrTab->tElement == NULL)
	{
		RFCPTOOL_MemFree(pStrTab);
		return NULL;
	}
	
	return pStrTab;
}


void RFCPTOOL_DestroyStrTab(void *hStrTab)
{
	RFCPTOOL_STR_TAB *pStrTab;
	int index;
	
	pStrTab = hStrTab;
	
	if (pStrTab != NULL)
	{
		for (index = 0; index < pStrTab->count; ++index)
		{
			RFCPTOOL_MemFree(pStrTab->tElement[index].sStr);
		}
	}
	
	RFCPTOOL_MemFree(pStrTab->tElement);
	RFCPTOOL_MemFree(pStrTab);
}


int RFCPTOOL_GetStrTabCount(void *hStrTab)
{
	RFCPTOOL_STR_TAB *pStrTab;
	
	pStrTab = hStrTab;
	
	return (pStrTab != NULL) ? pStrTab->count : 0;
}


int RFCPTOOL_InsertStrTab(void *hStrTab, const char *sStr)
{
	RFCPTOOL_STR_TAB *pStrTab;
	char *sDest;
	int l, r, m;
	int cmp;
	
	pStrTab = hStrTab;
	
	assert(pStrTab != NULL);
	assert(pStrTab->count < pStrTab->countMax);
	
	l = 0;
	r = pStrTab->count - 1;
	m = 0;
	
	while (l <= r)
	{
		m = (l + r) / 2;
		
		cmp = strcmp(pStrTab->tElement[m].sStr, sStr);
		
		if (!cmp)
		{
			return -1;
		}
		
		if (cmp > 0)
		{
			r = m - 1;
		}
		else
		{
			l = ++m;
		}
	}
	
	sDest = RFCPTOOL_StrDup(sStr);
	
	if (sDest == NULL)
	{
		return 0;
	}
	
	l = pStrTab->count;
	
	while (--l >= m)
	{
		pStrTab->tElement[l + 1] = pStrTab->tElement[l];
	}
	
	pStrTab->tElement[m].index = ++pStrTab->count;
	pStrTab->tElement[m].sStr = sDest;
	
	return 1;
}


int RFCPTOOL_LookupStrTab(void *hStrTab, const char *sStr)
{
	RFCPTOOL_STR_TAB *pStrTab;
	int l, r, m;
	int cmp;
	
	pStrTab = hStrTab;
	
	assert(pStrTab != NULL);
	
	l = 0;
	r = pStrTab->count - 1;
	
	while (l <= r)
	{
		m = (l + r) / 2;
		
		cmp = strcmp(pStrTab->tElement[m].sStr, sStr);
		
		if (!cmp)
		{
			return pStrTab->tElement[m].index;
		}
		
		if (cmp > 0)
		{
			r = m - 1;
		}
		else
		{
			l = m + 1;
		}
	}
	
	return 0;
}


void* RFCPTOOL_CreateSequenceTab(void)
{
	RFCPTOOL_SEQUENCE_TAB *pSequenceTab;
	
	pSequenceTab = RFCPTOOL_MemAlloc(sizeof (RFCPTOOL_SEQUENCE_TAB));
	
	if (pSequenceTab != NULL)
	{
		pSequenceTab->pTree = NULL;
		pSequenceTab->count = 0;
	}
	
	return pSequenceTab;
}


void RFCPTOOL_DestroySequenceTab(void *hSequenceTab)
{
	RFCPTOOL_SEQUENCE_TAB *pSequenceTab;
	
	pSequenceTab = hSequenceTab;
	
	if (pSequenceTab != NULL)
	{
		L_DestroySequenceTree(pSequenceTab->pTree);
		RFCPTOOL_MemFree(pSequenceTab);
	}
}


RFCPTOOL_UINT32 RFCPTOOL_AddSequenceTab(void *hSequenceTab, const RFCPTOOL_UINT32 *sSequence, size_t len)
{
	RFCPTOOL_SEQUENCE_TAB *pSequenceTab;
	RFCPTOOL_SEQUENCE_NODE **ppSequenceNode;
	RFCPTOOL_SEQUENCE_NODE *pSequenceNode;
	RFCPTOOL_UINT32 index;
	
	pSequenceTab = hSequenceTab;
	
	assert(pSequenceTab != NULL);
	assert(sSequence != NULL);
	
	if (!len)
	{
		while (sSequence[len] != RFCPTOOL_TAG_TYPE_INVALID)
		{
			++len;
		}
	}
	
	assert((len >= RFCPTOOL_SEQUENCE_LEN_MIN) && (len <= RFCPTOOL_SEQUENCE_LEN_MAX));
	
	pSequenceNode = RFCPTOOL_MemAlloc(sizeof (RFCPTOOL_SEQUENCE_NODE) + sizeof (RFCPTOOL_UINT32) * (len + 1));
	
	if (pSequenceNode == NULL)
	{
		return RFCPTOOL_CODEPOINT_ERROR;
	}
	
	pSequenceNode->tpChildNode[0] = NULL;
	pSequenceNode->tpChildNode[1] = NULL;
	pSequenceNode->sSequence = (RFCPTOOL_UINT32*)&pSequenceNode[1];
	
	pSequenceNode->sSequence[len] = RFCPTOOL_TAG_TYPE_INVALID; /* terminator */
	
	while (len--)
	{
		pSequenceNode->sSequence[len] = sSequence[len];
	}
	
	++pSequenceTab->count;
	
	ppSequenceNode = &pSequenceTab->pTree;
	
	if (*ppSequenceNode != NULL)
	{
		for (index = pSequenceTab->count; index > 1; index >>= 1)
		{
			assert(*ppSequenceNode != NULL);
			
			ppSequenceNode = &(*ppSequenceNode)->tpChildNode[index & 1];
		}
		
		assert(*ppSequenceNode == NULL);
	}
	
	*ppSequenceNode = pSequenceNode;
	
	return pSequenceTab->count;
}


const RFCPTOOL_UINT32* RFCPTOOL_LookupSequenceTab(const void *hSequenceTab, RFCPTOOL_UINT32 index)
{
	const RFCPTOOL_SEQUENCE_TAB *pSequenceTab;
	const RFCPTOOL_SEQUENCE_NODE *pSequenceNode;
	
	pSequenceTab = hSequenceTab;
	
	assert(pSequenceTab != NULL);
	
	if ((index < 1) || (index > pSequenceTab->count))
	{
		return NULL;
	}
	
	pSequenceNode = pSequenceTab->pTree;
	
	while (index > 1)
	{
		assert(pSequenceNode != NULL);
		
		pSequenceNode = pSequenceNode->tpChildNode[index & 1];
		index >>= 1;
	}
	
	assert(pSequenceNode != NULL);
	
	return pSequenceNode->sSequence;
}


RFCPTOOL_DIR* RFCPTOOL_CreateDefaultDir(void)
{
	return L_CreateDir(getenv("RETROCPSPECDIR"));
}


RFCPTOOL_DIR* RFCPTOOL_CreateDir(const char *sDir)
{
	assert(sDir != NULL);
	
	return L_CreateDir(sDir);
}


void RFCPTOOL_DestroyDirList(RFCPTOOL_DIR **ppList)
{
	RFCPTOOL_DIR *pDir;
	
	while (*ppList != NULL)
	{
		pDir = *ppList;
		*ppList = pDir->pNext;
		
		RFCPTOOL_MemFree(pDir);
	}
}
