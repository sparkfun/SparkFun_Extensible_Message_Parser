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
#define BUFFER_OVERHEAD         68

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

uint8_t bufferArea[SEMP_ALIGNMENT_MASK + BUFFER_OVERHEAD + BUFFER_LENGTH];
uint8_t * buffer;
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
    size_t parseStateBytes;
    size_t scratchPadBytes;

    delay(1000);

    Serial.begin(115200);
    sempPrintLn(output);
    sempPrintStringLn(output, "Base_Test example sketch");
    sempPrintLn(output);

    // Test the display routines
    outputTest();

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
    bufferLength = BUFFER_OVERHEAD + BUFFER_LENGTH;

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

// Output a dashed line for the bounding box
void outputLine(int fieldWidth, char corner)
{
    output(corner);
    while (fieldWidth--)
        output('-');
    sempPrintCharLn(output, corner);
}

// Test the justification routines
void outputTest()
{
    const int fieldWidth32Bit = 11;
    const int fieldWidth64Bit = 20;

    // 32-bit right justification
    outputLine(fieldWidth32Bit, '.');

    output('|');
    sempPrintDecimalI32(output, 0, fieldWidth32Bit);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintDecimalI32(output, 1, fieldWidth32Bit);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintDecimalI32(output, -1, fieldWidth32Bit);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintDecimalI32(output, 0x7fffffff, fieldWidth32Bit);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintDecimalI32(output, (int32_t)0x80000000, fieldWidth32Bit);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintHex02x(output, 0xff, fieldWidth32Bit);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintHex04x(output, 0xffff, fieldWidth32Bit);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintHex08x(output, 0xffffffff, fieldWidth32Bit);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintHex0x02x(output, 0xff, fieldWidth32Bit);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintHex0x04x(output, 0xffff, fieldWidth32Bit);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintHex0x08x(output, 0xffffffff, fieldWidth32Bit);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintString(output, "Test", fieldWidth32Bit);
    sempPrintCharLn(output, '|');

    // 32-bit left justification
    output('|');
    sempPrintDecimalI32(output, 0, -fieldWidth32Bit);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintDecimalI32(output, 1, -fieldWidth32Bit);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintDecimalI32(output, -1, -fieldWidth32Bit);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintDecimalI32(output, 0x7fffffff, -fieldWidth32Bit);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintDecimalI32(output, (int32_t)0x80000000, -fieldWidth32Bit);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintHex02x(output, 0xff, -fieldWidth32Bit);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintHex04x(output, 0xffff, -fieldWidth32Bit);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintHex08x(output, 0xffffffff, -fieldWidth32Bit);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintHex0x02x(output, 0xff, -fieldWidth32Bit);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintHex0x04x(output, 0xffff, -fieldWidth32Bit);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintHex0x08x(output, 0xffffffff, -fieldWidth32Bit);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintString(output, "Test", -fieldWidth32Bit);
    sempPrintCharLn(output, '|');

    outputLine(fieldWidth32Bit, '\'');

    // 64-bit right justification
    outputLine(fieldWidth64Bit, '.');

    output('|');
    sempPrintDecimalI64(output, 0, fieldWidth64Bit);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintDecimalI64(output, 1, fieldWidth64Bit);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintDecimalI64(output, -1, fieldWidth64Bit);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintDecimalI64(output, 0x7fffffffffffffff, fieldWidth64Bit);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintDecimalI64(output, (int64_t)0x8000000000000000ull, fieldWidth64Bit);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintHex016x(output, 0xffffffffffffffffull, fieldWidth64Bit);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintHex0x016x(output, 0xffffffffffffffffull, fieldWidth64Bit);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintString(output, "This is a test", fieldWidth64Bit);
    sempPrintCharLn(output, '|');

    // 64-bit left justification
    output('|');
    sempPrintDecimalI64(output, 0, -fieldWidth64Bit);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintDecimalI64(output, 1, -fieldWidth64Bit);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintDecimalI64(output, -1, -fieldWidth64Bit);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintDecimalI64(output, 0x7fffffffffffffffull, -fieldWidth64Bit);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintDecimalI64(output, (int64_t)0x8000000000000000ull, -fieldWidth64Bit);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintDecimalU64(output, 0xffffffffffffffffull, -fieldWidth64Bit);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintHex016x(output, 0xffffffffffffffffull, -fieldWidth64Bit);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintHex0x016x(output, 0xffffffffffffffffull, -fieldWidth64Bit);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintString(output, "This is a test", -fieldWidth64Bit);
    sempPrintCharLn(output, '|');

    outputLine(fieldWidth64Bit, '\'');
}
