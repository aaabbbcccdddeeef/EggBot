
#include "parse.h"
#include "utility.h"
#include <ctype.h>
//#include "stepper.h"
#include <stdio.h>
#include "commands.h"
//#include "servo.h"
//#include "ebb.h"
//#include "analog.h"
//#include "serial.h"
#include <stdbool.h>
#include "usbd_cdc_if.h"

// This byte has each of its bits used as a separate error flag
uint8_t error_byte;

// Normally set to TRUE. Able to set FALSE to not send "OK" message after packet reception
bool g_ack_enable;

static bool extractHexDigits(uint32_t * acc, uint8_t digits);
static bool extractDecDigits(uint32_t * acc, uint8_t digits);

typedef struct {
  uint8_t c1;
  uint8_t c2;
  void (*func)(void);
} const parse_t;

const parse_t commandTable[] =
{
//  {'L', 'M', parseLMCommand},
//  {'R', 'S', parseRSCommand},
//  {'C', 'B', parseCBCommand},
//  {'C', 'U', parseCUCommand},
//  {'O', 'D', parseODCommand},
//  {'I', 'D', parseIDCommand},
  {'V', 'R', parseVRCommand},
//  {'A', 'R', parseARCommand},
//  {'P', 'I', parsePICommand},
//  {'P', 'O', parsePOCommand},
//  {'P', 'D', parsePDCommand},
//  {'M', 'R', parseMRCommand},
//  {'M', 'W', parseMWCommand},
//  {'P', 'C', parsePCCommand},
//  {'P', 'G', parsePGCommand},
//  {'S', 'M', parseSMCommand},
//  {'A', 'M', parseAMCommand},
//  {'S', 'P', parseSPCommand},
//  {'T', 'P', parseTPCommand},
//  {'Q', 'P', parseQPCommand},
//  {'E', 'M', parseEMCommand},
//  {'S', 'C', parseSCCommand},
//  {'S', 'N', parseSNCommand},
//  {'Q', 'N', parseQNCommand},
//  {'S', 'L', parseSLCommand},
//  {'Q', 'L', parseQLCommand},
//  {'Q', 'B', parseQBCommand},
//  {'N', 'I', parseNICommand},
//  {'N', 'D', parseNDCommand},
//  {'B', 'L', parseBLCommand},
//  {'T', '1', parseT1Command},
//  {'T', '2', parseT2Command},
//  {'Q', 'C', parseQCCommand},
//  {'Q', 'G', parseQGCommand},
//  {'S', 'E', parseSECommand},
//  {'S', '2', parseS2Command},
//  {'R', 'M', parseRMCommand},
//  {'Q', 'M', parseQMCommand},
//  {'A', 'C', parseACCommand},
//  {'E', 'S', parseESCommand},
//  {'X', 'M', parseXMCommand},
//  {'Q', 'S', parseQSCommand},
//  {'C', 'S', parseCSCommand},
//  {'S', 'T', ParseSTCommand},
//  {'Q', 'T', ParseQTCommand},
//  {'R', 'B', parseRBCommand},
//  {'S', 'S', ParseSSCommand},
//#if defined(BOARD_EBB)
//  {'Q', 'R', parseQRCommand},
//  {'S', 'R', parseSRCommand},
//#endif
//  {'H', 'M', parseHMCommand},
//  {'D', 'R', ParseDRCommand},
//  {'D', 'W', ParseDWCommand},
#if defined(DEBUG)
//  {'S', 'H', ParseSHCommand},
#endif
  {0x00, 0x00, NULL},             // Table terminator. Must have c1=0x00
};


/*
 *  Look at the new packet, see what command it is, and 
 * route it appropriately. We come in knowing that
 * our packet is in g_RX_buf[], and that the beginning
 * of the packet is at g_RX_buf_out, and the end (CR) is at
 * g_RX_buf_in. To make parsing simpler, ALL commands are exactly two characters
 * long.
 */
void parsePacket(void)
{
  uint16_t command;
  uint16_t testCommand;
  uint8_t i;

  // Always grab the first character (which is the first byte of the command)
  command = toupper(g_RX_buf[g_RX_buf_out]);
  advance_RX_buf_out();
  command = (command << 8) | toupper (g_RX_buf[g_RX_buf_out]);
  advance_RX_buf_out();

  // Now 'command' is equal to two bytes of our command
  i = 0;
  while (commandTable[i].c1 != 0x00)
  {
    testCommand = ((uint16_t)commandTable[i].c1 << 8) | commandTable[i].c2;
    if (command == testCommand)
    {
      commandTable[i].func();
      break;
    }
    i++;
  }
  if (commandTable[i].c1 == 0x00)
  {
    // Send back 'unknown command' error
    printf (
       "!8 Err: Unknown command '%c%c:%4X'\r\n"
      ,(uint8_t)(command >> 8)
      ,(uint8_t)command
      ,command
    );
  }

  // Double check that our output pointer is now at the ending <CR>
  // If it is not, this indicates that there were extra characters that
  // the command parsing routine didn't eat. This would be an error and needs
  // to be reported. (Ignore for Reset command because FIFO pointers get cleared.)
  if (
    (g_RX_buf[g_RX_buf_out] != kCR && 0 == error_byte)
    &&
    ('R' != command)
  )
  {
    bitset (error_byte, kERROR_BYTE_EXTRA_CHARACTERS);
  }

  // Clean up by skipping over any bytes we haven't eaten
  // This is safe since we parse each packet as we get a <CR>
  // (i.e. g_RX_buf_in doesn't move while we are in this routine)
  g_RX_buf_out = g_RX_buf_in;
}

// Print out the positive acknowledgment that the packet was received
// if we have acks turned on.
void print_ack(void)
{
  if (g_ack_enable)
  {
    printf(st_OK);
  }
}


// Look at the string in g_RX_buf[]
// Copy over all bytes from g_RX_buf_out into ReturnValue until you hit
// a comma or a CR or you've copied over MaxBytes characters. 
// Return the number of bytes copied. Advance g_RX_buf_out as you go.
uint8_t extract_string (
  unsigned char * ReturnValue, 
  uint8_t MaxBytes
)
{
  uint8_t bytes = 0;

  // Always terminate the string
  *ReturnValue = 0x00;

  // Check to see if we're already at the end
  if (kCR == g_RX_buf[g_RX_buf_out])
  {
    bitset (error_byte, kERROR_BYTE_MISSING_PARAMETER);
    return (0);
  }

  // Check for comma where ptr points
  if (g_RX_buf[g_RX_buf_out] != ',')
  {
    printf ("!5 Err: Need comma next, found: '%c'\r\n", g_RX_buf[g_RX_buf_out]);
    bitset (error_byte, kERROR_BYTE_PRINTED_ERROR);
    return (0);
  }

  // Move to the next character
  advance_RX_buf_out();

  while(1)
  {
    // Check to see if we're already at the end
    if (kCR == g_RX_buf[g_RX_buf_out] || ',' == g_RX_buf[g_RX_buf_out] || bytes >= MaxBytes)
    {
      return (bytes);
    }

    // Copy over a byte
    *ReturnValue = g_RX_buf[g_RX_buf_out];

    // Move to the next character
    advance_RX_buf_out();

    // Count this one
    bytes++;
    ReturnValue++;
  }

  return(bytes);
}


// Look at the string pointed to by ptr
// There should be a comma where ptr points to upon entry.
// If not, throw a comma error.
// If so, then look for up to like a ton of bytes after the
// comma for numbers, and put them all into one
// unsigned long accumulator. 
// Advance the pointer to the byte after the last number
// and return.
ExtractReturnType extract_number(
  ExtractType Type, 
  void * ReturnValue, 
  uint8_t Required
)
{
  uint32_t ULAccumulator;
  int32_t Accumulator = 0;
  bool Negative = false;

  // Check to see if we're already at the end
  if (kCR == g_RX_buf[g_RX_buf_out])
  {
    if (0 == Required)
    {
      bitset (error_byte, kERROR_BYTE_MISSING_PARAMETER);
    }
    return (kEXTRACT_MISSING_PARAMETER);
  }

  // Check for comma where ptr points
  if (g_RX_buf[g_RX_buf_out] != ',')
  {
    if (0 == Required)
    {
      printf ("!5 Err: Need comma next, found: '%c'\r\n", g_RX_buf[g_RX_buf_out]);
      bitset (error_byte, kERROR_BYTE_PRINTED_ERROR);
    }
    return (kEXTRACT_COMMA_MISSING);
  }

  // Move to the next character
  advance_RX_buf_out();

  // Check for end of command
  if (kCR == g_RX_buf[g_RX_buf_out])
  {
    if (0 == Required)
    {
      bitset (error_byte, kERROR_BYTE_MISSING_PARAMETER);
    }
    return (kEXTRACT_MISSING_PARAMETER);
  }

  // Now check for a sign character if we're not looking for ASCII chars
  if (
    ('-' == g_RX_buf[g_RX_buf_out]) 
    && 
    (
      (kASCII_CHAR != Type)
      &&
      (kUCASE_ASCII_CHAR != Type)
    )
  )
  {
    // It's an error if we see a negative sign on an unsigned value
    if (
      (kUINT8 == Type)
      ||
      (kUINT16 == Type)
      ||
      (kUINT32 == Type)
      ||
      (kHEX32 == Type)
    )
    {
      bitset (error_byte, kERROR_BYTE_PARAMETER_OUTSIDE_LIMIT);
      return (kEXTRACT_PARAMETER_OUTSIDE_LIMIT);
    }
    else
    {
      Negative = true;
      // Move to the next character
      advance_RX_buf_out();
    }
  }

  // Read in the digits of the number
  switch(Type)
  {
    case kINT8:
    case kUINT8:
      extractDecDigits(&ULAccumulator, 3);
      break;
      
    case kINT16:
    case kUINT16:
      extractDecDigits(&ULAccumulator, 5);
      break;
    
    case kUINT24:
      extractDecDigits(&ULAccumulator, 8);
      break;

    case kINT32:
    case kUINT32:
      extractDecDigits(&ULAccumulator, 10);
      break;
    
    case kHEX8:
      extractHexDigits(&ULAccumulator, 2);
      break;
      
    case kHEX16:
      extractHexDigits(&ULAccumulator, 4);
      break;

    case kHEX24:
      extractHexDigits(&ULAccumulator, 6);
      break;

    case kHEX32:
      extractHexDigits(&ULAccumulator, 8);
      break;

    case kASCII_CHAR:
    case kUCASE_ASCII_CHAR:
      // just copy the byte
      ULAccumulator = g_RX_buf[g_RX_buf_out];

      // Force uppercase if that's what type we have
      if (kUCASE_ASCII_CHAR == Type)
      {
        ULAccumulator = toupper(ULAccumulator);
      }

      // Move to the next character
      advance_RX_buf_out();
      break;
   
    default:
      break;
  }

  // Range check absolute values
  if (Negative)
  {
    if (
      (
        kINT8 == Type
        &&
        (ULAccumulator > (uint32_t)0x80)
      )
      ||
      (
        kINT16 == Type
        &&
        (ULAccumulator > (uint32_t)0x8000)
      )
      ||
      (
        kINT32 == Type
        &&
        (ULAccumulator > (uint32_t)0x80000000L)
      )
    )
    {
      bitset (error_byte, kERROR_BYTE_PARAMETER_OUTSIDE_LIMIT);
      return (kEXTRACT_PARAMETER_OUTSIDE_LIMIT);
    }

    Accumulator = ULAccumulator;
    // Then apply the negative if that's the right thing to do
    if (Negative)
    {
      Accumulator = -Accumulator;
    }
  }
  else
  {
    if (
      (
        kINT8 == Type
        &&
        (ULAccumulator > (uint32_t)0x7F)
      )
      ||
      (
        kUINT8 == Type
        &&
        (ULAccumulator > (uint32_t)0xFF)
      )
      ||
      (
        kINT16 == Type
        &&
        (ULAccumulator > (uint32_t)0x7FFF)
      )
      ||
      (
        kUINT16 == Type
        &&
        (ULAccumulator > (uint32_t)0xFFFF)
      )
      ||
      (
        kUINT24 == Type
        &&
        (ULAccumulator > (uint32_t)0xFFFFFFL)
      )
      ||
      (
        kINT32 == Type
        &&
        (ULAccumulator > (uint32_t)0x7FFFFFFFL)
      )
    )
    {
      bitset (error_byte, kERROR_BYTE_PARAMETER_OUTSIDE_LIMIT);
      return (kEXTRACT_PARAMETER_OUTSIDE_LIMIT);
    }

    if (kUINT32 != Type)
    {
      Accumulator = ULAccumulator;
    }
  }

  // If all went well, then copy the result
  switch (Type)
  {
    case kINT8:
      *(int8_t *)ReturnValue = (int8_t)Accumulator;
      break;
    case kUINT8:
    case kHEX8:
    case kASCII_CHAR:
    case kUCASE_ASCII_CHAR:
      *(uint8_t *)ReturnValue = (uint8_t)Accumulator;
      break;
    case kINT16:
      *(int16_t *)ReturnValue = (int16_t)Accumulator;
      break;
    case kUINT16:
    case kHEX16:
      *(uint16_t *)ReturnValue = (uint16_t)Accumulator;
      break;
///    case kUINT24:
///    case kHEX24:
///      *(uint24_t *)ReturnValue = (uint24_t)Accumulator;
///      break;
    case kINT32:
      *(int32_t *)ReturnValue = Accumulator;
      break;
    case kUINT32:
    case kHEX32:
      *(uint32_t *)ReturnValue = ULAccumulator;
      break;
    default:
      return (kEXTRACT_INVALID_TYPE);
  }
  return(kEXTRACT_OK);
}

// Loop 'digits' number of times, looking at the
// byte in input_buffer index *ptr, and if it is
// a digit, adding it to acc. Take care of 
// powers of ten as well. If you hit a non-numerical
// char, then return FALSE, otherwise return TRUE.
// Store result as you go in *acc.
static bool extractDecDigits(uint32_t * acc, uint8_t digits)
{
  uint8_t val;
  uint32_t result = 0;
  bool returnVal = true;

  while(digits)
  {
    val = g_RX_buf[g_RX_buf_out];
    if ((val >= 48) && (val <= 57))
    {
      result = (result * 10) + (val - 48);
      // Move to the next character
      advance_RX_buf_out();
    }
    else
    {
      returnVal = false;
      break;
    }
    digits--;
  }
  *acc = result;
  return(returnVal);
}

// Loop 'digits' number of times, looking at the
// byte in input_buffer index *ptr, and if it is
// a hex digit, adding it to acc. If you hit a non-hex
// character, then return FALSE, otherwise return TRUE.
// Store result as you go in *acc.
static bool extractHexDigits(uint32_t * acc, uint8_t digits)
{
  uint8_t val;
  uint32_t result = 0;
  bool returnVal = true;
  
  while(digits)
  {
    val = g_RX_buf[g_RX_buf_out];
    // Convert from lowercase 'a-f' to uppercase 'A-F'
    if ((val >= 97) && (val <= 102))
    {
      val -= 32;
    }
    // Is it a number from 0-9?
    if ((val >= '0') && (val <= '9'))
    {
      result = (result << 4) | (val - '0');
      // Move to the next character
      advance_RX_buf_out();
    }
    else if ((val >= 'A') && (val <= 'F'))
    {
      result = (result << 4) | (val - 55);
      // Move to the next character
      advance_RX_buf_out();
    }
    else
    {
      returnVal = false;
      break;
    }
    digits--;
  }
  *acc = result;
  return(returnVal);
}
