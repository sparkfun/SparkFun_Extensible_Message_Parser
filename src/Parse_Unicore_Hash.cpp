/*------------------------------------------------------------------------------
Parse_Unicore_Hash.cpp

Unicore hash (#) sentence parsing support routines

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

// Save room for the carriage return, linefeed and zero termination
#define UNICORE_HASH_BUFFER_OVERHEAD    (1 + 1 + 1)

//----------------------------------------
// Unicore hash (#) parse routines
//----------------------------------------

//
//    Unicore Hash (#) Sentence
//
//    +----------+---------+--------+---------+----------+----------+
//    | Preamble |  Name   | Comma  |  Data   | Asterisk | Checksum |
//    |  8 bits  | n bytes | 8 bits | n bytes |  8 bits  | 2 bytes  |
//    |     #    |         |    ,   |         |          |          |
//    +----------+---------+--------+---------+----------+----------+
//               |                            |
//               |<-------- Checksum -------->|
//

// Compute the CRC for a sentence containing an 8 byte CRC value
// One such example is the full version message
// #VERSION,97,GPS,FINE,2282,248561000,0,0,18,676;UM980,R4.10Build7923,HRPT00-S10C-P,2310415000001-MD22B1224962616,ff3bac96f31f9bdd,2022/09/28*7432d4ed
// CRC is calculated without the # or * characters
void sempUnicoreHashValidatCrc(SEMP_PARSE_STATE *parse)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;
    uint32_t crc;
    uint32_t crcRx;
    const uint8_t *data;

    // Compute the CRC for the message
    crc = 0;
    data = &parse->buffer[1];    // Skip over the hash '#'
    do
        crc = semp_crc32Table[(crc ^ *data) & 0xFF] ^ (crc >> 8);
    while (*++data != '*');

    // Get the received CRC vale
    crcRx = sempAsciiToNibble(*++data) << 28;
    crcRx |= sempAsciiToNibble(*++data) << 24;
    crcRx |= sempAsciiToNibble(*++data) << 20;
    crcRx |= sempAsciiToNibble(*++data) << 16;
    crcRx |= sempAsciiToNibble(*++data) << 12;
    crcRx |= sempAsciiToNibble(*++data) << 8;
    crcRx |= sempAsciiToNibble(*++data) << 4;
    crcRx |= sempAsciiToNibble(*++data);

    // Determine if the CRC is valid
    if (crc != crcRx)
    {
        // Display the checksum error
        sempPrintf(parse->printDebug,
                   "SEMP: %s Unicore hash (#) %s, 0x%04x (%d) bytes, bad CRC, "
                   "received 0x%08x, computed: 0x%08x",
                   parse->parserName,
                   scratchPad->unicoreHash.sentenceName,
                   parse->length, parse->length, crcRx, crc);
        return;
    }

    // Verify that enough space exists in the buffer
    if ((uint32_t)(parse->length + UNICORE_HASH_BUFFER_OVERHEAD) > parse->bufferLength)
    {
        // Sentence too long
        sempPrintf(parse->printDebug,
                   "SEMP %s: Unicore hash (#) sentence too long, increase the buffer size >= %d",
                   parse->parserName,
                   parse->length + UNICORE_HASH_BUFFER_OVERHEAD);

        // Start searching for a preamble byte
        parse->state = sempFirstByte;
        return;
    }

    // Always add the carriage return and line feed
    parse->buffer[parse->length++] = '\r';
    parse->buffer[parse->length++] = '\n';

    // Zero terminate the Unicore hash (#) sentence, don't count this in the length
    parse->buffer[parse->length] = 0;

    // Process this Unicore hash (#) sentence
    parse->eomCallback(parse, parse->type); // Pass parser array index
}

// Validate the checksum
void sempUnicoreHashValidateChecksum(SEMP_PARSE_STATE *parse)
{
    uint32_t checksum;
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;

    // Determine if a CRC was used for this message
    if (scratchPad->unicoreHash.checksumBytes > 2)
    {
        // This message is using a CRC instead of a checksum
        sempUnicoreHashValidatCrc(parse);
        return;
    }

    // Convert the checksum characters into binary
    checksum = sempAsciiToNibble(parse->buffer[parse->length - 2]) << 4;
    checksum |= sempAsciiToNibble(parse->buffer[parse->length - 1]);

    // Validate the checksum
    if ((checksum == parse->crc) || (parse->badCrc && (!parse->badCrc(parse))))
    {
        // Always add the carriage return and line feed
        parse->buffer[parse->length++] = '\r';
        parse->buffer[parse->length++] = '\n';

        // Zero terminate the Unicore hash (#) sentence, don't count this in the length
        parse->buffer[parse->length] = 0;

        // Process this Unicore hash (#) sentence
        parse->eomCallback(parse, parse->type); // Pass parser array index
    }
    else
        // Display the checksum error
        sempPrintf(parse->printDebug,
                   "SEMP: %s Unicore hash (#) %s, 0x%04x (%d) bytes, bad checksum, "
                   "received 0x%c%c, computed: 0x%02x",
                   parse->parserName,
                   scratchPad->unicoreHash.sentenceName,
                   parse->length, parse->length,
                   parse->buffer[parse->length - 2],
                   parse->buffer[parse->length - 1],
                   parse->crc);
}

// Read the linefeed
bool sempUnicoreHashLineFeed(SEMP_PARSE_STATE *parse, uint8_t data)
{
    int checksum;

    // Don't add the current character to the length
    parse->length -= 1;

    // Process the LF
    if (data == '\n')
    {
        // Pass the sentence to the upper layer
        sempUnicoreHashValidateChecksum(parse);

        // Start searching for a preamble byte
        parse->state = sempFirstByte;
        return true;
    }

    // Pass the sentence to the upper layer
    sempUnicoreHashValidateChecksum(parse);

    // Start searching for a preamble byte
    return sempFirstByte(parse, data);
}

// Read the remaining carriage return
bool sempUnicoreHashCarriageReturn(SEMP_PARSE_STATE *parse, uint8_t data)
{
    // Don't add the current character to the length
    parse->length -= 1;

    // Process the CR
    if (data == '\r')
    {
        // Pass the sentence to the upper layer
        sempUnicoreHashValidateChecksum(parse);

        // Start searching for a preamble byte
        parse->state = sempFirstByte;
        return true;
    }

    // Pass the sentence to the upper layer
    sempUnicoreHashValidateChecksum(parse);

    // Start searching for a preamble byte
    return sempFirstByte(parse, data);
}

// Read the line termination
bool sempUnicoreHashLineTermination(SEMP_PARSE_STATE *parse, uint8_t data)
{
    int checksum;

    // Don't add the current character to the length
    parse->length -= 1;

    // Process the line termination
    if (data == '\r')
    {
        parse->state = sempUnicoreHashLineFeed;
        return true;
    }
    else if (data == '\n')
    {
        parse->state = sempUnicoreHashCarriageReturn;
        return true;
    }

    // Pass the sentence to the upper layer
    sempUnicoreHashValidateChecksum(parse);

    // Start searching for a preamble byte
    return sempFirstByte(parse, data);
}

// Read the checksum bytes
bool sempUnicoreHashChecksumByte(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;

    // Account for this checksum character
    scratchPad->unicoreHash.bytesRemaining -= 1;

    // Validate the checksum character
    if (sempAsciiToNibble(parse->buffer[parse->length - 1]) < 0)
    {
        // Invalid checksum character
        sempPrintf(parse->printDebug,
                   "SEMP %s: Unicore hash (#) invalid checksum character %d",
                   parse->parserName,
                   scratchPad->unicoreHash.checksumBytes - scratchPad->unicoreHash.bytesRemaining);

        // Start searching for a preamble byte
        return sempFirstByte(parse, data);
    }

    // Valid checksum character
    if (!scratchPad->unicoreHash.bytesRemaining)
        parse->state = sempUnicoreHashLineTermination;
    return true;
}

// Read the sentence data
bool sempUnicoreHashFindAsterisk(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;
    if (data == '*')
    {
        scratchPad->unicoreHash.bytesRemaining = scratchPad->unicoreHash.checksumBytes;
        parse->state = sempUnicoreHashChecksumByte;
    }
    else
    {
        // Include this byte in the checksum
        parse->crc ^= data;

        // Verify that enough space exists in the buffer
        if ((uint32_t)(parse->length + UNICORE_HASH_BUFFER_OVERHEAD) > parse->bufferLength)
        {
            // sentence too long
            sempPrintf(parse->printDebug,
                       "SEMP %s: Unicore hash (#) sentence too long, increase the buffer size > %d",
                       parse->parserName,
                       parse->bufferLength);

            // Start searching for a preamble byte
            return sempFirstByte(parse, data);
        }
    }
    return true;
}

// Read the sentence name
bool sempUnicoreHashFindFirstComma(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;
    parse->crc ^= data;
    if ((data != ',') || (scratchPad->unicoreHash.sentenceNameLength == 0))
    {
        // Invalid data, start searching for a preamble byte
        uint8_t upper = data & ~0x20;
        if (((upper < 'A') || (upper > 'Z')) && ((data < '0') || (data > '9')))
        {
            sempPrintf(parse->printDebug,
                       "SEMP %s: Unicore hash (#) invalid sentence name character 0x%02x",
                       parse->parserName, data);
            return sempFirstByte(parse, data);
        }

        // Name too long, start searching for a preamble byte
        if (scratchPad->unicoreHash.sentenceNameLength == (sizeof(scratchPad->unicoreHash.sentenceName) - 1))
        {
            sempPrintf(parse->printDebug,
                       "SEMP %s: Unicore hash (#) sentence name > %d characters",
                       parse->parserName,
                       sizeof(scratchPad->unicoreHash.sentenceName) - 1);
            return sempFirstByte(parse, data);
        }

        // Save the sentence name
        scratchPad->unicoreHash.sentenceName[scratchPad->unicoreHash.sentenceNameLength++] = data;
    }
    else
    {
        // Zero terminate the sentence name
        scratchPad->unicoreHash.sentenceName[scratchPad->unicoreHash.sentenceNameLength++] = 0;

        // Determine the number of checksum bytes
        scratchPad->unicoreHash.checksumBytes = 2;
        if (strcasestr("VERSION", (const char *)scratchPad->unicoreHash.sentenceName))
            scratchPad->unicoreHash.checksumBytes = 8;
        parse->state = sempUnicoreHashFindAsterisk;
    }
    return true;
}

// Check for the preamble
bool sempUnicoreHashPreamble(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;
    if (data != '#')
        return false;
    scratchPad->unicoreHash.sentenceNameLength = 0;
    parse->state = sempUnicoreHashFindFirstComma;
    return true;
}

// Translates state value into an string, returns nullptr if not found
const char * sempUnicoreHashGetStateName(const SEMP_PARSE_STATE *parse)
{
    if (parse->state == sempUnicoreHashPreamble)
        return "sempUnicoreHashPreamble";
    if (parse->state == sempUnicoreHashFindFirstComma)
        return "sempUnicoreHashFindFirstComma";
    if (parse->state == sempUnicoreHashFindAsterisk)
        return "sempUnicoreHashFindAsterisk";
    if (parse->state == sempUnicoreHashChecksumByte)
        return "sempUnicoreHashChecksumByte";
    if (parse->state == sempUnicoreHashLineTermination)
        return "sempUnicoreHashLineTermination";
    if (parse->state == sempUnicoreHashCarriageReturn)
        return "sempUnicoreHashCarriageReturn";
    if (parse->state == sempUnicoreHashLineFeed)
        return "sempUnicoreHashLineFeed";
    return nullptr;
}

// Return the Unicore hash (#) sentence name as a string
const char * sempUnicoreHashGetSentenceName(const SEMP_PARSE_STATE *parse)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;
    return (const char *)scratchPad->unicoreHash.sentenceName;
}
