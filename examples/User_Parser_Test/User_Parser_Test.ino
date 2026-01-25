/*
  SparkFun user parser example sketch

  This example demonstrates how to write your own parser -
  with its own state machine and support methods

  License: MIT. Please see LICENSE.md for more details
*/

#include "User_Parser.h"

#include "../Common/dumpBuffer.ino"
#include "../Common/reportFatalError.ino"

//----------------------------------------
// Constants
//----------------------------------------

// Provide some valid and invalid NMEA sentences
const uint8_t rawDataStream[] =
{
    // Valid messages
    "AB1"   //  0

    // Invalid data, must skip over
    "ab2"   //  3

    // Valid messages
    "AB3"   //  6

    // Invalid character in the message header
    "Ab4"   //  9

    // Sentence too long by a single byte
    "ABC5"  // 12

    // Valid messages
    "AB6"   //  16
};

// Number of bytes in the rawDataStream
#define RAW_DATA_BYTES      (sizeof(rawDataStream) - 1)

SEMP_PARSER_DESCRIPTION * userParserTable[] =
{
    &userParserDescription
};
const int userParserCount = sizeof(userParserTable) / sizeof(userParserTable[0]);

//----------------------------------------
// Locals
//----------------------------------------

// Account for the largest message
uint8_t buffer[83];
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

    int dataIndex;

    delay(1000);

    Serial.begin(115200);
    Serial.println();
    Serial.println("User_Parser example sketch");
    Serial.println();

    // Verify the buffer size
    bufferLength = sempGetBufferLength(userParserTable, userParserCount);
    if (sizeof(buffer) < bufferLength)
    {
        Serial.printf("Set buffer size to >= %d\r\n", bufferLength);
        reportFatalError("Fix the buffer size!");
    }

    // Initialize the parser
    parse = sempBeginParser("User_Parser", userParserTable, userParserCount,
                            buffer, bufferLength, userMessage);
    if (!parse)
        reportFatalError("Failed to initialize the user parser");

    // Obtain a raw data stream from somewhere
    Serial.printf("Raw data stream: %d bytes\r\n", RAW_DATA_BYTES);

    // The raw data stream is passed to the parser one byte at a time
    sempEnableDebugOutput(parse);
    for (dataOffset = 0; dataOffset < RAW_DATA_BYTES; dataOffset++)
    {
        uint8_t data;
        const char * endState;
        const char * startState;

        // Get the parse state before entering the parser to enable
        // printing of the parser transition
        startState = sempGetStateName(parse);

        // Update the parser state based on the incoming byte
        data = rawDataStream[dataOffset];
        sempParseNextByte(parse, data);

        // Print the parser transition
        endState = sempGetStateName(parse);
        Serial.printf("0x%02x (%c), state: (%p) %s --> %s (%p)\r\n",
                      rawDataStream[dataOffset],
                      ((data >= ' ') && (data < 0x7f)) ? data : '.',
                      startState, startState, endState, endState);
    }

    // Done parsing the data
    sempStopParser(&parse);
    Serial.printf("All done\r\n");
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
void userMessage(SEMP_PARSE_STATE *parse, uint16_t type)
{
    static bool displayOnce = true;
    uint32_t offset;

    // Display the raw message
    Serial.println();
    offset = dataOffset + 1 - parse->length;
    Serial.printf("Valid Message %d, %d bytes at 0x%08x (%d)\r\n",
                  userParserGetMessageNumber(parse), parse->length, offset, offset);
    dumpBuffer(parse->buffer, parse->length);

    // Display the parser state
    if (displayOnce)
    {
        displayOnce = false;
        Serial.println();
        sempPrintParserConfiguration(parse, &Serial);
    }
}
