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

// Build the table listing all of the parsers
bool noParserPreamble(SEMP_PARSE_STATE *parse, uint8_t data);
SEMP_PARSE_ROUTINE const parserTable[] =
{
    noParserPreamble
};
const int parserCount = sizeof(parserTable) / sizeof(parserTable[0]);

const char * const parserNames[] =
{
    "No parser"
};
const int parserNameCount = sizeof(parserNames) / sizeof(parserNames[0]);

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
    delay(1000);

    Serial.begin(115200);
    Serial.println();
    Serial.println("Base_Test example sketch");
    Serial.println();

    // No parser table specified
    parse = sempInitParser(nullptr, parserCount,
                           parserNames, parserNameCount,
                           0, 3000, processMessage, "No parser");
    if (parse)
        reportFatalError("Failed to detect parserTable set to nullptr (0)");

    // Parser count is zero
    parse = sempInitParser(parserTable, 0,
                           parserNames, parserCount,
                           0, 3000, processMessage, "No parser");
    if (parse)
        reportFatalError("Failed because parserCount != nameTableCount");

    // No parser name table specified
    parse = sempInitParser(parserTable, parserCount,
                           nullptr, parserNameCount,
                           0, 3000, processMessage, "No parser");
    if (parse)
        reportFatalError("Failed to detect parserNameTable set to nullptr (0)");

    // Parser name count is zero
    parse = sempInitParser(parserTable, parserCount,
                           parserNames, 0,
                           0, 3000, processMessage, "No parser");
    if (parse)
        reportFatalError("Failed because parserCount != nameTableCount");

    // No end-of-message callback specified
    parse = sempInitParser(parserTable, parserCount,
                           parserNames, parserNameCount,
                           0, SEMP_MINIMUM_BUFFER_LENGTH, nullptr, "No parser");
    if (parse)
        reportFatalError("Failed to detect eomCallback set to nullptr (0)");

    // No name specified
    parse = sempInitParser(parserTable, parserCount,
                           parserNames, parserNameCount,
                           0, SEMP_MINIMUM_BUFFER_LENGTH, processMessage, nullptr);
    if (parse)
        reportFatalError("Failed to detect parserName set to nullptr (0)");

    // No name specified
    parse = sempInitParser(parserTable, parserCount,
                           parserNames, parserNameCount,
                           0, SEMP_MINIMUM_BUFFER_LENGTH, processMessage, "");
    if (parse)
        reportFatalError("Failed to detect parserName set to empty string");

    // Initialize the parser, specify a large scratch pad area
    // Display the parser allocation
    sempPrintErrorMessages = true;
    parse = sempInitParser(parserTable, parserCount,
                           parserNames, parserNameCount,
                           4091, 3000, processMessage, "Base Test Example");
    if (!parse)
        reportFatalError("Failed to initialize the parser");
    Serial.println("Parser successfully initialized");

    // Display the parser configuration
    sempPrintParserConfiguration(parse);

    // Obtain a raw data stream from somewhere
    Serial.printf("Raw data stream: %d bytes\r\n", RAW_DATA_BYTES);

    // The raw data stream is passed to the parser one byte at a time
    for (dataOffset = 0; dataOffset < RAW_DATA_BYTES; dataOffset++)
        // Update the parser state based on the incoming byte
        sempParseNextByte(parse, rawDataStream[dataOffset]);

    // Done parsing the data
    sempShutdownParser(&parse);
    Serial.println("Parser shutdown");
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

// Print a line of text
void sempExtPrintLineOfText(const char *string)
{
    Serial.println(string);
}

// Call back from within parser, for end of message
// Process a complete message incoming from parser
void processMessage(SEMP_PARSE_STATE *parse, uint8_t type)
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
        sempPrintParserConfiguration(parse);
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
