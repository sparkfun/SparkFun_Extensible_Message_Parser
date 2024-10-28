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
// Support routines
//----------------------------------------

// Compute the CRC for the Unicore data
uint32_t sempUnicoreBinaryComputeCrc(SEMP_PARSE_STATE *parse, uint8_t data)
{
    uint32_t crc;

    crc = parse->crc;
    crc = semp_crc32Table[(crc ^ data) & 0xff] ^ (crc >> 8);
    return crc;
}

// Print the Unicore message header
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

//----------------------------------------
// Unicore parse routines
//----------------------------------------

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

// Read the CRC
bool sempUnicoreBinaryReadCrc(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;

    // Determine if the entire message was read
    if (--scratchPad->unicoreBinary.bytesRemaining)
        // Need more bytes
        return true;

    // Call the end-of-message routine with this message
    if ((!parse->crc) || (parse->badCrc && (!parse->badCrc(parse))))
        parse->eomCallback(parse, parse->type); // Pass parser array index
    else
    {
        sempPrintf(parse->printDebug,
                   "SEMP: %s Unicore, bad CRC, "
                   "received %02x %02x %02x %02x, computed: %02x %02x %02x %02x",
                   parse->parserName,
                   parse->buffer[parse->length - 4],
                   parse->buffer[parse->length - 3],
                   parse->buffer[parse->length - 2],
                   parse->buffer[parse->length - 1],
                   scratchPad->unicoreBinary.crc & 0xff,
                   (scratchPad->unicoreBinary.crc >> 8) & 0xff,
                   (scratchPad->unicoreBinary.crc >> 16) & 0xff,
                   (scratchPad->unicoreBinary.crc >> 24) & 0xff);
    }
    parse->state = sempFirstByte;
    return false;
}

// Read the message data
bool sempUnicoreBinaryReadData(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;

    // Determine if the entire message was read
    if (!--scratchPad->unicoreBinary.bytesRemaining)
    {
        // The message data is complete, read the CRC
        scratchPad->unicoreBinary.bytesRemaining = 4;
        scratchPad->unicoreBinary.crc = parse->crc;
        parse->state = sempUnicoreBinaryReadCrc;
    }
    return true;
}

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
bool sempUnicoreBinaryReadHeader(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;

    if (parse->length >= sizeof(SEMP_UNICORE_HEADER))
    {
        // The header is complete, read the message data next
        SEMP_UNICORE_HEADER *header = (SEMP_UNICORE_HEADER *)parse->buffer;
        scratchPad->unicoreBinary.bytesRemaining = header->messageLength;
        parse->state = sempUnicoreBinaryReadData;
    }
    return true;
}

// Read the third sync byte
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

// Read the second sync byte
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

// Check for the preamble
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

// Translates state value into an string, returns nullptr if not found
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
