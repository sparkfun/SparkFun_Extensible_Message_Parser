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
        sempPrintf(parse->printDebug,
                   "USER_Parser: Bad second preamble byte after message %d",
                   scratchPad->messageNumber);

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

// Translates state value into an string, returns nullptr if not found
const char * userParserStateName(const SEMP_PARSE_STATE *parse)
{
    if (parse->state == userPreamble)
        return "userPreamble";
    if (parse->state == userSecondPreambleByte)
        return "userSecondPreambleByte";
    if (parse->state == userFindNumber)
        return "userFindNumber";
    return nullptr;
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

    delay(1000);

    Serial.begin(115200);
    Serial.println();
    Serial.println("User_Parser example sketch");
    Serial.println();

    // Initialize the parser
    parse = sempBeginParser(userParserTable, userParserCount,
                            userParserNames, userParserNameCount,
                            sizeof(USER_SCRATCH_PAD),
                            BUFFER_LENGTH, userMessage, "User_Parser");
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
        startState = getParseStateName(parse);

        // Update the parser state based on the incoming byte
        data = rawDataStream[dataOffset];
        sempParseNextByte(parse, data);

        // Print the parser transition
        endState = getParseStateName(parse);
        Serial.printf("0x%02x (%c), state: (%p) %s --> %s (%p)\r\n",
                      rawDataStream[dataOffset],
                      ((data >= ' ') && (data < 0x7f)) ? data : '.',
                      startState, startState, endState, endState);
    }

    // Done parsing the data
    sempStopParser(&parse);
    free(parse);
}

// Main loop processing after system is initialized
void loop()
{
}

// Call back from within parser, for end of message
// Process a complete message incoming from parser
void userMessage(SEMP_PARSE_STATE *parse, uint16_t type)
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

// Translate the state value into an ASCII state name
const char *getParseStateName(SEMP_PARSE_STATE *parse)
{
    const char *name;

    do
    {
        name = userParserStateName(parse);
        if (name)
            break;
        name = sempGetStateName(parse);
    } while (0);
    return name;
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
