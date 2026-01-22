/*------------------------------------------------------------------------------
Parse_SBF.cpp

Septentrio SBF message parsing support routines

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
#include "semp_crc_sbf.h" // CCITT cyclic redundancy checksum for SBF parsing

//----------------------------------------
// Types
//----------------------------------------

// SBF parser scratch area
typedef struct _SEMP_SBF_VALUES
{
    uint16_t expectedCRC;
    uint16_t computedCRC;
    uint16_t sbfID = 0;
    uint8_t sbfIDrev = 0;
    uint16_t length;
    uint16_t bytesRemaining;
    // Invalid data callback routine (parser-specific)
    SEMP_INVALID_DATA_CALLBACK invalidDataCallback = (SEMP_INVALID_DATA_CALLBACK)nullptr;
} SEMP_SBF_VALUES;

//------------------------------------------------------------------------------
// SBF parse routines
//
// The parser routines are placed in reverse order to define the routine before
// its use and eliminate forward declarations.  Removing the forward declaration
// helps reduce the exposure of the routines to the application layer.  The public
// data structures and routines are listed at the end of the file.
//------------------------------------------------------------------------------

//----------------------------------------
//----------------------------------------
bool sempSbfReadBytes(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SBF_VALUES *scratchPad = (SEMP_SBF_VALUES *)parse->scratchPad;

    scratchPad->computedCRC = semp_ccitt_crc_update(scratchPad->computedCRC, data);

    scratchPad->bytesRemaining--;

    if (scratchPad->bytesRemaining == 0)
    {
        parse->state = sempFirstByte;

        if ((scratchPad->computedCRC == scratchPad->expectedCRC)
            || (parse->badCrc && (!parse->badCrc(parse))))
        {
            parse->eomCallback(parse, parse->type); // Pass parser array index
        }
        else
        {
            sempPrintf(parse->printDebug,
                    "SEMP %s: SBF %d, 0x%04x (%d) bytes, bad CRC",
                    parse->parserName,
                    scratchPad->sbfID,
                    parse->length, parse->length);

            if (scratchPad->invalidDataCallback)
                scratchPad->invalidDataCallback(parse);
        }

        return false;
    }

    return true;
}

//----------------------------------------
// Check for Length MSB
//----------------------------------------
bool sempSbfLengthMSB(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SBF_VALUES *scratchPad = (SEMP_SBF_VALUES *)parse->scratchPad;

    scratchPad->computedCRC = semp_ccitt_crc_update(scratchPad->computedCRC, data);

    scratchPad->length |= ((uint16_t)data) << 8;

    if (scratchPad->length % 4 == 0)
    {
        scratchPad->bytesRemaining = scratchPad->length - 8; // Subtract 8 header bytes
        parse->state = sempSbfReadBytes;
        if (parse->verboseDebug)
            sempPrintf(parse->printDebug,
                      "SEMP %s: Incoming SBF %d, 0x%04x (%d) bytes",
                      parse->parserName,
                      scratchPad->sbfID,
                      scratchPad->bytesRemaining, scratchPad->bytesRemaining);
        return true;
    }
    // else
    sempPrintf(parse->printDebug,
            "SEMP %s: SBF, 0x%04x (%d) bytes, length not modulo 4",
            parse->parserName,
            parse->length, parse->length);

    if (scratchPad->invalidDataCallback)
        scratchPad->invalidDataCallback(parse);

    parse->state = sempFirstByte;
    return false;
}

//----------------------------------------
// Check for Length LSB
//----------------------------------------
bool sempSbfLengthLSB(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SBF_VALUES *scratchPad = (SEMP_SBF_VALUES *)parse->scratchPad;

    scratchPad->computedCRC = semp_ccitt_crc_update(scratchPad->computedCRC, data);

    scratchPad->length = data;

    parse->state = sempSbfLengthMSB;
    return true;
}

//----------------------------------------
// Check for ID byte 2
//----------------------------------------
bool sempSbfID2(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SBF_VALUES *scratchPad = (SEMP_SBF_VALUES *)parse->scratchPad;

    scratchPad->computedCRC = semp_ccitt_crc_update(scratchPad->computedCRC, data);

    scratchPad->sbfID |= ((uint16_t)data) << 8;
    scratchPad->sbfID &= 0x1FFF; // Limit ID to 13 bits
    scratchPad->sbfIDrev = data >> 5; // Limit ID revision to 3 bits

    parse->state = sempSbfLengthLSB;
    return true;
}

//----------------------------------------
// Check for ID byte 1
//----------------------------------------
bool sempSbfID1(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SBF_VALUES *scratchPad = (SEMP_SBF_VALUES *)parse->scratchPad;

    scratchPad->computedCRC = semp_ccitt_crc_update(scratchPad->computedCRC, data);

    scratchPad->sbfID = data;

    parse->state = sempSbfID2;
    return true;
}

//----------------------------------------
// Check for CRC byte 2
//----------------------------------------
bool sempSbfCRC2(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SBF_VALUES *scratchPad = (SEMP_SBF_VALUES *)parse->scratchPad;

    scratchPad->expectedCRC |= ((uint16_t)data) << 8;
    scratchPad->computedCRC = 0;

    parse->state = sempSbfID1;
    return true;
}

//----------------------------------------
// Check for CRC byte 1
//----------------------------------------
bool sempSbfCRC1(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SBF_VALUES *scratchPad = (SEMP_SBF_VALUES *)parse->scratchPad;

    scratchPad->expectedCRC = data;

    parse->state = sempSbfCRC2;
    return true;
}

//----------------------------------------
// Check for preamble 2
//----------------------------------------
bool sempSbfPreamble2(SEMP_PARSE_STATE *parse, uint8_t data)
{
    if (data == 0x40) // @
    {
        parse->state = sempSbfCRC1;
        return true;
    }

    // else
    sempPrintf(parse->printDebug,
            "SEMP %s: SBF, 0x%04x (%d) bytes, invalid preamble2",
            parse->parserName,
            parse->length, parse->length);

    SEMP_SBF_VALUES *scratchPad = (SEMP_SBF_VALUES *)parse->scratchPad;
    if (scratchPad->invalidDataCallback)
        scratchPad->invalidDataCallback(parse);

    parse->state = sempFirstByte;
    return false;
}

//----------------------------------------
// Check for the preamble
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//   data: First data byte in the stream of data to parse
//
// Outputs:
//   Returns true if the SBF parser recgonizes the input and false
//   if another parser should be used
//----------------------------------------
bool sempSbfPreamble(SEMP_PARSE_STATE *parse, uint8_t data)
{
    if (data == 0x24) // $ - same as NMEA
    {
        parse->state = sempSbfPreamble2;
        return true;
    }
    // else
    SEMP_SBF_VALUES *scratchPad = (SEMP_SBF_VALUES *)parse->scratchPad;
    if (scratchPad->invalidDataCallback)
        scratchPad->invalidDataCallback(parse);
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
const char * sempSbfGetStateName(const SEMP_PARSE_STATE *parse)
{
    if (parse->state == sempSbfPreamble)
        return "sempSbfPreamble";
    if (parse->state == sempSbfPreamble2)
        return "sempSbfPreamble2";
    if (parse->state == sempSbfCRC1)
        return "sempSbfCRC1";
    if (parse->state == sempSbfCRC2)
        return "sempSbfCRC2";
    if (parse->state == sempSbfID1)
        return "sempSbfID1";
    if (parse->state == sempSbfID2)
        return "sempSbfID2";
    if (parse->state == sempSbfLengthLSB)
        return "sempSbfLengthLSB";
    if (parse->state == sempSbfLengthMSB)
        return "sempSbfLengthMSB";
    if (parse->state == sempSbfReadBytes)
        return "sempSbfReadBytes";
    return nullptr;
}

//----------------------------------------
// Set the invalid data callback
//----------------------------------------
void sempSbfSetInvalidDataCallback(const SEMP_PARSE_STATE *parse, SEMP_INVALID_DATA_CALLBACK invalidDataCallback)
{
    SEMP_SBF_VALUES *scratchPad = (SEMP_SBF_VALUES *)parse->scratchPad;
    scratchPad->invalidDataCallback = invalidDataCallback;
}

//----------------------------------------
// Get the Block Number
//----------------------------------------
uint16_t sempSbfGetBlockNumber(const SEMP_PARSE_STATE *parse)
{
    SEMP_SBF_VALUES *scratchPad = (SEMP_SBF_VALUES *)parse->scratchPad;
    return scratchPad->sbfID;
}

//----------------------------------------
// Get the Block Revision
//----------------------------------------
uint8_t sempSbfGetBlockRevision(const SEMP_PARSE_STATE *parse)
{
    SEMP_SBF_VALUES *scratchPad = (SEMP_SBF_VALUES *)parse->scratchPad;
    return scratchPad->sbfIDrev;
}

//----------------------------------------
//----------------------------------------
uint8_t sempSbfGetU1(const SEMP_PARSE_STATE *parse, size_t offset)
{
    return parse->buffer[offset];
}

//----------------------------------------
//----------------------------------------
uint16_t sempSbfGetU2(const SEMP_PARSE_STATE *parse, size_t offset)
{
    uint16_t data = parse->buffer[offset];
    data |= ((uint16_t)parse->buffer[offset + 1]) << 8;
    return data;
}

//----------------------------------------
//----------------------------------------
uint32_t sempSbfGetU4(const SEMP_PARSE_STATE *parse, size_t offset)
{
    uint32_t data = 0;
    for (uint16_t i = 0; i < sizeof(data); i++)
        data |= ((uint32_t)parse->buffer[offset + i]) << (8 * i);
    return data;
}

//----------------------------------------
//----------------------------------------
uint64_t sempSbfGetU8(const SEMP_PARSE_STATE *parse, size_t offset)
{
    uint64_t data = 0;
    for (uint16_t i = 0; i < sizeof(data); i++)
        data |= ((uint64_t)parse->buffer[offset + i]) << (8 * i);
    return data;
}

//----------------------------------------
//----------------------------------------
int8_t sempSbfGetI1(const SEMP_PARSE_STATE *parse, size_t offset)
{
    union {
        uint8_t unsignedN;
        int8_t signedN;
    } unsignedSignedN;
    unsignedSignedN.unsignedN = sempSbfGetU1(parse, offset);
    return unsignedSignedN.signedN;
}

//----------------------------------------
//----------------------------------------
int16_t sempSbfGetI2(const SEMP_PARSE_STATE *parse, size_t offset)
{
    union {
        uint16_t unsignedN;
        int16_t signedN;
    } unsignedSignedN;
    unsignedSignedN.unsignedN = sempSbfGetU2(parse, offset);
    return unsignedSignedN.signedN;
}

//----------------------------------------
//----------------------------------------
int32_t sempSbfGetI4(const SEMP_PARSE_STATE *parse, size_t offset)
{
    union {
        uint32_t unsignedN;
        int32_t signedN;
    } unsignedSignedN;
    unsignedSignedN.unsignedN = sempSbfGetU4(parse, offset);
    return unsignedSignedN.signedN;
}

//----------------------------------------
//----------------------------------------
int64_t sempSbfGetI8(const SEMP_PARSE_STATE *parse, size_t offset)
{
    union {
        uint64_t unsignedN;
        int64_t signedN;
    } unsignedSignedN;
    unsignedSignedN.unsignedN = sempSbfGetU8(parse, offset);
    return unsignedSignedN.signedN;
}

//----------------------------------------
// Get the ID value
//----------------------------------------
uint16_t sempSbfGetId(const SEMP_PARSE_STATE *parse)
{
    SEMP_SBF_VALUES * scratchPad = (SEMP_SBF_VALUES *)parse->scratchPad;
    return scratchPad->sbfID;
}

//----------------------------------------
//----------------------------------------
float sempSbfGetF4(const SEMP_PARSE_STATE *parse, size_t offset)
{
    union {
        uint32_t unsignedN;
        float flt;
    } unsignedFloat;
    unsignedFloat.unsignedN = sempSbfGetU4(parse, offset);
    return unsignedFloat.flt;
}

//----------------------------------------
//----------------------------------------
double sempSbfGetF8(const SEMP_PARSE_STATE *parse, size_t offset)
{
    union {
        uint64_t unsignedN;
        double flt;
    } unsignedFloat;
    unsignedFloat.unsignedN = sempSbfGetU8(parse, offset);
    return unsignedFloat.flt;
}

//----------------------------------------
//----------------------------------------
const char *sempSbfGetString(const SEMP_PARSE_STATE *parse, size_t offset)
{
    return (const char *)(&parse->buffer[offset]);
}

//----------------------------------------
//----------------------------------------
bool sempSbfIsEncapsulatedNMEA(const SEMP_PARSE_STATE *parse)
{
    SEMP_SBF_VALUES *scratchPad = (SEMP_SBF_VALUES *)parse->scratchPad;
    return ((scratchPad->sbfID == 4097) && (parse->buffer[14] == 4));
}

//----------------------------------------
//----------------------------------------
bool sempSbfIsEncapsulatedRTCMv3(const SEMP_PARSE_STATE *parse)
{
    SEMP_SBF_VALUES *scratchPad = (SEMP_SBF_VALUES *)parse->scratchPad;
    return ((scratchPad->sbfID == 4097) && (parse->buffer[14] == 2));
}

//----------------------------------------
//----------------------------------------
uint16_t sempSbfGetEncapsulatedPayloadLength(const SEMP_PARSE_STATE *parse)
{
    return sempSbfGetU2(parse, 16);
}

//----------------------------------------
//----------------------------------------
const uint8_t *sempSbfGetEncapsulatedPayload(const SEMP_PARSE_STATE *parse)
{
    return (const uint8_t *)(sempSbfGetString(parse, 20));
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
SEMP_PARSER_DESCRIPTION sempSbfParserDescription =
{
    "SBF parser",               // parserName
    sempSbfPreamble,            // preamble
    sizeof(SEMP_SBF_VALUES),    // scratchPadBytes
};
