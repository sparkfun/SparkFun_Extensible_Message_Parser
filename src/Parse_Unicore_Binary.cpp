/*------------------------------------------------------------------------------
Parse_Unicore_Binary.cpp

Unicore message parsing support routines

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

//----------------------------------------
// Types
//----------------------------------------

// Unicore parser scratch area
typedef struct _SEMP_UNICORE_BINARY_VALUES
{
    uint32_t crc;            // Copy of CRC calculation before CRC bytes
    uint16_t bytesRemaining; // Bytes remaining in RTCM CRC calculation
} SEMP_UNICORE_BINARY_VALUES;

// Define the Unicore message header
typedef struct _SEMP_UNICORE_HEADER
{
    uint8_t syncA;            // 0xaa
    uint8_t syncB;            // 0x44
    uint8_t syncC;            // 0xb5
    uint8_t cpuIdlePercent;   // CPU Idle Percentage 0-100
    uint16_t messageId;       // Message ID
    uint16_t messageLength;   // Message Length
    uint8_t referenceTime;    // Reference time（GPST or BDST)
    uint8_t timeStatus;       // Time status
    uint16_t weekNumber;      // Reference week number
    uint32_t secondsOfWeek;   // GPS seconds from the beginning of the
                              // reference week, accurate to the millisecond
    uint32_t RESERVED;

    uint8_t releasedVersion;  // Release version
    uint8_t leapSeconds;      // Leap sec
    uint16_t outputDelayMSec; // Output delay time, ms
} SEMP_UNICORE_HEADER;

//------------------------------------------------------------------------------
// Support routines
//
// The parser support routines are placed before the parser routines to eliminate
// forward declarations.
//------------------------------------------------------------------------------

//----------------------------------------
// Compute the CRC for the Unicore data
//----------------------------------------
uint32_t sempUnicoreBinaryComputeCrc(SEMP_PARSE_STATE *parse, uint8_t data)
{
    uint32_t crc;

    crc = parse->crc;
    crc = semp_crc32Table[(crc ^ data) & 0xff] ^ (crc >> 8);
    return crc;
}

//------------------------------------------------------------------------------
// Unicore parse routines
//
// The parser routines are placed in reverse order to define the routine before
// its use and eliminate forward declarations.  Removing the forward declaration
// helps reduce the exposure of the routines to the application layer.  The public
// data structures and routines are listed at the end of the file.
//------------------------------------------------------------------------------

//
//    Unicore Binary Response
//
//    |<----- 24 byte header ------>|<--- length --->|<- 4 bytes ->|
//    |                             |                |             |
//    +------------+----------------+----------------+-------------+
//    |  Preamble  | See table 7-48 |      Data      |    CRC      |
//    |  3 bytes   |   21 bytes     |    n bytes     |   32 bits   |
//    | 0xAA 44 B5 |                |                |             |
//    +------------+----------------+----------------+-------------+
//    |                                              |
//    |<------------------------ CRC --------------->|
//

//----------------------------------------
// Read the CRC
//----------------------------------------
bool sempUnicoreBinaryReadCrc(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_OUTPUT output = parse->debugOutput;
    SEMP_UNICORE_BINARY_VALUES *scratchPad = (SEMP_UNICORE_BINARY_VALUES *)parse->scratchPad;

    // Determine if the entire message was read
    if (--scratchPad->bytesRemaining)
        // Need more bytes
        return true;

    // Call the end-of-message routine with this message
    if ((!parse->crc) || (parse->badCrc && (!parse->badCrc(parse))))
        parse->eomCallback(parse, parse->type); // Pass parser array index
    else if (output)
    {
        sempPrintString(output, "SEMP ");
        sempPrintString(output, parse->parserName);
        sempPrintString(output, ": Unicore, bad CRC, received ");
        sempPrintHex02x(output, parse->buffer[parse->length - 4]);
        output(' ');
        sempPrintHex02x(output, parse->buffer[parse->length - 3]);
        output(' ');
        sempPrintHex02x(output, parse->buffer[parse->length - 2]);
        output(' ');
        sempPrintHex02x(output, parse->buffer[parse->length - 1]);
        sempPrintString(output, ", computed: ");
        sempPrintHex02x(output, scratchPad->crc & 0xff);
        output(' ');
        sempPrintHex02x(output, (scratchPad->crc >> 8) & 0xff);
        output(' ');
        sempPrintHex02x(output, (scratchPad->crc >> 16) & 0xff);
        output(' ');
        sempPrintHex02xLn(output, (scratchPad->crc >> 24) & 0xff);
    }
    parse->state = sempFirstByte;
    return false;
}

//----------------------------------------
// Read the message data
//----------------------------------------
bool sempUnicoreBinaryReadData(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_UNICORE_BINARY_VALUES *scratchPad = (SEMP_UNICORE_BINARY_VALUES *)parse->scratchPad;

    // Determine if the entire message was read
    if (!--scratchPad->bytesRemaining)
    {
        // The message data is complete, read the CRC
        scratchPad->bytesRemaining = 4;
        scratchPad->crc = parse->crc;
        parse->state = sempUnicoreBinaryReadCrc;
    }
    return true;
}

//----------------------------------------
//  Header
//
//    Bytes     Field Description
//    -----     ----------------------------------------
//      1       CPU Idle Percentage 0-100
//      2       Message ID
//      2       Message Length
//      1       Reference time（GPST or BDST)
//      1       Time status
//      2       Reference week number
//      4       GPS seconds from the beginning of the reference week,
//              accurate to the millisecond
//      4       Reserved
//      1       Release version
//      1       Leap sec
//      2       Output delay time, ms
//
// Read the header
//----------------------------------------
bool sempUnicoreBinaryReadHeader(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_UNICORE_BINARY_VALUES *scratchPad = (SEMP_UNICORE_BINARY_VALUES *)parse->scratchPad;

    if (parse->length >= sizeof(SEMP_UNICORE_HEADER))
    {
        // The header is complete, read the message data next
        SEMP_UNICORE_HEADER *header = (SEMP_UNICORE_HEADER *)parse->buffer;
        scratchPad->bytesRemaining = header->messageLength;
        SEMP_OUTPUT output = parse->debugOutput;
        if (parse->verboseDebug && output)
        {
            sempPrintString(output, "SEMP ");
            sempPrintString(output, parse->parserName);
            sempPrintString(output, ": Incoming Unicore ");
            sempPrintHex0x04x(output, header->messageLength);
            sempPrintString(output, " (");
            sempPrintDecimalU32(output, header->messageLength);
            sempPrintStringLn(output, ") bytes");
        }
        parse->state = sempUnicoreBinaryReadData;
    }
    return true;
}

//----------------------------------------
// Read the third sync byte
//----------------------------------------
bool sempUnicoreBinaryBinarySync3(SEMP_PARSE_STATE *parse, uint8_t data)
{
    // Verify sync byte 3
    if (data != 0xB5)
        // Invalid sync byte, start searching for a preamble byte
        return sempFirstByte(parse, data);

    // Read the header next
    parse->state = sempUnicoreBinaryReadHeader;
    return true;
}

//----------------------------------------
// Read the second sync byte
//----------------------------------------
bool sempUnicoreBinaryBinarySync2(SEMP_PARSE_STATE *parse, uint8_t data)
{
    // Verify sync byte 2
    if (data != 0x44)
        // Invalid sync byte, start searching for a preamble byte
        return sempFirstByte(parse, data);

    // Look for the last sync byte
    parse->state = sempUnicoreBinaryBinarySync3;
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
//   Returns true if the Unicore Binary parser recgonizes the input and
//   false if another parser should be used
//----------------------------------------
bool sempUnicoreBinaryPreamble(SEMP_PARSE_STATE *parse, uint8_t data)
{
    // Determine if this is the beginning of a Unicore message
    if (data != 0xAA)
        return false;

    // Look for the second sync byte
    parse->crc = 0;
    parse->computeCrc = sempUnicoreBinaryComputeCrc;
    parse->crc = parse->computeCrc(parse, data);
    parse->state = sempUnicoreBinaryBinarySync2;
    return true;
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
const char * sempUnicoreBinaryGetStateName(const SEMP_PARSE_STATE *parse)
{
    if (parse->state == sempUnicoreBinaryPreamble)
        return "sempUnicoreBinaryPreamble";
    if (parse->state == sempUnicoreBinaryBinarySync2)
        return "sempUnicoreBinaryBinarySync2";
    if (parse->state == sempUnicoreBinaryBinarySync3)
        return "sempUnicoreBinaryBinarySync3";
    if (parse->state == sempUnicoreBinaryReadHeader)
        return "sempUnicoreBinaryReadHeader";
    if (parse->state == sempUnicoreBinaryReadData)
        return "sempUnicoreBinaryReadData";
    if (parse->state == sempUnicoreBinaryReadCrc)
        return "sempUnicoreBinaryReadCrc";
    return nullptr;
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
SEMP_PARSER_DESCRIPTION sempUnicoreBinaryParserDescription =
{
    "Unicore binary parser",            // parserName
    sempUnicoreBinaryPreamble,          // preamble
    sempUnicoreBinaryGetStateName,      // State to state name translation routine
    3000,   /* ??? */                   // minimumParseAreaBytes
    sizeof(SEMP_UNICORE_BINARY_VALUES), // scratchPadBytes
    0,                                  // payloadOffset
};

//----------------------------------------
// Print the Unicore message header
//----------------------------------------
void sempUnicoreBinaryPrintHeader(SEMP_PARSE_STATE *parse)
{
    SEMP_UNICORE_HEADER * header;

    Print *print = parse->printError;
    if (print)
    {
        sempPrintln(print, "Unicore Message Header");
        header = (SEMP_UNICORE_HEADER *)parse->buffer;
        sempPrintf(print, "      0x%02x: Sync A", header->syncA);
        sempPrintf(print, "      0x%02x: Sync B", header->syncB);
        sempPrintf(print, "      0x%02x: Sync C", header->syncC);
        sempPrintf(print, "      %3d%%: CPU Idle Time", header->cpuIdlePercent);
        sempPrintf(print, "     %5d: Message ID", header->messageId);
        sempPrintf(print, "     %5d: Message Length (bytes)", header->messageLength);
        sempPrintf(print, "       %3d: Reference Time", header->referenceTime);
        sempPrintf(print, "      0x%02x: Time Status", header->timeStatus);
        sempPrintf(print, "     %5d: Week Number", header->weekNumber);
        sempPrintf(print, "%10d: Seconds of Week", header->secondsOfWeek);
        sempPrintf(print, "0x%08x: RESERVED", header->RESERVED);
        sempPrintf(print, "       %3d: Release Version", header->releasedVersion);
        sempPrintf(print, "       %3d: Leap Seconds", header->leapSeconds);
        sempPrintf(print, "     %5d: Output Delay (mSec)", header->outputDelayMSec);
    }
}
