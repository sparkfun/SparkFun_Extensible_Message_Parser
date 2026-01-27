/*------------------------------------------------------------------------------
Parse_RTCM.cpp

RTCM message parsing support routines

The parser routines within a parser module are typically placed in
reverse order within the module.  This lets the routine declaration
proceed the routine use and eliminates the need for forward declaration.
Removing the forward declaration helps reduce the exposure of the
routines to the application layer.  As such only the preamble routine
should need to be listed in SparkFun_Extensible_Message_Parser.h.

License: MIT. Please see LICENSE.md for more details
------------------------------------------------------------------------------*/

#include <stdio.h>
#include "SparkFun_Extensible_Message_Parser.h"
#include "semp_crc24q.h" // 24-bit CRC-24Q cyclic redundancy checksum for RTCM parsing

//----------------------------------------
// Types
//----------------------------------------

// RTCM parser scratch area
typedef struct _SEMP_RTCM_VALUES
{
    uint32_t crc;            // Copy of CRC calculation before CRC bytes
    uint16_t bytesRemaining; // Bytes remaining in RTCM CRC calculation
    uint16_t message;        // Message number
} SEMP_RTCM_VALUES;

//----------------------------------------
// Support routines
//
// The parser support routines are placed before the parser routines to eliminate
// forward declarations.
//----------------------------------------

//----------------------------------------
// Compute the CRC one byte at a time
//
// Inputs:
//   crc: Current CRC value
//   data: Byte to encode into the CRC value
//
// Outputs:
//   Returns the updated CRC value
//----------------------------------------
uint32_t sempRtcmComputeCrc24q(SEMP_PARSE_STATE *parse, uint8_t data)
{
    uint32_t crc = parse->crc;
    crc = ((parse)->crc << 8) ^ semp_crc24qTable[data ^ (((parse)->crc >> 16) & 0xff)];
    return crc & 0x00ffffff;
}

//------------------------------------------------------------------------------
// RTCM parse routines
//
// The parser routines are placed in reverse order to define the routine before
// its use and eliminate forward declarations.  Removing the forward declaration
// helps reduce the exposure of the routines to the application layer.  The public
// data structures and routines are listed at the end of the file.
//------------------------------------------------------------------------------

//
//    RTCM Standard 10403.2 - Chapter 4, Transport Layer
//
//    |<------------- 3 bytes ------------>|<----- length ----->|<- 3 bytes ->|
//    |                                    |                    |             |
//    +----------+--------+----------------+---------+----------+-------------+
//    | Preamble |  Fill  | Message Length | Message |   Fill   |   CRC-24Q   |
//    |  8 bits  | 6 bits |    10 bits     |  n-bits | 0-7 bits |   24 bits   |
//    |   0xd3   | 000000 |   (in bytes)   |         |   zeros  |             |
//    +----------+--------+----------------+---------+----------+-------------+
//    |                                                         |
//    |<------------------------ CRC -------------------------->|
//

//----------------------------------------
// Read the CRC
//----------------------------------------
bool sempRtcmReadCrc(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_OUTPUT output;
    SEMP_RTCM_VALUES *scratchPad = (SEMP_RTCM_VALUES *)parse->scratchPad;

    // Account for this data byte
    scratchPad->bytesRemaining -= 1;

    // Wait until all the data is received
    if (scratchPad->bytesRemaining > 0)
        return true;

    // Process the message if CRC is valid
    output = parse->debugOutput;
    if ((parse->crc == 0) || (parse->badCrc && (!parse->badCrc(parse))))
    {
        if ((parse->length == 6) && output)
        {
            sempPrintString(output, "SEMP ");
            sempPrintString(output, parse->parserName);
            sempPrintString(output, ": RTCM ");
            sempPrintHex0x04x(output, parse->length);
            sempPrintString(output, " (");
            sempPrintDecimalU32(output, parse->length);
            sempPrintStringLn(output, ") bytes, \"filler\" message");
        }
        parse->eomCallback(parse, parse->type); // Pass parser array index
    }

    // Display the RTCM messages with bad CRC
    else if (output)
    {
        sempPrintString(output, "SEMP ");
        sempPrintString(output, parse->parserName);
        sempPrintString(output, ": RTCM ");
        sempPrintDecimalI32(output, scratchPad->message);
        sempPrintString(output, ", ");
        sempPrintHex0x04x(output, parse->length);
        sempPrintString(output, " (");
        sempPrintDecimalU32(output, parse->length);
        sempPrintString(output, ") bytes, bad CRC, received ");
        sempPrintHex02x(output, parse->buffer[parse->length - 3]);
        output(' ');
        sempPrintHex02x(output, parse->buffer[parse->length - 2]);
        output(' ');
        sempPrintHex02x(output, parse->buffer[parse->length - 1]);
        sempPrintString(output, ", computed: ");
        sempPrintHex02x(output, (scratchPad->crc >> 16) & 0xff);
        sempPrintHex02x(output, (scratchPad->crc >> 8) & 0xff);
        sempPrintHex02xLn(output, scratchPad->crc & 0xff);
    }

    // Search for another preamble byte
    parse->state = sempFirstByte;
    return false;
}

//----------------------------------------
// Read the rest of the message
//----------------------------------------
bool sempRtcmReadData(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_RTCM_VALUES *scratchPad = (SEMP_RTCM_VALUES *)parse->scratchPad;

    // Account for this data byte
    scratchPad->bytesRemaining -= 1;

    // Wait until all the data is received
    if (scratchPad->bytesRemaining <= 0)
    {
        scratchPad->crc = parse->crc;
        scratchPad->bytesRemaining = 3;
        parse->state = sempRtcmReadCrc;
    }
    return true;
}

//----------------------------------------
// Read the lower 4 bits of the message number
//----------------------------------------
bool sempRtcmReadMessage2(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_RTCM_VALUES *scratchPad = (SEMP_RTCM_VALUES *)parse->scratchPad;

    scratchPad->message |= data >> 4;
    scratchPad->bytesRemaining -= 1;
    parse->state = sempRtcmReadData;
    return true;
}

//----------------------------------------
// Read the upper 8 bits of the message number
//----------------------------------------
bool sempRtcmReadMessage1(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_RTCM_VALUES *scratchPad = (SEMP_RTCM_VALUES *)parse->scratchPad;

    scratchPad->message = data << 4;
    scratchPad->bytesRemaining -= 1;
    parse->state = sempRtcmReadMessage2;
    return true;
}

//----------------------------------------
// Read the lower 8 bits of the length
//----------------------------------------
bool sempRtcmReadLength2(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_OUTPUT output;
    SEMP_RTCM_VALUES *scratchPad = (SEMP_RTCM_VALUES *)parse->scratchPad;

    scratchPad->bytesRemaining |= data;
    output = parse->debugOutput;
    if (parse->verboseDebug && output)
    {
        sempPrintString(output, "SEMP ");
        sempPrintString(output, parse->parserName);
        sempPrintString(output, ": Incoming RTCM ");
        sempPrintDecimalI32(output, scratchPad->message);
        sempPrintString(output, ", ");
        sempPrintHex0x04x(output, scratchPad->bytesRemaining);
        sempPrintString(output, " (");
        sempPrintDecimalI32(output, scratchPad->bytesRemaining);
        sempPrintStringLn(output, ") bytes");
    }

    if (scratchPad->bytesRemaining == 0) // Handle zero length messages - 10403 "filler" messages
    {
        scratchPad->message = 0; // Clear the previous message number
        scratchPad->crc = parse->crc;
        scratchPad->bytesRemaining = 3;
        parse->state = sempRtcmReadCrc; // Jump to the CRC
    }
    else
    {
        parse->state = sempRtcmReadMessage1;
    }
    return true;
}

//----------------------------------------
// Read the upper two bits of the length
//----------------------------------------
bool sempRtcmReadLength1(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_RTCM_VALUES *scratchPad = (SEMP_RTCM_VALUES *)parse->scratchPad;

    // Verify the length byte - check the 6 MS bits are all zero
    if (data & (~3))
        // Invalid length, start searching for a preamble byte
        return sempFirstByte(parse, data);

    // Save the upper 2 bits of the length
    scratchPad->bytesRemaining = data << 8;
    parse->state = sempRtcmReadLength2;
    return true;
}

//----------------------------------------
// Check for the preamble
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//   data: First data byte in the stream of data to parse
//
// Outputs:
//   Returns true if the RTCM parser recgonizes the input and false
//   if another parser should be used
//----------------------------------------
bool sempRtcmPreamble(SEMP_PARSE_STATE *parse, uint8_t data)
{
    if (data == 0xd3)
    {
        // Start the CRC with this byte
        parse->computeCrc = sempRtcmComputeCrc24q;
        parse->crc = parse->computeCrc(parse, data);

        // Get the message length
        parse->state = sempRtcmReadLength1;
        return true;
    }
    return false;
}

//----------------------------------------
// Translates state value into an string, returns nullptr if not found
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//
// Outputs
//   Returns the address of the zero terminated state name string
//----------------------------------------
const char * sempRtcmGetStateName(const SEMP_PARSE_STATE *parse)
{
    if (parse->state == sempRtcmPreamble)
        return "sempRtcmPreamble";
    if (parse->state == sempRtcmReadLength1)
        return "sempRtcmReadLength1";
    if (parse->state == sempRtcmReadLength2)
        return "sempRtcmReadLength2";
    if (parse->state == sempRtcmReadMessage1)
        return "sempRtcmReadMessage1";
    if (parse->state == sempRtcmReadMessage2)
        return "sempRtcmReadMessage2";
    if (parse->state == sempRtcmReadData)
        return "sempRtcmReadData";
    if (parse->state == sempRtcmReadCrc)
        return "sempRtcmReadCrc";
    return nullptr;
}

//----------------------------------------
// Display the contents of the scratch pad
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//   output: Address of a routine to output a character
//----------------------------------------
void sempRtcmPrintScratchPad(SEMP_PARSE_STATE *parse, SEMP_OUTPUT output)
{
    SEMP_RTCM_VALUES *scratchPad;

    // Get the scratch pad address
    scratchPad = (SEMP_RTCM_VALUES *)parse->scratchPad;

    // Display the CRC
    sempPrintString(output, "    crc: ");
    sempPrintHex0x04xLn(output, scratchPad->crc);

    // Display the remaining bytes
    sempPrintString(output, "    bytesRemaining: ");
    sempPrintDecimalU32Ln(output, scratchPad->bytesRemaining);

    // Display the message number
    sempPrintString(output, "    message: ");
    sempPrintDecimalU32Ln(output, scratchPad->message);
}

//------------------------------------------------------------------------------
// Public data and routines
//
// The following data structures and routines are listed in the .h file and are
// exposed to the SEMP routine and application layer.
//------------------------------------------------------------------------------

//----------------------------------------
// Describe the parser
//----------------------------------------
SEMP_PARSER_DESCRIPTION sempRtcmParserDescription =
{
    "RTCM parser",              // parserName
    sempRtcmPreamble,           // preamble
    sempRtcmGetStateName,       // State to state name translation routine
    sempRtcmPrintScratchPad,    // Print the contents of the scratch pad
    1029,                       // minimumParseAreaBytes
    sizeof(SEMP_RTCM_VALUES),   // scratchPadBytes
    0,                          // payloadOffset
};

//----------------------------------------
// Get the message number
//----------------------------------------
uint16_t sempRtcmGetMessageNumber(const SEMP_PARSE_STATE *parse)
{
    SEMP_RTCM_VALUES *scratchPad = (SEMP_RTCM_VALUES *)parse->scratchPad;
    return scratchPad->message;
}

//----------------------------------------
// Get signed integer with width bits, starting at bit start
//----------------------------------------
int64_t sempRtcmGetSignedBits(const SEMP_PARSE_STATE *parse, size_t start, size_t width)
{
    uint8_t *ptr = parse->buffer;
    ptr += 3; // Skip the preamble and length bytes

    union
    {
        uint64_t unsigned64;
        int64_t signed64;
    } result;

    result.unsigned64 = 0;

    uint64_t twosComplement = 0xFFFFFFFFFFFFFFFF;

    bool isNegative;

    size_t count = 0;
    uint8_t bitMask = 0x80;

    // Skip whole bytes (8 bits)
    ptr += start / 8;
    count += (start / 8) * 8;

    // Loop until we reach the start bit
    while (count < start)
    {
        bitMask >>= 1; // Shift the bit mask
        count++;       // Increment the count

        if (bitMask == 0) // Have we counted 8 bits?
        {
            ptr++;          // Point to the next byte
            bitMask = 0x80; // Reset the bit mask
        }
    }

    isNegative = *ptr & bitMask; // Record the first bit - indicates in the number is negative

    // We have reached the start bit and ptr is pointing at the correct byte
    // Now extract width bits, incrementing ptr and shifting bitMask as we go
    while (count < (start + width))
    {
        if (*ptr & bitMask)       // Is the bit set?
            result.unsigned64 |= 1; // Set the corresponding bit in result

        bitMask >>= 1;        // Shift the bit mask
        count++;              // Increment the count
        twosComplement <<= 1; // Shift the two's complement mask (clear LSB)

        if (bitMask == 0) // Have we counted 8 bits?
        {
            ptr++;          // Point to the next byte
            bitMask = 0x80; // Reset the bit mask
        }

        if (count < (start + width)) // Do we need to shift result?
            result.unsigned64 <<= 1;   // Shift the result
    }

    // Handle negative number
    if (isNegative)
        result.unsigned64 |= twosComplement; // OR in the two's complement mask

    return result.signed64;
}

//----------------------------------------
// Get unsigned integer with width bits, starting at bit start
//----------------------------------------
uint64_t sempRtcmGetUnsignedBits(const SEMP_PARSE_STATE *parse, size_t start, size_t width)
{
    uint8_t *ptr = parse->buffer;
    ptr += 3; // Skip the preamble and length bytes

    uint64_t result = 0;
    size_t count = 0;
    uint8_t bitMask = 0x80;

    // Skip whole bytes (8 bits)
    ptr += start / 8;
    count += (start / 8) * 8;

    // Loop until we reach the start bit
    while (count < start)
    {
        bitMask >>= 1; // Shift the bit mask
        count++;       // Increment the count

        if (bitMask == 0) // Have we counted 8 bits?
        {
            ptr++;          // Point to the next byte
            bitMask = 0x80; // Reset the bit mask
        }
    }

    // We have reached the start bit and ptr is pointing at the correct byte
    // Now extract width bits, incrementing ptr and shifting bitMask as we go
    while (count < (start + width))
    {
        if (*ptr & bitMask) // Is the bit set?
            result |= 1;      // Set the corresponding bit in result

        bitMask >>= 1; // Shift the bit mask
        count++;       // Increment the count

        if (bitMask == 0) // Have we counted 8 bits?
        {
            ptr++;          // Point to the next byte
            bitMask = 0x80; // Reset the bit mask
        }

        if (count < (start + width)) // Do we need to shift result?
            result <<= 1;              // Shift the result
    }

    return result;
}
