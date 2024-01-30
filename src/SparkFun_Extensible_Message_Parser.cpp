/*------------------------------------------------------------------------------
SparkFun_Extensible_Message_Parser.cpp

  Parse messages from GNSS radios
------------------------------------------------------------------------------*/

#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include "SparkFun_Extensible_Message_Parser.h"

//----------------------------------------
// Constants
//----------------------------------------

#define SEMP_ALIGNMENT_MASK        7

//----------------------------------------
// Macros
//----------------------------------------

#define SEMP_ALIGN(x)   ((x + SEMP_ALIGNMENT_MASK) & (~SEMP_ALIGNMENT_MASK))

//----------------------------------------
// Globals
//----------------------------------------

bool sempPrintErrorMessages;

//----------------------------------------
// Support routines
//----------------------------------------

// Allocate the parse structure
SEMP_PARSE_STATE * sempAllocateParseStructure(
    Print *printError,
    uint16_t scratchPadBytes,
    size_t bufferLength
    )
{
    int length;
    SEMP_PARSE_STATE *parse = nullptr;
    int parseBytes;

    // Print the scratchPad area size
    sempPrintf(printError, "scratchPadBytes: 0x%04x (%d) bytes",
               scratchPadBytes, scratchPadBytes);

    // Align the scratch patch area
    if (scratchPadBytes < SEMP_ALIGN(scratchPadBytes))
    {
        scratchPadBytes = SEMP_ALIGN(scratchPadBytes);
        sempPrintf(printError,
                   "scratchPadBytes: 0x%04x (%d) bytes after alignment",
                   scratchPadBytes, scratchPadBytes);
    }

    // Determine the minimum length for the scratch pad
    length = SEMP_ALIGN(sizeof(SEMP_SCRATCH_PAD));
    if (scratchPadBytes < length)
    {
        scratchPadBytes = length;
        sempPrintf(printError,
                   "scratchPadBytes: 0x%04x (%d) bytes after mimimum size adjustment",
                   scratchPadBytes, scratchPadBytes);
    }
    parseBytes = SEMP_ALIGN(sizeof(SEMP_PARSE_STATE));
    sempPrintf(printError, "parseBytes: 0x%04x (%d)", parseBytes, parseBytes);

    // Verify the minimum bufferLength
    if (bufferLength < SEMP_MINIMUM_BUFFER_LENGTH)
    {
        sempPrintf(printError,
                   "SEMP: Increasing bufferLength from %d to %d bytes, minimum size adjustment",
                   bufferLength, SEMP_MINIMUM_BUFFER_LENGTH);
        bufferLength = SEMP_MINIMUM_BUFFER_LENGTH;
    }

    // Allocate the parser
    length = parseBytes + scratchPadBytes;
    parse = (SEMP_PARSE_STATE *)malloc(length + bufferLength);
    sempPrintf(printError, "parse: %p", parse);

    // Initialize the parse structure
    if (parse)
    {
        // Zero the parse structure
        memset(parse, 0, length);

        // Set the scratch pad area address
        parse->scratchPad = ((void *)parse) + parseBytes;
        parse->printError = printError;
        sempPrintf(parse->printError, "parse->scratchPad: %p", parse->scratchPad);

        // Set the buffer address and length
        parse->bufferLength = bufferLength;
        parse->buffer = (uint8_t *)(parse->scratchPad + scratchPadBytes);
        sempPrintf(parse->printError, "parse->buffer: %p", parse->buffer);
    }
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

// Translate the type value into a name
const char * sempGetTypeName(SEMP_PARSE_STATE *parse, uint8_t type)
{
    if (!type)
        return "Scanning for preamble";
    if (type > parse->parserCount)
        return "Unknown parser";
    return parse->parserNames[type - 1];
}

// Print the parser's configuration
void sempPrintParserConfiguration(SEMP_PARSE_STATE *parse)
{
    if (parse->printError)
    {
        sempPrintln(parse->printError, "SparkFun Extensible Message Parser");
        sempPrintf(parse->printError, "    Name: %s", parse->parserName);
        sempPrintf(parse->printError, "    parsers: %p", parse->parsers);
        sempPrintf(parse->printError, "    parserNames: %p", parse->parserNames);
        sempPrintf(parse->printError, "    parserCount: %d", parse->parserCount);
        sempPrintf(parse->printError, "    printError: %p", parse->printError);
        sempPrintf(parse->printError, "    printDebug: %p", parse->printDebug);
        sempPrintf(parse->printError, "    Scratch Pad: %p (%d bytes)",
                   (void *)parse->scratchPad, parse->buffer - (uint8_t *)parse->scratchPad);
        sempPrintf(parse->printError, "    computeCrc: %p", (void *)parse->computeCrc);
        sempPrintf(parse->printError, "    crc: 0x%08x", parse->crc);
        sempPrintf(parse->printError, "    State: %p%s", (void *)parse->state,
                   (parse->state == sempFirstByte) ? " (sempFirstByte)" : "");
        sempPrintf(parse->printError, "    EomCallback: %p", (void *)parse->eomCallback);
        sempPrintf(parse->printError, "    Buffer: %p (%d bytes)",
                   (void *)parse->buffer, parse->bufferLength);
        sempPrintf(parse->printError, "    length: %d message bytes", parse->length);
        sempPrintf(parse->printError, "    type: %d (%s)", parse->type, sempGetTypeName(parse, parse->type));
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

// Enable or disable debug output.  Specify nullptr to disable output.
void sempSetPrintDebug(SEMP_PARSE_STATE *parse, Print *printDebug)
{
    // Set the class to print debug output
    parse->printDebug = printDebug;
}

// Enable or disable error output.  Specify nullptr to disable output.
void sempSetPrintError(SEMP_PARSE_STATE *parse, Print *printError)
{
    // Set the class to print error output
    parse->printError = printError;
}

//----------------------------------------
// Parse routines
//----------------------------------------

// Initialize the parser
SEMP_PARSE_STATE *sempInitParser(
    const SEMP_PARSE_ROUTINE *parserTable,
    uint16_t parserCount,
    const char * const *parserNameTable,
    uint16_t parserNameCount,
    uint16_t scratchPadBytes,
    size_t bufferLength,
    SEMP_EOM_CALLBACK eomCallback,
    const char *parserName,
    Print *printError
    )
{
    SEMP_PARSE_STATE *parse = nullptr;

    do
    {
        // Validate the parse type names table
        if (parserCount != parserNameCount)
        {
            sempPrintln(printError, "SEMP: Please fix parserTable and parserNameTable parserCount != parserNameCount");
            break;
        }

        // Validate the parserTable address is not nullptr
        if (!parserTable)
        {
            sempPrintln(printError, "SEMP: Please specify a parserTable data structure");
            break;
        }

        // Validate the parserNameTable address is not nullptr
        if (!parserNameTable)
        {
            sempPrintln(printError, "SEMP: Please specify a parserNameTable data structure");
            break;
        }

        // Validate the end-of-message callback routine address is not nullptr
        if (!eomCallback)
        {
            sempPrintln(printError, "SEMP: Please specify an eomCallback routine");
            break;
        }

        // Verify the parser name
        if ((!parserName) || (!strlen(parserName)))
        {
            sempPrintln(printError, "SEMP: Please provide a name for the parser");
            break;
        }

        // Verify that there is at least one parser in the table
        if (!parserCount)
        {
            sempPrintln(printError, "SEMP: Please provide at least one parser in parserTable");
            break;
        }

        // Validate the parser address is not nullptr
        parse = sempAllocateParseStructure(printError, scratchPadBytes, bufferLength);
        if (!parse)
        {
            sempPrintln(printError, "SEMP: Failed to allocate the parse structure");
            break;
        }

        // Initialize the parser
        parse->parsers = parserTable;
        parse->parserCount = parserCount;
        parse->parserNames = parserNameTable;
        parse->state = sempFirstByte;
        parse->eomCallback = eomCallback;
        parse->parserName = parserName;

        // Display the parser configuration
        if (sempPrintErrorMessages)
            sempPrintParserConfiguration(parse);
    } while (0);

    // Return the parse structure address
    return parse;
}

// Wait for the first byte in the GPS message
bool sempFirstByte(SEMP_PARSE_STATE *parse, uint8_t data)
{
    int index;
    SEMP_PARSE_ROUTINE parseRoutine;

    // Add this byte to the buffer
    parse->crc = 0;
    parse->computeCrc = nullptr;
    parse->length = 0;
    parse->type = 0;
    parse->buffer[parse->length++] = data;

    // Walk through the parse table
    for (index = 0; index < parse->parserCount; index++)
    {
        parseRoutine = parse->parsers[index];
        if (parseRoutine(parse, data))
        {
            parse->type = index + 1;
            return true;
        }
    }

    // preamble byte not found
    parse->state = sempFirstByte;
    return false;
}

// Parse the next byte
void sempParseNextByte(SEMP_PARSE_STATE *parse, uint8_t data)
{
    // Verify that enough space exists in the buffer
    if (parse->length >= parse->bufferLength)
    {
        // Message too long
        sempPrintf(parse->printError, "SEMP %s NMEA: Message too long, increase the buffer size > %d",
                   parse->parserName,
                   parse->bufferLength);

        // Start searching for a preamble byte
        parse->type = sempFirstByte(parse, data);
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

// Shutdown the parser
void sempShutdownParser(SEMP_PARSE_STATE **parse)
{
    // Free the parse structure if it was specified
    if (parse && *parse)
    {
        free(*parse);
        *parse = nullptr;
    }
}
