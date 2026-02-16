
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
 *  File:          /src/rfcptool_def.h//
 *  Version:       01.00!00
 *  Environments:  C90 [ C99 ]
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


#ifndef RFCPTOOL_DEF_H
#define RFCPTOOL_DEF_H

#if __STDC_VERSION__ >= 199901
#include <stdint.h>
#endif


#define RFCPTOOL_DOMAIN_NAME_LEN_MAX      8
#define RFCPTOOL_TARGET_LEN_MAX           6
#define RFCPTOOL_IDENTIFIER_LEN_MAX       39

#define RFCPTOOL_SEQUENCE_LEN_MIN         1
#define RFCPTOOL_SEQUENCE_LEN_MAX         16

#define RFCPTOOL_CODEPOINT_NUL            0x0000  /* character literal '\0' (null) */
#define RFCPTOOL_CODEPOINT_BEL            0x0007  /* character literal '\a' (bell) */
#define RFCPTOOL_CODEPOINT_BS             0x0008  /* character literal '\b' (backspace) */
#define RFCPTOOL_CODEPOINT_HT             0x0009  /* character literal '\t' (horizontal tab) */
#define RFCPTOOL_CODEPOINT_LF             0x000A  /* character literal '\n' (line feed) */
#define RFCPTOOL_CODEPOINT_VT             0x000B  /* character literal '\v' (vertical tab) */
#define RFCPTOOL_CODEPOINT_FF             0x000C  /* character literal '\f' (form feed) */
#define RFCPTOOL_CODEPOINT_CR             0x000D  /* character literal '\r' (carriage return) */

#define RFCPTOOL_CODEPOINT_MAX            0x00126FC1
#define RFCPTOOL_CODEPOINT_EOF            0xFFFFFFFE
#define RFCPTOOL_CODEPOINT_ERROR          0xFFFFFFFF

#define RFCPTOOL_TAG_INVALID              0x00000000
#define RFCPTOOL_TAG_IGNORE               0x00000001
#define RFCPTOOL_TAG_SHIFT_IN             0x00000002
#define RFCPTOOL_TAG_MULTIBYTE_INVALID    0x00000003
#define RFCPTOOL_TAG_MULTIBYTE_IGNORE     0x00000004
#define RFCPTOOL_TAG_MULTIBYTE_IDENTITY   0x00000005
#define RFCPTOOL_TAG_SHIFT_OUT_INVALID    0x00000006
#define RFCPTOOL_TAG_SHIFT_OUT_IGNORE     0x00000007
#define RFCPTOOL_TAG_SHIFT_OUT_IDENTITY   0x00000008

#define RFCPTOOL_TAG_MASK_VALUE           0x1FFFFFFF
#define RFCPTOOL_TAG_TYPE_CODEPOINT       0x00000000  /* value is codepoint in the range 0x00000000..0x00126FC1 */
#define RFCPTOOL_TAG_TYPE_SEQUENCE        0x20000000  /* value is index of codepoint sequence table */
#define RFCPTOOL_TAG_TYPE_SEQUENCE_INV    0x40000000  /* value is index of invertible codepoint sequence table */
#define RFCPTOOL_TAG_TYPE_MULTIBYTE       0x60000000  /* value is index of multibyte codepoint table */
#define RFCPTOOL_TAG_TYPE_SHIFT_OUT       0x80000000  /* value is index of shift out codepoint table */
#define RFCPTOOL_TAG_TYPE_PREDEFINED      0xA0000000  /* value is RFCPTOOL_TAG_* (see above) */
#define RFCPTOOL_TAG_TYPE_UNSPECIFIED     0xC0000000  /* value must be zero */
#define RFCPTOOL_TAG_TYPE_INVALID         0xE0000000  /* value must be zero */
#define RFCPTOOL_TAG_MASK_TYPE            0xE0000000


#if __STDC_VERSION__ >= 199901

typedef uint8_t RFCPTOOL_UINT8;
typedef uint16_t RFCPTOOL_UINT16;
typedef uint32_t RFCPTOOL_UINT32;

#else

typedef unsigned char RFCPTOOL_UINT8;
typedef unsigned short RFCPTOOL_UINT16;
typedef unsigned long RFCPTOOL_UINT32;

#endif


typedef struct RFCPTOOL_CP_TAB {
	char sIdentifier[RFCPTOOL_IDENTIFIER_LEN_MAX + 1];  /* shift-out backward identifier (always starts with a letter, may be empty) */
	RFCPTOOL_UINT32 tTag[256];                          /* see RFCPTOOL_TAG_* */
} RFCPTOOL_CP_TAB;


#endif
