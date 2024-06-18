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
// Support routines
//----------------------------------------

uint32_t sempRtcmComputeCrc24q(SEMP_PARSE_STATE *parse, uint8_t data)
{
    uint32_t crc = parse->crc;
    crc = ((parse)->crc << 8) ^ semp_crc24qTable[data ^ (((parse)->crc >> 16) & 0xff)];
    return crc & 0x00ffffff;
}

//----------------------------------------
// RTCM parse routines
//----------------------------------------

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

// Read the CRC
bool sempRtcmReadCrc(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;

    // Account for this data byte
    scratchPad->rtcm.bytesRemaining -= 1;

    // Wait until all the data is received
    if (scratchPad->rtcm.bytesRemaining > 0)
        return true;

    // Process the message if CRC is valid
    if ((parse->crc == 0) || (parse->badCrc && (!parse->badCrc(parse))))
        parse->eomCallback(parse, parse->type); // Pass parser array index

    // Display the RTCM messages with bad CRC
    else
        sempPrintf(parse->printDebug,
                   "SEMP: %s RTCM %d, 0x%04x (%d) bytes, bad CRC, "
                   "received %02x %02x %02x, computed: %02x %02x %02x",
                   parse->parserName,
                   scratchPad->rtcm.message,
                   parse->length, parse->length,
                   parse->buffer[parse->length - 3],
                   parse->buffer[parse->length - 2],
                   parse->buffer[parse->length - 1],
                   (scratchPad->rtcm.crc >> 16) & 0xff,
                   (scratchPad->rtcm.crc >> 8) & 0xff,
                   scratchPad->rtcm.crc & 0xff);

    // Search for another preamble byte
    parse->state = sempFirstByte;
    return false;
}

// Read the rest of the message
bool sempRtcmReadData(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;

    // Account for this data byte
    scratchPad->rtcm.bytesRemaining -= 1;

    // Wait until all the data is received
    if (scratchPad->rtcm.bytesRemaining <= 0)
    {
        scratchPad->rtcm.crc = parse->crc;
        scratchPad->rtcm.bytesRemaining = 3;
        parse->state = sempRtcmReadCrc;
    }
    return true;
}

// Read the lower 4 bits of the message number
bool sempRtcmReadMessage2(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;

    scratchPad->rtcm.message |= data >> 4;
    scratchPad->rtcm.bytesRemaining -= 1;
    parse->state = sempRtcmReadData;
    return true;
}

// Read the upper 8 bits of the message number
bool sempRtcmReadMessage1(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;

    scratchPad->rtcm.message = data << 4;
    scratchPad->rtcm.bytesRemaining -= 1;
    parse->state = sempRtcmReadMessage2;
    return true;
}

// Read the lower 8 bits of the length
bool sempRtcmReadLength2(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;

    scratchPad->rtcm.bytesRemaining |= data;
    parse->state = sempRtcmReadMessage1;
    return true;
}

// Read the upper two bits of the length
bool sempRtcmReadLength1(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;

    // Verify the length byte - check the 6 MS bits are all zero
    if (data & (~3))
        // Invalid length, start searching for a preamble byte
        return sempFirstByte(parse, data);

    // Save the upper 2 bits of the length
    scratchPad->rtcm.bytesRemaining = data << 8;
    parse->state = sempRtcmReadLength2;
    return true;
}

// Check for the preamble
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

// Translates state value into an string, returns nullptr if not found
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

// Get the message number
uint16_t sempRtcmGetMessageNumber(const SEMP_PARSE_STATE *parse)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;
    return scratchPad->rtcm.message;
}
