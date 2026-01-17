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

//----------------------------------------
// Support routines
//----------------------------------------

// Compute the scratch pad length
// Inputs:
//   parseTable: Address of an array of SEMP_PARSER_DESCRIPTION addresses
//   parserCount:  Number of entries in the parseTable
//
// Outputs:
//    Returns the number of bytes needed for the scratch pad
size_t sempGetScratchPadLength(SEMP_PARSER_DESCRIPTION **parserTable,
                               uint16_t parserCount)
{
    size_t length;

    // Walk the list of parser descriptions and determine the largest
    // scratch pad size.
    length = 0;
    for (int index = 0; index < parserCount; index++)
        if (length < parserTable[index]->scratchPadBytes)
            length = parserTable[index]->scratchPadBytes;

    // Align the scratch pad area
    return SEMP_ALIGN(length);
}

// Locate the parse structure
// Inputs:
//   scratchPadBytes: Desired size of the scratch pad in bytes
//   buffer: Address of the buffer
//   bufferLength: Total number of bytes in the buffer
//   printDebug: Device to output any debug messages, may be nullptr
//
// Outputs:
//    Returns the number of bytes needed for the scratch pad
SEMP_PARSE_STATE * sempLocateParseStructure(uint16_t scratchPadBytes,
                                            uint8_t *buffer,
                                            size_t bufferLength,
                                            Print *printDebug)
{
    int length;
    SEMP_PARSE_STATE * parse;
    size_t parseDataBytes;
    size_t parseStateBytes;

    // Determine the size needed for the parse state
    parseStateBytes = SEMP_ALIGN(sizeof(SEMP_PARSE_STATE));
    sempPrintf(printDebug, "0x%08lx (%ld): parseStateBytes",
               parseStateBytes, parseStateBytes);

    // Determine the minimum length for the scratch pad
    sempPrintf(printDebug, "0x%08lx (%ld): scratchPadBytes",
               scratchPadBytes, scratchPadBytes);

    // Determine the remaining length for the parser data
    parseDataBytes = bufferLength - parseStateBytes - scratchPadBytes;
    if ((parseDataBytes > bufferLength) || (parseDataBytes < SEMP_MINIMUM_BUFFER_LENGTH))
    {
        // Determine the minimum buffer size
        length = parseStateBytes + scratchPadBytes + SEMP_MINIMUM_BUFFER_LENGTH;
        sempPrintf(printDebug, "SEMP ERROR: Increase buffer size to >= %d bytes for >= %d bytes of parse data\r\n",
                   length, SEMP_MINIMUM_BUFFER_LENGTH);

        // Handle the buffer error cases
        return nullptr;
    }
    sempPrintf(printDebug, "0x%08lx (%ld): parseDataBytes",
               parseDataBytes, parseDataBytes);

    // Set the parser
    parse = (SEMP_PARSE_STATE *)buffer;
    sempPrintf(printDebug, "%p: parse state", (void *)parse);

    // Zero the parse structure
    memset(parse, 0, parseStateBytes + scratchPadBytes + parseDataBytes);

    // Set the scratch pad area address
    parse->scratchPad = ((uint8_t *)parse) + parseStateBytes;
    parse->printDebug = printDebug;
    sempPrintf(parse->printDebug, "%p: parse->scratchPad", parse->scratchPad);

    // Set the buffer address and length
    parse->bufferLength = parseDataBytes;
    parse->buffer = ((uint8_t *)parse->scratchPad + scratchPadBytes);
    sempPrintf(parse->printDebug, "%p: parse->buffer", parse->buffer);

    // Use nullptr for scratchPad when length is zero
    if (scratchPadBytes == 0)
        parse->scratchPad = nullptr;
    return parse;
}

// Convert nibble to ASCII
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

// Translate the type value into an ASCII type name
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

// Print the parser's configuration
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

// Format and print a line of text
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

// Print a line of error text
void sempPrintln(Print *print, const char *string)
{
    if (print)
        print->println(string);
}

// Translates state value into an ASCII state name
const char * sempGetStateName(const SEMP_PARSE_STATE *parse)
{
    if (parse && (parse->state == sempFirstByte))
        return "sempFirstByte";
    return "Unknown state";
}

// Disable debug output
void sempDisableDebugOutput(SEMP_PARSE_STATE *parse)
{
    if (parse)
        parse->printDebug = nullptr;
}

// Enable debug output
void sempEnableDebugOutput(SEMP_PARSE_STATE *parse, Print *print, bool verbose)
{
    if (parse)
    {
        parse->printDebug = print;
        parse->verboseDebug = verbose;
    }
}

// Disable error output
void sempDisableErrorOutput(SEMP_PARSE_STATE *parse)
{
    if (parse)
        parse->printError = nullptr;
}

// Enable error output
void sempEnableErrorOutput(SEMP_PARSE_STATE *parse, Print *print)
{
    if (parse)
        parse->printError = print;
}

//----------------------------------------
// Parse routines
//----------------------------------------

// Initialize the parser
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
    SEMP_PARSE_STATE *parse = nullptr;
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

        // Validate the parser address is not nullptr
        scratchPadBytes = sempGetScratchPadLength(parserTable, parserCount);
        parse = sempLocateParseStructure(scratchPadBytes,
                                         buffer,
                                         bufferLength,
                                         printDebug);
        if (!parse)
        {
            sempPrintln(printError, "SEMP: Failed to allocate the parse structure");
            break;
        }

        // Initialize the parse state
        parse->printError = printError;
        parse->parsers = parserTable;
        parse->parserCount = parserCount;
        parse->state = sempFirstByte;
        parse->eomCallback = eomCallback;
        parse->parserName = parserTableName;
        parse->badCrc = badCrc;

        // Display the parser configuration
        sempPrintParserConfiguration(parse, parse->printDebug);
    } while (0);

    // Return the parse structure address
    return parse;
}

// Wait for the first byte in the data stream
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

        // Indicate no parser selected
        parse->type = parse->parserCount;

        // Walk through the parse table
        for (index = 0; index < parse->parserCount; index++)
        {
            // Determine if this parser is able to parse this stream
            parseDescripion = parse->parsers[index];
            if (parseDescripion->preamble(parse, data))
            {
                // This parser claims to be able to parse this stream
                parse->type = index;
                return true;
            }
        }

        // Preamble byte not found, continue searching for a preamble byte
        parse->state = sempFirstByte;
    }
    return false;
}

// Compute the necessary buffer length in bytes to support the scratch pad
// and parse buffer lengths.
// Inputs:
//   parseTable: Address of an array of SEMP_PARSER_DESCRIPTION addresses
//   parserCount:  Number of entries in the parseTable
//   parseBufferBytes: Desired size of the parse buffer in bytes
//   printDebug: Device to output any debug messages, may be nullptr
//
// Outputs:
//    Returns the number of bytes needed for the buffer that contains
//    the SEMP parser state, a scratch pad area and the parse buffer
size_t sempGetBufferLength(SEMP_PARSER_DESCRIPTION **parserTable,
                           uint16_t parserCount,
                           size_t parserBufferBytes,
                           Print *printDebug)
{
    size_t length;
    size_t parseStateBytes;
    size_t scratchPadBytes;

    // Determine the size needed to maintain the parser state
    parseStateBytes = SEMP_ALIGN(sizeof(SEMP_PARSE_STATE));

    // Determine the minimum length for the scratch pad
    scratchPadBytes = sempGetScratchPadLength(parserTable, parserCount);

    // Verify the minimum bufferLength
    if (parserBufferBytes < SEMP_MINIMUM_BUFFER_LENGTH)
    {
        sempPrintf(printDebug,
                   "SEMP: Increasing parserBufferBytes from %ld to %ld bytes, minimum size adjustment",
                   parserBufferBytes, SEMP_MINIMUM_BUFFER_LENGTH);
        parserBufferBytes = SEMP_MINIMUM_BUFFER_LENGTH;
    }

    // Determine the final buffer length
    length = parseStateBytes + scratchPadBytes + parserBufferBytes;
    sempPrintf(printDebug, "SEMP: Buffer length needed is %ld bytes", length);
    return length;
}

// Parse the next byte
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

// Parse the next bytes
void sempParseNextBytes(SEMP_PARSE_STATE *parse, const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++)
        sempParseNextByte(parse, *data++);
}

// Shutdown the parser
void sempStopParser(SEMP_PARSE_STATE **parse)
{
    // Prevent further references to the structure
    if (parse && *parse)
        *parse = nullptr;
}

//----------------------------------------
// Parser specific routines
//----------------------------------------

// Abort NMEA parsing on a non-printable char
void sempNmeaAbortOnNonPrintable(SEMP_PARSE_STATE *parse, bool abort)
{
    if (parse)
        parse->nmeaAbortOnNonPrintable = abort;
}

// Abort Unicore hash parsing on a non-printable char
void sempUnicoreHashAbortOnNonPrintable(SEMP_PARSE_STATE *parse, bool abort)
{
    if (parse)
        parse->unicoreHashAbortOnNonPrintable = abort;
}
