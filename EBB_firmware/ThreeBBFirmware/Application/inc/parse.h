/*********************************************************************
 *
 *                3BB Firmware
 *
 *********************************************************************
 * FileName:        parse.h
 * Company:         Schmalz Haus LLC
 * Author:          Brian Schmalz
 *
 * Software License Agreement
 *
 * Copyright (c) 2020-2021, Brian Schmalz of Schmalz Haus LLC
 * All rights reserved.
 * Based on EiBotBoard (EBB) Firmware, written by Brian Schmalz of
 *   Schmalz Haus LLC
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * 1. Redistributions of source code must retain the above
 * copyright notice, this list of conditions and the following
 * disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials
 * provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of
 * its contributors may be used to endorse or promote products
 * derived from this software without specific prior written
 * permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __PARSE_H__
#define __PARSE_H__

#include <stdbool.h>
#include <stdint.h>

#define advance_RX_buf_out()              \
{                                         \
  g_RX_buf_out++;                         \
  if (kRX_BUF_SIZE == g_RX_buf_out)       \
  {                                       \
    g_RX_buf_out = 0;                     \
  }                                       \
}

// defines for the error_byte byte - each bit has a meaning
#define kERROR_BYTE_STEPS_TO_FAST           1 // If you ask us to step more than 25 steps/ms
#define kERROR_BYTE_TX_BUF_OVERRUN          2
#define kERROR_BYTE_RX_BUFFER_OVERRUN       3
#define kERROR_BYTE_MISSING_PARAMETER       4
#define kERROR_BYTE_PRINTED_ERROR           5 // We've already printed out an error
#define kERROR_BYTE_PARAMETER_OUTSIDE_LIMIT 6
#define kERROR_BYTE_EXTRA_CHARACTERS        7
#define kERROR_BYTE_UNKNOWN_COMMAND         8 // Part of command parser, not error handler

#define kREQUIRED false
#define kOPTIONAL true

// Enum for extract_num() function parameter
// NOTE: C18 supports UINT24 type, but not INT24, so it does not appear here
typedef enum {
  kINT8,           // One byte, signed
  kUINT8,          // One byte, unsigned
  kHEX8,           // One byte (two characters) of hex
  kINT16,          // Two bytes, signed
  kUINT16,         // Two bytes, unsigned
  kHEX16,          // Two bytes (four characters) of hex
  kUINT24,         // Three bytes, unsigned
  kHEX24,          // Three bytes (six characters) of hex
  kINT32,          // Four bytes, signed
  kUINT32,         // Four bytes, unsigned
  kHEX32,          // Four bytes (eight characters) of hex
  kASCII_CHAR,     // ASCII character, read in as byte
  kUCASE_ASCII_CHAR  // ASCII character, must be uppercase
} ExtractType;

typedef enum {
  kEXTRACT_OK = 0
 ,kEXTRACT_PARAMETER_OUTSIDE_LIMIT
 ,kEXTRACT_COMMA_MISSING
 ,kEXTRACT_MISSING_PARAMETER
 ,kEXTRACT_INVALID_TYPE
} ExtractReturnType;

extern uint8_t error_byte;
extern bool g_ack_enable;


ExtractReturnType extract_number(ExtractType Type, void * ReturnValue, unsigned char Required);
uint8_t extract_string(unsigned char * ReturnValue, uint8_t MaxBytes);
void print_ack(void);
void parsePacket(void);

#endif	/* PARSE_H */

