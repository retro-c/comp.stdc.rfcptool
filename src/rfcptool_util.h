	
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
 *  File:          /src/rfcptool_util.h//
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


#ifndef RFCPTOOL_UTIL_H
#define RFCPTOOL_UTIL_H


#include "rfcptool_def.h"


#define RFCPTOOL_IS_STR_FLAG_IGNORE_CASE  0x0001  /* ignore case on string compare */
#define RFCPTOOL_IS_STR_FLAG_PREFIX       0x0002  /* compare string is prefix of (compared string does not need to be terminated) */


typedef struct RFCPTOOL_DIR {
	struct RFCPTOOL_DIR *pNext;
	const char *sPath;
	char *sFileNameBuf;  /* maximum character count (including '\0') is RFCPTOOL_DOMAIN_NAME_LEN_MAX + 5 */
} RFCPTOOL_DIR;


void RFCPTOOL_Error(const char *sMsgPrefix, const char *sPathPrefix, const char *sPath, const char *sMsgSuffix, int line);    /* sPath and sMsgSuffix may be NULL; sPathPrefix is ignored if sPath is NULL; line may be 0 */
void RFCPTOOL_Warning(const char *sMsgPrefix, const char *sPathPrefix, const char *sPath, const char *sMsgSuffix, int line);  /* sPath and sMsgSuffix may be NULL; sPathPrefix is ignored if sPath is NULL; line may be 0 */

void* RFCPTOOL_MemAlloc(size_t size);
void RFCPTOOL_MemFree(void *pPtr);

size_t RFCPTOOL_EncodePCS(RFCPTOOL_UINT32 codepoint, RFCPTOOL_UINT8 tBuf[]);

char RFCPTOOL_ToUpper(int c);

int RFCPTOOL_IsUpper(int c);
int RFCPTOOL_IsAlpha(int c);
int RFCPTOOL_IsDigit(int c);
int RFCPTOOL_IsAlNum(int c);

int RFCPTOOL_IsStr(const char *sStr1, const char *sStr2, RFCPTOOL_UINT16 flags);
int RFCPTOOL_StrCmp(const char *sStr1, const char *sStr2, size_t lenMax);
size_t RFCPTOOL_StrCopy(char *sDest, const char *sSrc, size_t countMax);  /* WARNING: sDest will be terminated by '\0' only if strlen(sSrc) < countMax */

char* RFCPTOOL_StrDup(const char *sStr);

void* RFCPTOOL_CreateStrTab(int countMax);
void RFCPTOOL_DestroyStrTab(void *hStrTab);
int RFCPTOOL_GetStrTabCount(void *hStrTab);
int RFCPTOOL_InsertStrTab(void *hStrTab, const char *sStr);
int RFCPTOOL_LookupStrTab(void *hStrTab, const char *sStr);  /* returns 1-based index */

void* RFCPTOOL_CreateSequenceTab(void);
void RFCPTOOL_DestroySequenceTab(void *hSequenceTab);
RFCPTOOL_UINT32 RFCPTOOL_AddSequenceTab(void *hSequenceTab, const RFCPTOOL_UINT32 *sSequence, size_t len);  /* returns 1-based index; if len is 0, sSequence must be terminated by RFCPTOOL_CODEPOINT_TYPE_INVALID */
const RFCPTOOL_UINT32* RFCPTOOL_LookupSequenceTab(const void *hSequenceTab, RFCPTOOL_UINT32 index);

RFCPTOOL_DIR* RFCPTOOL_CreateDefaultDir(void);       /* may return NULL (shall not be considered an error) */
RFCPTOOL_DIR* RFCPTOOL_CreateDir(const char *sDir);
void RFCPTOOL_DestroyDirList(RFCPTOOL_DIR **ppList);


#endif
