/*------------------------------------------------------------------------------
Parse_NMEA.cpp

NMEA sentence parsing support routines

The parser routines within a parser module are typically placed in
reverse order within the module.  This lets the routine declaration
proceed the routine use and eliminates the need for forward declaration.
Removing the forward declaration helps reduce the exposure of the
routines to the application layer.  As such only the preamble routine
should need to be listed in SparkFun_Extensible_Message_Parser.h.

License: MIT. Please see LICENSE.md for more details
------------------------------------------------------------------------------*/

#include <stdio.h>
#include "SparkFun_Extensible_Message_Parser.h"

//----------------------------------------
// Constants
//----------------------------------------

// Save room for the asterisk, checksum, carriage return, linefeed and zero termination
#define NMEA_BUFFER_OVERHEAD    (1 + 2 + 2 + 1)

//----------------------------------------
// Types
//----------------------------------------

// NMEA parser scratch area
typedef struct _SEMP_NMEA_VALUES
{
    uint8_t sentenceName[SEMP_NMEA_SENTENCE_NAME_BYTES]; // Sentence name
    uint8_t sentenceNameLength; // Length of the sentence name
} SEMP_NMEA_VALUES;

//------------------------------------------------------------------------------
// NMEA parse routines
//
// The parser routines are placed in reverse order to define the routine before
// its use and eliminate forward declarations.  Removing the forward declaration
// helps reduce the exposure of the routines to the application layer.  The public
// data structures and routines are listed at the end of the file.
//------------------------------------------------------------------------------

//
//    NMEA Sentence
//
//    +----------+---------+--------+---------+----------+----------+
//    | Preamble |  Name   | Comma  |  Data   | Asterisk | Checksum |
//    |  8 bits  | n bytes | 8 bits | n bytes |  8 bits  | 2 bytes  |
//    |     $    |         |    ,   |         |          |          |
//    +----------+---------+--------+---------+----------+----------+
//               |                            |
//               |<-------- Checksum -------->|
//

//----------------------------------------
// Validate the checksum
//----------------------------------------
bool sempNmeaValidateChecksum(SEMP_PARSE_STATE *parse, size_t bytesToIgnore)
{
    int checksum;
    size_t length;
    SEMP_OUTPUT output;
    SEMP_NMEA_VALUES *scratchPad = (SEMP_NMEA_VALUES *)parse->scratchPad;

    // Ignore the CR and LF at the end of the buffer
    length = parse->length - bytesToIgnore;

    // Convert the checksum characters into binary
    checksum = sempAsciiToNibble(parse->buffer[length - 2]) << 4;
    checksum |= sempAsciiToNibble(parse->buffer[length - 1]);

    // Validate the checksum
    if (((checksum == parse->crc) || (parse->badCrc && (!parse->badCrc(parse))))
        && ((length + 2) <= parse->bufferLength))
    {
        // Always add the carriage return and line feed
        parse->buffer[length++] = '\r';
        parse->buffer[length++] = '\n';

        // Zero terminate the NMEA sentence, don't count this in the length
        parse->buffer[length] = 0;

        // Process this NMEA sentence
        parse->length = length;
        parse->eomCallback(parse, parse->type); // Pass parser array index
        return true;
    }

    // Display the error
    output = parse->debugOutput;
    if (output)
    {
        if ((length + 2) > parse->bufferLength)
        {
            sempPrintString(output, "ERROR SEMP ");
            sempPrintString(output, parse->parserName);
            sempPrintString(output, ": NMEA buffer is too small, increase >= ");
            sempPrintDecimalI32Ln(output, length + 2);
        }
        else
        {
            // Display the checksum error
            sempPrintString(output, "SEMP ");
            sempPrintString(output, parse->parserName);
            sempPrintString(output, ": NMEA ");
            sempPrintString(output, (const char *)scratchPad->sentenceName);
            sempPrintString(output, ", ");
            sempPrintHex0x04x(output, length);
            sempPrintString(output, " (");
            sempPrintDecimalI32(output, length);
            sempPrintString(output, ") bytes, bad checksum, received ");
            sempPrintString(output, "0x");
            output(parse->buffer[length - 2]);
            output(parse->buffer[length - 1]);
            sempPrintString(output, ", computed: ");
            sempPrintHex0x02xLn(output, parse->crc);
        }
    }

    // The data character is in the buffer, remove it because the caller
    // passes it to sempFirstByte.
    parse->length -= 1;
    sempInvalidDataCallback(parse);
    return false;
}

//----------------------------------------
// Read the linefeed
//----------------------------------------
bool sempNmeaLineFeed(SEMP_PARSE_STATE *parse, uint8_t data)
{
    // Validate the checksum and pass the sentence to the upper layer
    // Process the LF
    if (sempNmeaValidateChecksum(parse, 2) && (data == '\n'))
    {
        // Start searching for a preamble byte
        parse->state = sempFirstByte;
        return true;
    }

    // Start searching for a preamble byte
    return sempFirstByte(parse, data);
}

//----------------------------------------
// Read the remaining carriage return
//----------------------------------------
bool sempNmeaCarriageReturn(SEMP_PARSE_STATE *parse, uint8_t data)
{
    // Validate the checksum and pass the sentence to the upper layer
    // Process the CR
    if (sempNmeaValidateChecksum(parse, 2) && (data == '\r'))
    {
        // Start searching for a preamble byte
        parse->state = sempFirstByte;
        return true;
    }

    // Start searching for a preamble byte
    return sempFirstByte(parse, data);
}

//----------------------------------------
// Read the line termination
//----------------------------------------
bool sempNmeaLineTermination(SEMP_PARSE_STATE *parse, uint8_t data)
{
    // Process the line termination
    if (data == '\r')
    {
        parse->state = sempNmeaLineFeed;
        return true;
    }
    else if (data == '\n')
    {
        parse->state = sempNmeaCarriageReturn;
        return true;
    }

    // Validate the checksum and pass the sentence to the upper layer
    sempNmeaValidateChecksum(parse, 1);

    // Start searching for a preamble byte
    return sempFirstByte(parse, data);
}

//----------------------------------------
// Read the second checksum byte
//----------------------------------------
bool sempNmeaChecksumByte2(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_OUTPUT output;

    // Validate the checksum character
    if (sempAsciiToNibble(parse->buffer[parse->length - 1]) >= 0)
    {
        parse->state = sempNmeaLineTermination;
        return true;
    }

    // Invalid checksum character
    output = parse->debugOutput;
    if (output)
    {
        sempPrintString(output, "SEMP ");
        sempPrintString(output, parse->parserName);
        sempPrintStringLn(output, ": NMEA invalid second checksum character");
    }

    // The data character is in the buffer, remove it and pass the
    // remaining data to the invalid data handler
    parse->length -= 1;
    sempInvalidDataCallback(parse);

    // Start searching for a preamble byte
    return sempFirstByte(parse, data);
}

//----------------------------------------
// Read the first checksum byte
//----------------------------------------
bool sempNmeaChecksumByte1(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_OUTPUT output;

    // Validate the checksum character
    if (sempAsciiToNibble(parse->buffer[parse->length - 1]) >= 0)
    {
        parse->state = sempNmeaChecksumByte2;
        return true;
    }

    // Invalid checksum character
    output = parse->debugOutput;
    if (output)
    {
        sempPrintString(output, "SEMP ");
        sempPrintString(output, parse->parserName);
        sempPrintStringLn(output, ": NMEA invalid first checksum character");
    }

    // The data character is in the buffer, remove it and pass the
    // remaining data to the invalid data handler
    parse->length -= 1;
    sempInvalidDataCallback(parse);

    // Start searching for a preamble byte
    return sempFirstByte(parse, data);
}

//----------------------------------------
// Read the sentence data
//----------------------------------------
bool sempNmeaFindAsterisk(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_OUTPUT output;
    SEMP_NMEA_VALUES *scratchPad = (SEMP_NMEA_VALUES *)parse->scratchPad;

    if (data == '*')
        parse->state = sempNmeaChecksumByte1;
    else
    {
        // Include this byte in the checksum
        parse->crc ^= data;

        // Abort on a non-printable char - if enabled
        output = parse->debugOutput;
        if (parse->nmeaAbortOnNonPrintable)
        {
            if ((data < ' ') || (data > '~'))
            {
                if (output)
                {
                    sempPrintString(output, "SEMP ");
                    sempPrintString(output, parse->parserName);
                    sempPrintString(output, ": NMEA ");
                    sempPrintString(output, (const char *)scratchPad->sentenceName);
                    sempPrintStringLn(output, " abort on non-printable char");
                }

                // The data character is in the buffer, remove it and
                // pass the remaining data to the invalid data handler
                parse->length -= 1;
                sempInvalidDataCallback(parse);

                // Start searching for a preamble byte
                return sempFirstByte(parse, data);
            }
        }

        // Verify that enough space exists in the buffer
        if ((uint32_t)(parse->length + NMEA_BUFFER_OVERHEAD) > parse->bufferLength)
        {
            // sentence too long
            if (output)
            {
                sempPrintString(output, "SEMP ");
                sempPrintString(output, parse->parserName);
                sempPrintString(output, ": NMEA sentence too long, increase the buffer size > ");
                sempPrintDecimalI32Ln(output, parse->bufferLength);
            }

            // The data character is in the buffer, remove it and pass the
            // remaining data to the invalid data handler
            parse->length -= 1;
            sempInvalidDataCallback(parse);

            // Start searching for a preamble byte
            return sempFirstByte(parse, data);
        }
    }
    return true;
}

//----------------------------------------
// Read the sentence name
//----------------------------------------
bool sempNmeaFindFirstComma(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_OUTPUT output = parse->debugOutput;
    SEMP_NMEA_VALUES *scratchPad = (SEMP_NMEA_VALUES *)parse->scratchPad;
    parse->crc ^= data;
    if ((data != ',') || (scratchPad->sentenceNameLength == 0))
    {
        // Invalid data, start searching for a preamble byte
        uint8_t upper = data & ~0x20;
        if (((upper < 'A') || (upper > 'Z')) && ((data < '0') || (data > '9')))
        {
            if (output)
            {
                sempPrintString(output, "SEMP ");
                sempPrintString(output, parse->parserName);
                sempPrintString(output, ": NMEA invalid sentence name character ");
                sempPrintHex0x02xLn(output, data);
            }

            // The data character is in the buffer, remove it and pass the
            // remaining data to the invalid data handler
            parse->length -= 1;
            sempInvalidDataCallback(parse);
            return sempFirstByte(parse, data);
        }

        // Name too long, start searching for a preamble byte
        if (scratchPad->sentenceNameLength == (sizeof(scratchPad->sentenceName) - 1))
        {
            if (output)
            {
                sempPrintString(output, "SEMP ");
                sempPrintString(output, parse->parserName);
                sempPrintString(output, ": NMEA sentence name > ");
                sempPrintDecimalI32(output, sizeof(scratchPad->sentenceName) - 1);
                sempPrintStringLn(output, " characters");
            }
\
            // The data character is in the buffer, remove it and pass the
            // remaining data to the invalid data handler
            parse->length -= 1;
            sempInvalidDataCallback(parse);
            return sempFirstByte(parse, data);
        }

        // Save the sentence name
        scratchPad->sentenceName[scratchPad->sentenceNameLength++] = data;
    }
    else
    {
        // Zero terminate the sentence name
        scratchPad->sentenceName[scratchPad->sentenceNameLength++] = 0;
        parse->state = sempNmeaFindAsterisk;
    }
    return true;
}

//----------------------------------------
// Check for the preamble
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//   data: First data byte in the stream of data to parse
//
// Outputs:
//   Returns true if the NMEA parser recgonizes the input and false
//   if another parser should be used
//----------------------------------------
bool sempNmeaPreamble(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_NMEA_VALUES *scratchPad = (SEMP_NMEA_VALUES *)parse->scratchPad;
    if (data != '$')
        return false;
    scratchPad->sentenceNameLength = 0;
    parse->state = sempNmeaFindFirstComma;
    return true;
}

//----------------------------------------
// Translates state value into an string, returns nullptr if not found
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//
// Outputs
//   Returns the address of the zero terminated state name string
//----------------------------------------
const char * sempNmeaGetStateName(const SEMP_PARSE_STATE *parse)
{
    if (parse->state == sempNmeaPreamble)
        return "sempNmeaPreamble";
    if (parse->state == sempNmeaFindFirstComma)
        return "sempNmeaFindFirstComma";
    if (parse->state == sempNmeaFindAsterisk)
        return "sempNmeaFindAsterisk";
    if (parse->state == sempNmeaChecksumByte1)
        return "sempNmeaChecksumByte1";
    if (parse->state == sempNmeaChecksumByte2)
        return "sempNmeaChecksumByte2";
    if (parse->state == sempNmeaLineTermination)
        return "sempNmeaLineTermination";
    if (parse->state == sempNmeaCarriageReturn)
        return "sempNmeaCarriageReturn";
    if (parse->state == sempNmeaLineFeed)
        return "sempNmeaLineFeed";
    return nullptr;
}

//------------------------------------------------------------------------------
// Public data and routines
//
// The following data structures and routines are listed in the .h file and are
// exposed to the SEMP routine and application layer.
//------------------------------------------------------------------------------

//----------------------------------------
// Describe the parser
//----------------------------------------
SEMP_PARSER_DESCRIPTION sempNmeaParserDescription =
{
    "NMEA parser",              // parserName
    sempNmeaPreamble,           // preamble
    sempNmeaGetStateName,       // State to state name translation routine
    82,                         // minimumParseAreaBytes
    sizeof(SEMP_NMEA_VALUES),   // scratchPadBytes
    0,                          // payloadOffset
};

//----------------------------------------
// Abort NMEA parsing on a non-printable char
//----------------------------------------
void sempNmeaAbortOnNonPrintable(SEMP_PARSE_STATE *parse, bool abort)
{
    if (parse)
        parse->nmeaAbortOnNonPrintable = abort;
}

//----------------------------------------
// Return the NMEA sentence name as a string
//----------------------------------------
const char * sempNmeaGetSentenceName(const SEMP_PARSE_STATE *parse)
{
    SEMP_NMEA_VALUES *scratchPad = (SEMP_NMEA_VALUES *)parse->scratchPad;
    return (const char *)scratchPad->sentenceName;
}
