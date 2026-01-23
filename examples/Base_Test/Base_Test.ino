/*
  SparkFun base test example sketch, verify it builds

  License: MIT. Please see LICENSE.md for more details
*/

#include "No_Parser.h"

#include "../Common/output.ino"
#include "../Common/dumpBuffer.ino"
#include "../Common/reportFatalError.ino"

//----------------------------------------
// Constants
//----------------------------------------

#define BUFFER_LENGTH           3000
#define BUFFER_OVERHEAD         72

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

uint8_t buffer[BUFFER_OVERHEAD + BUFFER_LENGTH];
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
    size_t bufferOverhead;
    size_t minimumBufferBytes;

    delay(1000);

    Serial.begin(115200);
    sempPrintLn(output);
    sempPrintStringLn(output, "Base_Test example sketch");
    sempPrintLn(output);

    // Determine the buffer overhead (no parse area)
    bufferOverhead = sempComputeBufferOverhead(parserTable,
                                               parserCount,
                                               nullptr,
                                               nullptr,
                                               nullptr,
                                               nullptr);

    // Determine the buffer length
    bufferLength = sempGetBufferLength(parserTable, parserCount, BUFFER_LENGTH);
    if (sizeof(buffer) != bufferLength)
    {
        sempPrintString(output, "Set BUFFER_OVERHEAD to ");
        sempPrintDecimalI32(output, bufferLength - BUFFER_LENGTH);
        reportFatalError("Update BUFFER_OVERHEAD value!");
    }

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

    // No buffer specified
    parse = sempBeginParser("No parser", parserTable, parserCount,
                            buffer, 0, processMessage);
    if (parse)
        reportFatalError("Failed to detect no buffer");

    // Zero length usable buffer specified
    parse = sempBeginParser("No parser", parserTable, parserCount,
                            buffer, bufferOverhead, processMessage);
    if (parse)
        reportFatalError("Failed to detect zero length buffer");

    // Get the minimum buffer size
    minimumBufferBytes = sempGetBufferLength(parserTable, parserCount, 0);
    if ((minimumBufferBytes - bufferOverhead) != NO_PARSER_MINIMUM_BUFFER_SIZE)
        reportFatalError("Failed to get proper minimum buffer size");

    // Complain about buffer too small, but allow for testing
    parse = sempBeginParser("No parser", parserTable, parserCount,
                            buffer, minimumBufferBytes - 1, processMessage);
    if (!parse)
        reportFatalError("Failed to complain about minimum buffer size");

    // No end-of-message callback specified
    parse = sempBeginParser("No parser", parserTable, parserCount,
                            buffer, bufferLength, nullptr);
    if (parse)
        reportFatalError("Failed to detect eomCallback set to nullptr (0)");

    // Start using the entire buffer
    parse = sempBeginParser("Base Test Example", parserTable, parserCount,
                            buffer, bufferLength, processMessage, output);
    if (!parse)
        reportFatalError("Failed to initialize the parser");
    sempPrintStringLn(output, "Parser successfully initialized");

    // Display the parser configuration
    sempPrintString(output, "&parserTable: ");
    sempPrintAddrLn(output, parserTable);
    sempPrintParserConfiguration(parse, output);

    // Display the parse state
    sempPrintString(output, "Parse State: ");
    sempPrintStringLn(output, sempGetStateName(parse));

    // Display the parser type
    sempPrintString(output, "Parser Name: ");
    sempPrintStringLn(output, sempGetTypeName(parse, parse->type));

    // Obtain a raw data stream from somewhere
    sempEnableDebugOutput(parse);
    sempPrintString(output, "Raw data stream: ");
    sempPrintDecimalI32(output, RAW_DATA_BYTES);
    sempPrintStringLn(output, " bytes");

    // The raw data stream is passed to the parser one byte at a time
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
}

//----------------------------------------
// Call back from within parser, for end of message
// Process a complete message incoming from parser
//----------------------------------------
void processMessage(SEMP_PARSE_STATE *parse, uint16_t type)
{
    static bool displayOnce = true;
    uint32_t offset;

    // Display the raw message
    sempPrintLn(output);
    offset = dataOffset + 1 - parse->length;
    sempPrintString(output, "Valid Message: ");
    sempPrintDecimalI32(output, parse->length);
    sempPrintString(output, " bytes at ");
    sempPrintHex0x08x(output, offset);
    sempPrintString(output, " (");
    sempPrintDecimalI32(output, offset);
    sempPrintCharLn(output, ')');
    dumpBuffer(parse->buffer, parse->length);

    // Display the parser state
    if (displayOnce)
    {
        displayOnce = false;
        sempPrintLn(output);
        sempPrintParserConfiguration(parse, output);
    }
}
