/*
  SparkFun SBF test example sketch

  This example demonstrates how to parse a Septentrio SBF data stream

  License: MIT. Please see LICENSE.md for more details
*/

#include <SparkFun_Extensible_Message_Parser.h> //http://librarymanager/All#SparkFun_Extensible_Message_Parser

#include "../Common/output.ino"
#include "../Common/dumpBuffer.ino"
#include "../Common/reportFatalError.ino"

//----------------------------------------
// Constants
//----------------------------------------

// Build the table listing all of the parsers
SEMP_PARSER_DESCRIPTION * parserTable[] =
{
    &sempSbfParserDescription
};
const int parserCount = sizeof(parserTable) / sizeof(parserTable[0]);

const char * const parserNames[] =
{
    "SBF parser"
};
const int parserNameCount = sizeof(parserNames) / sizeof(parserNames[0]);

// Provide some valid and invalid SBF messages
const uint8_t rawDataStream[] =
{
    // Invalid data - must skip over
    0, 1, 2, 3, 4, 5, 6, 7,                         //     0

    // SBF Block 5914 (ReceiverTime)
    0x24, 0x40, 0x06, 0xFE, 0x1A, 0x17, 0x18, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x12, 0x00, 0x00, 0x00,

    // SBF Block 5914 (ReceiverTime) - invalid length
    0x24, 0x40, 0x06, 0xFE, 0x1A, 0x17, 0x17, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x12, 0x00, 0x00, 0x00,

    // SBF Block 5914 (ReceiverTime) - invalid CRC
    0x24, 0x40, 0x06, 0xFE, 0x1A, 0x17, 0x18, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x80, 0x80,
    0xFF, 0x80, 0x80, 0x80, 0x12, 0x00, 0x00, 0x00,

    // Invalid data - must skip over
    0, 1, 2, 3, 4, 5, 6, 7,                         //     0

    // SBF Block 4007 (PVTGeodetic)
    0x24, 0x40, 0xC4, 0x86, 0xA7, 0x4F, 0x60, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x20, 0x5F, 0xA0, 0x12, 0xC2, 0x00, 0x00, 0x00, 0x20, 0x5F, 0xA0, 0x12, 0xC2,
    0x00, 0x00, 0x00, 0x20, 0x5F, 0xA0, 0x12, 0xC2, 0xF9, 0x02, 0x95, 0xD0, 0xF9, 0x02, 0x95, 0xD0,
    0xF9, 0x02, 0x95, 0xD0, 0xF9, 0x02, 0x95, 0xD0, 0xF9, 0x02, 0x95, 0xD0, 0x00, 0x00, 0x00, 0x20,
    0x5F, 0xA0, 0x12, 0xC2, 0xF9, 0x02, 0x95, 0xD0, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00,

};

// Number of bytes in the rawDataStream
#define RAW_DATA_BYTES      (sizeof(rawDataStream) / sizeof(rawDataStream[0]))

//----------------------------------------
// Locals
//----------------------------------------

// Account for the largest SBF messages
uint8_t buffer[3088];
uint32_t dataOffset;
SEMP_PARSE_STATE *parse;

//------------------------------------------------------------------------------
// Test routines
//------------------------------------------------------------------------------

//----------------------------------------
// Application entry point used to initialize the system
//----------------------------------------
void setup()
{
    size_t bufferLength;

    delay(1000);

    Serial.begin(115200);
    sempPrintLn(output);
    sempPrintStringLn(output, "SBF_Test example sketch");
    sempPrintLn(output);

    // Verify the buffer size
    bufferLength = sempGetBufferLength(parserTable, parserCount);
    if (sizeof(buffer) < bufferLength)
    {
        sempPrintString(output, "Set buffer size to >= ");
        sempPrintDecimalI32Ln(output, bufferLength);
        reportFatalError("Fix the buffer size!");
    }

    // Initialize the parser
    parse = sempBeginParser("SBF_Test", parserTable, parserCount,
                            buffer, bufferLength, processMessage, output);
    if (!parse)
        reportFatalError("Failed to initialize the parser");

    // Obtain a raw data stream from somewhere
    sempPrintString(output, "Raw data stream: ");
    sempPrintDecimalI32(output, RAW_DATA_BYTES);
    sempPrintStringLn(output, " bytes");

    // The raw data stream is passed to the parser one byte at a time
    sempDebugOutputEnable(parse);
    for (dataOffset = 0; dataOffset < RAW_DATA_BYTES; dataOffset++)
        // Update the parser state based on the incoming byte
        sempParseNextByte(parse, rawDataStream[dataOffset]);

    // Done parsing the data
    sempStopParser(&parse);
    sempPrintStringLn(output, "All done");
}

//----------------------------------------
// Main loop processing, repeatedly called after system is initialized by setup
//----------------------------------------
void loop()
{
    // Nothing to do here...
}

//----------------------------------------
// Call back from within parser, for end of message
// Process a complete message incoming from parser
//----------------------------------------
void processMessage(SEMP_PARSE_STATE *parse, uint16_t type)
{
    uint32_t offset;

    // Display the raw message
    sempPrintLn(output);
    offset = dataOffset + 1 - parse->length;
    sempPrintString(output, "Valid SBF message block ");
    sempPrintDecimalI32(output, sempSbfGetId(parse));
    sempPrintString(output, " : ");
    sempPrintDecimalI32(output, parse->length);
    sempPrintString(output, " bytes at ");
    sempPrintHex0x08x(output, offset);
    sempPrintString(output, " (");
    sempPrintDecimalI32(output, offset);
    sempPrintCharLn(output, ')');
    dumpBuffer(parse->buffer, parse->length);

    // Display Block Number
    sempPrintString(output, "SBF Block Number: ");
    sempPrintDecimalI32Ln(output, sempSbfGetBlockNumber(parse));

    // If this is PVTGeodetic, extract some data
    if (sempSbfGetBlockNumber(parse) == 4007)
    {
        sempPrintString(output, "TOW: ");
        sempPrintDecimalI32Ln(output, sempSbfGetU4(parse, 8));
        sempPrintString(output, "Mode: ");
        sempPrintDecimalI32Ln(output, sempSbfGetU1(parse, 14));
        sempPrintString(output, "Error: ");
        sempPrintDecimalI32Ln(output, sempSbfGetU1(parse, 15));
        Serial.printf("Latitude: %.7g\r\n", sempSbfGetF8(parse, 16) * 180.0 / PI);
        Serial.printf("Longitude: %.7g\r\n", sempSbfGetF8(parse, 24) * 180.0 / PI);
    }

    // Display the parser state
    static bool displayOnce = true;
    if (displayOnce)
    {
        displayOnce = false;
        sempPrintLn(output);
        sempPrintParserConfiguration(parse, output);
    }
}
