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
SEMP_PARSE_STATE * sempAllocateParseStructure(uint16_t scratchPadBytes, size_t bufferLength)
{
    int length;
    char line[128];
    SEMP_PARSE_STATE *parse = nullptr;
    int parseBytes;

    // Print the scratchPad area size
    if (sempPrintErrorMessages)
    {
        sprintf(line, "scratchPadBytes: 0x%04x (%d) bytes", scratchPadBytes, scratchPadBytes);
        sempExtPrintLineOfText(line);
    }

    // Align the scratch patch area
    if (scratchPadBytes < SEMP_ALIGN(scratchPadBytes))
    {
        scratchPadBytes = SEMP_ALIGN(scratchPadBytes);
        if (sempPrintErrorMessages)
        {
            sprintf(line, "scratchPadBytes: 0x%04x (%d) bytes after alignment", scratchPadBytes, scratchPadBytes);
            sempExtPrintLineOfText(line);
        }
    }

    // Determine the minimum length for the scratch pad
    length = SEMP_ALIGN(sizeof(SEMP_SCRATCH_PAD));
    if (scratchPadBytes < length)
    {
        scratchPadBytes = length;
        if (sempPrintErrorMessages)
        {
            sprintf(line, "scratchPadBytes: 0x%04x (%d) bytes after mimimum size adjustment", scratchPadBytes, scratchPadBytes);
            sempExtPrintLineOfText(line);
        }
    }
    parseBytes = SEMP_ALIGN(sizeof(SEMP_PARSE_STATE));
    if (sempPrintErrorMessages)
    {
        sprintf(line, "parseBytes: 0x%04x (%d)", parseBytes, parseBytes);
        sempExtPrintLineOfText(line);
    }

    // Verify the minimum bufferLength
    if (bufferLength < SEMP_MINIMUM_BUFFER_LENGTH)
    {
        if (sempPrintErrorMessages)
        {
            char line[128];
            sprintf(line, "SEMP: Increasing bufferLength from %d to %d bytes, minimum size adjustment",
                    bufferLength, SEMP_MINIMUM_BUFFER_LENGTH);
            sempExtPrintLineOfText(line);
        }
        bufferLength = SEMP_MINIMUM_BUFFER_LENGTH;
    }

    // Allocate the parser
    length = parseBytes + scratchPadBytes;
    parse = (SEMP_PARSE_STATE *)malloc(length + bufferLength);
    if (sempPrintErrorMessages)
    {
        sprintf(line, "parse: %p", parse);
        sempExtPrintLineOfText(line);
    }

    // Initialize the parse structure
    if (parse)
    {
        // Zero the parse structure
        memset(parse, 0, length);

        // Set the scratch pad area address
        parse->scratchPad = ((void *)parse) + parseBytes;
        if (sempPrintErrorMessages)
        {
            sprintf(line, "parse->scratchPad: %p", parse->scratchPad);
            sempExtPrintLineOfText(line);
        }

        // Set the buffer address and length
        parse->bufferLength = bufferLength;
        parse->buffer = (uint8_t *)(parse->scratchPad + scratchPadBytes);
        if (sempPrintErrorMessages)
        {
            sprintf(line, "parse->buffer: %p", parse->buffer);
            sempExtPrintLineOfText(line);
        }
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
    char line[256];

    sempExtPrintLineOfText("SparkFun Extensible Message Parser");
    sprintf(line, "    Name: %s", parse->parserName);
    sempExtPrintLineOfText(line);
    sprintf(line, "    Scratch Pad: %p (%d bytes)",
            (void *)parse->scratchPad, parse->buffer - (uint8_t *)parse->scratchPad);
    sempExtPrintLineOfText(line);
    sprintf(line, "    Buffer: %p (%d bytes)",
            (void *)parse->buffer, parse->bufferLength);
    sempExtPrintLineOfText(line);
    sprintf(line, "    State: %p%s", (void *)parse->state,
            (parse->state == sempFirstByte) ? " (sempFirstByte)" : "");
    sempExtPrintLineOfText(line);
    sprintf(line, "    EomCallback: %p", (void *)parse->eomCallback);
    sempExtPrintLineOfText(line);
    sprintf(line, "    computeCrc: %p", (void *)parse->computeCrc);
    sempExtPrintLineOfText(line);
    sprintf(line, "    crc: 0x%08x", parse->crc);
    sempExtPrintLineOfText(line);
    sprintf(line, "    length: %d", parse->length);
    sempExtPrintLineOfText(line);
    sprintf(line, "    type: %d (%s)", parse->type, sempGetTypeName(parse, parse->type));
    sempExtPrintLineOfText(line);
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
    const char *parserName
    )
{
    SEMP_PARSE_STATE *parse = nullptr;

    do
    {
        // Validate the parse type names table
        if (parserCount != parserNameCount)
        {
            sempExtPrintLineOfText("SEMP: Please fix parserTable and parserNameTable parserCount != parserNameCount");
            break;
        }

        // Validate the parserTable address is not nullptr
        if (!parserTable)
        {
            sempExtPrintLineOfText("SEMP: Please specify a parserTable data structure");
            break;
        }

        // Validate the parserNameTable address is not nullptr
        if (!parserNameTable)
        {
            sempExtPrintLineOfText("SEMP: Please specify a parserNameTable data structure");
            break;
        }

        // Validate the end-of-message callback routine address is not nullptr
        if (!eomCallback)
        {
            sempExtPrintLineOfText("SEMP: Please specify an eomCallback routine");
            break;
        }

        // Verify the parser name
        if ((!parserName) || (!strlen(parserName)))
        {
            sempExtPrintLineOfText("SEMP: Please provide a name for the parser");
            break;
        }

        // Verify that there is at least one parser in the table
        if (!parserCount)
        {
            sempExtPrintLineOfText("SEMP: Please provide at least one parser in parserTable");
            break;
        }

        // Validate the parser address is not nullptr
        parse = sempAllocateParseStructure(scratchPadBytes, bufferLength);
        if (!parse)
        {
            sempExtPrintLineOfText("SEMP: Failed to allocate the parse structure");
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
        char line[128];
        sprintf(line, "SEMP %s NMEA: Message too long, increase the buffer size > %d",
                parse->parserName,
                parse->bufferLength);
        sempExtPrintLineOfText(line);

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
