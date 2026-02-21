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

const uint8_t modeResponse[] = 
{
    // This is the UM980's response to the "MODE\r\n" command
    // Note that the $ is _included_ in the checksum,
    // causing badNmeaChecksum to be called
    "$command,MODE,response: OK*5D\r\n"
    "#MODE,97,GPS,FINE,2406,465982000,17548,0,18,962;MODE ROVER SURVEY,*1B\r\n"
};

const uint8_t bestNavResponse[] =
{
    // This is the response to "BESTNAVB 1.00\r\n"
    "$command,BESTNAVB 1.00,response: OK*7A\r\n"
};

const uint8_t unicoreHash8ByteCrc[] =
{
    // Valid Unicore hash (#) sentence with an 8 byte checksum
    "#VERSION,40,GPS,UNKNOWN,1,1000,0,0,18,15;UM980,R4.10Build7923,HRPT00-S10C-P,"
    "2310415000001-MD22B1225023842,ff3b1e9611b3b07b,2022/09/28*b164c965\r\n"
};

const uint8_t unicoreRecTime[] =
{
    // This is a valid periodic RECTIMEB message
    0xAA,0x44,0xB5,0x61,0x66,0x00,0x2C,0x00,0x00,0xA0,0x66,0x09,0xE8,0x5D,0xC6,0x1B,
    0x8C,0x44,0x00,0x00,0x00,0x12,0x13,0x00,0x00,0x00,0x00,0x00,0x17,0x77,0xC4,0x19,
    0x0C,0x0F,0x31,0x3F,0x7F,0x38,0xFC,0xC2,0xF5,0x4A,0x3C,0x3E,0x00,0x00,0x00,0x00,
    0x00,0x00,0x32,0xC0,0xEA,0x07,0x00,0x00,0x02,0x14,0x09,0x1A,0x58,0x1B,0x00,0x00,
    0x01,0x00,0x00,0x00,0x63,0xA7,0x47,0xDB,
};

typedef struct _DataStream
{
    size_t length;
    const uint8_t *data;
} DataStream;

#define DATA_STREAM_INIT(x, extraBytes)     {sizeof(x) - extraBytes, &x[0]}
const DataStream dataStream[] =
{
    DATA_STREAM_INIT(modeResponse, 1),
    DATA_STREAM_INIT(bestNavResponse, 1),
    DATA_STREAM_INIT(unicoreHash8ByteCrc, 1),
    DATA_STREAM_INIT(unicoreRecTime, 0),
};

#define DATA_STREAM_ENTRIES     (sizeof(dataStream) / sizeof(dataStream[0]))

//----------------------------------------
// Locals
//----------------------------------------

// Account for the largest Unicore messages
uint8_t buffer[3095];
int byteOffset;
int dataIndex;
uint32_t dataOffset;
SEMP_PARSE_STATE *parse;

//------------------------------------------------------------------------------
// Test routines
//------------------------------------------------------------------------------

// Alternate checksum for NMEA parser needed during setup
// Based on the Unicore_GNSS_Arduino_Library v2.0.0
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
    checksum = sempAsciiToNibble(parse->buffer[parse->length - 1]);
    checksum |= sempAsciiToNibble(parse->buffer[parse->length - 2]) << 4;

    // Determine if the checksum also includes the '$' or '#'
    alternateChecksum = parse->crc ^ (parse->type ? '#' : '$');
    badChecksum = (alternateChecksum != checksum);

    // Display bad checksums
    if (!badChecksum)
    {
        sempPrintString(output, "Unicore: Message improperly includes ");
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
    int rawDataBytes;

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
    rawDataBytes = 0;
    for (dataIndex = 0; dataIndex < DATA_STREAM_ENTRIES; dataIndex++)
        rawDataBytes += dataStream[dataIndex].length;
    sempPrintString(output, "Raw data stream: ");
    sempPrintDecimalI32(output,rawDataBytes);
    sempPrintStringLn(output, " bytes");

    // The raw data stream is passed to the parser one byte at a time
    sempDebugOutputEnable(parse, output);
    for (dataIndex = 0; dataIndex < DATA_STREAM_ENTRIES; dataIndex++)
    {
        for (byteOffset = 0; byteOffset < dataStream[dataIndex].length; byteOffset++)
        {
            // Update the parser state based on the incoming byte
            sempParseNextByte(parse, dataStream[dataIndex].data[byteOffset]);
            dataOffset += 1;
        }
    }

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
