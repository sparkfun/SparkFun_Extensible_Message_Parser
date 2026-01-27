/*
  SparkFun RTCM test example sketch

  This example demonstrates how to parse a RTCM data stream

  License: MIT. Please see LICENSE.md for more details
*/

#include <SparkFun_Extensible_Message_Parser.h> //http://librarymanager/All#SparkFun_Extensible_Message_Parser

#include "../Common/output.ino"
#include "../Common/dumpBuffer.ino"
#include "../Common/reportFatalError.ino"

//----------------------------------------
// Constants
//----------------------------------------

// Build the table listing all of the parsers
SEMP_PARSER_DESCRIPTION * parserTable[] =
{
    &sempRtcmParserDescription
};
const int parserCount = sizeof(parserTable) / sizeof(parserTable[0]);

// Provide some valid and invalid RTCM messages
const uint8_t rawDataStream[] =
{
    // Junk to ignore
    0, 1, 2, 3, 4, 5, 6, 7,

    // Valid RTCM messages
    0xd3, 0x00, 0x13, 0x3e, 0xd0, 0x00, 0x03, 0x8e,
    0xd9, 0xaa, 0x78, 0x90, 0x80, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3b, 0xc6,
    0x32,

    0xd3, 0x00, 0x37, 0x43, 0x20, 0x00, 0x6a, 0x3c,
    0x88, 0x80, 0x00, 0x20, 0x00, 0x08, 0x21, 0x40,
    0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x7b, 0x7b, 0x33, 0x8b, 0x31, 0x65, 0x7b, 0xa3,
    0x5f, 0xbe, 0x63, 0xc3, 0xaa, 0x9b, 0x89, 0x33,
    0x4f, 0x40, 0x00, 0x01, 0x00, 0x00, 0x04, 0x00,
    0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x05, 0xb4,
    0x59, 0x88, 0x0f, 0xe1, 0x13,

    // Bad message length
    0xd3, 0x20, 0x13, 0x3e, 0xd0, 0x00, 0x03, 0x8e,
    0xd9, 0xaa, 0x78, 0x90, 0x80, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3b, 0xc6,
    0x32,

    // Bad CRC
    0xd3, 0x00, 0x37, 0x43, 0x20, 0x00, 0x6a, 0x3c,
    0x88, 0x80, 0x00, 0x20, 0x00, 0x08, 0x21, 0x40,
    0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x7b, 0x7b, 0x33, 0x8b, 0x31, 0x65, 0x7b, 0xa3,
    0x5f, 0xbe, 0x63, 0xc3, 0xaa, 0x9b, 0x89, 0x33,
    0x4f, 0x40, 0x00, 0x01, 0x00, 0x00, 0x04, 0x00,
    0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x05, 0xb4,
    0x59, 0x88, 0x0f, 0xe1, 0x14,
};

// Number of bytes in the rawDataStream
#define RAW_DATA_BYTES      sizeof(rawDataStream)

//----------------------------------------
// Locals
//----------------------------------------

// Account for the largest RTCM message
uint8_t buffer[1109];
uint32_t dataOffset;
SEMP_PARSE_STATE *parse;

//------------------------------------------------------------------------------
// Test routines
//------------------------------------------------------------------------------

//----------------------------------------
// Application entry point used to initialize the system
//----------------------------------------
void setup()
{
    size_t bufferLength;

    delay(1000);

    Serial.begin(115200);
    sempPrintLn(output);
    sempPrintStringLn(output, "RTCM_Test example sketch");
    sempPrintLn(output);

    // Verify the buffer size
    bufferLength = sempGetBufferLength(parserTable, parserCount);
    if (sizeof(buffer) < bufferLength)
    {
        sempPrintString(output, "Set buffer size to >= ");
        sempPrintDecimalI32Ln(output, bufferLength);
        reportFatalError("Fix the buffer size!");
    }

    // Initialize the parser
    parse = sempBeginParser("RTCM_Test", parserTable, parserCount,
                            buffer, bufferLength, processMessage, output);
    if (!parse)
        reportFatalError("Failed to initialize the parser");

    // Obtain a raw data stream from somewhere
    sempPrintString(output, "Raw data stream: ");
    sempPrintDecimalI32(output, RAW_DATA_BYTES);
    sempPrintStringLn(output, " bytes");

    // The raw data stream is passed to the parser one byte at a time
    sempDebugOutputEnable(parse);
    for (dataOffset = 0; dataOffset < RAW_DATA_BYTES; dataOffset++)
        // Update the parser state based on the incoming byte
        sempParseNextByte(parse, rawDataStream[dataOffset]);

    // Done parsing the data
    sempStopParser(&parse);
    sempPrintStringLn(output, "All done");
}

//----------------------------------------
// Main loop processing, repeatedly called after system is initialized by setup
//----------------------------------------
void loop()
{
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
    sempPrintString(output, "Valid RTCM ");
    sempPrintDecimalI32(output, sempRtcmGetMessageNumber(parse));
    sempPrintString(output, " message: ");
    sempPrintDecimalI32(output, parse->length);
    sempPrintString(output, " bytes at ");
    sempPrintHex0x08x(output, offset);
    sempPrintString(output, " (");
    sempPrintDecimalI32(output, offset);
    sempPrintCharLn(output, ')');
    dumpBuffer(parse->buffer, parse->length);

    sempPrintString(output, "Using sempRtcmGetUnsignedBits: message number is: ");
    sempPrintDecimalI32Ln(output, sempRtcmGetUnsignedBits(parse, 0, 12));
    if (sempRtcmGetMessageNumber(parse) == 1005)
    {
        sempPrintString(output, "RTCM 1005 ARP is: X ");
        sempPrintDecimalU64(output, sempRtcmGetSignedBits(parse, 34, 38));
        sempPrintString(output, " Y ");
        sempPrintDecimalU64(output, sempRtcmGetSignedBits(parse, 74, 38));
        sempPrintString(output, " Z ");
        sempPrintDecimalU64Ln(output, sempRtcmGetSignedBits(parse, 114, 38));
    }

    // Display the parser state
    if (displayOnce)
    {
        displayOnce = false;
        sempPrintLn(output);
        sempPrintParserConfiguration(parse, output);
    }
}
