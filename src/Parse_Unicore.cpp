/*------------------------------------------------------------------------------
Parse_Unicore.cpp

Unicore message parsing support routines

The parser routines within a parser module are typically placed in
reverse order within the module.  This lets the routine declaration
proceed the routine use and eliminates the need for forward declaration.
Removing the forward declaration helps reduce the exposure of the
routines to the application layer.  As such only the preamble routine
should need to be listed in SparkFun_Extensible_Message_Parser.h.
------------------------------------------------------------------------------*/

#include <stdio.h>
#include "SparkFun_Extensible_Message_Parser.h"
#include "semp_crc32.h"

//----------------------------------------
// Constants
//----------------------------------------

#define UNICORE_HEADER_LENGTH                   ((uint16_t)24)
#define UNICORE_OFFSET_HEADER_MESSAGE_LENGTH    ((uint16_t)6)

//----------------------------------------
// Structure definitions
//----------------------------------------

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

//----------------------------------------
// Support routines
//----------------------------------------

// Compute the CRC for the Unicore data
uint32_t sempUnicoreComputeCrc(SEMP_PARSE_STATE *parse, uint8_t data)
{
    uint32_t crc;

    crc = parse->crc;
    crc = semp_crc32Table[(crc ^ data) & 0xff] ^ (crc >> 8);
    return crc;
}

// Print the Unicore message header
void sempUnicorePrintHeader(SEMP_PARSE_STATE *parse)
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
bool sempUnicoreReadCrc(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;

    // Determine if the entire message was read
    if (--scratchPad->unicore.bytesRemaining)
        // Need more bytes
        return true;

    // Call the end-of-message routine with this message
    if (!parse->crc)
        parse->eomCallback(parse, parse->type); // Pass parser array index
    else
    {
        SEMP_UNICORE_HEADER *header = (SEMP_UNICORE_HEADER *)parse->buffer;
        sempPrintf(parse->printDebug,
                   "SEMP: %s Unicore, bad CRC, "
                   "expecting %02x %02x %02x %02x, actual: %02x %02x %02x %02x",
                   parse->parserName,
                   scratchPad->unicore.crc & 0xff,
                   (scratchPad->unicore.crc >> 8) & 0xff,
                   (scratchPad->unicore.crc >> 16) & 0xff,
                   (scratchPad->unicore.crc >> 24) & 0xff,
                   parse->buffer[parse->length - 4],
                   parse->buffer[parse->length - 3],
                   parse->buffer[parse->length - 2],
                   parse->buffer[parse->length - 1]);
    }
    parse->state = sempFirstByte;
    return false;
}

// Read the message data
bool sempUnicoreReadData(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;

    // Determine if the entire message was read
    if (!--scratchPad->unicore.bytesRemaining)
    {
        // The message data is complete, read the CRC
        scratchPad->unicore.bytesRemaining = 4;
        scratchPad->unicore.crc = parse->crc;
        parse->state = sempUnicoreReadCrc;
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
bool sempUnicoreReadHeader(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;

    if (parse->length >= sizeof(SEMP_UNICORE_HEADER))
    {
        // The header is complete, read the message data next
        SEMP_UNICORE_HEADER *header = (SEMP_UNICORE_HEADER *)parse->buffer;
        scratchPad->unicore.bytesRemaining = header->messageLength;
        parse->state = sempUnicoreReadData;
    }
    return true;
}

// Read the third sync byte
bool sempUnicoreBinarySync3(SEMP_PARSE_STATE *parse, uint8_t data)
{
    // Verify sync byte 3
    if (data != 0xB5)
        // Invalid sync byte, start searching for a preamble byte
        return sempFirstByte(parse, data);

    // Read the header next
    parse->state = sempUnicoreReadHeader;
    return true;
}

// Read the second sync byte
bool sempUnicoreBinarySync2(SEMP_PARSE_STATE *parse, uint8_t data)
{
    // Verify sync byte 2
    if (data != 0x44)
        // Invalid sync byte, start searching for a preamble byte
        return sempFirstByte(parse, data);

    // Look for the last sync byte
    parse->state = sempUnicoreBinarySync3;
    return true;
}

// Check for the preamble
bool sempUnicorePreamble(SEMP_PARSE_STATE *parse, uint8_t data)
{
    // Determine if this is the beginning of a Unicore message
    if (data != 0xAA)
        return false;

    // Look for the second sync byte
    parse->crc = 0;
    parse->computeCrc = sempUnicoreComputeCrc;
    parse->crc = parse->computeCrc(parse, data);
    parse->state = sempUnicoreBinarySync2;
    return true;
}

// Translates state value into an string, returns nullptr if not found
const char * sempUnicoreGetStateName(const SEMP_PARSE_STATE *parse)
{
    if (parse->state == sempUnicorePreamble)
        return "sempUnicorePreamble";
    if (parse->state == sempUnicoreBinarySync2)
        return "sempUnicoreBinarySync2";
    if (parse->state == sempUnicoreBinarySync3)
        return "sempUnicoreBinarySync3";
    if (parse->state == sempUnicoreReadHeader)
        return "sempUnicoreReadHeader";
    if (parse->state == sempUnicoreReadData)
        return "sempUnicoreReadData";
    if (parse->state == sempUnicoreReadCrc)
        return "sempUnicoreReadCrc";
    return nullptr;
}
