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
// NMEA parse routines
//----------------------------------------

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

// Validate the checksum
void sempNmeaValidateChecksum(SEMP_PARSE_STATE *parse)
{
    int checksum;
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;

    // Convert the checksum characters into binary
    checksum = sempAsciiToNibble(parse->buffer[parse->length - 2]) << 4;
    checksum |= sempAsciiToNibble(parse->buffer[parse->length - 1]);

    // Validate the checksum
    if ((checksum == parse->crc) || (parse->badCrc && (!parse->badCrc(parse))))
    {
        // Always add the carriage return and line feed
        parse->buffer[parse->length++] = '\r';
        parse->buffer[parse->length++] = '\n';

        // Zero terminate the NMEA sentence, don't count this in the length
        parse->buffer[parse->length] = 0;

        // Process this NMEA sentence
        parse->eomCallback(parse, parse->type); // Pass parser array index
    }
    else
        // Display the checksum error
        sempPrintf(parse->printDebug,
                   "SEMP: %s NMEA %s, 0x%04x (%d) bytes, bad checksum, "
                   "received 0x%c%c, computed: 0x%02x",
                   parse->parserName,
                   scratchPad->nmea.sentenceName,
                   parse->length, parse->length,
                   parse->buffer[parse->length - 2],
                   parse->buffer[parse->length - 1],
                   parse->crc);
}

// Read the linefeed
bool sempNmeaLineFeed(SEMP_PARSE_STATE *parse, uint8_t data)
{
    // Don't add the current character to the length
    parse->length -= 1;

    // Process the LF
    if (data == '\n')
    {
        // Pass the sentence to the upper layer
        sempNmeaValidateChecksum(parse);

        // Start searching for a preamble byte
        parse->state = sempFirstByte;
        return true;
    }

    // Pass the sentence to the upper layer
    sempNmeaValidateChecksum(parse);

    // Start searching for a preamble byte
    return sempFirstByte(parse, data);
}

// Read the remaining carriage return
bool sempNmeaCarriageReturn(SEMP_PARSE_STATE *parse, uint8_t data)
{
    // Don't add the current character to the length
    parse->length -= 1;

    // Process the CR
    if (data == '\r')
    {
        // Pass the sentence to the upper layer
        sempNmeaValidateChecksum(parse);

        // Start searching for a preamble byte
        parse->state = sempFirstByte;
        return true;
    }

    // Pass the sentence to the upper layer
    sempNmeaValidateChecksum(parse);

    // Start searching for a preamble byte
    return sempFirstByte(parse, data);
}

// Read the line termination
bool sempNmeaLineTermination(SEMP_PARSE_STATE *parse, uint8_t data)
{
    // Don't add the current character to the length
    parse->length -= 1;

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

    // Pass the sentence to the upper layer
    sempNmeaValidateChecksum(parse);

    // Start searching for a preamble byte
    return sempFirstByte(parse, data);
}

// Read the second checksum byte
bool sempNmeaChecksumByte2(SEMP_PARSE_STATE *parse, uint8_t data)
{
    // Validate the checksum character
    if (sempAsciiToNibble(parse->buffer[parse->length - 1]) >= 0)
    {
        parse->state = sempNmeaLineTermination;
        return true;
    }

    // Invalid checksum character
    sempPrintf(parse->printDebug,
               "SEMP %s: NMEA invalid second checksum character",
               parse->parserName);

    // Start searching for a preamble byte
    return sempFirstByte(parse, data);
}

// Read the first checksum byte
bool sempNmeaChecksumByte1(SEMP_PARSE_STATE *parse, uint8_t data)
{
    // Validate the checksum character
    if (sempAsciiToNibble(parse->buffer[parse->length - 1]) >= 0)
    {
        parse->state = sempNmeaChecksumByte2;
        return true;
    }

    // Invalid checksum character
    sempPrintf(parse->printDebug,
               "SEMP %s: NMEA invalid first checksum character",
               parse->parserName);

    // Start searching for a preamble byte
    return sempFirstByte(parse, data);
}

// Read the sentence data
bool sempNmeaFindAsterisk(SEMP_PARSE_STATE *parse, uint8_t data)
{
    if (data == '*')
        parse->state = sempNmeaChecksumByte1;
    else
    {
        // Include this byte in the checksum
        parse->crc ^= data;

        // Verify that enough space exists in the buffer
        if ((uint32_t)(parse->length + NMEA_BUFFER_OVERHEAD) > parse->bufferLength)
        {
            // sentence too long
            sempPrintf(parse->printDebug,
                       "SEMP %s: NMEA sentence too long, increase the buffer size > %d",
                       parse->parserName,
                       parse->bufferLength);

            // Start searching for a preamble byte
            return sempFirstByte(parse, data);
        }
    }
    return true;
}

// Read the sentence name
bool sempNmeaFindFirstComma(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;
    parse->crc ^= data;
    if ((data != ',') || (scratchPad->nmea.sentenceNameLength == 0))
    {
        // Invalid data, start searching for a preamble byte
        uint8_t upper = data & ~0x20;
        if (((upper < 'A') || (upper > 'Z')) && ((data < '0') || (data > '9')))
        {
            sempPrintf(parse->printDebug,
                       "SEMP %s: NMEA invalid sentence name character 0x%02x",
                       parse->parserName, data);
            return sempFirstByte(parse, data);
        }

        // Name too long, start searching for a preamble byte
        if (scratchPad->nmea.sentenceNameLength == (sizeof(scratchPad->nmea.sentenceName) - 1))
        {
            sempPrintf(parse->printDebug,
                       "SEMP %s: NMEA sentence name > %ld characters",
                       parse->parserName,
                       sizeof(scratchPad->nmea.sentenceName) - 1);
            return sempFirstByte(parse, data);
        }

        // Save the sentence name
        scratchPad->nmea.sentenceName[scratchPad->nmea.sentenceNameLength++] = data;
    }
    else
    {
        // Zero terminate the sentence name
        scratchPad->nmea.sentenceName[scratchPad->nmea.sentenceNameLength++] = 0;
        parse->state = sempNmeaFindAsterisk;
    }
    return true;
}

// Check for the preamble
bool sempNmeaPreamble(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;
    if (data != '$')
        return false;
    scratchPad->nmea.sentenceNameLength = 0;
    parse->state = sempNmeaFindFirstComma;
    return true;
}

// Translates state value into an string, returns nullptr if not found
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

// Return the NMEA sentence name as a string
const char * sempNmeaGetSentenceName(const SEMP_PARSE_STATE *parse)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;
    return (const char *)scratchPad->nmea.sentenceName;
}
