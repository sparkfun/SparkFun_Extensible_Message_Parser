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
bool nmeaLineTermination(SEMP_PARSE_STATE *parse, uint8_t data)
{
    int checksum;

    // Process the line termination
    if ((data == '\r') && (data == '\n'))
        return true;

    // Start searching for a preamble byte
    return sempFirstByte(parse, data);
}

// Read the second checksum byte
bool nmeaChecksumByte2(SEMP_PARSE_STATE *parse, uint8_t data)
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
        if (sempPrintErrorMessages)
        {
            char line[128];
            sprintf(line, "SEMP %s: NMEA invalid second checksum character",
                    parse->parserName);
            sempExtPrintLineOfText(line);
        }

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
        parse->buffer[parse->length++] = 0;

        // Process this NMEA message
        parse->eomCallback(parse, parse->type);

        // Remove any CR or LF that follow
        parse->state = nmeaLineTermination;
        parse->length = 0;
        return true;
    }

    // Display the checksum error
    if (sempPrintErrorMessages)
    {
        char error[128];
        sprintf(error,
                "SEMP: %s NMEA %s, %2d bytes, bad checksum, "
                "expecting 0x%c%c, computed: 0x%02x",
                parse->parserName,
                scratchPad->nmea.messageName,
                parse->length,
                parse->buffer[parse->length - 2],
                parse->buffer[parse->length - 1],
                parse->crc);
        sempExtPrintLineOfText(error);
    }

    // Start searching for a preamble byte
    parse->state = sempFirstByte;
    return false;
}

// Read the first checksum byte
bool nmeaChecksumByte1(SEMP_PARSE_STATE *parse, uint8_t data)
{
    // Validate the checksum character
    if (sempAsciiToNibble(parse->buffer[parse->length - 1]) >= 0)
    {
        parse->state = nmeaChecksumByte2;
        return true;
    }

    // Invalid checksum character
    if (sempPrintErrorMessages)
    {
        char line[128];
        sprintf(line, "SEMP %s: NMEA invalid first checksum character",
                parse->parserName);
        sempExtPrintLineOfText(line);
    }

    // Start searching for a preamble byte
    return sempFirstByte(parse, data);
}

// Read the message data
bool nmeaFindAsterisk(SEMP_PARSE_STATE *parse, uint8_t data)
{
    if (data == '*')
        parse->state = nmeaChecksumByte1;
    else
    {
        // Include this byte in the checksum
        parse->crc ^= data;

        // Verify that enough space exists in the buffer
        if ((parse->length + NMEA_BUFFER_OVERHEAD) > parse->bufferLength)
        {
            // Message too long
            if (sempPrintErrorMessages)
            {
                char line[128];
                sprintf(line, "SEMP %s: NMEA message too long, increase the buffer size > %d",
                        parse->parserName,
                        parse->bufferLength);
                sempExtPrintLineOfText(line);
            }

            // Start searching for a preamble byte
            return sempFirstByte(parse, data);
        }
    }
    return true;
}

// Read the message name
bool nmeaFindFirstComma(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;
    parse->crc ^= data;
    if ((data != ',') || (scratchPad->nmea.messageNameLength == 0))
    {
        // Invalid data, start searching for a preamble byte
        if ((data < 'A') || (data > 'Z'))
        {
            if (sempPrintErrorMessages)
            {
                char line[128];
                sprintf(line, "SEMP %s: NMEA invalid message name character",
                        parse->parserName);
                sempExtPrintLineOfText(line);
            }
            return sempFirstByte(parse, data);
        }

        // Name too long, start searching for a preamble byte
        if (scratchPad->nmea.messageNameLength == (sizeof(scratchPad->nmea.messageName) - 1))
        {
            if (sempPrintErrorMessages)
            {
                char line[128];
                sprintf(line, "SEMP %s: NMEA message name > %d characters",
                        parse->parserName,
                        sizeof(scratchPad->nmea.messageName) - 1);
                sempExtPrintLineOfText(line);
            }
            return sempFirstByte(parse, data);
        }

        // Save the message name
        scratchPad->nmea.messageName[scratchPad->nmea.messageNameLength++] = data;
    }
    else
    {
        // Zero terminate the message name
        scratchPad->nmea.messageName[scratchPad->nmea.messageNameLength++] = 0;
        parse->state = nmeaFindAsterisk;
    }
    return true;
}

// Check for the preamble
bool nmeaPreamble(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;
    if (data != '$')
        return false;
    scratchPad->nmea.messageNameLength = 0;
    parse->state = nmeaFindFirstComma;
    return true;
}
