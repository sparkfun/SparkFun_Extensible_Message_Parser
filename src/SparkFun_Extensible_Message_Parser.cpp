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
    Print *printError,
    Print *printDebug,
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
        parse->printError = printError;
        parse->parsers = parserTable;
        parse->parserCount = parserCount;
        parse->state = sempFirstByte;
        parse->eomCallback = eomCallback;
        parse->parserName = parserTableName;
        parse->badCrc = badCrc;
        parse->type = parserCount;  // No active parser

        // Display the parser configuration
        sempPrintParserConfiguration(parse, parse->printDebug);
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
void sempDisableDebugOutput(SEMP_PARSE_STATE *parse)
{
    if (parse)
        parse->printDebug = nullptr;
}

//----------------------------------------
// Disable error output
//----------------------------------------
void sempDisableErrorOutput(SEMP_PARSE_STATE *parse)
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
                           Print *printDebug)
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
        sempPrintf(printDebug, "SEMP: Increasing parse area from %d to %d bytes, due to minimize size requirement",
                   parseAreaBytes, parseArea);
        parseAreaBytes = parseArea;
    }

    // Verify the parse area meets the minimum payload offset
    if (parseAreaBytes < payloadOffset)
    {
        // Display the minimum buffer length
        sempPrintf(printDebug, "SEMP: Increasing parse area from %d to %d bytes, due to payload offset requirement",
                   parseAreaBytes, payloadOffset);
        parseAreaBytes = payloadOffset;
    }

    // Verify that there is a parse area
    if (parseAreaBytes < 1)
    {
        // Display the minimum buffer length
        sempPrintf(printDebug, "SEMP: Increasing parse area from %d to %d bytes, requires at least one byte",
                   parseAreaBytes, 1);
        parseAreaBytes = 1;
    }

    // Determine the final buffer length
    bufferLength = bufferOverhead + parseAreaBytes;

    // Display the buffer length
    sempPrintf(printDebug, "SEMP: Buffer length %d bytes", bufferLength);
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
// Get an 8-bit integer value
//----------------------------------------
uint8_t sempGetU1(const SEMP_PARSE_STATE *parse, size_t offset)
{
    uint8_t value;
    SEMP_PARSER_DESCRIPTION *parserDescription = parse->parsers[parse->type];
    value = *(uint8_t *)&parse->buffer[offset + parserDescription->payloadOffset];
    return value;
}

//----------------------------------------
// Get an 8-bit integer value
//----------------------------------------
uint8_t sempGetU1NoOffset(const SEMP_PARSE_STATE *parse, size_t offset)
{
    uint8_t value;
    SEMP_PARSER_DESCRIPTION *parserDescription = parse->parsers[parse->type];
    value = *(uint8_t *)&parse->buffer[offset];
    return value;
}

//----------------------------------------
// Get a 16-bit integer value
//----------------------------------------
uint16_t sempGetU2(const SEMP_PARSE_STATE *parse, size_t offset)
{
    uint16_t value;
    SEMP_PARSER_DESCRIPTION *parserDescription = parse->parsers[parse->type];
    memcpy(&value, &parse->buffer[offset + parserDescription->payloadOffset], sizeof(value));
    return value;
}

//----------------------------------------
// Get a 16-bit integer value
//----------------------------------------
uint16_t sempGetU2NoOffset(const SEMP_PARSE_STATE *parse, size_t offset)
{
    uint16_t value;
    SEMP_PARSER_DESCRIPTION *parserDescription = parse->parsers[parse->type];
    memcpy(&value, &parse->buffer[offset], sizeof(value));
    return value;
}

//----------------------------------------
// Get a 32-bit integer value
//----------------------------------------
uint32_t sempGetU4(const SEMP_PARSE_STATE *parse, size_t offset)
{
    uint32_t value;
    SEMP_PARSER_DESCRIPTION *parserDescription = parse->parsers[parse->type];
    memcpy(&value, &parse->buffer[offset + parserDescription->payloadOffset], sizeof(value));
    return value;
}

//----------------------------------------
// Get a 32-bit integer value
//----------------------------------------
uint32_t sempGetU4NoOffset(const SEMP_PARSE_STATE *parse, size_t offset)
{
    uint32_t value;
    SEMP_PARSER_DESCRIPTION *parserDescription = parse->parsers[parse->type];
    memcpy(&value, &parse->buffer[offset], sizeof(value));
    return value;
}

//----------------------------------------
// Get a 64-bit integer value
//----------------------------------------
uint64_t sempGetU8(const SEMP_PARSE_STATE *parse, size_t offset)
{
    uint64_t value;
    SEMP_PARSER_DESCRIPTION *parserDescription = parse->parsers[parse->type];
    memcpy(&value, &parse->buffer[offset + parserDescription->payloadOffset], sizeof(value));
    return value;
}

//----------------------------------------
// Get a 64-bit integer value
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
// Print the parser's configuration
//----------------------------------------
void sempPrintParserConfiguration(SEMP_PARSE_STATE *parse, Print *print)
{
    if (print && parse)
    {
        sempPrintln(print, "SparkFun Extensible Message Parser");
        sempPrintf(print, "    parserName: %p (%s)", parse->parserName, parse->parserName);
        sempPrintf(print, "    parsers: %p", (void *)parse->parsers);
        sempPrintf(print, "    parserCount: %d", parse->parserCount);
        sempPrintf(print, "    printError: %p", parse->printError);
        sempPrintf(print, "    printDebug: %p", parse->printDebug);
        sempPrintf(print, "    verboseDebug: %d", parse->verboseDebug);
        sempPrintf(print, "    nmeaAbortOnNonPrintable: %d", parse->nmeaAbortOnNonPrintable);
        sempPrintf(print, "    unicoreHashAbortOnNonPrintable: %d", parse->unicoreHashAbortOnNonPrintable);
        sempPrintf(print, "    Scratch Pad: %p (%ld bytes)",
                   (void *)parse->scratchPad,
                   parse->scratchPad ? (parse->buffer - (uint8_t *)parse->scratchPad)
                                     : 0);
        sempPrintf(print, "    computeCrc: %p", (void *)parse->computeCrc);
        sempPrintf(print, "    crc: 0x%08x", parse->crc);
        sempPrintf(print, "    State: %p%s", (void *)parse->state,
                   (parse->state == sempFirstByte) ? " (sempFirstByte)" : "");
        sempPrintf(print, "    EomCallback: %p", (void *)parse->eomCallback);
        sempPrintf(print, "    Buffer: %p (%d bytes)",
                   (void *)parse->buffer, parse->bufferLength);
        sempPrintf(print, "    length: %d message bytes", parse->length);
        sempPrintf(print, "    type: %d (%s)", parse->type, sempGetTypeName(parse, parse->type));
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
// Enable debug output
//----------------------------------------
void sempEnableDebugOutput(SEMP_PARSE_STATE *parse, Print *print, bool verbose)
{
    if (parse)
    {
        parse->printDebug = print;
        parse->verboseDebug = verbose;
    }
}

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
