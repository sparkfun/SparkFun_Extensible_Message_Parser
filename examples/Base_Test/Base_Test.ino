/*
  SparkFun base test example sketch, verify it builds

  License: MIT. Please see LICENSE.md for more details
*/

#include "No_Parser.h"

#include "../Common/dumpBuffer.ino"
#include "../Common/reportFatalError.ino"

//----------------------------------------
// Constants
//----------------------------------------

const uint8_t rawDataStream[] =
{
    0, 1, 2, 3, 4, 5
};

#define RAW_DATA_BYTES      sizeof(rawDataStream)

// Build the table listing all of the parsers
SEMP_PARSER_DESCRIPTION * parserTable[] =
{
    &noParserDescription
};
const int parserCount = sizeof(parserTable) / sizeof(parserTable[0]);

//----------------------------------------
// Locals
//----------------------------------------

uint32_t dataOffset;
SEMP_PARSE_STATE *parse;

//----------------------------------------
// Test routine
//----------------------------------------

void setup()
{
    uint8_t * buffer;
    size_t bufferLength;

    delay(1000);

    Serial.begin(115200);
    Serial.println();
    Serial.println("Base_Test example sketch");
    Serial.println();

    // No name specified
    parse = sempBeginParser(nullptr, parserTable, parserCount,
                            buffer, bufferLength, processMessage);
    if (parse)
        reportFatalError("Failed to detect parserName set to nullptr (0)");

    // No name specified
    parse = sempBeginParser("", parserTable, parserCount,
                            buffer, bufferLength, processMessage);
    if (parse)
        reportFatalError("Failed to detect parserName set to empty string");

    // No parser table specified
    bufferLength = sempGetBufferLength(parserTable, parserCount, 3000);
    buffer = (uint8_t *)malloc(bufferLength);
    parse = sempBeginParser("No parser", nullptr, parserCount,
                            buffer, bufferLength, processMessage);
    if (parse)
        reportFatalError("Failed to detect parserTable set to nullptr (0)");

    // Parser count is zero
    parse = sempBeginParser("No parser", parserTable, 0,
                            buffer, bufferLength, processMessage);
    if (parse)
        reportFatalError("Failed because parserCount != nameTableCount");

    // No buffer specified
    parse = sempBeginParser("No parser", parserTable, parserCount,
                            nullptr, bufferLength, processMessage);
    if (parse)
        reportFatalError("Failed to detect buffer set to nullptr (0)");

    // Too small a buffer specified
    parse = sempBeginParser("No parser", parserTable, parserCount,
                            buffer, 0, processMessage);
    if (parse)
        reportFatalError("Failed to detect buffer set to nullptr (0)");

    // No end-of-message callback specified
    parse = sempBeginParser("No parser", parserTable, parserCount,
                            buffer, bufferLength, nullptr);
    if (parse)
        reportFatalError("Failed to detect eomCallback set to nullptr (0)");
    free(buffer);

    // Initialize the parser, specify a large scratch pad area
    bufferLength = sempGetBufferLength(parserTable, parserCount, 3000);
    buffer = (uint8_t *)malloc(bufferLength);

    parse = sempBeginParser("Base Test Example", parserTable, parserCount,
                            buffer, bufferLength, processMessage);
    if (!parse)
        reportFatalError("Failed to initialize the parser");
    Serial.println("Parser successfully initialized");

    // Display the parser configuration
    Serial.printf("&parserTable: %p\r\n", parserTable);
    sempPrintParserConfiguration(parse, &Serial);

    // Display the parse state
    Serial.printf("Parse State: %s\r\n", sempGetStateName(parse));

    // Display the parser type
    Serial.printf("Parser Name: %s\r\n", sempGetTypeName(parse, parse->type));

    // Obtain a raw data stream from somewhere
    sempEnableDebugOutput(parse);
    Serial.printf("Raw data stream: %d bytes\r\n", RAW_DATA_BYTES);

    // The raw data stream is passed to the parser one byte at a time
    for (dataOffset = 0; dataOffset < RAW_DATA_BYTES; dataOffset++)
        // Update the parser state based on the incoming byte
        sempParseNextByte(parse, rawDataStream[dataOffset]);

    // Done parsing the data
    sempStopParser(&parse);
    free(buffer);
    Serial.printf("All done\r\n");
}

void loop()
{
}

// Call back from within parser, for end of message
// Process a complete message incoming from parser
void processMessage(SEMP_PARSE_STATE *parse, uint16_t type)
{
    static bool displayOnce = true;
    uint32_t offset;

    // Display the raw message
    Serial.println();
    offset = dataOffset + 1 - parse->length;
    Serial.printf("Valid Message: %d bytes at 0x%08x (%d)\r\n",
                  parse->length, offset, offset);
    dumpBuffer(parse->buffer, parse->length);

    // Display the parser state
    if (displayOnce)
    {
        displayOnce = false;
        Serial.println();
        sempPrintParserConfiguration(parse, &Serial);
    }
}
