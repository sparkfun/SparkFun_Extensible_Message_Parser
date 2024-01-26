/*
  SparkFun user parser example sketch

  License: MIT. Please see LICENSE.md for more details
*/

#include <SparkFun_Extensible_Message_Parser.h> //http://librarymanager/All#SparkFun_Extensible_Message_Parser

//----------------------------------------
// Types
//----------------------------------------

typedef struct _USER_SCRATCH_PAD
{
    uint32_t messageNumber; // Count the valid messages
} USER_SCRATCH_PAD;

//----------------------------------------
// User Parser
//
// The routines below are specified in the reverse order of execution
// to eliminate the need for forward references and possibly exposing
// them via a header file to the application.
//----------------------------------------

// Check for a number
bool userFindNumber(SEMP_PARSE_STATE *parse, uint8_t data)
{
    USER_SCRATCH_PAD *scratchPad = (USER_SCRATCH_PAD *)parse->scratchPad;
    if ((data < '0') || (data > '9'))
        return sempFirstByte(parse, data);

    // Account for the valid message
    scratchPad->messageNumber += 1;

    // Pass the valid message to the end-of-message handler
    parse->eomCallback(parse, parse->type);

    // Start searching for a preamble byte
    parse->state = sempFirstByte;
    return true;
}

// Check for the second preamble byte
bool userSecondPreambleByte(SEMP_PARSE_STATE *parse, uint8_t data)
{
    USER_SCRATCH_PAD *scratchPad = (USER_SCRATCH_PAD *)parse->scratchPad;

    // Validate the second preamble byte
    if (data != 'B')
    {
        // Print the error
        if (sempPrintErrorMessages)
        {
            char line[128];
            sprintf(line, "USER_Parser: Bad second preamble byte after message %d", scratchPad->messageNumber);
            sempExtPrintLineOfText(line);
        }

        // Start searching for a preamble byte
        return sempFirstByte(parse, data);
    }

    // Valid preamble, search for a number
    parse->state = userFindNumber;
    return true;
}

// Check for the first preamble byte
bool userPreamble(SEMP_PARSE_STATE *parse, uint8_t data)
{
    // Validate the first preamble byte
    if (data != 'A')
        // Start searching for a preamble byte
        return false;

    // Valid preamble byte, search for the next preamble byte
    parse->state = userSecondPreambleByte;
    return true;
}

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
#define RAW_DATA_BYTES      sizeof(rawDataStream)

// Account for the largest message
#define BUFFER_LENGTH   3

SEMP_PARSE_ROUTINE const userParserTable[] =
{
    userPreamble
};
const int userParserCount = sizeof(userParserTable) / sizeof(userParserTable[0]);

const char * const userParserNames[] =
{
    "User parser"
};
const int userParserNameCount = sizeof(userParserNames) / sizeof(userParserNames[0]);

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
    int dataIndex;
    int rawDataBytes;

    delay(1000);

    Serial.begin(115200);
    Serial.println();
    Serial.println("Muliple_Parser example sketch");
    Serial.println();

    // Initialize the parser
    sempPrintErrorMessages = true;
    parse = sempInitParser(userParserTable, userParserCount,
                           userParserNames, userParserNameCount,
                           sizeof(USER_SCRATCH_PAD),
                           BUFFER_LENGTH, userMessage, "User_Parser");
    if (!parse)
        reportFatalError("Failed to initialize the user parser");

    // Obtain a raw data stream from somewhere
    Serial.printf("Raw data stream: %d bytes\r\n", rawDataBytes);

    // The raw data stream is passed to the parser one byte at a time
    for (dataOffset = 0; dataOffset < RAW_DATA_BYTES; dataOffset++)
        // Update the parser state based on the incoming byte
        sempParseNextByte(parse, rawDataStream[dataOffset]);

    // Done parsing the data
    sempShutdownParser(&parse);
    free(parse);
}

// Main loop processing after system is initialized
void loop()
{
}

// Output a line of text for the SparkFun Extensible Message Parser
void sempExtPrintLineOfText(const char *string)
{
    Serial.println(string);
}

// Call back from within parser, for end of message
// Process a complete message incoming from parser
void userMessage(SEMP_PARSE_STATE *parse, uint8_t type)
{
    USER_SCRATCH_PAD *scratchPad = (USER_SCRATCH_PAD *)parse->scratchPad;
    static bool displayOnce = true;
    uint32_t offset;

    // Display the raw message
    Serial.println();
    offset = dataOffset + 1 - parse->length;
    Serial.printf("Valid Message %d, %d bytes at 0x%08x (%d)\r\n",
                  scratchPad->messageNumber, parse->length, offset, offset);
    dumpBuffer(parse->buffer, parse->length);

    // Display the parser state
    if (displayOnce)
    {
        displayOnce = false;
        Serial.println();
        sempPrintParserConfiguration(parse);
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
        Serial.printf("0x%08lx: ", offset);

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
