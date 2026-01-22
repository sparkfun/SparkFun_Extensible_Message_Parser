/*------------------------------------------------------------------------------
Parse_SPARTN.cpp

SPARTN message parsing support routines

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
#include "semp_crc_spartn.h" // 4/8/16/24/32-bit cyclic redundancy checksums for SPARTN parsing

//----------------------------------------
// Types
//----------------------------------------

// SPARTN parser scratch area
typedef struct _SEMP_SPARTN_VALUES
{
    uint16_t frameCount;
    uint16_t crcBytes;
    uint16_t TF007toTF016;

    uint8_t messageType;
    uint16_t payloadLength;
    uint16_t EAF;
    uint8_t crcType;
    uint8_t frameCRC;
    uint8_t messageSubtype;
    uint16_t timeTagType;
    uint16_t authenticationIndicator;
    uint16_t embeddedApplicationLengthBytes;
} SEMP_SPARTN_VALUES;

//------------------------------------------------------------------------------
// SPARTN parse routines
//
// The parser routines are placed in reverse order to define the routine before
// its use and eliminate forward declarations.  Removing the forward declaration
// helps reduce the exposure of the routines to the application layer.  The public
// data structures and routines are listed at the end of the file.
//------------------------------------------------------------------------------

//----------------------------------------
//----------------------------------------
bool sempSpartnReadTF018(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SPARTN_VALUES *scratchPad = (SEMP_SPARTN_VALUES *)parse->scratchPad;

    scratchPad->frameCount++;
    if (scratchPad->frameCount == scratchPad->crcBytes)
    {
        uint16_t numBytes = 4 + scratchPad->TF007toTF016 + scratchPad->payloadLength + scratchPad->embeddedApplicationLengthBytes;
        uint8_t *ptr = &parse->buffer[numBytes];
        bool valid;
        switch (scratchPad->crcType)
        {
        case 0:
        {
            uint8_t expected = *ptr;
            // Don't include the preamble in the CRC
            valid =  ((semp_uSpartnCrc8(&parse->buffer[1], numBytes - 1) == expected)
                      || (parse->badCrc && (!parse->badCrc(parse))));
        }
        break;
        case 1:
        {
            uint16_t expected = *ptr++;
            expected <<= 8;
            expected |= *ptr;
            // Don't include the preamble in the CRC
            valid =  ((semp_uSpartnCrc16(&parse->buffer[1], numBytes - 1) == expected)
                      || (parse->badCrc && (!parse->badCrc(parse))));
        }
        break;
        case 2:
        {
            uint32_t expected = *ptr++;
            expected <<= 8;
            expected |= *ptr++;
            expected <<= 8;
            expected |= *ptr;
            // Don't include the preamble in the CRC
            valid =  ((semp_uSpartnCrc24(&parse->buffer[1], numBytes - 1) == expected)
                      || (parse->badCrc && (!parse->badCrc(parse))));
        }
        break;
        default:
        {
            uint32_t expected = *ptr++;
            expected <<= 8;
            expected |= *ptr++;
            expected <<= 8;
            expected |= *ptr++;
            expected <<= 8;
            expected |= *ptr;
            // Don't include the preamble in the CRC
            valid =  ((semp_uSpartnCrc32(&parse->buffer[1], numBytes - 1) == expected)
                      || (parse->badCrc && (!parse->badCrc(parse))));
        }
        break;
        }
        if (valid)
            parse->eomCallback(parse, parse->type); // Pass parser array index
        else
            sempPrintf(parse->printDebug,
                    "SEMP %s: SPARTN %d %d, 0x%04x (%d) bytes, bad CRC",
                    parse->parserName,
                    scratchPad->messageType,
                    scratchPad->messageSubtype,
                    parse->length, parse->length);
        parse->state = sempFirstByte;
        return false;
    }

    return true;
}

//----------------------------------------
//----------------------------------------
bool sempSpartnReadTF017(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SPARTN_VALUES *scratchPad = (SEMP_SPARTN_VALUES *)parse->scratchPad;

    scratchPad->frameCount++;
    if (scratchPad->frameCount == scratchPad->embeddedApplicationLengthBytes)
    {
        parse->state = sempSpartnReadTF018;
        scratchPad->frameCount = 0;
    }

    return true;
}

//----------------------------------------
//----------------------------------------
bool sempSpartnReadTF016(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SPARTN_VALUES *scratchPad = (SEMP_SPARTN_VALUES *)parse->scratchPad;

    scratchPad->frameCount++;
    if (scratchPad->frameCount == scratchPad->payloadLength)
    {
        if (scratchPad->embeddedApplicationLengthBytes > 0)
        {
            parse->state = sempSpartnReadTF017;
            scratchPad->frameCount = 0;
        }
        else
        {
            parse->state = sempSpartnReadTF018;
            scratchPad->frameCount = 0;
        }
    }

    return true;
}

//----------------------------------------
//----------------------------------------
bool sempSpartnReadTF009(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SPARTN_VALUES *scratchPad = (SEMP_SPARTN_VALUES *)parse->scratchPad;

    scratchPad->frameCount++;
    if (scratchPad->frameCount == scratchPad->TF007toTF016)
    {
        if (scratchPad->EAF == 0)
        {
            scratchPad->authenticationIndicator = 0;
            scratchPad->embeddedApplicationLengthBytes = 0;
        }
        else
        {
            scratchPad->authenticationIndicator = (data >> 3) & 0x07;
            if (scratchPad->authenticationIndicator <= 1)
                scratchPad->embeddedApplicationLengthBytes = 0;
            else
            {
                switch (data & 0x07)
                {
                case 0:
                    scratchPad->embeddedApplicationLengthBytes = 8; // 64 bits
                    break;
                case 1:
                    scratchPad->embeddedApplicationLengthBytes = 12; // 96 bits
                    break;
                case 2:
                    scratchPad->embeddedApplicationLengthBytes = 16; // 128 bits
                    break;
                case 3:
                    scratchPad->embeddedApplicationLengthBytes = 32; // 256 bits
                    break;
                default:
                    scratchPad->embeddedApplicationLengthBytes = 64; // 512 / TBD bits
                    break;
                }
            }
        }
        scratchPad->frameCount = 0;
        parse->state = sempSpartnReadTF016;
    }

    return true;
}

//----------------------------------------
//----------------------------------------
bool sempSpartnReadTF007(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SPARTN_VALUES *scratchPad = (SEMP_SPARTN_VALUES *)parse->scratchPad;

    scratchPad->messageSubtype = data >> 4;
    scratchPad->timeTagType = (data >> 3) & 0x01;
    if (scratchPad->timeTagType == 0)
        scratchPad->TF007toTF016 = 4;
    else
        scratchPad->TF007toTF016 = 6;
    if (scratchPad->EAF > 0)
        scratchPad->TF007toTF016 += 2;
    scratchPad->frameCount = 1;

    parse->state = sempSpartnReadTF009;

    return true;
}

//----------------------------------------
//----------------------------------------
bool sempSpartnReadTF002TF006(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SPARTN_VALUES *scratchPad = (SEMP_SPARTN_VALUES *)parse->scratchPad;

    if (scratchPad->frameCount == 0)
    {
        scratchPad->messageType = data >> 1;
        scratchPad->payloadLength = data & 0x01;
    }
    if (scratchPad->frameCount == 1)
    {
        scratchPad->payloadLength <<= 8;
        scratchPad->payloadLength |= data;
    }
    if (scratchPad->frameCount == 2)
    {
        scratchPad->payloadLength <<= 1;
        scratchPad->payloadLength |= data >> 7;
        scratchPad->EAF = (data >> 6) & 0x01;
        scratchPad->crcType = (data >> 4) & 0x03;
        switch (scratchPad->crcType)
        {
        case 0:
            scratchPad->crcBytes = 1;
            break;
        case 1:
            scratchPad->crcBytes = 2;
            break;
        case 2:
            scratchPad->crcBytes = 3;
            break;
        default:
            scratchPad->crcBytes = 4;
            break;
        }
        scratchPad->frameCRC = data & 0x0F;
        parse->buffer[3] = parse->buffer[3] & 0xF0; // Zero the 4 LSBs before calculating the CRC
        if (semp_uSpartnCrc4(&parse->buffer[1], 3) == scratchPad->frameCRC)
        {
            parse->buffer[3] = data; // Restore TF005 and TF006 now we know the data is valid
            if (parse->verboseDebug)
                sempPrintf(parse->printDebug,
                        "SEMP %s: Incoming SPARTN %d %d, 0x%04x (%d) bytes",
                        parse->parserName,
                        scratchPad->messageType,
                        scratchPad->messageSubtype,
                        scratchPad->payloadLength, scratchPad->payloadLength);
            parse->state = sempSpartnReadTF007;
        }
        else
        {
            // Invalid header CRC
            parse->buffer[3] = data; // Restore the byte now we know the data is invalid
            parse->state = sempFirstByte;

            sempPrintf(parse->printDebug,
                    "SEMP %s: SPARTN %d, 0x%04x (%d) bytes, bad header CRC",
                    parse->parserName,
                    scratchPad->messageType,
                    parse->length, parse->length);

            return false;
        }
    }

    scratchPad->frameCount++;
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
//   Returns true if the SPARTN parser recgonizes the input and false
//   if another parser should be used
//----------------------------------------
bool sempSpartnPreamble(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SPARTN_VALUES *scratchPad = (SEMP_SPARTN_VALUES *)parse->scratchPad;

    if (data == 0x73)
    {
        scratchPad->frameCount = 0;
        parse->state = sempSpartnReadTF002TF006;
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
const char * sempSpartnGetStateName(const SEMP_PARSE_STATE *parse)
{
    if (parse->state == sempSpartnPreamble)
        return "sempSpartnPreamble";
    if (parse->state == sempSpartnReadTF002TF006)
        return "sempSpartnReadTF002TF006";
    if (parse->state == sempSpartnReadTF007)
        return "sempSpartnReadTF007";
    if (parse->state == sempSpartnReadTF009)
        return "sempSpartnReadTF009";
    if (parse->state == sempSpartnReadTF016)
        return "sempSpartnReadTF016";
    if (parse->state == sempSpartnReadTF017)
        return "sempSpartnReadTF017";
    if (parse->state == sempSpartnReadTF018)
        return "sempSpartnReadTF018";
    return nullptr;
}

//----------------------------------------
// Get the message number
//----------------------------------------
uint8_t sempSpartnGetMessageType(const SEMP_PARSE_STATE *parse)
{
    SEMP_SPARTN_VALUES *scratchPad = (SEMP_SPARTN_VALUES *)parse->scratchPad;
    return scratchPad->messageType;
}

//----------------------------------------
// Get the message subtype number
//----------------------------------------
uint8_t sempSpartnGetMessageSubType(const SEMP_PARSE_STATE *parse)
{
    SEMP_SPARTN_VALUES *scratchPad = (SEMP_SPARTN_VALUES *)parse->scratchPad;
    return scratchPad->messageSubtype;
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
SEMP_PARSER_DESCRIPTION sempSpartnParserDescription =
{
    "SPARTN parser",            // parserName
    sempSpartnPreamble,         // preamble
    sizeof(SEMP_SPARTN_VALUES), // scratchPadBytes
};
