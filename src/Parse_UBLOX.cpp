/*------------------------------------------------------------------------------
Parse_UBLOX.cpp

U-Blox message parsing support routines

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
// U-BLOX parse routines
//----------------------------------------

//
//    U-BLOX Message
//
//    |<-- Preamble --->|
//    |                 |
//    +--------+--------+---------+--------+---------+---------+--------+--------+
//    |  SYNC  |  SYNC  |  Class  |   ID   | Length  | Payload |  CK_A  |  CK_B  |
//    | 8 bits | 8 bits |  8 bits | 8 bits | 2 bytes | n bytes | 8 bits | 8 bits |
//    |  0xb5  |  0x62  |         |        |         |         |        |        |
//    +--------+--------+---------+--------+---------+---------+--------+--------+
//                      |                                      |
//                      |<------------- Checksum ------------->|
//
//  8-Bit Fletcher Algorithm, which is used in the TCP standard (RFC 1145)
//  http://www.ietf.org/rfc/rfc1145.txt
//  Checksum calculation
//      Initialization: CK_A = CK_B = 0
//      CK_A += data
//      CK_B += CK_A
//

// Read the CK_B byte
bool sempUbloxCkB(SEMP_PARSE_STATE *parse, uint8_t data)
{
    bool badChecksum;
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;

    // Validate the checksum
    badChecksum =
        ((parse->buffer[parse->length - 2] != scratchPad->ublox.ck_a) || (parse->buffer[parse->length - 1] != scratchPad->ublox.ck_b));

    // Process this message if checksum is valid
    if ((badChecksum == false) || (parse->badCrc && (!parse->badCrc(parse))))
        parse->eomCallback(parse, parse->type); // Pass parser array index
    else
        sempPrintf(parse->printDebug,
                   "SEMP %s: UBLOX bad checksum received 0x%02x%02x computed 0x%02x%02x",
                   parse->parserName,
                   parse->buffer[parse->length - 2], parse->buffer[parse->length - 1],
                   scratchPad->ublox.ck_a, scratchPad->ublox.ck_b);

    // Search for the next preamble byte
    parse->length = 0;
    parse->state = sempFirstByte;
    return false;
}

// Read the CK_A byte
bool sempUbloxCkA(SEMP_PARSE_STATE *parse, uint8_t data)
{
    parse->state = sempUbloxCkB;
    return true;
}

// Read the payload
bool sempUbloxPayload(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;

    // Compute the checksum over the payload
    if (scratchPad->ublox.bytesRemaining--)
    {
        // Calculate the checksum
        scratchPad->ublox.ck_a += data;
        scratchPad->ublox.ck_b += scratchPad->ublox.ck_a;
        return true;
    }
    return sempUbloxCkA(parse, data);
}

// Read the second length byte
bool sempUbloxLength2(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;

    // Calculate the checksum
    scratchPad->ublox.ck_a += data;
    scratchPad->ublox.ck_b += scratchPad->ublox.ck_a;

    // Save the second length byte
    scratchPad->ublox.bytesRemaining |= ((uint16_t)data) << 8;
    parse->state = sempUbloxPayload;
    return true;
}

// Read the first length byte
bool sempUbloxLength1(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;

    // Calculate the checksum
    scratchPad->ublox.ck_a += data;
    scratchPad->ublox.ck_b += scratchPad->ublox.ck_a;

    // Save the first length byte
    scratchPad->ublox.bytesRemaining = data;
    parse->state = sempUbloxLength2;
    return true;
}

// Read the ID byte
bool sempUbloxId(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;

    // Calculate the checksum
    scratchPad->ublox.ck_a += data;
    scratchPad->ublox.ck_b += scratchPad->ublox.ck_a;

    // Save the ID as the lower 8-bits of the message
    scratchPad->ublox.message |= data;
    parse->state = sempUbloxLength1;
    return true;
}

// Read the class byte
bool sempUbloxClass(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;

    // Start the checksum calculation
    scratchPad->ublox.ck_a = data;
    scratchPad->ublox.ck_b = data;

    // Save the class as the upper 8-bits of the message
    scratchPad->ublox.message = ((uint16_t)data) << 8;
    parse->state = sempUbloxId;
    return true;
}

// Read the second sync byte
bool sempUbloxSync2(SEMP_PARSE_STATE *parse, uint8_t data)
{
    // Verify the sync 2 byte
    if (data != 0x62)
    {
        // Display the invalid data
        sempPrintf(parse->printDebug,
                   "SEMP %s: UBLOX invalid second sync byte",
                   parse->parserName);

        // Invalid sync 2 byte, start searching for a preamble byte
        return sempFirstByte(parse, data);
    }

    parse->state = sempUbloxClass;
    return true;
}

// Check for the preamble
bool sempUbloxPreamble(SEMP_PARSE_STATE *parse, uint8_t data)
{
    if (data != 0xb5)
        return false;
    parse->state = sempUbloxSync2;
    return true;
}

// Translates state value into an string, returns nullptr if not found
const char * sempUbloxGetStateName(const SEMP_PARSE_STATE *parse)
{
    if (parse->state == sempUbloxPreamble)
        return "sempUbloxPreamble";
    if (parse->state == sempUbloxSync2)
        return "sempUbloxSync2";
    if (parse->state == sempUbloxClass)
        return "sempUbloxClass";
    if (parse->state == sempUbloxId)
        return "sempUbloxId";
    if (parse->state == sempUbloxLength1)
        return "sempUbloxLength1";
    if (parse->state == sempUbloxLength2)
        return "sempUbloxLength2";
    if (parse->state == sempUbloxPayload)
        return "sempUbloxPayload";
    if (parse->state == sempUbloxCkA)
        return "sempUbloxCkA";
    if (parse->state == sempUbloxCkB)
        return "sempUbloxCkB";
    return nullptr;
}

// Get the message number
uint16_t sempUbloxGetMessageNumber(const SEMP_PARSE_STATE *parse)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;
    return scratchPad->ublox.message;
}
