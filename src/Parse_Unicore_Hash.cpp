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

// Length of the sentence name array
#define SEMP_UNICORE_HASH_SENTENCE_NAME_BYTES    16

//----------------------------------------
// Types
//----------------------------------------

// Unicore hash (#) parser scratch area
typedef struct _SEMP_UNICORE_HASH_VALUES
{
    uint8_t bytesRemaining;     // Bytes remaining in field
    uint8_t checksumBytes;      // Number of checksum bytes
    uint8_t sentenceName[SEMP_UNICORE_HASH_SENTENCE_NAME_BYTES]; // Sentence name
    uint8_t sentenceNameLength; // Length of the sentence name
} SEMP_UNICORE_HASH_VALUES;

//------------------------------------------------------------------------------
// Unicore hash (#) parse routines
//
// The parser routines are placed in reverse order to define the routine before
// its use and eliminate forward declarations.  Removing the forward declaration
// helps reduce the exposure of the routines to the application layer.  The public
// data structures and routines are listed at the end of the file.
//------------------------------------------------------------------------------

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

//----------------------------------------
// Compute the CRC for a sentence containing an 8 byte CRC value
// One such example is the full version message
// #VERSION,97,GPS,FINE,2282,248561000,0,0,18,676;UM980,R4.10Build7923,HRPT00-S10C-P,2310415000001-MD22B1224962616,ff3bac96f31f9bdd,2022/09/28*7432d4ed
// CRC is calculated without the # or * characters
//----------------------------------------
void sempUnicoreHashValidatCrc(SEMP_PARSE_STATE *parse)
{
    SEMP_OUTPUT output = parse->debugOutput;
    SEMP_UNICORE_HASH_VALUES *scratchPad = (SEMP_UNICORE_HASH_VALUES *)parse->scratchPad;
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
        if (output)
        {
            sempPrintString(output, "SEMP ");
            sempPrintString(output, parse->parserName);
            sempPrintString(output, ": Unicore hash (#) ");
            sempPrintString(output, (const char *)scratchPad->sentenceName);
            sempPrintString(output, ", ");
            sempPrintHex0x04x(output, parse->length);
            sempPrintString(output, " (");
            sempPrintDecimalU32(output, parse->length);
            sempPrintString(output, ") bytes, bad CRC received ");
            sempPrintHex0x08x(output, crcRx);
            sempPrintString(output, ", computed: ");
            sempPrintHex0x02xLn(output, crc);
        }
        return;
    }

    // Verify that enough space exists in the buffer
    if ((uint32_t)(parse->length + UNICORE_HASH_BUFFER_OVERHEAD) > parse->bufferLength)
    {
        // Sentence too long
        if (output)
        {
            sempPrintString(output, "SEMP ");
            sempPrintString(output, parse->parserName);
            sempPrintString(output, ": Unicore hash (#) ");
            sempPrintString(output, "sentence too long, increase the buffer size >= ");
            sempPrintDecimalU32Ln(output, parse->buffer - (uint8_t *)parse + parse->length + UNICORE_HASH_BUFFER_OVERHEAD);
        }

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

//----------------------------------------
// Validate the checksum
//----------------------------------------
void sempUnicoreHashValidateChecksum(SEMP_PARSE_STATE *parse)
{
    uint32_t checksum;
    SEMP_OUTPUT output = parse->debugOutput;
    SEMP_UNICORE_HASH_VALUES *scratchPad = (SEMP_UNICORE_HASH_VALUES *)parse->scratchPad;

    // Determine if a CRC was used for this message
    if (scratchPad->checksumBytes > 2)
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
    else if (output)
    {
        // Display the checksum error
        sempPrintString(output, "SEMP ");
        sempPrintString(output, parse->parserName);
        sempPrintString(output, ": Unicore hash (#) ");
        sempPrintString(output, (const char *)scratchPad->sentenceName);
        sempPrintString(output, ", ");
        sempPrintHex0x04x(output, parse->length);
        sempPrintString(output, " (");
        sempPrintDecimalU32(output, parse->length);
        sempPrintString(output, ") bytes, bad checksum, received 0x");
        output(parse->buffer[parse->length - 2]);
        output( parse->buffer[parse->length - 1]);
        sempPrintString(output, ", computed: ");
        sempPrintHex0x02xLn(output, parse->crc);
    }
}

//----------------------------------------
// Read the linefeed
//----------------------------------------
bool sempUnicoreHashLineFeed(SEMP_PARSE_STATE *parse, uint8_t data)
{
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

//----------------------------------------
// Read the remaining carriage return
//----------------------------------------
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

//----------------------------------------
// Read the line termination
//----------------------------------------
bool sempUnicoreHashLineTermination(SEMP_PARSE_STATE *parse, uint8_t data)
{
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

//----------------------------------------
// Read the checksum bytes
//----------------------------------------
bool sempUnicoreHashChecksumByte(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_OUTPUT output = parse->debugOutput;
    SEMP_UNICORE_HASH_VALUES *scratchPad = (SEMP_UNICORE_HASH_VALUES *)parse->scratchPad;

    // Account for this checksum character
    scratchPad->bytesRemaining -= 1;

    // Validate the checksum character
    if (sempAsciiToNibble(parse->buffer[parse->length - 1]) < 0)
    {
        // Invalid checksum character
        if (output)
        {
            sempPrintString(output, "SEMP ");
            sempPrintString(output, parse->parserName);
            sempPrintString(output, ": Unicore hash (#) invalid checksum character ");
            sempPrintDecimalI32Ln(output, scratchPad->checksumBytes - scratchPad->bytesRemaining);
        }

        // Start searching for a preamble byte
        return sempFirstByte(parse, data);
    }

    // Valid checksum character
    if (!scratchPad->bytesRemaining)
        parse->state = sempUnicoreHashLineTermination;
    return true;
}

//----------------------------------------
// Read the sentence data
//----------------------------------------
bool sempUnicoreHashFindAsterisk(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_OUTPUT output = parse->debugOutput;
    SEMP_UNICORE_HASH_VALUES *scratchPad = (SEMP_UNICORE_HASH_VALUES *)parse->scratchPad;
    if (data == '*')
    {
        scratchPad->bytesRemaining = scratchPad->checksumBytes;
        parse->state = sempUnicoreHashChecksumByte;
    }
    else
    {
        // Include this byte in the checksum
        parse->crc ^= data;

        // Abort on a non-printable char - if enabled
        if (parse->unicoreHashAbortOnNonPrintable)
        {
            if ((data < ' ') || (data > '~'))
            {
                if (output)
                {
                    sempPrintString(output, "SEMP ");
                    sempPrintString(output, parse->parserName);
                    sempPrintString(output, ": Unicore hash (#) ");
                    sempPrintString(output, (const char *)scratchPad->sentenceName);
                    sempPrintStringLn(output, " abort on non-printable char");
                }

                // Start searching for a preamble byte
                return sempFirstByte(parse, data);
            }
        }

        // Verify that enough space exists in the buffer
        if ((uint32_t)(parse->length + UNICORE_HASH_BUFFER_OVERHEAD) > parse->bufferLength)
        {
            // sentence too long
            if (output)
            {
                sempPrintString(output, "SEMP ");
                sempPrintString(output, parse->parserName);
                sempPrintString(output, ": Unicore hash (#) sentence too long, increase the buffer size > ");
                sempPrintDecimalI32Ln(output, parse->bufferLength);
            }

            // Start searching for a preamble byte
            return sempFirstByte(parse, data);
        }
    }
    return true;
}

//----------------------------------------
// Read the sentence name
//----------------------------------------
bool sempUnicoreHashFindFirstComma(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_OUTPUT output = parse->debugOutput;
    SEMP_UNICORE_HASH_VALUES *scratchPad = (SEMP_UNICORE_HASH_VALUES *)parse->scratchPad;
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
                sempPrintString(output, ": Unicore hash (#) invalid sentence name character ");
                sempPrintHex0x02xLn(output, data);
            }
            return sempFirstByte(parse, data);
        }

        // Name too long, start searching for a preamble byte
        if (scratchPad->sentenceNameLength == (sizeof(scratchPad->sentenceName) - 1))
        {
            if (output)
            {
                sempPrintString(output, "SEMP ");
                sempPrintString(output, parse->parserName);
                sempPrintString(output, ": Unicore hash (#) sentence name > ");
                sempPrintDecimalI32(output, sizeof(scratchPad->sentenceName) - 1);
                sempPrintStringLn(output, " characters");
            }
            return sempFirstByte(parse, data);
        }

        // Save the sentence name
        scratchPad->sentenceName[scratchPad->sentenceNameLength++] = data;
    }
    else
    {
        // Zero terminate the sentence name
        scratchPad->sentenceName[scratchPad->sentenceNameLength++] = 0;

        // Determine the number of checksum bytes
        scratchPad->checksumBytes = 8;
        if (strstr((const char *)scratchPad->sentenceName, "MODE") != NULL)
            scratchPad->checksumBytes = 2;
        parse->state = sempUnicoreHashFindAsterisk;
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
//   Returns true if the Unicore Hash parser recgonizes the input and
//   false if another parser should be used
//----------------------------------------
bool sempUnicoreHashPreamble(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_UNICORE_HASH_VALUES *scratchPad = (SEMP_UNICORE_HASH_VALUES *)parse->scratchPad;
    if (data != '#')
        return false;
    scratchPad->sentenceNameLength = 0;
    parse->state = sempUnicoreHashFindFirstComma;
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

//----------------------------------------
// Display the contents of the scratch pad
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//   output: Address of a routine to output a character
//----------------------------------------
void sempUnicoreHashPrintScratchPad(SEMP_PARSE_STATE *parse, SEMP_OUTPUT output)
{
    SEMP_UNICORE_HASH_VALUES *scratchPad;

    // Get the scratch pad address
    scratchPad = (SEMP_UNICORE_HASH_VALUES *)parse->scratchPad;

    // Display the remaining bytes
    sempPrintString(output, "    bytesRemaining: ");
    sempPrintDecimalU32Ln(output, scratchPad->bytesRemaining);

    // Display the checksum bytes
    sempPrintString(output, "    checksumBytes: ");
    sempPrintDecimalU32Ln(output, scratchPad->checksumBytes);

    // Display the sentence length
    sempPrintString(output, "    sentenceNameLength: ");
    sempPrintDecimalU32Ln(output, scratchPad->sentenceNameLength);

    // Display the sentence name
    sempDumpBuffer(output, scratchPad->sentenceName, sizeof(scratchPad->sentenceName));
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
SEMP_PARSER_DESCRIPTION sempUnicoreHashParserDescription =
{
    "Unicore hash parser",              // parserName
    sempUnicoreHashPreamble,            // preamble
    sempUnicoreHashGetStateName,        // State to state name translation routine
    sempUnicoreHashPrintScratchPad,     // Print the contents of the scratch pad
    145,                                // minimumParseAreaBytes
    sizeof(SEMP_UNICORE_HASH_VALUES),   // scratchPadBytes
    0,                                  // payloadOffset
};

//----------------------------------------
// Abort Unicore hash parsing on a non-printable char
//----------------------------------------
void sempUnicoreHashAbortOnNonPrintable(SEMP_PARSE_STATE *parse, bool abort)
{
    if (parse)
        parse->unicoreHashAbortOnNonPrintable = abort;
}

//----------------------------------------
// Return the Unicore hash (#) sentence name as a string
//----------------------------------------
const char * sempUnicoreHashGetSentenceName(const SEMP_PARSE_STATE *parse)
{
    SEMP_UNICORE_HASH_VALUES *scratchPad = (SEMP_UNICORE_HASH_VALUES *)parse->scratchPad;
    return (const char *)scratchPad->sentenceName;
}
