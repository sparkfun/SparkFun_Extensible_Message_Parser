/*------------------------------------------------------------------------------
Parse_NMEA.cpp

NMEA message parsing support routines

The parser routines within a parser module are typically placed in
reverse order within the module.  This lets the routine declaration
proceed the routine use and eliminates the need for forward declaration.
Removing the forward declaration helps reduce the exposure of the
routines to the application layer.  As such only the preamble routine
should need to be listed in SparkFun_Extensible_Message_Parser.h.
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
//    NMEA Message
//
//    +----------+---------+--------+---------+----------+----------+
//    | Preamble |  Name   | Comma  |  Data   | Asterisk | Checksum |
//    |  8 bits  | n bytes | 8 bits | n bytes |  8 bits  | 2 bytes  |
//    |     $    |         |    ,   |         |          |          |
//    +----------+---------+--------+---------+----------+----------+
//               |                            |
//               |<-------- Checksum -------->|
//

// Read the line termination
bool sempNmeaLineTermination(SEMP_PARSE_STATE *parse, uint8_t data)
{
    int checksum;

    // Process the line termination
    if ((data == '\r') || (data == '\n'))
        return true;

    // Start searching for a preamble byte
    return sempFirstByte(parse, data);
}

// Read the second checksum byte
bool sempNmeaChecksumByte2(SEMP_PARSE_STATE *parse, uint8_t data)
{
    int checksum;
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;

    // Convert the checksum characters into binary
    checksum = sempAsciiToNibble(parse->buffer[parse->length - 1]);
    checksum |= sempAsciiToNibble(parse->buffer[parse->length - 2]) << 4;

    // Validate the checksum character
    if (checksum < 0)
    {
        // Invalid checksum character
        sempPrintf(parse->printDebug,
                   "SEMP %s: NMEA invalid second checksum character",
                   parse->parserName);

        // Start searching for a preamble byte
        return sempFirstByte(parse, data);
    }

    // Validate the checksum
    if (checksum == parse->crc)
    {
        // Add CR and LF to the message
        parse->buffer[parse->length++] = '\r';
        parse->buffer[parse->length++] = '\n';

        // Zero terminate the string
        parse->buffer[parse->length] = 0;

        // Process this NMEA message
        parse->eomCallback(parse, parse->type); // Pass parser array index

        // Remove any CR or LF that follow
        parse->state = sempNmeaLineTermination;
        parse->length = 0;
        return true;
    }

    // Display the checksum error
    sempPrintf(parse->printDebug,
               "SEMP: %s NMEA %s, %2d bytes, bad checksum, "
               "received 0x%c%c, computed: 0x%02x",
               parse->parserName,
               scratchPad->nmea.messageName,
               parse->length,
               parse->buffer[parse->length - 2],
               parse->buffer[parse->length - 1],
               parse->crc);

    // Start searching for a preamble byte
    parse->state = sempFirstByte;
    return false;
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

// Read the message data
bool sempNmeaFindAsterisk(SEMP_PARSE_STATE *parse, uint8_t data)
{
    if (data == '*')
        parse->state = sempNmeaChecksumByte1;
    else
    {
        // Include this byte in the checksum
        parse->crc ^= data;

        // Verify that enough space exists in the buffer
        if ((parse->length + NMEA_BUFFER_OVERHEAD) > parse->bufferLength)
        {
            // Message too long
            sempPrintf(parse->printDebug,
                       "SEMP %s: NMEA message too long, increase the buffer size > %d",
                       parse->parserName,
                       parse->bufferLength);

            // Start searching for a preamble byte
            return sempFirstByte(parse, data);
        }
    }
    return true;
}

// Read the message name
bool sempNmeaFindFirstComma(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;
sempPrintf(parse->printError, "crc: 0x%02x", parse->crc);
    parse->crc ^= data;
    if ((data != ',') || (scratchPad->nmea.messageNameLength == 0))
    {
        // Invalid data, start searching for a preamble byte
        uint8_t upper = data & ~0x20;
sempPrintf(parse->printError, "upper: 0x%02x", upper);
        if (((upper < 'A') || (upper > 'Z')) && ((data < '0') || (data > '9')))
        {
            sempPrintf(parse->printDebug,
                       "SEMP %s: NMEA invalid message name character 0x%02x",
                       parse->parserName, data);
            return sempFirstByte(parse, data);
        }

        // Name too long, start searching for a preamble byte
        if (scratchPad->nmea.messageNameLength == (sizeof(scratchPad->nmea.messageName) - 1))
        {
            sempPrintf(parse->printDebug,
                       "SEMP %s: NMEA message name > %d characters",
                       parse->parserName,
                       sizeof(scratchPad->nmea.messageName) - 1);
            return sempFirstByte(parse, data);
        }

        // Save the message name
        scratchPad->nmea.messageName[scratchPad->nmea.messageNameLength++] = data;
    }
    else
    {
        // Zero terminate the message name
        scratchPad->nmea.messageName[scratchPad->nmea.messageNameLength++] = 0;
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
    scratchPad->nmea.messageNameLength = 0;
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
    return nullptr;
}
