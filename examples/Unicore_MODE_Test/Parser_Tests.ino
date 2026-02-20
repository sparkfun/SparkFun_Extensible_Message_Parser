/*
  SparkFun Unicore binary test example sketch

  This example demonstrates how to parse a Unicore Binary data stream

  License: MIT. Please see LICENSE.md for more details
*/

#include <SparkFun_Extensible_Message_Parser.h> //http://librarymanager/All#SparkFun_Extensible_Message_Parser

//----------------------------------------
// Constants
//----------------------------------------

// parserTable index values
#define UM980_NMEA_PARSER_INDEX 0
#define UM980_UNICORE_HASH_PARSER_INDEX 1
#define UM980_UNICORE_BINARY_PARSER_INDEX 2

// Build the table listing all of the parsers
const SEMP_PARSER_DESCRIPTION * parserTable[] =
{
    &sempNmeaParserDescription,
    &sempUnicoreHashParserDescription,
    &sempUnicoreBinaryParserDescription,
};
const int parserCount = sizeof(parserTable) / sizeof(parserTable[0]);

const uint8_t rawDataStream[] =
{
    // This is the UM980's response to the "MODE\r\n" command
    // Note that the $ is _included_ in the checksum,
    // causing badNmeaChecksum to be called
    // "$command,MODE,response: OK*5D\r\n"
    // "#MODE,97,GPS,FINE,2406,465982000,17548,0,18,962;MODE ROVER SURVEY,*1B\r\n"
    0x24,0x63,0x6F,0x6D,0x6D,0x61,0x6E,0x64,0x2C,0x4D,0x4F,0x44,0x45,0x2C,0x72,0x65,
    0x73,0x70,0x6F,0x6E,0x73,0x65,0x3A,0x20,0x4F,0x4B,0x2A,0x35,0x44,0x0D,0x0A,
    0x23,0x4D,0x4F,0x44,0x45,0x2C,0x39,0x37,0x2C,0x47,0x50,0x53,0x2C,0x46,0x49,0x4E,
    0x45,0x2C,0x32,0x34,0x30,0x36,0x2C,0x34,0x36,0x35,0x39,0x38,0x32,0x30,0x30,0x30,
    0x2C,0x31,0x37,0x35,0x34,0x38,0x2C,0x30,0x2C,0x31,0x38,0x2C,0x39,0x36,0x32,0x3B,
    0x4D,0x4F,0x44,0x45,0x20,0x52,0x4F,0x56,0x45,0x52,0x20,0x53,0x55,0x52,0x56,0x45,
    0x59,0x2C,0x2A,0x31,0x42,0x0D,0x0A,

    // This is the response to "BESTNAVB 1.00\r\n"
    // "$command,BESTNAVB 1.00,response: OK*7A\r\n"
    0x24,0x63,0x6F,0x6D,0x6D,0x61,0x6E,0x64,0x2C,0x42,0x45,0x53,0x54,0x4E,0x41,0x56,
    0x42,0x20,0x31,0x2E,0x30,0x30,0x2C,0x72,0x65,0x73,0x70,0x6F,0x6E,0x73,0x65,0x3A,
    0x20,0x4F,0x4B,0x2A,0x37,0x41,0x0D,0x0A,

    // This is a valid periodic RECTIMEB message
    0xAA,0x44,0xB5,0x61,0x66,0x00,0x2C,0x00,0x00,0xA0,0x66,0x09,0xE8,0x5D,0xC6,0x1B,
    0x8C,0x44,0x00,0x00,0x00,0x12,0x13,0x00,0x00,0x00,0x00,0x00,0x17,0x77,0xC4,0x19,
    0x0C,0x0F,0x31,0x3F,0x7F,0x38,0xFC,0xC2,0xF5,0x4A,0x3C,0x3E,0x00,0x00,0x00,0x00,
    0x00,0x00,0x32,0xC0,0xEA,0x07,0x00,0x00,0x02,0x14,0x09,0x1A,0x58,0x1B,0x00,0x00,
    0x01,0x00,0x00,0x00,0x63,0xA7,0x47,0xDB,
};

// Number of bytes in the rawDataStream
#define RAW_DATA_BYTES      (sizeof(rawDataStream))

//----------------------------------------
// Locals
//----------------------------------------

// Account for the largest Unicore messages
uint8_t buffer[3095];
uint32_t dataOffset;
SEMP_PARSE_STATE *parse;

//------------------------------------------------------------------------------
// Test routines
//------------------------------------------------------------------------------

// Alternate checksum for NMEA parser needed during setup
// ****************************************************
// * Based on the Unicore_GNSS_Arduino_Library v2.0.0 *
// ****************************************************
bool badNmeaChecksum(SEMP_PARSE_STATE *parse)
{
    int alternateChecksum;
    bool badChecksum;
    int checksum;

    // Check if this is a NMEA parser: NMEA or Unicore Hash
    if (parse->type >= UM980_UNICORE_BINARY_PARSER_INDEX)
        return false;

    // Older UM980 firmware during setup is improperly adding the '$'
    // into the checksum calculation.  Convert the received checksum
    // characters into binary.
    // ************************************************************
    // * Please see Issue #91                                     *
    // * With SEMP >= v2.0.0                                      *
    // * For NMEA: the last two bytes are CR LF, not the checksum *
    // * For Unicore Hash: the last two bytes are the checksum    *
    // ************************************************************
    checksum = sempAsciiToNibble(parse->buffer[parse->length - 1]);
    checksum |= sempAsciiToNibble(parse->buffer[parse->length - 2]) << 4;

    // Determine if the checksum also includes the '$' or '#'
    alternateChecksum = parse->crc ^ (parse->type ? '#' : '$');
    badChecksum = (alternateChecksum != checksum);

    // Display bad checksums
    if (!badChecksum)
    {
        sempPrintString(output, "Unicore Lib: Message improperly includes ");
        sempPrintChar(output, parse->buffer[0]);
        sempPrintStringLn(output, " in checksum");
        sempDumpBuffer(output, parse->buffer, parse->length);
    }
    return badChecksum;
}

//----------------------------------------
// Run the parser tests
//----------------------------------------
void parserTests()
{
    size_t bufferLength;

    // Verify the buffer size
    bufferLength = sempGetBufferLength(parserTable, parserCount);
    if (sizeof(buffer) < bufferLength)
    {
        sempPrintString(output, "Set buffer size to >= ");
        sempPrintDecimalI32Ln(output, bufferLength);
        reportFatalError("Fix the buffer size!");
    }

    // Initialize the parser - with badNmeaChecksum
    parse = sempBeginParser("Unicore_Test", parserTable, parserCount, buffer,
                            bufferLength, processMessage, output, output,
                            badNmeaChecksum);
    if (!parse)
        reportFatalError("Failed to initialize the parser");

    // Obtain a raw data stream from somewhere
    sempPrintString(output, "Raw data stream: ");
    sempPrintDecimalI32(output, RAW_DATA_BYTES);
    sempPrintStringLn(output, " bytes");

    // The raw data stream is passed to the parser one byte at a time
    sempDebugOutputEnable(parse, output);
    for (dataOffset = 0; dataOffset < RAW_DATA_BYTES; dataOffset++)
        // Update the parser state based on the incoming byte
        sempParseNextByte(parse, rawDataStream[dataOffset]);

    // Done parsing the data
    sempStopParser(&parse);
}

//----------------------------------------
// Call back from within parser, for end of message
// Process a complete message incoming from parser
//----------------------------------------
void processMessage(SEMP_PARSE_STATE *parse, uint16_t type)
{
    static bool displayOnce = true;
    uint32_t offset;

    // Display the raw message
    sempPrintLn(output);
    offset = dataOffset + 1 - parse->length;
    sempPrintString(output, "Valid Unicore message: ");
    sempPrintHex0x04x(output, parse->length);
    sempPrintString(output, " (");
    sempPrintDecimalI32(output, parse->length);
    sempPrintString(output, ") bytes at ");
    sempPrintHex0x08x(output, offset);
    sempPrintString(output, " (");
    sempPrintDecimalI32(output, offset);
    sempPrintCharLn(output, ')');
    sempDumpBuffer(output, parse->buffer, parse->length);

    // Display the parser state
    if (displayOnce)
    {
        displayOnce = false;
        sempPrintLn(output);
        sempUnicoreBinaryPrintHeader(parse);
        sempPrintLn(output);
        sempPrintParserConfiguration(parse, output);
    }
}

//----------------------------------------
// Print the error message every 15 seconds
//
// Inputs:
//   errorMsg: Zero-terminated error message string to output every 15 seconds
//----------------------------------------
void reportFatalError(const char *errorMsg)
{
    while (1)
    {
        sempPrintString(output, "HALTED: ");
        sempPrintStringLn(output, errorMsg);
        sleep(15);
    }
}
