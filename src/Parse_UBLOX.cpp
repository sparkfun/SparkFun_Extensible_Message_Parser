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
// Constants
//----------------------------------------

// UBLOX payload offset
#define SEMP_UBLOX_PAYLOAD_OFFSET   6

//----------------------------------------
// Types
//----------------------------------------

// UBLOX parser scratch area
typedef struct _SEMP_UBLOX_VALUES
{
    uint16_t bytesRemaining; // Bytes remaining in field
    uint8_t messageClass;    // Message Class
    uint8_t messageId;       // Message ID
    uint16_t payloadLength;  // Payload length
    uint8_t ck_a;            // U-blox checksum byte 1
    uint8_t ck_b;            // U-blox checksum byte 2
} SEMP_UBLOX_VALUES;

//------------------------------------------------------------------------------
// U-BLOX parse routines
//
// The parser routines are placed in reverse order to define the routine before
// its use and eliminate forward declarations.  Removing the forward declaration
// helps reduce the exposure of the routines to the application layer.  The public
// data structures and routines are listed at the end of the file.
//------------------------------------------------------------------------------

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

//----------------------------------------
// Read the CK_B byte
//----------------------------------------
bool sempUbloxCkB(SEMP_PARSE_STATE *parse, uint8_t data)
{
    bool badChecksum;
    SEMP_UBLOX_VALUES *scratchPad = (SEMP_UBLOX_VALUES *)parse->scratchPad;

    // Validate the checksum
    badChecksum =
        ((parse->buffer[parse->length - 2] != scratchPad->ck_a) || (parse->buffer[parse->length - 1] != scratchPad->ck_b));

    // Process this message if checksum is valid
    if ((badChecksum == false) || (parse->badCrc && (!parse->badCrc(parse))))
        parse->eomCallback(parse, parse->type); // Pass parser array index
    else
        sempPrintf(parse->printDebug,
                   "SEMP %s: UBLOX bad checksum received 0x%02x%02x computed 0x%02x%02x",
                   parse->parserName,
                   parse->buffer[parse->length - 2], parse->buffer[parse->length - 1],
                   scratchPad->ck_a, scratchPad->ck_b);

    // Search for the next preamble byte
    parse->length = 0;
    parse->state = sempFirstByte;
    return false;
}

//----------------------------------------
// Read the CK_A byte
//----------------------------------------
bool sempUbloxCkA(SEMP_PARSE_STATE *parse, uint8_t data)
{
    parse->state = sempUbloxCkB;
    return true;
}

//----------------------------------------
// Read the payload
//----------------------------------------
bool sempUbloxPayload(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_UBLOX_VALUES *scratchPad = (SEMP_UBLOX_VALUES *)parse->scratchPad;

    // Compute the checksum over the payload
    if (scratchPad->bytesRemaining--)
    {
        // Calculate the checksum
        scratchPad->ck_a += data;
        scratchPad->ck_b += scratchPad->ck_a;
        return true;
    }
    return sempUbloxCkA(parse, data);
}

//----------------------------------------
// Read the second length byte
//----------------------------------------
bool sempUbloxLength2(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_UBLOX_VALUES *scratchPad = (SEMP_UBLOX_VALUES *)parse->scratchPad;

    // Calculate the checksum
    scratchPad->ck_a += data;
    scratchPad->ck_b += scratchPad->ck_a;

    // Save the second length byte
    scratchPad->bytesRemaining |= ((uint16_t)data) << 8;
    scratchPad->payloadLength = scratchPad->bytesRemaining;
    if (scratchPad->bytesRemaining == 0) // Handle zero length messages - e.g. UBX-UPD
        parse->state = sempUbloxCkA; // Jump to CRC
    else
    {
        if (parse->verboseDebug)
            sempPrintf(parse->printDebug,
                       "SEMP %s: Incoming UBLOX 0x%02X:0x%02X, 0x%04x (%d) bytes",
                       parse->parserName,
                       scratchPad->messageClass, scratchPad->messageId,
                       scratchPad->payloadLength, scratchPad->payloadLength);
        parse->state = sempUbloxPayload;
    }
    return true;
}

//----------------------------------------
// Read the first length byte
//----------------------------------------
bool sempUbloxLength1(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_UBLOX_VALUES *scratchPad = (SEMP_UBLOX_VALUES *)parse->scratchPad;

    // Calculate the checksum
    scratchPad->ck_a += data;
    scratchPad->ck_b += scratchPad->ck_a;

    // Save the first length byte
    scratchPad->bytesRemaining = data;
    parse->state = sempUbloxLength2;
    return true;
}

//----------------------------------------
// Read the ID byte
//----------------------------------------
bool sempUbloxId(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_UBLOX_VALUES *scratchPad = (SEMP_UBLOX_VALUES *)parse->scratchPad;

    // Calculate the checksum
    scratchPad->ck_a += data;
    scratchPad->ck_b += scratchPad->ck_a;

    scratchPad->messageId = data; // Save the ID
    parse->state = sempUbloxLength1;
    return true;
}

//----------------------------------------
// Read the class byte
//----------------------------------------
bool sempUbloxClass(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_UBLOX_VALUES *scratchPad = (SEMP_UBLOX_VALUES *)parse->scratchPad;

    // Start the checksum calculation
    scratchPad->ck_a = data;
    scratchPad->ck_b = data;

    scratchPad->messageClass = data; // Save the Class
    parse->state = sempUbloxId;
    return true;
}

//----------------------------------------
// Read the second sync byte
//----------------------------------------
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

//----------------------------------------
// Check for the preamble
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//   data: First data byte in the stream of data to parse
//
// Outputs:
//   Returns true if the UBLOX parser recgonizes the input and false
//   if another parser should be used
//----------------------------------------
bool sempUbloxPreamble(SEMP_PARSE_STATE *parse, uint8_t data)
{
    if (data != 0xb5)
        return false;
    parse->state = sempUbloxSync2;
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

//------------------------------------------------------------------------------
// Public data and routines
//
// The following data structures and routines are listed in the .h file and are
// exposed to the SEMP routine and application layer.
//------------------------------------------------------------------------------

//----------------------------------------
// Describe the parser
//----------------------------------------
SEMP_PARSER_DESCRIPTION sempUbloxParserDescription =
{
    "U-Blox parser",            // parserName
    sempUbloxPreamble,          // preamble
    sempUbloxGetStateName,      // State to state name translation routine
    sizeof(SEMP_UBLOX_VALUES),  // scratchPadBytes
    SEMP_UBLOX_PAYLOAD_OFFSET,  // payloadOffset
};

//----------------------------------------
// Get the message Class
//----------------------------------------
uint8_t sempUbloxGetMessageClass(const SEMP_PARSE_STATE *parse)
{
    SEMP_UBLOX_VALUES *scratchPad = (SEMP_UBLOX_VALUES *)parse->scratchPad;
    return scratchPad->messageClass;
}

//----------------------------------------
// Get the message ID
//----------------------------------------
uint8_t sempUbloxGetMessageId(const SEMP_PARSE_STATE *parse)
{
    SEMP_UBLOX_VALUES *scratchPad = (SEMP_UBLOX_VALUES *)parse->scratchPad;
    return scratchPad->messageId;
}

//----------------------------------------
// Get the message number: |- Class (8 bits) -||- ID (8 bits) -|
//----------------------------------------
uint16_t sempUbloxGetMessageNumber(const SEMP_PARSE_STATE *parse)
{
    SEMP_UBLOX_VALUES *scratchPad = (SEMP_UBLOX_VALUES *)parse->scratchPad;
    uint16_t message = ((uint16_t)scratchPad->messageClass) << 8;
    message |= (uint16_t)scratchPad->messageId;
    return message;
}

//----------------------------------------
// Get the Payload Length
//----------------------------------------
size_t sempUbloxGetPayloadLength(const SEMP_PARSE_STATE *parse)
{
    SEMP_UBLOX_VALUES *scratchPad = (SEMP_UBLOX_VALUES *)parse->scratchPad;
    return scratchPad->payloadLength;
}
