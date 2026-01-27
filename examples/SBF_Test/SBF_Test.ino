/*
  SparkFun SBF test example sketch

  This example demonstrates how to parse a Septentrio SBF data stream

  License: MIT. Please see LICENSE.md for more details
*/

#include <SparkFun_Extensible_Message_Parser.h> //http://librarymanager/All#SparkFun_Extensible_Message_Parser

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

// Account for the largest SBF messages
#define BUFFER_LENGTH   2048

//----------------------------------------
// Locals
//----------------------------------------

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
    delay(1000);

    Serial.begin(115200);
    Serial.println();
    Serial.println("SBF_Test example sketch");
    Serial.println();

    // Initialize the parser
    size_t bufferLength = sempGetBufferLength(parserTable, parserCount, BUFFER_LENGTH);
    uint8_t * buffer = (uint8_t *)malloc(bufferLength);
    parse = sempBeginParser("SBF_Test", parserTable, parserCount,
                            buffer, bufferLength, processMessage);
    if (!parse)
        reportFatalError("Failed to initialize the parser");

    // Obtain a raw data stream from somewhere
    Serial.printf("Raw data stream: %d bytes\r\n", RAW_DATA_BYTES);

    // The raw data stream is passed to the parser one byte at a time
    sempEnableDebugOutput(parse);
    for (dataOffset = 0; dataOffset < RAW_DATA_BYTES; dataOffset++)
        // Update the parser state based on the incoming byte
        sempParseNextByte(parse, rawDataStream[dataOffset]);

    // Done parsing the data
    sempStopParser(&parse);
    free(buffer);
    Serial.printf("All done\r\n");
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
    Serial.println();
    offset = dataOffset + 1 - parse->length;
    Serial.printf("Valid SBF message block %d : %d bytes at 0x%08lx (%ld)\r\n",
                  sempSbfGetId(parse),
                  parse->length, offset, offset);
    dumpBuffer(parse->buffer, parse->length);

    // Display Block Number
    Serial.print("SBF Block Number: ");
    Serial.println(sempSbfGetBlockNumber(parse));

    // If this is PVTGeodetic, extract some data
    if (sempSbfGetBlockNumber(parse) == 4007)
    {
        Serial.printf("TOW: %ld\r\n", sempSbfGetU4(parse, 8));
        Serial.printf("Mode: %d\r\n", sempSbfGetU1(parse, 14));
        Serial.printf("Error: %d\r\n", sempSbfGetU1(parse, 15));
        Serial.printf("Latitude: %.7g\r\n", sempSbfGetF8(parse, 16) * 180.0 / PI);
        Serial.printf("Longitude: %.7g\r\n", sempSbfGetF8(parse, 24) * 180.0 / PI);
    }

    // Display the parser state
    static bool displayOnce = true;
    if (displayOnce)
    {
        displayOnce = false;
        Serial.println();
        sempPrintParserConfiguration(parse, &Serial);
    }
}
