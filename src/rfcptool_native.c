
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
 *  File:          /src/rfcptool_native.c//
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

#include <stdio.h>
#include "rfcptool_def.h"
#include "rfcptool_compile.h"
#include "rfcptool_native.h"


/*****************************************************************************************
 *
 *  I N T E R N A L   F U N C T I O N S
 *
 *****************************************************************************************/


static void L_InitTab(RFCPTOOL_UINT32 tCodepoint[256])
{
	RFCPTOOL_UINT32 tag;
	int c;
	
	for (c = 0; c < 256; ++c)
	{
		tCodepoint[c] = RFCPTOOL_TAG_TYPE_PREDEFINED | RFCPTOOL_TAG_INVALID;
	}
	
	tCodepoint['\0'] = RFCPTOOL_CODEPOINT_NUL;
	tCodepoint['\a'] = RFCPTOOL_CODEPOINT_BEL;
	tCodepoint['\b'] = RFCPTOOL_CODEPOINT_BS;
	tCodepoint['\t'] = RFCPTOOL_CODEPOINT_HT;
	tCodepoint['\n'] = RFCPTOOL_CODEPOINT_LF;
	tCodepoint['\v'] = RFCPTOOL_CODEPOINT_VT;
	tCodepoint['\f'] = RFCPTOOL_CODEPOINT_FF;
	tCodepoint['\r'] = RFCPTOOL_CODEPOINT_CR;
	
	tag = 0x0020;
	
	tCodepoint[' '] = tag++;
	tCodepoint['!'] = tag++;
	tCodepoint['\"'] = tag++;
	tCodepoint['#'] = tag++;
	tCodepoint['$'] = tag++;
	tCodepoint['%'] = tag++;
	tCodepoint['&'] = tag++;
	tCodepoint['\''] = tag++;
	tCodepoint['('] = tag++;
	tCodepoint[')'] = tag++;
	tCodepoint['*'] = tag++;
	tCodepoint['+'] = tag++;
	tCodepoint[','] = tag++;
	tCodepoint['-'] = tag++;
	tCodepoint['.'] = tag++;
	tCodepoint['/'] = tag++;
	
	for (c = '0'; c <= '9'; ++c)
	{
		tCodepoint[c] = tag++;
	}
	
	tCodepoint[':'] = tag++;
	tCodepoint[';'] = tag++;
	tCodepoint['<'] = tag++;
	tCodepoint['='] = tag++;
	tCodepoint['>'] = tag++;
	tCodepoint['?'] = tag++;
	
	tag = 0x0041;
	
	for (c = 'A'; c <= 'I'; ++c)
	{
		tCodepoint[c] = tag++;
	}
	
	for (c = 'J'; c <= 'R'; ++c)
	{
		tCodepoint[c] = tag++;
	}
	
	for (c = 'S'; c <= 'Z'; ++c)
	{
		tCodepoint[c] = tag++;
	}
	
	tCodepoint['['] = tag++;
	tCodepoint['\\'] = tag++;
	tCodepoint[']'] = tag++;
	tCodepoint['^'] = tag++;
	tCodepoint['_'] = tag++;
	
	tag = 0x0061;
	
	for (c = 'a'; c <= 'i'; ++c)
	{
		tCodepoint[c] = tag++;
	}
	
	for (c = 'j'; c <= 'r'; ++c)
	{
		tCodepoint[c] = tag++;
	}
	
	for (c = 's'; c <= 'z'; ++c)
	{
		tCodepoint[c] = tag++;
	}
	
	tCodepoint['{'] = tag++;
	tCodepoint['|'] = tag++;
	tCodepoint['}'] = tag++;
	tCodepoint['~'] = tag++;
}


/*****************************************************************************************
 *
 *  E X T E R N A L   F U N C T I O N S
 *
 *****************************************************************************************/


int RFCPTOOL_Native(const char *sDestPath, int fOverwrite)
{
	RFCPTOOL_CP_TAB tab;
	RFCPTOOL_CP_TAB *pTab;
	
	tab.sIdentifier[0] = '\0';
	
	L_InitTab(tab.tTag);
	
	pTab = &tab;
	
	if (!RFCPTOOL_Compile(sDestPath, fOverwrite, &pTab, 1, NULL))
	{
		return 0;
	}
	
	return 1;
}
