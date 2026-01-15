/*
  SparkFun RTCM test example sketch

  This example demonstrates how to parse a RTCM data stream

  License: MIT. Please see LICENSE.md for more details
*/

#include <SparkFun_Extensible_Message_Parser.h> //http://librarymanager/All#SparkFun_Extensible_Message_Parser

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

// Account for the largest RTCM messages
#define BUFFER_LENGTH   62

//----------------------------------------
// Locals
//----------------------------------------

uint32_t dataOffset;
SEMP_PARSE_STATE *parse;

//----------------------------------------
// Test routine
//----------------------------------------

// Initialize the system
void setup()
{
    delay(1000);

    Serial.begin(115200);
    Serial.println();
    Serial.println("RTCM_Test example sketch");
    Serial.println();

    // Initialize the parser
    size_t bufferLength = sempGetBufferLength(parserTable,
                                              parserCount,
                                              BUFFER_LENGTH);
    uint8_t * buffer = (uint8_t *)malloc(bufferLength);
    parse = sempBeginParser("RTCM_Test", parserTable, parserCount,
                            buffer, bufferLength, processMessage);
    if (!parse)
        reportFatalError("Failed to initialize the parser");

    // Obtain a raw data stream from somewhere
    Serial.printf("Raw data stream: %d bytes\r\n", RAW_DATA_BYTES);

    // The raw data stream is passed to the parser one byte at a time
    sempEnableDebugOutput(parse);
    for (dataOffset = 0; dataOffset < RAW_DATA_BYTES; dataOffset++)
        // Update the parser state based on the incoming byte
        sempParseNextByte(parse, rawDataStream[dataOffset]);

    // Done parsing the data
    sempStopParser(&parse);
    free(buffer);
    Serial.printf("All done\r\n");
}

// Main loop processing after system is initialized
void loop()
{
}

// Call back from within parser, for end of message
// Process a complete message incoming from parser
void processMessage(SEMP_PARSE_STATE *parse, uint16_t type)
{
    static bool displayOnce = true;
    uint32_t offset;

    // Display the raw message
    Serial.println();
    offset = dataOffset + 1 - parse->length;
    Serial.printf("Valid RTCM %d message: %d bytes at 0x%08x (%d)\r\n",
                  sempRtcmGetMessageNumber(parse), (int)parse->length, (unsigned int)offset, (int)offset);
    dumpBuffer(parse->buffer, parse->length);
    Serial.printf("Using sempRtcmGetUnsignedBits: message number is: %lld\r\n",
                  sempRtcmGetUnsignedBits(parse, 0, 12));
    if (sempRtcmGetMessageNumber(parse) == 1005)
    {
        Serial.printf("RTCM 1005 ARP is: X %lld Y %lld Z %lld\r\n",
                      sempRtcmGetSignedBits(parse, 34, 38),
                      sempRtcmGetSignedBits(parse, 74, 38),
                      sempRtcmGetSignedBits(parse, 114, 38));
    }

    // Display the parser state
    if (displayOnce)
    {
        displayOnce = false;
        Serial.println();
        sempPrintParserConfiguration(parse, &Serial);
    }
}

// Display the contents of a buffer
void dumpBuffer(const uint8_t *buffer, uint16_t length)
{
    int bytes;
    const uint8_t *end;
    int index;
    uint16_t offset;

    end = &buffer[length];
    offset = 0;
    while (buffer < end)
    {
        // Determine the number of bytes to display on the line
        bytes = end - buffer;
        if (bytes > (16 - (offset & 0xf)))
            bytes = 16 - (offset & 0xf);

        // Display the offset
        Serial.printf("0x%08x: ", offset);

        // Skip leading bytes
        for (index = 0; index < (offset & 0xf); index++)
            Serial.printf("   ");

        // Display the data bytes
        for (index = 0; index < bytes; index++)
            Serial.printf("%02x ", buffer[index]);

        // Separate the data bytes from the ASCII
        for (; index < (16 - (offset & 0xf)); index++)
            Serial.printf("   ");
        Serial.printf(" ");

        // Skip leading bytes
        for (index = 0; index < (offset & 0xf); index++)
            Serial.printf(" ");

        // Display the ASCII values
        for (index = 0; index < bytes; index++)
            Serial.printf("%c", ((buffer[index] < ' ') || (buffer[index] >= 0x7f)) ? '.' : buffer[index]);
        Serial.printf("\r\n");

        // Set the next line of data
        buffer += bytes;
        offset += bytes;
    }
}

// Print the error message every 15 seconds
void reportFatalError(const char *errorMsg)
{
    while (1)
    {
        Serial.print("HALTED: ");
        Serial.print(errorMsg);
        Serial.println();
        sleep(15);
    }
}
