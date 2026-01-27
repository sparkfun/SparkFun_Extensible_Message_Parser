/*------------------------------------------------------------------------------
SparkFun_Extensible_Message_Parser.cpp

Parse messages from GNSS radios

License: MIT. Please see LICENSE.md for more details
------------------------------------------------------------------------------*/

#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include "SparkFun_Extensible_Message_Parser.h"
#include "semp_crc32.h"

//----------------------------------------
// Constants
//----------------------------------------

#define SEMP_ALIGNMENT_MASK        7

const uint64_t sempPower10U64[] =
{
    10ull * 1000ull * 1000ull * 1000ull * 1000ull * 1000ull * 1000ull,  // 0
    1ull * 1000ull * 1000ull * 1000ull * 1000ull * 1000ull * 1000ull,   // 1
    100ull * 1000ull * 1000ull * 1000ull * 1000ull * 1000ull,           // 2
    10ull * 1000ull * 1000ull * 1000ull * 1000ull * 1000ull,            // 3
    1ull * 1000ull * 1000ull * 1000ull * 1000ull * 1000ull,             // 4
    100ull * 1000ull * 1000ull * 1000ull * 1000ull,                     // 5
    10ull * 1000ull * 1000ull * 1000ull * 1000ull,                      // 6
    1ull * 1000ull * 1000ull * 1000ull * 1000ull,                       // 7
    100ull * 1000ull * 1000ull * 1000ull,                               // 8
    10ull * 1000ull * 1000ull * 1000ull,                                // 9

    1ull * 1000ull * 1000ull * 1000ull,                                 // 10
    100ull * 1000ull * 1000ull,                                         // 11
    10ull * 1000ull * 1000ull,                                          // 12
    1ull * 1000ull * 1000ull,                                           // 13
    100ull * 1000ull,                                                   // 14
    10ull * 1000ull,                                                    // 15
    1ull * 1000ull,                                                     // 16
    100ull,                                                             // 17
    10ull,                                                              // 18
    1ull,                                                               // 19
};
const int sempPower10U64Entries = sizeof(sempPower10U64) / sizeof(sempPower10U64[0]);

//----------------------------------------
// Macros
//----------------------------------------

// Align x to multiples of 8: 0->0; 1->8; 8->8; 9->16
#define SEMP_ALIGN(x)   ((x + SEMP_ALIGNMENT_MASK) & (~SEMP_ALIGNMENT_MASK))

//------------------------------------------------------------------------------
// SparkFun Extensible Message Parser API routines
//
// These routines are called by parsers and applications
//------------------------------------------------------------------------------

//----------------------------------------
// Convert an ASCII character (0-9, A-F, or a-f) into a 4-bit binary value
//----------------------------------------
int sempAsciiToNibble(int data)
{
    // Convert the value to lower case
    data |= 0x20;
    if ((data >= 'a') && (data <= 'f'))
        return data - 'a' + 10;
    if ((data >= '0') && (data <= '9'))
        return data - '0';
    return -1;
}

//----------------------------------------
// Initialize the parser
//----------------------------------------
SEMP_PARSE_STATE *sempBeginParser(
    const char *parserTableName,
    SEMP_PARSER_DESCRIPTION **parserTable,
    uint16_t parserCount,
    uint8_t * buffer,
    size_t bufferLength,
    SEMP_EOM_CALLBACK eomCallback,
    SEMP_OUTPUT debugOutput,
    Print *printError,
    SEMP_BAD_CRC_CALLBACK badCrc
    )
{
    int bufferOverhead;
    SEMP_PARSE_STATE *parse = nullptr;
    size_t parseAreaBytes;
    size_t parseStateBytes;
    size_t payloadOffset;
    size_t scratchPadBytes;

    do
    {
        // Verify the name
        if ((!parserTableName) || (!strlen(parserTableName)))
        {
            sempPrintln(printError, "SEMP: Please provide a name for the parserTable");
            break;
        }

        // Validate the parserTable address is not nullptr
        if (!parserTable)
        {
            sempPrintln(printError, "SEMP: Please specify a parserTable data structure");
            break;
        }

        // Validate the buffer is specified
        if (!buffer)
        {
            sempPrintln(printError, "SEMP: Please specify a buffer");
            break;
        }

        // Validate the end-of-message callback routine address is not nullptr
        if (!eomCallback)
        {
            sempPrintln(printError, "SEMP: Please specify an eomCallback routine");
            break;
        }

        // Verify that there is at least one parser in the table
        if (!parserCount)
        {
            sempPrintln(printError, "SEMP: Please provide at least one parser in parserTable");
            break;
        }

        // Determine the buffer overhead
        bufferOverhead = sempComputeBufferOverhead(parserTable,
                                                   parserCount,
                                                   &parseAreaBytes,
                                                   &payloadOffset,
                                                   &parseStateBytes,
                                                   &scratchPadBytes);

        // Verify that there is a parsing area within the buffer
        if (bufferLength <= bufferOverhead)
        {
            // Buffer too small, display the error message
            if (printError)
                sempPrintf(printError, "SEMP ERROR: Buffer too small, increase size to >= %d bytes (1 byte of parse area",
                           bufferOverhead + 1);
            break;
        }

        // Determine if the buffer meets the minimum size requirements
        if (parseAreaBytes < payloadOffset)
            parseAreaBytes = payloadOffset;
        if ((bufferLength < (bufferOverhead + parseAreaBytes)) && printError)
        {
            // Buffer too small, display the error message
            sempPrintf(printError, "SEMP ERROR: Buffer too small, increase size to >= %d bytes (%d bytes of parse area)",
                       bufferOverhead + parseAreaBytes, parseAreaBytes);
            sempPrintf(printError, "SEMP WARNING: Continuing on to support testing!");
        }

        // Initialize the buffer
        memset(buffer, 0, bufferLength);

        // Divide up the buffer
        parse = (SEMP_PARSE_STATE *)buffer;
        parse->scratchPad = ((uint8_t *)parse) + parseStateBytes;
        parse->buffer = ((uint8_t *)parse->scratchPad + scratchPadBytes);
        parse->bufferLength = bufferLength - bufferOverhead;

        // Initialize the parse state
        parse->debugOutput = debugOutput;
        parse->printError = printError;
        parse->parsers = parserTable;
        parse->parserCount = parserCount;
        parse->state = sempFirstByte;
        parse->eomCallback = eomCallback;
        parse->parserName = parserTableName;
        parse->badCrc = badCrc;
        parse->type = parserCount;  // No active parser

        // Display the parser configuration
        sempPrintParserConfiguration(parse, parse->debugOutput);
    } while (0);

    // Return the parse structure address
    return parse;
}

//----------------------------------------
// Compute the buffer size required without a parse area (overhead)
//----------------------------------------
size_t sempComputeBufferOverhead(SEMP_PARSER_DESCRIPTION **parserTable,
                                 uint16_t parserCount,
                                 size_t *parseAreaBytes,
                                 size_t *payloadOffset,
                                 size_t *parseStateBytes,
                                 size_t *scratchPadBytes)
{
    size_t minParseArea;
    size_t parseState;
    size_t minPayloadOffset;
    size_t minScratchArea;

    // Walk the list of parser descriptions and determine the minimum
    // buffer size.
    minParseArea = 0;
    minPayloadOffset = 0;
    minScratchArea = 0;
    for (int index = 0; index < parserCount; index++)
    {
        // Maximize the parse area
        if (minParseArea < parserTable[index]->minimumParseAreaBytes)
            minParseArea = parserTable[index]->minimumParseAreaBytes;

        // Maximize the payload offset
        if (minPayloadOffset < parserTable[index]->payloadOffset)
            minPayloadOffset = parserTable[index]->payloadOffset;

        // Maximize the scratch area
        if (minScratchArea < parserTable[index]->scratchPadBytes)
            minScratchArea = parserTable[index]->scratchPadBytes;
    }

    // Align the various areas
    parseState = SEMP_ALIGN(sizeof(SEMP_PARSE_STATE));
    minScratchArea = SEMP_ALIGN(minScratchArea);

    // Return the minimum values
    if (parseAreaBytes)
        *parseAreaBytes = minParseArea;
    if (payloadOffset)
        *payloadOffset = minPayloadOffset;
    if (parseStateBytes)
        *parseStateBytes = parseState;
    if (scratchPadBytes)
        *scratchPadBytes = minScratchArea;

    // Return the buffer size
    return parseState + minScratchArea;
}

//----------------------------------------
// Disable debug output
//----------------------------------------
void sempDebugOutputDisable(SEMP_PARSE_STATE *parse)
{
    if (parse)
        parse->debugOutput = nullptr;
}

//----------------------------------------
// Enable debug output
//----------------------------------------
void sempDebugOutputEnable(SEMP_PARSE_STATE *parse,
                           SEMP_OUTPUT output,
                           bool verbose)
{
    if (parse)
    {
        parse->debugOutput = output;
        parse->verboseDebug = verbose;
    }
}

//----------------------------------------
// Disable error output
//----------------------------------------
void sempErrorOutputDisable(SEMP_PARSE_STATE *parse)
{
    if (parse)
        parse->printError = nullptr;
}

//----------------------------------------
// Wait for the first byte in the data stream
//----------------------------------------
bool sempFirstByte(SEMP_PARSE_STATE *parse, uint8_t data)
{
    int index;
    SEMP_PARSER_DESCRIPTION *parseDescripion;

    if (parse)
    {
        // Add this byte to the buffer
        parse->crc = 0;
        parse->computeCrc = nullptr;
        parse->length = 0;
        parse->buffer[parse->length++] = data;

        // Walk through the parse table
        for (parse->type = 0; parse->type < parse->parserCount; parse->type++)
        {
            // Determine if this parser is able to parse this stream
            parseDescripion = parse->parsers[index];
            if (parseDescripion->preamble(parse, data))
                // This parser claims to be able to parse this stream
                return true;
        }

        // Preamble not found, pass this data to another parser if requested
        if (parse->invalidData)
            parse->invalidData(parse->buffer, parse->length);

        // Continue searching for a preamble byte
        parse->state = sempFirstByte;
    }
    return false;
}

//----------------------------------------
// Compute the necessary buffer length in bytes to support the scratch pad
// and parse buffer lengths
//----------------------------------------
size_t sempGetBufferLength(SEMP_PARSER_DESCRIPTION **parserTable,
                           uint16_t parserCount,
                           size_t desiredParseAreaSize,
                           SEMP_OUTPUT output)
{
    size_t bufferLength;
    size_t bufferOverhead;
    size_t parseArea;
    size_t parseAreaBytes;
    size_t payloadOffset;

    // Determine the buffer overhead size (no parse area)
    bufferOverhead = sempComputeBufferOverhead(parserTable,
                                               parserCount,
                                               &parseArea,
                                               &payloadOffset,
                                               nullptr,
                                               nullptr);

    // Verify the parse area meets the minimum buffer length
    parseAreaBytes = desiredParseAreaSize;
    if (parseAreaBytes < parseArea)
    {
        // Display the minimum buffer length
        if (output)
        {
            sempPrintString(output, "SEMP: Increasing parse area from ");
            sempPrintDecimalU32(output, parseAreaBytes);
            sempPrintString(output, " to ");
            sempPrintDecimalU32(output, parseArea);
            sempPrintStringLn(output, " bytes, due to minimize size requirement");
        }
        parseAreaBytes = parseArea;
    }

    // Verify the parse area meets the minimum payload offset
    if (parseAreaBytes < payloadOffset)
    {
        // Display the minimum buffer length
        if (output)
        {
            sempPrintString(output, "SEMP: Increasing parse area from ");
            sempPrintDecimalU32(output, parseAreaBytes);
            sempPrintString(output, " to ");
            sempPrintDecimalU32(output, payloadOffset);
            sempPrintStringLn(output, " bytes, due to payload offset requirement");
        }
        parseAreaBytes = payloadOffset;
    }

    // Verify that there is a parse area
    if (parseAreaBytes < 1)
    {
        // Display the minimum buffer length
        if (output)
        {
            sempPrintString(output, "SEMP: Increasing parse area from ");
            sempPrintDecimalU32(output, parseAreaBytes);
            sempPrintStringLn(output, " to 1 byte, requires at least one byte");
        }
        parseAreaBytes = 1;
    }

    // Determine the final buffer length
    bufferLength = bufferOverhead + parseAreaBytes;

    // Display the buffer length
    if (output)
    {
        sempPrintString(output, "SEMP: Buffer length ");
        sempPrintDecimalU32(output, bufferLength);
        sempPrintStringLn(output, " bytes");
    }
    return bufferLength;
}

//----------------------------------------
// Get a 32-bit floating point value
//----------------------------------------
float sempGetF4(const SEMP_PARSE_STATE *parse, uint16_t offset)
{
    float value;
    SEMP_PARSER_DESCRIPTION *parserDescription = parse->parsers[parse->type];
    memcpy(&value, &parse->buffer[offset + parserDescription->payloadOffset], sizeof(value));
    return value;
}

//----------------------------------------
// Get a 32-bit floating point value
//----------------------------------------
float sempGetF4NoOffset(const SEMP_PARSE_STATE *parse, uint16_t offset)
{
    float value;
    SEMP_PARSER_DESCRIPTION *parserDescription = parse->parsers[parse->type];
    memcpy(&value, &parse->buffer[offset], sizeof(value));
    return value;
}

//----------------------------------------
// Get a 64-bit floating point (double) value
//----------------------------------------
double sempGetF8(const SEMP_PARSE_STATE *parse, size_t offset)
{
    double value;
    SEMP_PARSER_DESCRIPTION *parserDescription = parse->parsers[parse->type];
    memcpy(&value, &parse->buffer[offset + parserDescription->payloadOffset], sizeof(value));
    return value;
}

//----------------------------------------
// Get a 64-bit floating point (double) value
//----------------------------------------
double sempGetF8NoOffset(const SEMP_PARSE_STATE *parse, size_t offset)
{
    double value;
    SEMP_PARSER_DESCRIPTION *parserDescription = parse->parsers[parse->type];
    memcpy(&value, &parse->buffer[offset], sizeof(value));
    return value;
}

//----------------------------------------
// Get an 8-bit integer value
//----------------------------------------
int8_t sempGetI1(const SEMP_PARSE_STATE *parse, size_t offset)
{
    int8_t value;
    SEMP_PARSER_DESCRIPTION *parserDescription = parse->parsers[parse->type];
    value = *(int8_t *)&parse->buffer[offset + parserDescription->payloadOffset];
    return value;
}

//----------------------------------------
// Get an 8-bit integer value
//----------------------------------------
int8_t sempGetI1NoOffset(const SEMP_PARSE_STATE *parse, size_t offset)
{
    int8_t value;
    SEMP_PARSER_DESCRIPTION *parserDescription = parse->parsers[parse->type];
    value = *(int8_t *)&parse->buffer[offset];
    return value;
}

//----------------------------------------
// Get a 16-bit integer value
//----------------------------------------
int16_t sempGetI2(const SEMP_PARSE_STATE *parse, size_t offset)
{
    int16_t value;
    SEMP_PARSER_DESCRIPTION *parserDescription = parse->parsers[parse->type];
    memcpy(&value, &parse->buffer[offset + parserDescription->payloadOffset], sizeof(value));
    return value;
}

//----------------------------------------
// Get a 16-bit integer value
//----------------------------------------
int16_t sempGetI2NoOffset(const SEMP_PARSE_STATE *parse, size_t offset)
{
    int16_t value;
    SEMP_PARSER_DESCRIPTION *parserDescription = parse->parsers[parse->type];
    memcpy(&value, &parse->buffer[offset], sizeof(value));
    return value;
}

//----------------------------------------
// Get the number of digits in a 32-bit signed number
//----------------------------------------
int sempGetI4Digits(int32_t value)
{
    if (value < 0)
        return sempGetU4Digits(1 + ~(uint32_t)value) + 1;
    return sempGetU8Digits(value);
}

//----------------------------------------
// Get a 32-bit integer value
//----------------------------------------
int32_t sempGetI4(const SEMP_PARSE_STATE *parse, size_t offset)
{
    int32_t value;
    SEMP_PARSER_DESCRIPTION *parserDescription = parse->parsers[parse->type];
    memcpy(&value, &parse->buffer[offset + parserDescription->payloadOffset], sizeof(value));
    return value;
}

//----------------------------------------
// Get a 32-bit integer value
//----------------------------------------
int32_t sempGetI4NoOffset(const SEMP_PARSE_STATE *parse, size_t offset)
{
    int32_t value;
    SEMP_PARSER_DESCRIPTION *parserDescription = parse->parsers[parse->type];
    memcpy(&value, &parse->buffer[offset], sizeof(value));
    return value;
}

//----------------------------------------
// Get a 64-bit integer value
//----------------------------------------
int64_t sempGetI8(const SEMP_PARSE_STATE *parse, size_t offset)
{
    int64_t value;
    SEMP_PARSER_DESCRIPTION *parserDescription = parse->parsers[parse->type];
    memcpy(&value, &parse->buffer[offset + parserDescription->payloadOffset], sizeof(value));
    return value;
}

//----------------------------------------
// Get the number of digits in a 64-bit signed number
//----------------------------------------
int sempGetI8Digits(int64_t value)
{
    if (value < 0)
        return sempGetU8Digits(1 + ~(uint64_t)value) + 1;
    return sempGetU8Digits(value);
}

//----------------------------------------
// Get a 64-bit integer value
//----------------------------------------
int64_t sempGetI8NoOffset(const SEMP_PARSE_STATE *parse, size_t offset)
{
    int64_t value;
    SEMP_PARSER_DESCRIPTION *parserDescription = parse->parsers[parse->type];
    memcpy(&value, &parse->buffer[offset], sizeof(value));
    return value;
}

//----------------------------------------
// Translates state value into an ASCII state name
//----------------------------------------
const char * sempGetStateName(const SEMP_PARSE_STATE *parse)
{
    const char * stateName;

    if (parse)
    {
        if (parse->state == sempFirstByte)
            return "sempFirstByte";
        stateName = parse->parsers[parse->type]->getStateName(parse);
        if (stateName)
            return stateName;
    }
    return "Unknown state";
}

//----------------------------------------
// Get a zero terminated string address
//----------------------------------------
const char * sempGetString(const SEMP_PARSE_STATE *parse, size_t offset)
{
    SEMP_PARSER_DESCRIPTION *parserDescription = parse->parsers[parse->type];
    return (const char *)&parse->buffer[offset + parserDescription->payloadOffset];
}

//----------------------------------------
// Get a zero terminated string address
//----------------------------------------
const char * sempGetStringNoOffset(const SEMP_PARSE_STATE *parse, size_t offset)
{
    SEMP_PARSER_DESCRIPTION *parserDescription = parse->parsers[parse->type];
    return (const char *)&parse->buffer[offset];
}

//----------------------------------------
// Translate the type value into an ASCII type name
//----------------------------------------
const char * sempGetTypeName(SEMP_PARSE_STATE *parse, uint16_t type)
{
    const char *name = "Unknown parser";

    if (parse)
    {
        if (type < parse->parserCount)
            name = parse->parsers[type]->parserName;
        else if (type == parse->parserCount)
            name = "SEMP scanning for preamble";
    }
    return name;
}

//----------------------------------------
// Get an 8-bit unsigned integer value
//----------------------------------------
uint8_t sempGetU1(const SEMP_PARSE_STATE *parse, size_t offset)
{
    uint8_t value;
    SEMP_PARSER_DESCRIPTION *parserDescription = parse->parsers[parse->type];
    value = *(uint8_t *)&parse->buffer[offset + parserDescription->payloadOffset];
    return value;
}

//----------------------------------------
// Get an 8-bit unsigned integer value
//----------------------------------------
uint8_t sempGetU1NoOffset(const SEMP_PARSE_STATE *parse, size_t offset)
{
    uint8_t value;
    SEMP_PARSER_DESCRIPTION *parserDescription = parse->parsers[parse->type];
    value = *(uint8_t *)&parse->buffer[offset];
    return value;
}

//----------------------------------------
// Get a 16-bit unsigned integer value
//----------------------------------------
uint16_t sempGetU2(const SEMP_PARSE_STATE *parse, size_t offset)
{
    uint16_t value;
    SEMP_PARSER_DESCRIPTION *parserDescription = parse->parsers[parse->type];
    memcpy(&value, &parse->buffer[offset + parserDescription->payloadOffset], sizeof(value));
    return value;
}

//----------------------------------------
// Get a 16-bit unsigned integer value
//----------------------------------------
uint16_t sempGetU2NoOffset(const SEMP_PARSE_STATE *parse, size_t offset)
{
    uint16_t value;
    SEMP_PARSER_DESCRIPTION *parserDescription = parse->parsers[parse->type];
    memcpy(&value, &parse->buffer[offset], sizeof(value));
    return value;
}

//----------------------------------------
// Get a 32-bit unsigned integer value
//----------------------------------------
uint32_t sempGetU4(const SEMP_PARSE_STATE *parse, size_t offset)
{
    uint32_t value;
    SEMP_PARSER_DESCRIPTION *parserDescription = parse->parsers[parse->type];
    memcpy(&value, &parse->buffer[offset + parserDescription->payloadOffset], sizeof(value));
    return value;
}

//----------------------------------------
// Get the number of digits in a 32-bit unsigned number
//----------------------------------------
int sempGetU4Digits(uint32_t value)
{
    return sempGetU8Digits(value);
}

//----------------------------------------
// Get a 32-bit unsigned integer value
//----------------------------------------
uint32_t sempGetU4NoOffset(const SEMP_PARSE_STATE *parse, size_t offset)
{
    uint32_t value;
    SEMP_PARSER_DESCRIPTION *parserDescription = parse->parsers[parse->type];
    memcpy(&value, &parse->buffer[offset], sizeof(value));
    return value;
}

//----------------------------------------
// Get a 64-bit unsigned integer value
//----------------------------------------
uint64_t sempGetU8(const SEMP_PARSE_STATE *parse, size_t offset)
{
    uint64_t value;
    SEMP_PARSER_DESCRIPTION *parserDescription = parse->parsers[parse->type];
    memcpy(&value, &parse->buffer[offset + parserDescription->payloadOffset], sizeof(value));
    return value;
}

//----------------------------------------
// Get the number of digits in a 64-bit unsigned number
//----------------------------------------
int sempGetU8Digits(uint64_t value)
{
    int digits;
    int index;

    for (index = 0; index < (sempPower10U64Entries - 1); index++)
    {
        if (sempPower10U64[index] <= value)

            break;
    }
    digits = sempPower10U64Entries - index;
    return digits;
}

//----------------------------------------
// Get a 64-bit unsigned integer value
//----------------------------------------
uint64_t sempGetU8NoOffset(const SEMP_PARSE_STATE *parse, size_t offset)
{
    uint64_t value;
    SEMP_PARSER_DESCRIPTION *parserDescription = parse->parsers[parse->type];
    memcpy(&value, &parse->buffer[offset], sizeof(value));
    return value;
}

//----------------------------------------
// Perform the invalid data callback
//----------------------------------------
void sempInvalidDataCallback(SEMP_PARSE_STATE *parse)
{
    if (parse->invalidData)
        parse->invalidData(parse->buffer, parse->length);
    parse->state = sempFirstByte;
}

//----------------------------------------
// Justify a value within a field
//----------------------------------------
int sempJustifyField(SEMP_OUTPUT output, int fieldWidth, int digits)
{
    if (fieldWidth)
    {
        // Left justified fields are negative
        if (fieldWidth < 0)
            // Convert to positive to output next time
            return -fieldWidth;

        // Right justified fields are positive
        while (fieldWidth-- > digits)
            output(' ');
    }

    // Done with the justification
    return 0;
}

//----------------------------------------
// Convert nibble to ASCII
//----------------------------------------
char sempNibbleToAscii(int nibble)
{
    nibble = nibble & 0xf;
    return (nibble < 10) ? nibble + '0' : nibble + 'a' - 10;
}

//----------------------------------------
// Parse the next byte
//----------------------------------------
void sempParseNextByte(SEMP_PARSE_STATE *parse, uint8_t data)
{
    if (parse)
    {
        // Verify that enough space exists in the buffer
        if (parse->length >= parse->bufferLength)
        {
            // Message too long
            sempPrintf(parse->printError, "SEMP %s: Message too long, increase the buffer size > %d\r\n",
                       parse->parserName,
                       parse->bufferLength);

            // Pass this data to another parser if requested
            if (parse->invalidData)
                parse->invalidData(parse->buffer, parse->length);

            // Start searching for a preamble byte
            sempFirstByte(parse, data);
            return;
        }

        // Save the data byte
        parse->buffer[parse->length++] = data;

        // Compute the CRC value for the message
        if (parse->computeCrc)
            parse->crc = parse->computeCrc(parse, data);

        // Update the parser state based on the incoming byte
        parse->state(parse, data);
    }
}

//----------------------------------------
// Parse the next bytes
//----------------------------------------
void sempParseNextBytes(SEMP_PARSE_STATE *parse, const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++)
        sempParseNextByte(parse, *data++);
}

//----------------------------------------
// Set the invalid data callback
//----------------------------------------
void sempSetInvalidDataCallback(SEMP_PARSE_STATE *parse,
                                SEMP_INVALID_DATA_CALLBACK invalidDataCallback)
{
    parse->invalidData = invalidDataCallback;
}

//----------------------------------------
// Display an address value
//----------------------------------------
void sempPrintAddr(SEMP_OUTPUT output, const void *addr)
{
    // Determine if output is necessary
    if (output)
        // Output the hex value
        sempPrintHex0x08x(output, (uintptr_t)addr);
}

//----------------------------------------
// Display an address value followed by a CR and LF
//----------------------------------------
void sempPrintAddrLn(SEMP_OUTPUT output, const void *addr)
{
    // Determine if output is necessary
    if (output)
    {
        // Output the hex value
        sempPrintHex0x08x(output, (uintptr_t)addr);
        sempPrintLn(output);
    }
}

//----------------------------------------
// Display a character
//----------------------------------------
void sempPrintChar(SEMP_OUTPUT output, char character)
{
    // Determine if output is necessary
    if (output)
        // Output the character
        output(character);
}

//----------------------------------------
// Display a character followed by a CR and LF
//----------------------------------------
void sempPrintCharLn(SEMP_OUTPUT output, char character)
{
    // Determine if output is necessary
    if (output)
    {
        // Output the character
        output(character);
        sempPrintLn(output);
    }
}

//----------------------------------------
// Display a signed 32-bit decimal value
//----------------------------------------
void sempPrintDecimalI32(SEMP_OUTPUT output, int32_t value, int fieldWidth)
{
    int digits;

    // Determine if output is necessary
    if (output)
    {
        // Count the digits
        digits = 0;
        if (fieldWidth)
        {
            digits = sempGetI4Digits(value);

            // Right justify the field with a positive fieldWidth value
            fieldWidth = sempJustifyField(output, fieldWidth, digits);
        }

        // Display the minus sign if necessary
        if (value < 0)
            output('-');
        sempPrintDecimalU32(output, (uint32_t)((value >= 0) ? value : -value), 0);

        // Left justify the field if necessary
        if (fieldWidth)
            sempJustifyField(output, fieldWidth, digits);
    }
}

//----------------------------------------
// Display a signed 32-bit decimal value followed by a CR and LF
//----------------------------------------
void sempPrintDecimalI32Ln(SEMP_OUTPUT output, int32_t value, int fieldWidth)
{
    // Determine if output is necessary
    if (output)
    {
        sempPrintDecimalI32(output, value, fieldWidth);
        sempPrintLn(output);
    }
}

//----------------------------------------
// Display a signed 64-bit decimal value
//----------------------------------------
void sempPrintDecimalI64(SEMP_OUTPUT output, int64_t value, int fieldWidth)
{
    int digits;

    // Determine if output is necessary
    if (output)
    {
        // Count the digits
        digits = 0;
        if (fieldWidth)
        {
            digits = sempGetI8Digits(value);

            // Right justify the field with a positive fieldWidth value
            fieldWidth = sempJustifyField(output, fieldWidth, digits);
        }

        // Display the minus sign if necessary
        if (value < 0)
            output('-');
        sempPrintDecimalU64(output, (uint64_t)((value >= 0) ? value : -value), 0);

        // Left justify the field if necessary
        if (fieldWidth)
            sempJustifyField(output, fieldWidth, digits);
    }
}

//----------------------------------------
// Display a signed 64-bit decimal value followed by a CR and LF
//----------------------------------------
void sempPrintDecimalI64Ln(SEMP_OUTPUT output, int64_t value, int fieldWidth)
{
    // Determine if output is necessary
    if (output)
    {
        sempPrintDecimalI64(output, value, fieldWidth);
        sempPrintLn(output);
    }
}

//----------------------------------------
// Display an unsigned 32-bit decimal value
//----------------------------------------
void sempPrintDecimalU32(SEMP_OUTPUT output, uint32_t value, int fieldWidth)
{
    int digit;
    int digits;
    int index;
    const uint32_t power10[] =
    {
        1 * 1000 * 1000 * 1000,
        100 * 1000 * 1000,
        10 * 1000 * 1000,
        1 * 1000 * 1000,
        100 * 1000,
        10 * 1000,
        1 * 1000,
        100,
        10,
        1,
    };
    const int entries = sizeof(power10) / sizeof(power10[0]);
    bool suppressZeros;
    uint32_t u32;

    // Determine if output is necessary
    if (output)
    {
        u32 = value;

        // Count the digits
        digits = 0;
        if (fieldWidth)
        {
            digits = sempGetU4Digits(value);

            // Right justify the field with a positive fieldWidth value
            fieldWidth = sempJustifyField(output, fieldWidth, digits);
        }

        // Output the decimal value
        suppressZeros = true;
        for (index = 0; index < entries; index++)
        {
            digit = u32 / power10[index];
            u32 -= digit * power10[index];
            if ((digit == 0) && suppressZeros && (index != (entries - 1)))
                continue;
            suppressZeros = false;
            output(sempNibbleToAscii(digit));
        }

        // Left justify the field if necessary
        if (fieldWidth)
            sempJustifyField(output, fieldWidth, digits);
    }
}

//----------------------------------------
// Display an unsigned 32-bit decimal value followed by a CR and LF
//----------------------------------------
void sempPrintDecimalU32Ln(SEMP_OUTPUT output, uint32_t value, int fieldWidth)
{
    // Determine if output is necessary
    if (output)
    {
        sempPrintDecimalU32(output, value, fieldWidth);
        sempPrintLn(output);
    }
}

//----------------------------------------
// Display an unsigned 64-bit decimal value
//----------------------------------------
void sempPrintDecimalU64(SEMP_OUTPUT output, uint64_t value, int fieldWidth)
{
    int digit;
    int digits;
    int index;
    bool suppressZeros;
    uint64_t u64;

    // Determine if output is necessary
    if (output)
    {
        u64 = value;

        // Count the digits
        digits = 0;
        if (fieldWidth)
        {
            digits = sempGetU8Digits(value);

            // Right justify the field with a positive fieldWidth value
            fieldWidth = sempJustifyField(output, fieldWidth, digits);
        }

        // Output the decimal value
        suppressZeros = true;
        for (index = 0; index < sempPower10U64Entries; index++)
        {
            digit = u64 / sempPower10U64[index];
            u64 -= digit * sempPower10U64[index];
            if ((digit == 0) && suppressZeros && (index != (sempPower10U64Entries - 1)))
                continue;
            suppressZeros = false;
            output(sempNibbleToAscii(digit));
        }

        // Left justify the field if necessary
        if (fieldWidth)
            sempJustifyField(output, fieldWidth, digits);
    }
}

//----------------------------------------
// Display an unsigned 64-bit decimal value followed by a CR and LF
//----------------------------------------
void sempPrintDecimalU64Ln(SEMP_OUTPUT output, uint64_t value, int fieldWidth)
{
    // Determine if output is necessary
    if (output)
    {
        sempPrintDecimalU64(output, value, fieldWidth);
        sempPrintLn(output);
    }
}

//----------------------------------------
// Display a hex value
//----------------------------------------
void sempPrintHex016x(SEMP_OUTPUT output, uint64_t value, int fieldWidth)
{
    // Determine if output is necessary
    if (output)
    {
        // Right justify the value
        if (fieldWidth)
            fieldWidth = sempJustifyField(output, fieldWidth, 16);

        // Output the hex value
        sempPrintHex08x(output, value >> 32, 0);
        sempPrintHex08x(output, value, 0);

        // Left justify the value
        if (fieldWidth)
            fieldWidth = sempJustifyField(output, fieldWidth, 16);
    }
}

//----------------------------------------
// Display a hex value followed by a CR and LF
//----------------------------------------
void sempPrintHex016xLn(SEMP_OUTPUT output, uint64_t value, int fieldWidth)
{
    // Determine if output is necessary
    if (output)
    {
        // Output the hex value
        sempPrintHex016x(output, value, fieldWidth);
        sempPrintLn(output);
    }
}

//----------------------------------------
// Display a hex value
//----------------------------------------
void sempPrintHex02x(SEMP_OUTPUT output, uint8_t value, int fieldWidth)
{
    // Determine if output is necessary
    if (output)
    {
        // Right justify the value
        if (fieldWidth)
            fieldWidth = sempJustifyField(output, fieldWidth, 2);

        // Output the hex value
        output(sempNibbleToAscii(value >> 4));
        output(sempNibbleToAscii(value));

        // Left justify the value
        if (fieldWidth)
            fieldWidth = sempJustifyField(output, fieldWidth, 2);
    }
}

//----------------------------------------
// Display a hex value followed by a CR and LF
//----------------------------------------
void sempPrintHex02xLn(SEMP_OUTPUT output, uint8_t value, int fieldWidth)
{
    // Determine if output is necessary
    if (output)
    {
        // Output the hex value
        sempPrintHex02x(output, value, fieldWidth);
        sempPrintLn(output);
    }
}

//----------------------------------------
// Display a hex value
//----------------------------------------
void sempPrintHex04x(SEMP_OUTPUT output, uint16_t value, int fieldWidth)
{
    // Determine if output is necessary
    if (output)
    {
        // Right justify the value
        if (fieldWidth)
            fieldWidth = sempJustifyField(output, fieldWidth, 4);

        // Output the hex value
        sempPrintHex02x(output, value >> 8, 0);
        sempPrintHex02x(output, value, 0);

        // Left justify the value
        if (fieldWidth)
            fieldWidth = sempJustifyField(output, fieldWidth, 4);
    }
}

//----------------------------------------
// Display a hex value followed by a CR and LF
//----------------------------------------
void sempPrintHex04xLn(SEMP_OUTPUT output, uint16_t value, int fieldWidth)
{
    // Determine if output is necessary
    if (output)
    {
        // Output the hex value
        sempPrintHex04x(output, value, fieldWidth);
        sempPrintLn(output);
    }
}

//----------------------------------------
// Display a hex value
//----------------------------------------
void sempPrintHex08x(SEMP_OUTPUT output, uint32_t value, int fieldWidth)
{
    // Determine if output is necessary
    if (output)
    {
        // Right justify the value
        if (fieldWidth)
            fieldWidth = sempJustifyField(output, fieldWidth, 8);

        // Output the hex value
        sempPrintHex04x(output, value >> 16, 0);
        sempPrintHex04x(output, value, 0);

        // Left justify the value
        if (fieldWidth)
            fieldWidth = sempJustifyField(output, fieldWidth, 8);
    }
}

//----------------------------------------
// Display a hex value followed by a CR and LF
//----------------------------------------
void sempPrintHex08xLn(SEMP_OUTPUT output, uint32_t value, int fieldWidth)
{
    // Determine if output is necessary
    if (output)
    {
        // Output the hex value
        sempPrintHex08x(output, value, fieldWidth);
        sempPrintLn(output);
    }
}

//----------------------------------------
// Display a hex value
//----------------------------------------
void sempPrintHex0x016x(SEMP_OUTPUT output, uint64_t value, int fieldWidth)
{
    // Determine if output is necessary
    if (output)
    {
        // Right justify the value
        if (fieldWidth)
            fieldWidth = sempJustifyField(output, fieldWidth, 18);

        // Output the hex value
        sempPrintString(output, "0x");
        sempPrintHex016x(output, value, 0);

        // Left justify the value
        if (fieldWidth)
            fieldWidth = sempJustifyField(output, fieldWidth, 18);
    }
}

//----------------------------------------
// Display a hex value followed by a CR and LF
//----------------------------------------
void sempPrintHex0x016xLn(SEMP_OUTPUT output, uint64_t value, int fieldWidth)
{
    // Determine if output is necessary
    if (output)
    {
        // Output the hex value
        sempPrintHex0x016x(output, value, fieldWidth);
        sempPrintLn(output);
    }
}

//----------------------------------------
// Display a hex value
//----------------------------------------
void sempPrintHex0x02x(SEMP_OUTPUT output, uint8_t value, int fieldWidth)
{
    // Determine if output is necessary
    if (output)
    {
        // Right justify the value
        if (fieldWidth)
            fieldWidth = sempJustifyField(output, fieldWidth, 4);

        // Output the hex value
        sempPrintString(output, "0x");
        sempPrintHex02x(output, value, 0);

        // Left justify the value
        if (fieldWidth)
            fieldWidth = sempJustifyField(output, fieldWidth, 4);
    }
}

//----------------------------------------
// Display a hex value followed by a CR and LF
//----------------------------------------
void sempPrintHex0x02xLn(SEMP_OUTPUT output, uint8_t value, int fieldWidth)
{
    // Determine if output is necessary
    if (output)
    {
        // Output the hex value
        sempPrintHex0x02x(output, value, fieldWidth);
        sempPrintLn(output);
    }
}

//----------------------------------------
// Display a hex value
//----------------------------------------
void sempPrintHex0x04x(SEMP_OUTPUT output, uint16_t value, int fieldWidth)
{
    // Determine if output is necessary
    if (output)
    {
        // Right justify the value
        if (fieldWidth)
            fieldWidth = sempJustifyField(output, fieldWidth, 6);

        // Output the hex value
        sempPrintString(output, "0x");
        sempPrintHex04x(output, value, 0);

        // Left justify the value
        if (fieldWidth)
            fieldWidth = sempJustifyField(output, fieldWidth, 6);
    }
}

//----------------------------------------
// Display a hex value followed by a CR and LF
//----------------------------------------
void sempPrintHex0x04xLn(SEMP_OUTPUT output, uint16_t value, int fieldWidth)
{
    // Determine if output is necessary
    if (output)
    {
        // Output the hex value
        sempPrintHex0x04x(output, value, fieldWidth);
        sempPrintLn(output);
    }
}

//----------------------------------------
// Display a hex value
//----------------------------------------
void sempPrintHex0x08x(SEMP_OUTPUT output, uint32_t value, int fieldWidth)
{
    // Determine if output is necessary
    if (output)
    {
        // Right justify the value
        if (fieldWidth)
            fieldWidth = sempJustifyField(output, fieldWidth, 10);

        // Output the hex value
        sempPrintString(output, "0x");
        sempPrintHex08x(output, value, 0);

        // Left justify the value
        if (fieldWidth)
            fieldWidth = sempJustifyField(output, fieldWidth, 10);
    }
}

//----------------------------------------
// Display a hex value followed by a CR and LF
//----------------------------------------
void sempPrintHex0x08xLn(SEMP_OUTPUT output, uint32_t value, int fieldWidth)
{
    // Determine if output is necessary
    if (output)
    {
        // Output the hex value
        sempPrintHex0x08x(output, value, fieldWidth);
        sempPrintLn(output);
    }
}

//----------------------------------------
// Display a carriage return and line feed
//----------------------------------------
void sempPrintLn(SEMP_OUTPUT output)
{
    // Determine if output is necessary
    if (output)
    {
        // Output the carriage return and linefeed
        output('\r');
        output('\n');
    }
}

//----------------------------------------
// Print the parser's configuration
//----------------------------------------
void sempPrintParserConfiguration(SEMP_PARSE_STATE *parse, SEMP_OUTPUT output)
{
    if (output && parse)
    {
        sempPrintString(output, "SparkFun Extensible Message Parser\r\n");

        sempPrintString(output, "    parserName: ");
        sempPrintAddr(output, (void *)parse->parserName);
        sempPrintString(output, " (");
        sempPrintString(output, parse->parserName);
        sempPrintCharLn(output, ')');

        sempPrintString(output, "    parsers: ");
        sempPrintAddrLn(output, (void *)parse->parsers);

        sempPrintString(output, "    parserCount: ");
        sempPrintDecimalI32Ln(output, parse->parserCount);

        sempPrintString(output, "    debugOutput: ");
        sempPrintAddrLn(output, (void *)parse->debugOutput);

        sempPrintString(output, "    printError: ");
        sempPrintAddrLn(output, (void *)parse->printError);

        sempPrintString(output, "    verboseDebug: ");
        sempPrintDecimalI32Ln(output, parse->verboseDebug);

        sempPrintString(output, "    nmeaAbortOnNonPrintable: ");
        sempPrintDecimalI32Ln(output, parse->nmeaAbortOnNonPrintable);

        sempPrintString(output, "    unicoreHashAbortOnNonPrintable: ");
        sempPrintDecimalI32Ln(output, parse->unicoreHashAbortOnNonPrintable);

        sempPrintString(output, "    scratchPad: ");
        sempPrintAddr(output, (void *)parse->scratchPad);
        sempPrintString(output, " (");
        if (parse->scratchPad)
            sempPrintDecimalU32(output, parse->buffer - (uint8_t *)parse->scratchPad);
        else
            output('0');
        sempPrintStringLn(output, " bytes)");

        sempPrintString(output, "    badCrc: ");
        sempPrintAddrLn(output, (void *)parse->badCrc);

        sempPrintString(output, "    computeCrc: ");
        sempPrintAddrLn(output, (void *)parse->computeCrc);

        sempPrintString(output, "    crc: 0x");
        sempPrintHex08xLn(output, parse->crc);

        sempPrintString(output, "    state: ");
        sempPrintAddr(output, (void *)parse->state);
        sempPrintStringLn(output, (parse->state == sempFirstByte) ? " (sempFirstByte)"
                                                                  : "");

        sempPrintString(output, "    eomCallback: ");
        sempPrintAddrLn(output, (void *)parse->eomCallback);

        sempPrintString(output, "    invalidData: ");
        sempPrintAddrLn(output, (void *)parse->invalidData);

        sempPrintString(output, "    buffer: ");
        sempPrintAddr(output, (void *)parse->buffer);
        sempPrintString(output, " (");
        sempPrintDecimalU32(output, parse->bufferLength);
        sempPrintStringLn(output, " bytes)");

        sempPrintString(output, "    length: ");
        sempPrintDecimalU32(output, parse->length);
        sempPrintStringLn(output, " message bytes");

        sempPrintString(output, "    type: ");
        sempPrintDecimalI32(output, parse->type);
        sempPrintString(output, " (");
        sempPrintString(output, sempGetTypeName(parse, parse->type));
        sempPrintCharLn(output, ')');
    }
}

//----------------------------------------
// Display a string
//----------------------------------------
void sempPrintString(SEMP_OUTPUT output, const char * string, int fieldWidth)
{
    int stringWidth;

    // Determine if output is necessary
    if (output && string)
    {
        // Right justify the string
        stringWidth = 0;
        if (fieldWidth)
        {
            stringWidth = strlen(string);
            fieldWidth = sempJustifyField(output, fieldWidth, stringWidth);
        }

        // Output the string a character at a time
        while (*string)
            output(*string++);

        // Left justify the string
        if (fieldWidth)
            fieldWidth = sempJustifyField(output, fieldWidth, stringWidth);
    }
}

//----------------------------------------
// Display a string followed by a CR and LF
//----------------------------------------
void sempPrintStringLn(SEMP_OUTPUT output, const char * string, int fieldWidth)
{
    // Determine if output is necessary
    if (output && string)
    {
        // Output the string a character at a time
        sempPrintString(output, string, fieldWidth);
        sempPrintLn(output);
    }
}

//----------------------------------------
// Shutdown the parser
//----------------------------------------
void sempStopParser(SEMP_PARSE_STATE **parse)
{
    // Prevent further references to the structure
    if (parse && *parse)
        *parse = nullptr;
}

//------------------------------------------------------------------------------
// V1 routines to eliminate
//------------------------------------------------------------------------------

//----------------------------------------
// Enable error output
//----------------------------------------
void sempEnableErrorOutput(SEMP_PARSE_STATE *parse, Print *print)
{
    if (parse)
        parse->printError = print;
}

//----------------------------------------
// Format and print a line of text
//----------------------------------------
void sempPrintf(Print *print, const char *format, ...)
{
    if (print)
    {
        // https://stackoverflow.com/questions/42131753/wrapper-for-printf
        va_list args;
        va_start(args, format);
        va_list args2;

        va_copy(args2, args);
        char buf[vsnprintf(nullptr, 0, format, args) + sizeof("\r\n")];

        vsnprintf(buf, sizeof buf, format, args2);

        // Add CR+LF
        buf[sizeof(buf) - 3] = '\r';
        buf[sizeof(buf) - 2] = '\n';
        buf[sizeof(buf) - 1] = '\0';

        print->write(buf, strlen(buf));

        va_end(args);
        va_end(args2);
    }
}

//----------------------------------------
// Print a line of error text
//----------------------------------------
void sempPrintln(Print *print, const char *string)
{
    if (print)
        print->println(string);
}
