
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
 *  File:          /src/rfcptool_cpspec.c//
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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "rfcptool_def.h"
#include "rfcptool_util.h"
#include "rfcptool_file.h"
#include "rfcptool_compile.h"
#include "rfcptool_cpspec.h"


/*****************************************************************************************
 *
 *  M A C R O   D E F I N I T I O N S
 *
 *****************************************************************************************/


#define RFCPTOOL_CPSPEC_TAB_INDEX_MAX     319  /* maximum codepage table count is 320 (up to 319 from forward reference identifiers according to CPSPEC data format plus initial codepage table) */


/*****************************************************************************************
 *
 *  T Y P E   D E F I N I T I O N S
 *
 *****************************************************************************************/


typedef struct RFCPTOOL_CPSPEC_REF_RANGE {
	RFCPTOOL_UINT32 codeMin;  /* first code which is referenced in the destination codepage table */
	RFCPTOOL_UINT32 codeMax;  /* last code which is referenced in the destination codepage table */
	RFCPTOOL_UINT32 code;     /* first code of the source codepage table which is "copied" to the destination codepage table */
} RFCPTOOL_CPSPEC_REF_RANGE;


typedef struct RFCPTOOL_CPSPEC_REF_PREDEFINED {
	RFCPTOOL_UINT32 codeMin;  /* first code which is referenced in the predefined codepage table (must be adjusted to destination codepage table) */
	RFCPTOOL_UINT32 codeMax;  /* last code which is referenced in the predefined codepage table (must be adjusted to destination codepage table) */
	RFCPTOOL_UINT32 tag;
} RFCPTOOL_CPSPEC_REF_PREDEFINED;


typedef struct RFCPTOOL_CPSPEC_REF {
	struct RFCPTOOL_CPSPEC_REF *pNext;
	RFCPTOOL_CP_TAB *pTab;                  /* may be NULL (unresolved multibyte or shift-out forward reference) */
	union {
		RFCPTOOL_UINT32 *pTag;              /* valid if pTab == NULL */
		RFCPTOOL_CPSPEC_REF_RANGE *pRange;  /* valid if pTab != NULL; used temporarily to adjust codeMax of pending mapping reference (may be NULL) */
	} tab;
	RFCPTOOL_CPSPEC_REF_RANGE range;
} RFCPTOOL_CPSPEC_REF;


typedef struct RFCPTOOL_CPSPEC_REF_IDENTIFIER {
	char sIdentifier[RFCPTOOL_IDENTIFIER_LEN_MAX + 1];
	RFCPTOOL_CPSPEC_REF *pList;
} RFCPTOOL_CPSPEC_REF_IDENTIFIER;


typedef struct RFCPTOOL_CPSPEC_TAB_CTX {
	RFCPTOOL_CP_TAB *tpTab[RFCPTOOL_CPSPEC_TAB_INDEX_MAX + 1];
	int count;
	void *hSequenceTab;
	char sDomain[RFCPTOOL_DOMAIN_NAME_LEN_MAX + 1];                               /* name of the next domain if references remain unresolved */
	char sIdentifier[RFCPTOOL_IDENTIFIER_LEN_MAX + 1];                            /* shift-out backward identifier (always starts with a letter) */
	RFCPTOOL_CPSPEC_REF_PREDEFINED tRefPredefined[256];                           /* temporary table of mapping references to predefined codepages */
	RFCPTOOL_CPSPEC_REF_IDENTIFIER *tpRefPending[RFCPTOOL_CPSPEC_TAB_INDEX_MAX];  /* table of unresolved references (initial reference will be single) */
	RFCPTOOL_CPSPEC_REF_IDENTIFIER *tpRefCurrent[RFCPTOOL_CPSPEC_TAB_INDEX_MAX];  /* table of references matching the current block (initial reference will be single) */
	int refPredefinedCount;                                                       /* number of elements in tRefPredefined <= 256 */
	int refPendingCount;                                                          /* number of elements in tpRefPending <= RFCPTOOL_CPSPEC_TAB_INDEX_MAX */
	int refCurrentCount;                                                          /* number of elements in tpRefCurrent <= RFCPTOOL_CPSPEC_TAB_INDEX_MAX */
	int refIdentifierCount;                                                       /* number of total identifiers used by references <= RFCPTOOL_CPSPEC_TAB_INDEX_MAX */
	RFCPTOOL_DIR *pDirList;                                                       /* directory list */
	RFCPTOOL_DIR *pDir;                                                           /* directory of the currently opened file */
	RFCPTOOL_FILE_TEXT *pSrcFile;                                                 /* handle of the currently opened file*/
	int line;                                                                     /* input line (ignore if pSrcFile == NULL) */
	int offset;                                                                   /* file buffer code (ignore if pSrcFile == NULL) */
	int fError;                                                                   /* error state (ignore if pSrcFile == NULL) */
} RFCPTOOL_CPSPEC_TAB_CTX;


/*****************************************************************************************
 *
 *  I N T E R N A L   S U P P O R T I N G   F U N C T I O N S
 *
 *****************************************************************************************/


static void L_DestroyRefIdentifier(RFCPTOOL_CPSPEC_REF_IDENTIFIER *pRefIdentifier)
{
	RFCPTOOL_CPSPEC_REF *pRef;
	
	assert(pRefIdentifier != NULL);
	
	while (pRefIdentifier->pList != NULL)
	{
		pRef = pRefIdentifier->pList;
		pRefIdentifier->pList = pRef->pNext;
		
		RFCPTOOL_MemFree(pRef);
	}
	
	RFCPTOOL_MemFree(pRefIdentifier);
}


static size_t L_IsValidName(const char *sName, size_t lenMax, char c)
{
	size_t pos;
	
	assert(sName != NULL);
	assert(lenMax >= 2);
	
	if (!RFCPTOOL_IsAlpha(sName[0]))
	{
		return 0;
	}
	
	pos = 0;
	
	while (sName[++pos] != c)
	{
		if (pos >= lenMax)
		{
			return 0;
		}
		
		switch (sName[pos])
		{
		case '\0':
			
			return 0;
			
		case '-':
			
			if (++pos >= lenMax)
			{
				return 0;
			}
		}
		
		if (!RFCPTOOL_IsAlNum(sName[pos]))
		{
			return 0;
		}
	}
	
	return pos;
}


static int L_IsValidIdentifier(const char *sIdentifier, int fLeadingZeros)
{
	RFCPTOOL_UINT32 number;
	size_t len;
	
	assert(sIdentifier != NULL);
	
	if (RFCPTOOL_IsDigit(sIdentifier[0]) || fLeadingZeros)
	{
		number = 0;
		
		if ((sIdentifier[0] >= '1') && (sIdentifier[0] <= '9'))
		{
			len = 0;
			
			do
			{
				number = (number * 10) + (sIdentifier[len++] - '0');
			}
			while ((len < RFCPTOOL_IDENTIFIER_LEN_MAX) && RFCPTOOL_IsDigit(sIdentifier[len]) && (number <= 0xFFFE));
			
			if ((sIdentifier[len] == '\0') && (number <= 0xFFFE))
			{
				return 1;
			}
		}
	}
	else if (L_IsValidName(sIdentifier, RFCPTOOL_IDENTIFIER_LEN_MAX, '\0'))
	{
		return 1;
	}
	
	return 0;
}


static void L_ResolveUnspecified(RFCPTOOL_UINT32 tTag[256])
{
	RFCPTOOL_UINT32 code;
	
	for (code = 0; code < 256; ++code)
	{
		if (tTag[code] == RFCPTOOL_TAG_TYPE_UNSPECIFIED)
		{
			tTag[code] = RFCPTOOL_TAG_TYPE_PREDEFINED | RFCPTOOL_TAG_INVALID;
		}
	}
}


static int L_AdjustRange(RFCPTOOL_CPSPEC_REF_RANGE *pDestRange, const RFCPTOOL_CPSPEC_REF_RANGE *pSrcRange, RFCPTOOL_UINT32 codeMin, RFCPTOOL_UINT32 codeMax, int fIdentity)
{
	RFCPTOOL_UINT32 min, max;
	
	min = pSrcRange->codeMin;
	max = pSrcRange->codeMax;
	
	if ((codeMin > pSrcRange->code + (max - min)) || (codeMax < pSrcRange->code))
	{
		return 0;  /* out of range */
	}
	
	if (codeMin > pSrcRange->code)
	{
		min += codeMin - pSrcRange->code;
	}
	
	if (max > pSrcRange->codeMin + (codeMax - pSrcRange->code))
	{
		max = pSrcRange->codeMin + (codeMax - pSrcRange->code);
	}
	
	pDestRange->codeMin = min;
	pDestRange->codeMax = max;
	
	if (!fIdentity)
	{
		/* "=" reference: */
		
		if (codeMin < pSrcRange->code)
		{
			pDestRange->code = pSrcRange->code - codeMin;
		}
		else
		{
			pDestRange->code = 0;
		}
	}
	else
	{
		/* "==" reference: */
		
		if (codeMin < pSrcRange->code)
		{
			pDestRange->code = pSrcRange->code;
		}
		else
		{
			pDestRange->code = codeMin;
		}
	}
	
	return 1;
}


/*****************************************************************************************
 *
 *  I N T E R N A L   C O N T E X T - B A S E D   F U N C T I O N S
 *
 *****************************************************************************************/


static void L_UnexpectedEOF(RFCPTOOL_CPSPEC_TAB_CTX *pCtx)
{
	if (!pCtx->fError)
	{
		RFCPTOOL_Error("Unexpected end of file", " in codepage specification file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
		pCtx->fError = 1;
	}
}


static void L_ClearCtx(RFCPTOOL_CPSPEC_TAB_CTX *pCtx)
{
	assert(pCtx != NULL);
	
	if (pCtx->pSrcFile != NULL)
	{
		RFCPTOOL_FileDestroy(&pCtx->pSrcFile->ref);
		pCtx->pSrcFile = NULL;
	}
	
	while (pCtx->refPendingCount > 0)
	{
		L_DestroyRefIdentifier(pCtx->tpRefPending[--pCtx->refPendingCount]);
	}
	
	while (pCtx->refCurrentCount > 0)
	{
		L_DestroyRefIdentifier(pCtx->tpRefCurrent[--pCtx->refCurrentCount]);
	}
	
	while (pCtx->count > 0)
	{
		RFCPTOOL_MemFree(pCtx->tpTab[--pCtx->count]);
	}
	
	RFCPTOOL_DestroySequenceTab(pCtx->hSequenceTab);
	pCtx->hSequenceTab = NULL;
}


static char L_ReadFile(RFCPTOOL_CPSPEC_TAB_CTX *pCtx)
{
	int fComment;
	char c1, c2;
	
	assert(pCtx != NULL);
	assert(pCtx->pSrcFile != NULL);
	assert(pCtx->pDir != NULL);
	
	if (pCtx->fError)
	{
		return '\0';
	}
	
	fComment = 0;
	
	c1 = RFCPTOOL_FILE_PEEK_CHAR(pCtx->pSrcFile, pCtx->offset);
	
	while (c1 != '\0')
	{
		if (pCtx->offset >= RFCPTOOL_FILE_BUF_SIZE - 1)  /* ensure that at least two characters are in the buffer */
		{
			if (!RFCPTOOL_FileReadText(pCtx->pSrcFile, pCtx->offset))
			{
				pCtx->fError = 1;
				return 0;
			}
			
			pCtx->offset = 0;
		}
		
		++pCtx->offset;
		
		switch (c1)
		{
		case '\0':
			
			return '\0';
			
		case ';':
			
			fComment = 1;
			break;
			
		case '\n':
			
			fComment = 0;
			++pCtx->line;
			/* no break */
			
		case ' ':
			
			c2 = RFCPTOOL_FILE_PEEK_CHAR(pCtx->pSrcFile, pCtx->offset);
			
			if (!fComment && (c2 != ';') && (c2 != '\n') && (c2 != ' ') && (c2 != '\0'))
			{
				return ' ';
			}
			break;
			
		default:
			
			if (!(RFCPTOOL_IsUpper(c1) || RFCPTOOL_IsDigit(c1)) && (c1 != '\"') && (c1 != '(') && (c1 != ')') && (c1 != '*') && (c1 != '+') && (c1 != ',') && (c1 != '-') && (c1 != '.') && (c1 != '/') && ((c1 != ':') && (c1 != '<') && (c1 != '=') && (c1 != '>') && (c1 != '?')))
			{
				RFCPTOOL_Error("Invalid character", " in codepage specification file ", pCtx->pDir->sPath, NULL, pCtx->line);
				pCtx->fError = 1;
				return '\0';
			}
			
			if (!fComment)
			{
				return c1;
			}
		}
		
		c1 = RFCPTOOL_FILE_PEEK_CHAR(pCtx->pSrcFile, pCtx->offset);
	}
	
	return '\0';
}


static int L_OpenFile(RFCPTOOL_CPSPEC_TAB_CTX *pCtx)
{
	size_t pos;
	int fSkip;
	
	assert(pCtx != NULL);
	assert(pCtx->pSrcFile == NULL);
	assert(pCtx->pDirList != NULL);
	
	if (pCtx->sDomain[0] == '\0')
	{
		RFCPTOOL_Error("Unresolved codepage reference(s) left", NULL, NULL, NULL, 0);
		return 0;
	}
	
	pos = RFCPTOOL_StrCopy(pCtx->pDirList->sFileNameBuf, pCtx->sDomain, RFCPTOOL_DOMAIN_NAME_LEN_MAX);
	
	pCtx->pDirList->sFileNameBuf[pos++] = '.';
	pCtx->pDirList->sFileNameBuf[pos++] = 'C';
	pCtx->pDirList->sFileNameBuf[pos++] = 'P';
	pCtx->pDirList->sFileNameBuf[pos++] = 'S';
	pCtx->pDirList->sFileNameBuf[pos] = '\0';
	
	pCtx->pDir = pCtx->pDirList;
	
	/* while file open was not successful or file type was invalid, try next directory in the list (however, abort on errors): */
	
	pCtx->pSrcFile = RFCPTOOL_FileOpenInputCPSPEC(pCtx->pDir->sPath, pCtx->sDomain, &fSkip);
	
	while (fSkip)
	{
		pCtx->pDir = pCtx->pDir->pNext;
		
		if (pCtx->pDir == NULL)
		{
			RFCPTOOL_Error("No valid codepage specification", " file ", pCtx->pDirList->sPath, " found", 0);
			return 0;
		}
		
		strcpy(pCtx->pDir->sFileNameBuf, pCtx->pDirList->sFileNameBuf);
		
		pCtx->pSrcFile = RFCPTOOL_FileOpenInputCPSPEC(pCtx->pDir->sPath, pCtx->sDomain, &fSkip);
	}
	
	if (pCtx->pSrcFile == NULL)
	{
		return 0;
	}
	
	if (pCtx->sDomain[0] != '\0')
	{
		if (!L_IsValidName(pCtx->sDomain, RFCPTOOL_DOMAIN_NAME_LEN_MAX, '\0'))
		{
			RFCPTOOL_Error("Invalid domain header", " in codepage specification file ", pCtx->pSrcFile->ref.sPath, NULL, 0);
			RFCPTOOL_FileDestroy(&pCtx->pSrcFile->ref);
			pCtx->pSrcFile = NULL;
			return 0;
		}
	}

	pCtx->line = pCtx->pSrcFile->headerLines + 1;
	pCtx->offset = 0;
	
	return 1;
}


static int L_SkipIdentifierSequence(RFCPTOOL_CPSPEC_TAB_CTX *pCtx)
{
	assert(pCtx != NULL);
	assert(pCtx->pSrcFile != NULL);
	assert(pCtx->pDir != NULL);
	
	while (RFCPTOOL_FILE_PEEK_CHAR(pCtx->pSrcFile, pCtx->offset) != '\0')
	{
		switch (L_ReadFile(pCtx))
		{
		case '(':
			
			return 1;
			
		case '<':
			
			return -1;
		}
	}
	
	L_UnexpectedEOF(pCtx);
	
	return 0;
}


static int L_SkipBlock(RFCPTOOL_CPSPEC_TAB_CTX *pCtx)
{
	int fSequence;
	
	assert(pCtx != NULL);
	assert(pCtx->pSrcFile != NULL);
	assert(pCtx->pDir != NULL);
	
	fSequence = 0;
	
	while (RFCPTOOL_FILE_PEEK_CHAR(pCtx->pSrcFile, pCtx->offset) != '\0')
	{
		switch (L_ReadFile(pCtx))
		{
		case '(':
			
			if (fSequence)
			{
				RFCPTOOL_Error("Invalid \'(\'", " in codepage specification file ", pCtx->pSrcFile->ref.sPath, NULL, 0);
				return 0;
			}
			
			fSequence = 1;
			break;
			
		case ')':
			
			if (!fSequence)
			{
				return 1;
			}
			
			fSequence = 0;
		}
	}
	
	L_UnexpectedEOF(pCtx);
	
	return 0;
}


static void L_AppendRefPredefined(RFCPTOOL_CPSPEC_TAB_CTX *pCtx, RFCPTOOL_UINT32 code, RFCPTOOL_UINT32 tag)
{
	RFCPTOOL_CPSPEC_REF_PREDEFINED *pPredefined;
	
	assert(pCtx != NULL);
	assert(pCtx->refPredefinedCount < 256);
	
	pPredefined = &pCtx->tRefPredefined[pCtx->refPredefinedCount++];
	
	pPredefined->codeMin = code;
	pPredefined->codeMax = 0xFF;
	pPredefined->tag = tag;
}


static RFCPTOOL_CPSPEC_REF_IDENTIFIER* L_InsertRef(RFCPTOOL_CPSPEC_TAB_CTX *pCtx, const char *sIdentifier, RFCPTOOL_CP_TAB *pTab, RFCPTOOL_UINT32 *pTag)
{
	RFCPTOOL_CPSPEC_REF_IDENTIFIER *pRefIdentifier;
	RFCPTOOL_CPSPEC_REF *pRef;
	int indexMin;
	int indexMax;
	int index;
	
	assert(pCtx != NULL);
	assert(sIdentifier != NULL);
	
	/* lookup position of new reference in pending references table: */
	
	pRefIdentifier = NULL;
	
	indexMin = 0;
	indexMax = pCtx->refPendingCount - 1;
	
	while ((indexMin <= indexMax) && (pRefIdentifier == NULL))
	{
		index = (indexMin + indexMax) / 2;
		
		switch (RFCPTOOL_StrCmp(sIdentifier, pCtx->tpRefPending[index]->sIdentifier, RFCPTOOL_IDENTIFIER_LEN_MAX + 1))
		{
		case -1:
			
			indexMax = index - 1;
			break;
			
		case 1:
			
			indexMin = index + 1;
			break;
			
		default:
			
			pRefIdentifier = pCtx->tpRefPending[index];
		}
	}
	
	/* create new identifier reference if identifier was not found: */
	
	if (pRefIdentifier == NULL)
	{
		if (pCtx->refIdentifierCount >= RFCPTOOL_CPSPEC_TAB_INDEX_MAX)
		{
			RFCPTOOL_Error("Too may identifier references in codepage specification file(s)", NULL, NULL, NULL, 0);
			pCtx->fError = 1;
			return NULL;
		}
		
		pRefIdentifier = RFCPTOOL_MemAlloc(sizeof (RFCPTOOL_CPSPEC_REF_IDENTIFIER));
		
		if (pRefIdentifier == NULL)
		{
			pCtx->fError = 1;
			return NULL;
		}
		
		RFCPTOOL_StrCopy(pRefIdentifier->sIdentifier, sIdentifier, RFCPTOOL_IDENTIFIER_LEN_MAX + 1);
		
		pRefIdentifier->pList = NULL;
		
		index = pCtx->refPendingCount++;
		
		while (index > indexMin)
		{
			pCtx->tpRefPending[index] = pCtx->tpRefPending[index - 1];
			--index;
		}
		
		pCtx->tpRefPending[indexMin] = pRefIdentifier;
		
		++pCtx->refIdentifierCount;
	}
	
	/* insert reference: */
	
	pRef = RFCPTOOL_MemAlloc(sizeof(RFCPTOOL_CPSPEC_REF));
	
	if (pRef == NULL)
	{
		pCtx->fError = 1;
		return NULL;
	}
	
	pRef->pTab = pTab;
	pRef->range.codeMin = 0x00;
	pRef->range.codeMax = 0xFF;
	pRef->range.code = 0x00;
	
	if (pTab != NULL)
	{
		assert(pTag == NULL);
		
		pRef->tab.pRange = NULL;
		
	}
	else
	{
		pRef->tab.pTag = pTag;
	}
	
	pRef->pNext = pRefIdentifier->pList;
	pRefIdentifier->pList = pRef;
	
	return pRefIdentifier;
}


static char L_ParseIdentifier(RFCPTOOL_CPSPEC_TAB_CTX *pCtx, char *sIdentifier, char c)
{
	int fLeadingZeros;
	int pos;
	
	assert(pCtx != NULL);
	assert(sIdentifier != NULL);
	
	fLeadingZeros = 0;
	pos = 0;
	
	if (RFCPTOOL_IsAlpha(c))
	{
		do
		{
			sIdentifier[pos++] = c;
			
			c = L_ReadFile(pCtx);  /* skip leading zeros */
		}
		while ((pos <= RFCPTOOL_IDENTIFIER_LEN_MAX) && (RFCPTOOL_IsAlNum(c) || (c == '-')));
	}
	else if (RFCPTOOL_IsDigit(c))
	{
		while (c == '0')
		{
			fLeadingZeros = 1;
			c = L_ReadFile(pCtx);
		}
		
		while ((pos <= RFCPTOOL_IDENTIFIER_LEN_MAX) && RFCPTOOL_IsDigit(c))
		{
			sIdentifier[pos++] = c;
			
			c = L_ReadFile(pCtx);
		}
	}
	else
	{
		RFCPTOOL_Error("Codepage identifier expected", " in codepage specification file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
		pCtx->fError = 1;
		return '\0';
	}
	
	if (c == '\0')
	{
		L_UnexpectedEOF(pCtx);
		return '\0';
	}
	
	if (pos <= RFCPTOOL_IDENTIFIER_LEN_MAX)
	{
		sIdentifier[pos] = '\0';
	}
	
	if (!L_IsValidIdentifier(sIdentifier, fLeadingZeros))
	{
		RFCPTOOL_Error("Invalid codepage identifier", " in codepage specification file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
		pCtx->fError = 1;
		return '\0';
	}
	
	return c;
}


static int L_ParseBackwardIdentifier(RFCPTOOL_CPSPEC_TAB_CTX *pCtx)
{
	char c;
	
	c = L_ReadFile(pCtx);
	
	if (c == ' ')
	{
		c = L_ReadFile(pCtx);  /* skip optional whitespace */
	}
	
	if (!RFCPTOOL_IsAlpha(c))
	{
		if (!pCtx->fError)
		{
			RFCPTOOL_Error("Backward shift reference expected", " in codepage specification file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
			pCtx->fError = 1;
		}
		
		return 0;
	}
	
	c = L_ParseIdentifier(pCtx, pCtx->sIdentifier, c);
	
	if (c == ' ')
	{
		c = L_ReadFile(pCtx);  /* skip optional whitespace */
	}
	
	if (c != '(')
	{
		if (!pCtx->fError)
		{
			RFCPTOOL_Error("\'(\' expected", " in codepage specification file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
			pCtx->fError = 1;
		}
		
		return 0;
	}
	
	return 1;
}


static int L_ParseIdentifierSequence(RFCPTOOL_CPSPEC_TAB_CTX *pCtx)
{
	char sIdentifier[RFCPTOOL_IDENTIFIER_LEN_MAX + 1];
	int indexMin;
	int indexMax;
	int index;
	int result;
	char c;
	
	assert(pCtx != NULL);
	
	pCtx->sIdentifier[0] = '\0';
	
	if (!pCtx->refPendingCount)
	{
		return -1;  /* stop CP file processing when there is nothing more to do */
	}
	
	c = L_ReadFile(pCtx);
	
	if (c == ' ')
	{
		c = L_ReadFile(pCtx);  /* skip optional whitespace */
	}
	
	if (c == '\0')
	{
		return pCtx->fError ? 0 : -1;  /* error or end-of-file */
	}
	
	do
	{
		if (c == '?')
		{
			/* match any pending reference: */
			
			for (index = 0; index < pCtx->refPendingCount; ++index)
			{
				assert(pCtx->refCurrentCount < RFCPTOOL_CPSPEC_TAB_INDEX_MAX);
				
				pCtx->tpRefCurrent[pCtx->refCurrentCount++] = pCtx->tpRefPending[index];
			}
			
			pCtx->refPendingCount = 0;
			
			result = L_SkipIdentifierSequence(pCtx);
			
			if (result >= 0)
			{
				return result;
			}
			
			return L_ParseBackwardIdentifier(pCtx);
		}
		
		c = L_ParseIdentifier(pCtx, sIdentifier, c);
		
		if (c == '\0')
		{
			assert(pCtx->fError);
			return 0;
		}
		
		indexMin = 0;
		indexMax = pCtx->refPendingCount - 1;
		
		while (indexMin <= indexMax)
		{
			index = (indexMin + indexMax) / 2;
			
			switch (RFCPTOOL_StrCmp(sIdentifier, pCtx->tpRefPending[index]->sIdentifier, RFCPTOOL_IDENTIFIER_LEN_MAX + 1))
			{
			case -1:
				
				indexMax = index - 1;
				break;
				
			case 1:
				
				indexMin = index + 1;
				break;
				
			default:
				
				/* determine first and last reference to sIdentifier: */
				
				indexMin = index;
				indexMax = index;
				
				while ((indexMin > 0) && !RFCPTOOL_StrCmp(sIdentifier, pCtx->tpRefPending[indexMin - 1]->sIdentifier, RFCPTOOL_IDENTIFIER_LEN_MAX + 1))
				{
					--indexMin;
				}
				
				while ((indexMax < pCtx->refPendingCount - 1) && !RFCPTOOL_StrCmp(sIdentifier, pCtx->tpRefPending[indexMax + 1]->sIdentifier, RFCPTOOL_IDENTIFIER_LEN_MAX + 1))
				{
					++indexMax;
				}
				
				/* move all pending references to sIdentifier to current: */
				
				for (index = indexMin; index <= indexMax; ++index)
				{
					assert(pCtx->refCurrentCount < RFCPTOOL_CPSPEC_TAB_INDEX_MAX);
					
					pCtx->tpRefCurrent[pCtx->refCurrentCount++] = pCtx->tpRefPending[index];
				}
				
				pCtx->refPendingCount -= indexMax - indexMin + 1;
				
				if (!pCtx->refPendingCount)
				{
					if (c == '(')
					{
						return 1;
					}
					
					result = L_SkipIdentifierSequence(pCtx);
					
					if (result >= 0)
					{
						return result;
					}
					
					return L_ParseBackwardIdentifier(pCtx);
				}
				
				while (indexMin < pCtx->refPendingCount)
				{
					pCtx->tpRefPending[indexMin++] = pCtx->tpRefPending[++indexMax];
				}
				
				indexMax = -1;  /* exit loop and continue looking up remaining pending references */
			}
		}
		
		if (c == ' ')
		{
			c = L_ReadFile(pCtx);  /* skip optional whitespace */
		}
		
		switch (c)
		{
		case ',':
			
			break;
			
		case '(':
			
			return 1;
			
		case '<':
			
			return L_ParseBackwardIdentifier(pCtx);
			
		default:
			
			if (!pCtx->fError)
			{
				RFCPTOOL_Error("\',\' or \'(\' expected", " in codepage specification file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
				pCtx->fError = 1;
				return 0;
			}
		}
		
		c = L_ReadFile(pCtx);
		
		if (c == ' ')
		{
			c = L_ReadFile(pCtx);  /* skip optional whitespace */
		}
	}
	while (!pCtx->fError);
	
	return 0;
}


static char L_ReadFullStop(RFCPTOOL_CPSPEC_TAB_CTX *pCtx)
{
	char c;
	
	assert(pCtx != NULL);
	
	c = L_ReadFile(pCtx);
	
	if (c == '.')
	{
		return c;
	}
	
	if (!pCtx->fError)
	{
		RFCPTOOL_Error("\'..\' expected", " in codepage specification file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
		pCtx->fError = 1;
	}
	
	return '\0';
}


static char L_ReadFValue(RFCPTOOL_CPSPEC_TAB_CTX *pCtx, RFCPTOOL_UINT32 *pValue, char c)
{
	RFCPTOOL_UINT32 value;
	
	assert(pCtx != NULL);
	assert(pValue != NULL);
	
	value = 0;
	
	do
	{
		if ((c >= '0') && (c <= '9'))
		{
			value = (value << 4) | (c - '0');
		}
		else if ((c >= 'A') && (c <= 'F'))
		{
			value = (value << 4) | (c - 'A' + 10);
		}
		else
		{
			if (!pCtx->fError)
			{
				RFCPTOOL_Error("Invalid codepoint value character", " in codepage specification file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
				pCtx->fError = 1;
			}
			
			return '\0';
		}
		
		if (value > RFCPTOOL_CODEPOINT_MAX)
		{
			value = RFCPTOOL_CODEPOINT_MAX + 1;  /* trigger codepoint value overflow error (continue parsing but ensure that value remains > RFCPTOOL_CODEPOINT_MAX on loop exit without overflow) */
		}
		
		c = L_ReadFile(pCtx);
	}
	while (RFCPTOOL_IsAlNum(c));
	
	if (value <= RFCPTOOL_CODEPOINT_MAX)
	{
		/* check for reserved codepoints (codepoint value overflows will be handled by caller): */
		
		if (((value >= 0xDD00) && (value <= 0xDFFF)) || ((value >= 0xFDD0) && (value <= 0xFDEF)) || ((value <= 0x10FFFF) && (((value & 0xFFFFFE) == 0xFFFFFE))))
		{
			if (!pCtx->fError)
			{
				RFCPTOOL_Error("Invalid codepoint", " in codepage specification file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
				pCtx->fError = 1;
			}
			
			return '\0';
		}
	}
	
	*pValue = value;
	
	return c;
}


static char L_ParseValue(RFCPTOOL_CPSPEC_TAB_CTX *pCtx, RFCPTOOL_UINT32 *pTag, int *pRange, int fAllowOffset, char c)
{
	RFCPTOOL_UINT32 sSequence[RFCPTOOL_SEQUENCE_LEN_MAX + 1];
	RFCPTOOL_UINT32 value;
	RFCPTOOL_UINT32 tag;
	size_t len;
	int fSequence;
	
	assert(pCtx != NULL);
	assert(pTag != NULL);
	assert(pRange != NULL);
	
	*pRange = 0;
	
	if (c == '(')
	{
		c = L_ReadFile(pCtx);
		
		if (c == ' ')
		{
			c = L_ReadFile(pCtx);  /* skip optional whitespace */
		}
		
		if (!RFCPTOOL_IsAlNum(c))
		{
			RFCPTOOL_Error("Codepoint sequence expected", " in codepage specification file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
			pCtx->fError = 1;
			return 0;
		}
		
		fSequence = 1;
	}
	else
	{
		fSequence = 0;
	}
	
	len = 0;
	tag = 0;
	
	do
	{
		c = L_ReadFValue(pCtx, &value, c);
		
		if (c == ':')
		{
			if (!fAllowOffset || fSequence || (c == ')') || (c == '\0'))
			{
				if (!pCtx->fError)
				{
					RFCPTOOL_Error("Offset not allowed at this location", " in codepage specification file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
					pCtx->fError = 1;
				}
				
				return '\0';
			}
			
			if (value <= 0xFF)
			{
				*pTag = value;
				
				return c;  /* caller should detect offset by !*pRange rather than c == ':' */
			}
			
			value = RFCPTOOL_CODEPOINT_MAX + 1;  /* trigger codepoint value overflow error */
		}
		
		if (value > RFCPTOOL_CODEPOINT_MAX)
		{
			RFCPTOOL_Error("Codepoint value overflow", " in codepage specification file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
			pCtx->fError = 1;
			return '\0';
		}
		
		if (c == '.')
		{
			do
			{
				c = L_ReadFullStop(pCtx);
				
				c = L_ReadFile(pCtx);
				
				switch (c)
				{
				case '.':
					
					if (!pCtx->fError)
					{
						RFCPTOOL_Error("Invalid \'.\'", " in codepage specification file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
						pCtx->fError = 1;
					}
					
					return '\0';
					
				case ' ':
					
					c = L_ReadFullStop(pCtx);
				}
			}
			while (c == '.');
			
			*pTag = value;
			
			c = L_ReadFValue(pCtx, &value, c);
			
			if ((value > RFCPTOOL_CODEPOINT_MAX) || (value <= *pTag) || ((*pTag < 0xDD00) && (value > 0xDFFF)) || ((*pTag < 0xFDD0) && (value > 0xFDEF)) || ((*pTag <= 0x10FFFF) && ((*pTag ^ value) & 0xFF0000)))
			{
				if (!pCtx->fError)
				{
					RFCPTOOL_Error("Invalid range", " in codepage specification file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
					pCtx->fError = 1;
				}
				
				return '\0';
			}
			
			*pRange = (int)(value - *pTag + 1);
			
			return c;
		}
		
		if (c == ' ')
		{
			c = L_ReadFile(pCtx);  /* skip optional whitespace */
		}
		
		if (pCtx->fError)
		{
			return '\0';
		}
		
		*pRange = 1;
		
		if (!fSequence)
		{
			*pTag = value;
			return c;
		}
		
		if (len >= RFCPTOOL_SEQUENCE_LEN_MAX)
		{
			RFCPTOOL_Error("Codepoint sequence has too many elements", " in codepage specification file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
			pCtx->fError = 1;
			return '\0';
		}
		
		sSequence[len++] = value;
		value = 0;
		
		if (c == ')')
		{
			c = L_ReadFile(pCtx);
			
			if (c == ' ')
			{
				c = L_ReadFile(pCtx);  /* skip optional whitespace */
			}
			
			if (!tag)
			{
				tag = RFCPTOOL_TAG_TYPE_SEQUENCE;  /* not invertible sequence by default */
			}
			
			tag |= RFCPTOOL_AddSequenceTab(pCtx->hSequenceTab, sSequence, len);  /* returns 0 on error or 1-based index of sequence otherwise */
			
			if (!(tag & RFCPTOOL_TAG_MASK_VALUE))
			{
				pCtx->fError = 1;
				return '\0';
			}
			
			*pTag = tag;
			
			return c;
		}
		
		if (c == '+')
		{
			c = L_ReadFile(pCtx);
			
			if (c == ' ')
			{
				c = L_ReadFile(pCtx);  /* skip optional whitespace */
			}
			
			if ((tag == RFCPTOOL_TAG_TYPE_SEQUENCE) && !pCtx->fError)
			{
				RFCPTOOL_Error("Unexpected \'+\'", " in codepage specification file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
				pCtx->fError = 1;
				return '\0';
			}
			
			tag = RFCPTOOL_TAG_TYPE_SEQUENCE_INV;
		}
		else if (tag != RFCPTOOL_TAG_TYPE_SEQUENCE_INV)
		{
			tag = RFCPTOOL_TAG_TYPE_SEQUENCE;
		}
		else
		{
			RFCPTOOL_Error("\'+\' expected", " in codepage specification file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
			pCtx->fError = 1;
			return '\0';
		}
	}
	while (!pCtx->fError);
	
	return '\0';
}


static RFCPTOOL_UINT32* L_GetMappingTagPtr(RFCPTOOL_CPSPEC_TAB_CTX *pCtx, RFCPTOOL_CPSPEC_REF *pMapping, RFCPTOOL_UINT32 code)
{
	RFCPTOOL_UINT32 *pCodepoint;
	
	assert(pCtx != NULL);
	assert(pMapping != NULL);
	assert(pMapping->pTab != NULL);
	
	if (code >= pMapping->range.code)
	{
		code = (code - pMapping->range.code) + pMapping->range.codeMin;
		
		if (code <= pMapping->range.codeMax)
		{
			pCodepoint = &pMapping->pTab->tTag[code];
			
			if (*pCodepoint == RFCPTOOL_TAG_TYPE_UNSPECIFIED)
			{
				return pCodepoint;
			}
		}
	}
	
	return NULL;
}


static int L_ApplyTag(RFCPTOOL_CPSPEC_TAB_CTX *pCtx, RFCPTOOL_UINT32 code, RFCPTOOL_UINT32 tag, const char *sIdentifier)
{
	/* will never fail if sIdentifier == NULL */
	
	RFCPTOOL_CPSPEC_REF_IDENTIFIER *tpRefIdentifier;
	RFCPTOOL_CPSPEC_REF *pRef;
	RFCPTOOL_UINT32 *pTag;
	int index;
	
	assert(pCtx != NULL);
	
	for (index = 0; index < pCtx->refCurrentCount; ++index)
	{
		tpRefIdentifier = pCtx->tpRefCurrent[index];
		
		for (pRef = tpRefIdentifier->pList; pRef != NULL; pRef = pRef->pNext)
		{
			pTag = L_GetMappingTagPtr(pCtx, pRef, code);
			
			if (pTag != NULL)
			{
				*pTag = tag;
				
				if (sIdentifier != NULL)
				{
					/* as codepoint table index is not known yet for multibyte or shift-out forward references, insert table index reference: */
					
					assert(sIdentifier[0] != '\0');
					
					if (L_InsertRef(pCtx, sIdentifier, NULL, pTag) == NULL)
					{
						pCtx->fError = 1;
						return 0;
					}
				}
			}
		}
	}
	
	return 1;
}


static void L_ApplyPredefined(RFCPTOOL_CPSPEC_TAB_CTX *pCtx)
{
	RFCPTOOL_CPSPEC_REF_RANGE range;
	RFCPTOOL_CPSPEC_REF_PREDEFINED *pPredefined;
	RFCPTOOL_CPSPEC_REF *pRef;
	RFCPTOOL_UINT32 code;
	int refCurrentIndex;
	int refPredefinedIndex;
	
	assert(pCtx != NULL);
	
	for (refCurrentIndex = 0; refCurrentIndex < pCtx->refCurrentCount; ++refCurrentIndex)
	{
		for (pRef = pCtx->tpRefCurrent[refCurrentIndex]->pList; pRef != NULL; pRef = pRef->pNext)
		{
			for (refPredefinedIndex = 0; refPredefinedIndex < pCtx->refPredefinedCount; ++refPredefinedIndex)
			{
				pPredefined = &pCtx->tRefPredefined[refPredefinedIndex];
				
				if (L_AdjustRange(&range, &pRef->range, pPredefined->codeMin, pPredefined->codeMax, pPredefined->tag ? 1 : 0))
				{
					if ((pPredefined->tag & RFCPTOOL_TAG_MASK_TYPE) == RFCPTOOL_TAG_TYPE_CODEPOINT)
					{
						/* reference to "/" (implicit LATIN-1 codepage): */
						
						for (code = range.codeMin; code <= range.codeMax; ++code)
						{
							if (pRef->pTab->tTag[code] == RFCPTOOL_TAG_TYPE_UNSPECIFIED)
							{
								pRef->pTab->tTag[code] = (code - range.codeMin) + range.code;
							}
						}
					}
					else
					{
						/* reference to "." or "-" (fixed codepoints, thus range.code can be ignored): */
						
						for (code = range.codeMin; code <= range.codeMax; ++code)
						{
							if (pRef->pTab->tTag[code] == RFCPTOOL_TAG_TYPE_UNSPECIFIED)
							{
								pRef->pTab->tTag[code] = pPredefined->tag;
							}
						}
					}
				}
			}
		}
	}
}


static char L_ParseRef(RFCPTOOL_CPSPEC_TAB_CTX *pCtx, RFCPTOOL_UINT32 code, RFCPTOOL_UINT32 codepoint)
{
	char sIdentifier[RFCPTOOL_IDENTIFIER_LEN_MAX + 1];
	RFCPTOOL_CPSPEC_REF_RANGE range;
	RFCPTOOL_CPSPEC_REF_IDENTIFIER *pRefPending;
	RFCPTOOL_CPSPEC_REF_IDENTIFIER *pRefCurrent;
	RFCPTOOL_CPSPEC_REF *pRef;
	int refPredefinedCount;
	int fPredefined;
	int fIdentity;
	int index;
	char c;
	
	assert((codepoint == RFCPTOOL_TAG_TYPE_CODEPOINT) || (codepoint == RFCPTOOL_TAG_TYPE_MULTIBYTE) || (codepoint == RFCPTOOL_TAG_TYPE_SHIFT_OUT));
	
	sIdentifier[0] = '\0';
	refPredefinedCount = pCtx->refPredefinedCount;
	fPredefined = 0;
	fIdentity = 0;
	
	c = L_ReadFile(pCtx);
	
	if ((c == '=') && (codepoint == RFCPTOOL_TAG_TYPE_CODEPOINT))
	{
		fIdentity = 1;  /* double equals sign */
		
		c = L_ReadFile(pCtx);
	}
	
	if (c == ' ')
	{
		c = L_ReadFile(pCtx);  /* skip optional whitespace */
	}
	
	switch (c)
	{
	case '-':
		
		switch (codepoint)
		{
		case RFCPTOOL_TAG_TYPE_MULTIBYTE:
			
			codepoint = RFCPTOOL_TAG_TYPE_PREDEFINED | RFCPTOOL_TAG_MULTIBYTE_INVALID;
			break;
			
		case RFCPTOOL_TAG_TYPE_SHIFT_OUT:
			
			codepoint = RFCPTOOL_TAG_TYPE_PREDEFINED | RFCPTOOL_TAG_SHIFT_OUT_INVALID;
			break;
			
		case RFCPTOOL_TAG_TYPE_CODEPOINT:
			
			L_AppendRefPredefined(pCtx, code, RFCPTOOL_TAG_TYPE_PREDEFINED | RFCPTOOL_TAG_INVALID);  /* will never fail */
			fPredefined = 1;
		}
		
		c = L_ReadFile(pCtx);
		break;
		
	case '.':
		
		switch (codepoint)
		{
		case RFCPTOOL_TAG_TYPE_MULTIBYTE:
			
			codepoint = RFCPTOOL_TAG_TYPE_PREDEFINED | RFCPTOOL_TAG_MULTIBYTE_IGNORE;
			break;
			
		case RFCPTOOL_TAG_TYPE_SHIFT_OUT:
			
			codepoint = RFCPTOOL_TAG_TYPE_PREDEFINED | RFCPTOOL_TAG_SHIFT_OUT_IGNORE;
			break;
			
		case RFCPTOOL_TAG_TYPE_CODEPOINT:
			
			L_AppendRefPredefined(pCtx, code, RFCPTOOL_TAG_TYPE_PREDEFINED | RFCPTOOL_TAG_IGNORE);  /* will never fail */
			fPredefined = 1;
		}
		
		c = L_ReadFile(pCtx);
		break;
		
	case '/':
		
		switch (codepoint)
		{
		case RFCPTOOL_TAG_TYPE_MULTIBYTE:
			
			codepoint = RFCPTOOL_TAG_TYPE_PREDEFINED | RFCPTOOL_TAG_MULTIBYTE_IDENTITY;
			break;
			
		case RFCPTOOL_TAG_TYPE_SHIFT_OUT:
			
			codepoint = RFCPTOOL_TAG_TYPE_PREDEFINED | RFCPTOOL_TAG_SHIFT_OUT_IDENTITY;
			break;
			
		case RFCPTOOL_TAG_TYPE_CODEPOINT:
			
			L_AppendRefPredefined(pCtx, code, fIdentity ? code : 0);  /* will never fail */
			fPredefined = 1;
		}
		
		c = L_ReadFile(pCtx);
		break;
		
	case '?':
		
		if (codepoint != RFCPTOOL_TAG_TYPE_CODEPOINT)
		{
			RFCPTOOL_Error("Invalid use of \'?\'", " in codepage specification file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
			pCtx->fError = 1;
			return '\0';
		}
		
		c = L_ReadFile(pCtx);
		break;
		
	default:
		
		c = L_ParseIdentifier(pCtx, sIdentifier, c);
	}
	
	if (codepoint != RFCPTOOL_TAG_TYPE_CODEPOINT)
	{
		/* multibyte or shift-out forward reference: */
		
		if (!L_ApplyTag(pCtx, code, codepoint, ((codepoint & RFCPTOOL_TAG_MASK_TYPE) != RFCPTOOL_TAG_TYPE_PREDEFINED) ? sIdentifier : NULL))
		{
			return '\0';
		}
		
		return c;
	}
	
	/* "=" or "==" reference: */
	
	if ((refPredefinedCount > 0) && (pCtx->tRefPredefined[refPredefinedCount - 1].codeMax >= 0xFF))
	{
		pCtx->tRefPredefined[refPredefinedCount - 1].codeMax = code - 1;  /* cutoff previous predefined reference */
	}
	else if (code > 0)
	{
		/* iterate through all current references: */
		
		for (index = 0; index < pCtx->refCurrentCount; ++index)
		{
			for (pRef = pCtx->tpRefCurrent[index]->pList; pRef != NULL; pRef = pRef->pNext)
			{
				assert(pRef != NULL);
				assert(pRef->pTab != NULL);
				
				if (pRef->tab.pRange != NULL)
				{
					if (pRef->tab.pRange->codeMax > pRef->range.codeMin + ((code - 1) - pRef->range.code))
					{
						pRef->tab.pRange->codeMax = pRef->range.codeMin + ((code - 1) - pRef->range.code);  /* cutoff previous reference */
					}
				}
			}
		}
	}
	
	if (fPredefined)
	{
		return c;
	}
	
	/* iterate through all current references: */
	
	for (index = 0; index < pCtx->refCurrentCount; ++index)
	{
		pRefCurrent = pCtx->tpRefCurrent[index];
		
		assert(pRefCurrent != NULL);
		
		for (pRef = pRefCurrent->pList; pRef != NULL; pRef = pRef->pNext)
		{
			if (L_AdjustRange(&range, &pRef->range, code, 0xFF, fIdentity))
			{
				pRefPending = L_InsertRef(pCtx, (sIdentifier[0] != '\0' ? sIdentifier : pRefCurrent->sIdentifier), pRef->pTab, NULL);
				
				if (pRefPending == NULL)
				{
					return '\0';
				}
				
				assert(pRefPending->pList != NULL);
				
				pRefPending->pList->range = range;
				
				pRef->tab.pRange = &pRefPending->pList->range;  /* remember latest reference */
			}
		}
	}
	
	return c;
}


static char L_ParseBackwardRef(RFCPTOOL_CPSPEC_TAB_CTX *pCtx, RFCPTOOL_UINT32 code, char c)
{
	char sIdentifier[RFCPTOOL_IDENTIFIER_LEN_MAX + 1];
	int index;
	
	if (c == ' ')
	{
		c = L_ReadFile(pCtx);  /* skip optional whitespace */
	}
	
	if (!RFCPTOOL_IsAlpha(c))
	{
		if (!pCtx->fError)
		{
			RFCPTOOL_Error("Backward shift reference expected", " in codepage specification file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
			pCtx->fError = 1;
		}
		
		return '\0';
	}
	
	c = L_ParseIdentifier(pCtx, sIdentifier, c);
	
	if (pCtx->fError)
	{
		return '\0';
	}
	
	index = pCtx->count;
	
	assert(index > 0);
	
	while (RFCPTOOL_StrCmp(pCtx->tpTab[--index]->sIdentifier, sIdentifier, RFCPTOOL_IDENTIFIER_LEN_MAX + 1))
	{
		if (!index)
		{
			RFCPTOOL_Error("Backward shift reference not found", " in codepage specification file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
			pCtx->fError = 1;
			return '\0';
		}
	}
	
	L_ApplyTag(pCtx, code, RFCPTOOL_TAG_TYPE_SHIFT_OUT | (RFCPTOOL_UINT32)index, NULL);  /* will never fail */
	
	return c;
}


static int L_ParseBlock(RFCPTOOL_CPSPEC_TAB_CTX *pCtx)
{
	RFCPTOOL_UINT32 minMappingRefOffset;
	RFCPTOOL_UINT32 code;
	RFCPTOOL_UINT32 tag;
	int range;
	char c;
	
	c = L_ReadFile(pCtx);
	
	if (c == ' ')
	{
		c = L_ReadFile(pCtx);  /* skip optional whitespace */
	}
	
	minMappingRefOffset = 0;
	code = 0;
	tag = RFCPTOOL_TAG_TYPE_INVALID;  /* marker for initial state */
	
	do
	{
		assert((tag == RFCPTOOL_TAG_TYPE_INVALID) || (tag == RFCPTOOL_TAG_TYPE_UNSPECIFIED));
		
		if (RFCPTOOL_IsAlNum(c) || (c == '('))
		{
			c = L_ParseValue(pCtx, &tag, &range, (tag == RFCPTOOL_TAG_TYPE_UNSPECIFIED) ? 0 : 1, c);
			
			if (!range)
			{
				/* tag contains new offset (i.e. current code): */
				
				code = tag;
				
				c = L_ReadFile(pCtx);
				
				if (c == ' ')
				{
					c = L_ReadFile(pCtx);  /* skip optional whitespace */
				}
				
				tag = RFCPTOOL_TAG_TYPE_UNSPECIFIED;  /* marker to identify consecutive offsets */
				
				continue;
			}
			
			if (c == ' ')
			{
				c = L_ReadFile(pCtx);  /* skip optional whitespace */
			}
			
			while (--range && (code <= 0xFF))
			{
				L_ApplyTag(pCtx, code++, tag++, NULL);  /* will never fail */
			}
		}
		
		if (code > 0xFF)
		{
			if (!pCtx->fError)
			{
				RFCPTOOL_Error("Item beyond boundary", " in codepage specification file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
				pCtx->fError = 1;
			}
			
			return 0;
		}
		
		if ((tag != RFCPTOOL_TAG_TYPE_INVALID) && (tag != RFCPTOOL_TAG_TYPE_UNSPECIFIED))
		{
			L_ApplyTag(pCtx, code++, tag, NULL);  /* will never fail */
		}
		else
		{
			switch (c)
			{
			case '=':
				
				if (code < minMappingRefOffset)
				{
					RFCPTOOL_Error("Non-consecutive mapping references", " in codepage specification file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
					pCtx->fError = 1;
					return 0;
				}
				
				minMappingRefOffset = code + 1;
				
				c = L_ParseRef(pCtx, code, RFCPTOOL_TAG_TYPE_CODEPOINT);
				break;
				
			case '*':
				
				c = L_ParseRef(pCtx, code++, RFCPTOOL_TAG_TYPE_MULTIBYTE);
				break;
				
			case '>':
				
				c = L_ParseRef(pCtx, code++, RFCPTOOL_TAG_TYPE_SHIFT_OUT);
				break;
				
			case '<':
				
				c = L_ReadFile(pCtx);
				
				if (c == '<')
				{
					c = L_ReadFile(pCtx);
					
					L_ApplyTag(pCtx, code++, RFCPTOOL_TAG_TYPE_PREDEFINED | RFCPTOOL_TAG_SHIFT_IN, NULL);  /* will never fail */
				}
				else
				{
					c = L_ParseBackwardRef(pCtx, code++, c);
				}
				break;
				
			case '/':
				
				c = L_ReadFile(pCtx);
				
				L_ApplyTag(pCtx, code, code, NULL);  /* will never fail */
				break;
				
			case ',':
				
				c = L_ReadFile(pCtx);
				
				++code;
				break;
				
			case '.':
				
				c = L_ReadFile(pCtx);
				
				L_ApplyTag(pCtx, code++, RFCPTOOL_TAG_TYPE_PREDEFINED | RFCPTOOL_TAG_IGNORE, NULL);  /* will never fail */
				break;
				
			case '-':
				
				c = L_ReadFile(pCtx);
				
				L_ApplyTag(pCtx, code++, RFCPTOOL_TAG_TYPE_PREDEFINED | RFCPTOOL_TAG_INVALID, NULL);  /* will never fail */
				break;
				
			default:
				
				if (!pCtx->fError)
				{
					RFCPTOOL_Error("Invalid symbol", " in codepage specification file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
					pCtx->fError = 1;
				}
				
				return 0;
			}
			
			if (c != ')')
			{
				if (c != ' ')
				{
					if (!pCtx->fError)
					{
						RFCPTOOL_Error("Whitespace or \')\' required", " in codepage specification file ", pCtx->pSrcFile->ref.sPath, NULL, pCtx->line);
						pCtx->fError = 1;
					}
					
					return 0;
				}
				
				c = L_ReadFile(pCtx);
			}
		}
		
		tag = RFCPTOOL_TAG_TYPE_INVALID;  /* marker for initial state */
	}
	while (c != ')');
	
	/* apply predefined mapping references (if given): */
	
	if (pCtx->refPredefinedCount > 0)
	{
		L_ApplyPredefined(pCtx);
		pCtx->refPredefinedCount = 0;
	}
	
	while (pCtx->refCurrentCount > 0)
	{
		L_DestroyRefIdentifier(pCtx->tpRefCurrent[--pCtx->refCurrentCount]);
	}
	
	return 1;
}


static int L_PrepareBlock(RFCPTOOL_CPSPEC_TAB_CTX *pCtx)
{
	RFCPTOOL_CPSPEC_REF *pRef;
	RFCPTOOL_CP_TAB *pTab;
	RFCPTOOL_UINT32 code;
	int index;
	
	assert(pCtx != NULL);
	
	pTab = NULL;
	
	/* if non-mapping references are given, create a new codepoint table and set its index on each pTag: */
	
	for (index = 0; index < pCtx->refCurrentCount; ++index)
	{
		for (pRef = pCtx->tpRefCurrent[index]->pList; pRef != NULL; pRef = pRef->pNext)
		{
			assert(pRef != NULL);
			
			if (pRef->pTab == NULL)
			{
				if (pTab == NULL)
				{
					/* create and initialize a new codepage table: */
					
					pTab = RFCPTOOL_MemAlloc(sizeof (RFCPTOOL_CP_TAB));
					
					if (pTab == NULL)
					{
						pCtx->fError = 1;
						return 0;
					}
					
					RFCPTOOL_StrCopy(pTab->sIdentifier, pCtx->sIdentifier, RFCPTOOL_IDENTIFIER_LEN_MAX + 1);  /* remember shift-out backward identifier (if given) */
					
					for (code = 0; code < 256; ++code)
					{
						pTab->tTag[code] = RFCPTOOL_TAG_TYPE_UNSPECIFIED;
					}
					
					pCtx->tpTab[pCtx->count++] = pTab;
				}
				
				assert(pTab != NULL);
				
				if (pRef->tab.pTag != NULL)
				{
					*pRef->tab.pTag |= (RFCPTOOL_UINT32)(pCtx->count - 1);  /* apply codepoint table index which is known now */
				}
				
				/* the reference now becomes a mapping reference: */
				
				pRef->pTab = pTab;
				pRef->tab.pRange = NULL;
			}
		}
	}
	
	return 1;
}


static int L_Load(RFCPTOOL_CPSPEC_TAB_CTX *pCtx)
{
	int fMatch;
	int result;
	
	while (pCtx->refPendingCount > 0)
	{
		if (!L_OpenFile(pCtx))
		{
			return 0;
		}
		
		fMatch = 0;
		
		result = L_ParseIdentifierSequence(pCtx);
		
		while (result > 0)
		{
			if (pCtx->refCurrentCount > 0)
			{
				fMatch = 1;
				
				result = L_PrepareBlock(pCtx);
				
				if (result)
				{
					result = L_ParseBlock(pCtx);
				}
			}
			else
			{
				result = L_SkipBlock(pCtx);
			}
			
			if (result)
			{
				result = L_ParseIdentifierSequence(pCtx);
			}
		}
		
		if (!result)
		{
			RFCPTOOL_FileDestroy(&pCtx->pSrcFile->ref);
			pCtx->pSrcFile = NULL;
			return 0;
		}
		
		RFCPTOOL_FileClose(&pCtx->pSrcFile->ref);
		RFCPTOOL_FileDestroy(&pCtx->pSrcFile->ref);
		pCtx->pSrcFile = NULL;
		
		if (!fMatch)
		{
			RFCPTOOL_Error("No matching codepages found", " in codepage specification file ", pCtx->pDir->sPath, NULL, 0);
			return 0;
		}
	}
	
	return 1;
}


/*****************************************************************************************
 *
 *  E X T E R N A L   F U N C T I O N S
 *
 *****************************************************************************************/


int RFCPTOOL_CPSpec(const char *sRef, const char *sDestPath, RFCPTOOL_DIR *pDirList, int fOverwrite)
{
	char sPath[RFCPTOOL_DOMAIN_NAME_LEN_MAX + 5];
	RFCPTOOL_CPSPEC_TAB_CTX ctx;
	RFCPTOOL_DIR dir;
	size_t pos;
	int fLeadingZeros;
	int index;
	
	assert(sRef != NULL);
	
	/* normalize and validate input: */
	
	pos = L_IsValidName(sRef, RFCPTOOL_DOMAIN_NAME_LEN_MAX, ':');
	
	if (!pos)
	{
		RFCPTOOL_Error("Valid codepage domain expected", " in codepage reference ", sRef, NULL, 0);
		return 0;
	}
	
	RFCPTOOL_StrCopy(ctx.sDomain, sRef, pos);
	ctx.sDomain[pos++] = '\0';
	
	/* skip leading zeros: */
	
	fLeadingZeros = 0;
	
	while (sRef[pos] == '0')
	{
		fLeadingZeros = 1;
		++pos;
	}
	
	if (!L_IsValidIdentifier(&sRef[pos], fLeadingZeros))
	{
		RFCPTOOL_Error("Valid codepage identifier expected", " in codepage reference ", sRef, NULL, 0);
		return 0;
	}
	
	/* initialize context: */
	
	sPath[0] = '\0';  /* add current directory to directory list */
	
	dir.pNext = pDirList;
	dir.sPath = sPath;
	dir.sFileNameBuf = sPath;
	
	ctx.count = 0;
	ctx.hSequenceTab = RFCPTOOL_CreateSequenceTab();
	ctx.refPredefinedCount = 0;
	ctx.refPendingCount = 0;
	ctx.refCurrentCount = 0;
	ctx.refIdentifierCount = 0;
	ctx.pDirList = &dir;
	ctx.pDir = NULL;
	ctx.pSrcFile = NULL;
	ctx.line = 0;
	ctx.offset = 0;
	ctx.fError = 0;
	
	if (ctx.hSequenceTab == NULL)
	{
		return 0;
	}
	
	/* insert initial reference and load codepage file: */
	
	if (L_InsertRef(&ctx, &sRef[pos], NULL, NULL) == NULL)
	{
		L_ClearCtx(&ctx);
		return 0;
	}
	
	if (!L_Load(&ctx))
	{
		L_ClearCtx(&ctx);
		return 0;
	}
	
	for (index = 0; index < ctx.count; ++index)
	{
		L_ResolveUnspecified(ctx.tpTab[index]->tTag);
	}
	
	/* write codepage: */
	
	if (!RFCPTOOL_Compile(sDestPath, fOverwrite, ctx.tpTab, ctx.count, ctx.hSequenceTab))
	{
		L_ClearCtx(&ctx);
		return 0;
	}
	
	L_ClearCtx(&ctx);
	
	return 1;
}
