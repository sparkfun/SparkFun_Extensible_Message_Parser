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
            SEMP_OUTPUT output = parse->debugOutput;
            if (output)
            {
                sempPrintString(output, "SEMP ");
                sempPrintString(output, parse->parserName);
                sempPrintString(output, ": SBF ");
                sempPrintDecimalI32(output, scratchPad->sbfID);
                sempPrintString(output, ", ");
                sempPrintHex0x04x(output, parse->length);
                sempPrintString(output, " (");
                sempPrintDecimalU32(output, parse->length);
                sempPrintStringLn(output, ") bytes, bad CRC");
            }
            sempInvalidDataCallback(parse);
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
    SEMP_OUTPUT output = parse->debugOutput;
    SEMP_SBF_VALUES *scratchPad = (SEMP_SBF_VALUES *)parse->scratchPad;

    scratchPad->computedCRC = semp_ccitt_crc_update(scratchPad->computedCRC, data);

    scratchPad->length |= ((uint16_t)data) << 8;

    if (scratchPad->length % 4 == 0)
    {
        scratchPad->bytesRemaining = scratchPad->length - 8; // Subtract 8 header bytes
        parse->state = sempSbfReadBytes;
        if (parse->verboseDebug && output)
        {
            sempPrintString(output, "SEMP ");
            sempPrintString(output, parse->parserName);
            sempPrintString(output, ": Incoming SBF ");
            sempPrintDecimalI32(output, scratchPad->sbfID);
            sempPrintString(output, ", ");
            sempPrintHex0x04x(output, scratchPad->bytesRemaining);
            sempPrintString(output, " (");
            sempPrintDecimalU32(output, scratchPad->bytesRemaining);
            sempPrintStringLn(output, ") bytes");
        }
        return true;
    }

    // else
    if (output)
    {
        sempPrintString(output, "SEMP ");
        sempPrintString(output, parse->parserName);
        sempPrintString(output, ": SBF, ");
        sempPrintHex0x04x(output, parse->length);
        sempPrintString(output, " (");
        sempPrintDecimalU32(output, parse->length);
        sempPrintStringLn(output, ") bytes, length not modulo 4");
    }
    sempInvalidDataCallback(parse);
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
    SEMP_OUTPUT output = parse->debugOutput;
    if (output)
    {
        sempPrintString(output, "SEMP ");
        sempPrintString(output, parse->parserName);
        sempPrintString(output, ": SBF, ");
        sempPrintHex0x04x(output, parse->length);
        sempPrintString(output, " (");
        sempPrintDecimalU32(output, parse->length);
        sempPrintStringLn(output, ") bytes, invalid preamble2");
    }

    SEMP_SBF_VALUES *scratchPad = (SEMP_SBF_VALUES *)parse->scratchPad;
    sempInvalidDataCallback(parse);
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
// Display the contents of the scratch pad
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//   output: Address of a routine to output a character
//----------------------------------------
void sempSbfPrintScratchPad(SEMP_PARSE_STATE *parse, SEMP_OUTPUT output)
{
    SEMP_SBF_VALUES *scratchPad;

    // Get the scratch pad address
    scratchPad = (SEMP_SBF_VALUES *)parse->scratchPad;

    // Display the expected CRC
    sempPrintString(output, "    expectedCRC: ");
    sempPrintHex0x04xLn(output, scratchPad->expectedCRC);

    // Display the computed CRC
    sempPrintString(output, "    computedCRC: ");
    sempPrintHex0x04xLn(output, scratchPad->computedCRC);

    // Display the SBF ID
    sempPrintString(output, "    sbfID: ");
    sempPrintDecimalU32Ln(output, scratchPad->sbfID);

    // Display the SBF Drev
    sempPrintString(output, "    sbfIDrev: ");
    sempPrintDecimalU32Ln(output, scratchPad->sbfIDrev);

    // Display the length
    sempPrintString(output, "    length: ");
    sempPrintDecimalU32Ln(output, scratchPad->length);

    // Display the remaining bytes
    sempPrintString(output, "    bytesRemaining: ");
    sempPrintDecimalU32Ln(output, scratchPad->bytesRemaining);
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
    sempSbfGetStateName,        // State to state name translation routine
    sempSbfPrintScratchPad,     // Print the contents of the scratch pad
    3000,   /* ??? */           // minimumParseAreaBytes
    sizeof(SEMP_SBF_VALUES),    // scratchPadBytes
    0,                          // payloadOffset
};

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
const uint8_t *sempSbfGetEncapsulatedPayload(const SEMP_PARSE_STATE *parse)
{
    return (const uint8_t *)(sempSbfGetString(parse, 20));
}

//----------------------------------------
//----------------------------------------
uint16_t sempSbfGetEncapsulatedPayloadLength(const SEMP_PARSE_STATE *parse)
{
    return sempSbfGetU2(parse, 16);
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
// Get the length
//----------------------------------------
uint16_t sempSbfGetLength(const SEMP_PARSE_STATE *parse)
{
    SEMP_SBF_VALUES * scratchPad = (SEMP_SBF_VALUES *)parse->scratchPad;
    return scratchPad->length;
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
