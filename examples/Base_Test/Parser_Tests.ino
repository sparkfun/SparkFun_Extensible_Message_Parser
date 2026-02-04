/*
  Parser_Tests.ino

  Run tests on the SparkFun Extensible Message Parser to verify proper
  operation.

  License: MIT. Please see LICENSE.md for more details
*/

#include "No_Parser.h"

//----------------------------------------
// Constants
//----------------------------------------

#define BUFFER_LENGTH           3000
#define BUFFER_OVERHEAD         68

const uint8_t rawDataStream[] =
{
    0, 1, 2, 3, 4, 5
};

#define RAW_DATA_BYTES      sizeof(rawDataStream)

// Build the table listing all of the parsers
const SEMP_PARSER_DESCRIPTION * parserTable[] =
{
    &noParserDescription
};
const int parserCount = sizeof(parserTable) / sizeof(parserTable[0]);

//----------------------------------------
// Locals
//----------------------------------------

uint8_t bufferArea[SEMP_ALIGNMENT_MASK + BUFFER_OVERHEAD + BUFFER_LENGTH];
uint8_t * buffer;
uint32_t dataOffset;
SEMP_PARSE_STATE *parse;

//------------------------------------------------------------------------------
// Test routines
//------------------------------------------------------------------------------

//----------------------------------------
// Run the parser tests
//----------------------------------------
void parserTests()
{
    size_t bufferLength;
    size_t bufferOverhead;
    size_t minimumBufferBytes;
    size_t parseStateBytes;
    size_t scratchPadBytes;

    // Determine the buffer overhead (no parse area)
    sempComputeBufferOverhead(parserTable,
                              parserCount,
                              nullptr,
                              nullptr,
                              &parseStateBytes,
                              &scratchPadBytes);
    bufferOverhead = parseStateBytes + scratchPadBytes;

    // Verify the buffer overhead
    if (bufferOverhead != BUFFER_OVERHEAD)
    {
        sempPrintString(output, "Set BUFFER_OVERHEAD to ");
        sempPrintDecimalI32Ln(output, bufferOverhead);
        reportFatalError("Update BUFFER_OVERHEAD value!");
    }

    // Align the buffer
    buffer = (uint8_t *)SEMP_ALIGN((uintptr_t)bufferArea);

    // Verify the unaligned buffer length
    bufferLength = sempGetBufferLength(parserTable, parserCount, BUFFER_LENGTH);
    if (bufferLength != sizeof(bufferArea))
    {
        sempPrintString(output, "ERROR: Buffer length, expected ");
        sempPrintDecimalI32(output, sizeof(bufferArea));
        sempPrintString(output, ", actual: ");
        sempPrintDecimalI32Ln(output, bufferLength);
        reportFatalError("Failed to verify the buffer length");
    }

    // Reduce the buffer size due to the alignment
    bufferLength = bufferOverhead + BUFFER_LENGTH;

    // No name specified
    parse = sempBeginParser(nullptr, parserTable, parserCount, buffer,
                            bufferLength, processMessage, output);
    if (parse)
        reportFatalError("Failed to detect parserName set to nullptr (0)");

    // No name specified
    parse = sempBeginParser("", parserTable, parserCount, buffer,
                            bufferLength, processMessage, output);
    if (parse)
        reportFatalError("Failed to detect parserName set to empty string");

    // No parser table specified
    parse = sempBeginParser("No parser", nullptr, parserCount, buffer,
                            bufferLength, processMessage, output);
    if (parse)
        reportFatalError("Failed to detect parserTable set to nullptr (0)");

    // Parser count is zero
    parse = sempBeginParser("No parser", parserTable, 0, buffer,
                            bufferLength, processMessage, output);
    if (parse)
        reportFatalError("Failed because parserCount != nameTableCount");

    // No buffer specified, nullptr
    parse = sempBeginParser("No parser", parserTable, parserCount, nullptr,
                            bufferLength, processMessage, output);
    if (parse)
        reportFatalError("Failed to detect buffer set to nullptr (0)");

    // No buffer specified, zero length
    parse = sempBeginParser("No parser", parserTable, parserCount, buffer,
                            0, processMessage, output);
    if (parse)
        reportFatalError("Failed to detect no buffer");

    // Zero length parse area specified
    parse = sempBeginParser("No parser", parserTable, parserCount, buffer,
                            bufferOverhead, processMessage, output);
    if (parse)
        reportFatalError("Failed to detect zero length buffer");

    // Get the minimum buffer size
    minimumBufferBytes = sempGetBufferLength(parserTable, parserCount, 0)
                       - SEMP_ALIGNMENT_MASK;
    if ((minimumBufferBytes - bufferOverhead) != NO_PARSER_MINIMUM_BUFFER_SIZE)
        reportFatalError("Failed to get proper minimum buffer size");

    // Complain about buffer too small, but allow for testing
    parse = sempBeginParser("No parser", parserTable, parserCount, buffer,
                            minimumBufferBytes - 1, processMessage, output);
    if (!parse)
        reportFatalError("Failed to complain about minimum buffer size");

    // No end-of-message callback specified
    parse = sempBeginParser("No parser", parserTable, parserCount, buffer,
                            bufferLength, nullptr, output);
    if (parse)
        reportFatalError("Failed to detect eomCallback set to nullptr (0)");

    // Test the buffer alignment
    for (int offset = 1; offset <= SEMP_ALIGNMENT_MASK; offset++)
    {
        uint8_t * offsetBuffer = buffer + offset;
        parse = sempBeginParser("Base Test Example", parserTable, parserCount,
                                offsetBuffer, bufferLength - offset,
                                processMessage, output, output);
        if (!parse)
            reportFatalError("Failed to initialize the parser");
        if ((uint8_t *)parse < offsetBuffer)
        {
            sempPrintString(output, "Offset ");
            sempPrintDecimalI32(output, offset);
            sempPrintString(output, ", parse: ");
            sempPrintAddr(output, parse);
            sempPrintString(output, " < ");
            sempPrintAddr(output, offsetBuffer);
            sempPrintStringLn(output, " buffer");
            reportFatalError("Parse state area located before buffer");
        }
        if ((uint8_t *)parse > &offsetBuffer[SEMP_ALIGNMENT_MASK])
        {
            sempPrintString(output, "Offset ");
            sempPrintDecimalI32(output, offset);
            sempPrintString(output, ", parse: ");
            sempPrintAddr(output, parse);
            sempPrintString(output, " > ");
            sempPrintAddr(output, &offsetBuffer[SEMP_ALIGNMENT_MASK]);
            sempPrintStringLn(output, " &buffer[SEMP_ALIGNMENT_MASK]");
            reportFatalError("Parse state area starts after alignment area");
        }
        if ((parse->buffer + parse->bufferLength) != &offsetBuffer[bufferLength - offset])
        {
            sempPrintString(output, "Offset ");
            sempPrintDecimalI32(output, offset);
            sempPrintString(output, ", parse->buffer + parse->bufferLength: ");
            sempPrintAddr(output, parse->buffer + parse->bufferLength);
            sempPrintString(output, " != ");
            sempPrintAddr(output, &offsetBuffer[bufferLength - offset]);
            sempPrintStringLn(output, " &buffer[bufferLength]");
            reportFatalError("Parse area length computation failed");
        }
    }

    // Display the parser configuration
    sempPrintString(output, "&parserTable: ");
    sempPrintAddrLn(output, parserTable);
    sempPrintParserConfiguration(parse, output);

    // Start using the entire buffer
    parse = sempBeginParser("Base Test Example", parserTable, parserCount,
                            buffer, bufferLength, processMessage, output,
                            output);
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
    sempDebugOutputEnable(parse, output);
    sempPrintString(output, "Raw data stream: ");
    sempPrintDecimalI32(output, RAW_DATA_BYTES);
    sempPrintStringLn(output, " bytes");

    // The raw data stream is passed to the parser one byte at a time
    for (dataOffset = 0; dataOffset < RAW_DATA_BYTES; dataOffset++)
        // Update the parser state based on the incoming byte
        sempParseNextByte(parse, rawDataStream[dataOffset]);

    // Done parsing the data
    sempStopParser(&parse);
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
    sempDumpBuffer(output, parse->buffer, parse->length);

    // Display the parser state
    if (displayOnce)
    {
        displayOnce = false;
        sempPrintLn(output);
        sempPrintParserConfiguration(parse, output);
    }
}

//----------------------------------------
// Print the error message every 15 seconds
//
// Inputs:
//   errorMsg: Zero-terminated error message string to output every 15 seconds
//----------------------------------------
void reportFatalError(const char *errorMsg)
{
    while (1)
    {
        sempPrintString(output, "HALTED: ");
        sempPrintStringLn(output, errorMsg);
        sleep(15);
    }
}
