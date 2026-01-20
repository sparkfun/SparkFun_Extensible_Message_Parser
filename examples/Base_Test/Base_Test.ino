/*
  SparkFun base test example sketch, verify it builds

  License: MIT. Please see LICENSE.md for more details
*/

#include <SparkFun_Extensible_Message_Parser.h> //http://librarymanager/All#SparkFun_Extensible_Message_Parser

//----------------------------------------
// Constants
//----------------------------------------

const uint8_t rawDataStream[] =
{
    0, 1, 2, 3, 4, 5
};

#define RAW_DATA_BYTES      sizeof(rawDataStream)

// Forward routine declarations
bool noParserPreamble(SEMP_PARSE_STATE *parse, uint8_t data);

// Build the description for the No parser
SEMP_PARSER_DESCRIPTION noParserDescription =
{
    "No parser",            // parserName
    noParserPreamble,       // preamble
};

// Build the table listing all of the parsers
SEMP_PARSER_DESCRIPTION * parserTable[] =
{
    &noParserDescription
};
const int parserCount = sizeof(parserTable) / sizeof(parserTable[0]);

//----------------------------------------
// Locals
//----------------------------------------

uint32_t dataOffset;
SEMP_PARSE_STATE *parse;

//----------------------------------------
// Test routine
//----------------------------------------

void setup()
{
    uint8_t * buffer;
    size_t bufferLength;

    delay(1000);

    Serial.begin(115200);
    Serial.println();
    Serial.println("Base_Test example sketch");
    Serial.println();

    // No name specified
    parse = sempBeginParser(nullptr, parserTable, parserCount,
                            0, buffer, bufferLength, processMessage);
    if (parse)
        reportFatalError("Failed to detect parserName set to nullptr (0)");

    // No name specified
    parse = sempBeginParser("", parserTable, parserCount,
                            0, buffer, bufferLength, processMessage);
    if (parse)
        reportFatalError("Failed to detect parserName set to empty string");

    // No parser table specified
    bufferLength = sempGetBufferLength(0, 3000);
    buffer = (uint8_t *)malloc(bufferLength);
    parse = sempBeginParser("No parser", nullptr, parserCount,
                            0, buffer, bufferLength, processMessage);
    if (parse)
        reportFatalError("Failed to detect parserTable set to nullptr (0)");

    // Parser count is zero
    parse = sempBeginParser("No parser", parserTable, 0,
                            0, buffer, bufferLength, processMessage);
    if (parse)
        reportFatalError("Failed because parserCount != nameTableCount");

    // No buffer specified
    parse = sempBeginParser("No parser", parserTable, parserCount,
                            0, nullptr, bufferLength, processMessage);
    if (parse)
        reportFatalError("Failed to detect buffer set to nullptr (0)");

    // Too small a buffer specified
    parse = sempBeginParser("No parser", parserTable, parserCount,
                            0, buffer, 0, processMessage);
    if (parse)
        reportFatalError("Failed to detect buffer set to nullptr (0)");

    // No end-of-message callback specified
    parse = sempBeginParser("No parser", parserTable, parserCount,
                            0, buffer, bufferLength, nullptr);
    if (parse)
        reportFatalError("Failed to detect eomCallback set to nullptr (0)");
    free(buffer);

    // Initialize the parser, specify a large scratch pad area
    bufferLength = sempGetBufferLength(4091, 3000);
    buffer = (uint8_t *)malloc(bufferLength);

    parse = sempBeginParser("Base Test Example", parserTable, parserCount,
                            4091, buffer, bufferLength, processMessage);
    if (!parse)
        reportFatalError("Failed to initialize the parser");
    Serial.println("Parser successfully initialized");

    // Display the parser configuration
    Serial.printf("&parserTable: %p\r\n", parserTable);
    sempPrintParserConfiguration(parse, &Serial);

    // Display the parse state
    Serial.printf("Parse State: %s\r\n", sempGetStateName(parse));

    // Display the parser type
    Serial.printf("Parser Name: %s\r\n", sempGetTypeName(parse, parse->type));

    // Obtain a raw data stream from somewhere
    sempEnableDebugOutput(parse);
    Serial.printf("Raw data stream: %d bytes\r\n", RAW_DATA_BYTES);

    // The raw data stream is passed to the parser one byte at a time
    for (dataOffset = 0; dataOffset < RAW_DATA_BYTES; dataOffset++)
        // Update the parser state based on the incoming byte
        sempParseNextByte(parse, rawDataStream[dataOffset]);

    // Done parsing the data
    sempStopParser(&parse);
    free(buffer);
    Serial.printf("All done\r\n");
}

void loop()
{
}

// Check for the preamble
bool noParserPreamble(SEMP_PARSE_STATE *parse, uint8_t data)
{
    // Preamble not found
    return false;
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
    Serial.printf("Valid Message: %d bytes at 0x%08x (%d)\r\n",
                  parse->length, offset, offset);
    dumpBuffer(parse->buffer, parse->length);

    // Display the parser state
    if (displayOnce)
    {
        displayOnce = false;
        Serial.println();
        sempPrintParserConfiguration(parse, &Serial);
    }
}

// Display the contents of a buffer
void dumpBuffer(uint8_t *buffer, uint16_t length)
{
    int bytes;
    uint8_t *end;
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
