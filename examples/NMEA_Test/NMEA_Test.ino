/*
  SparkFun NMEA test example sketch

  This example demonstrates how to parse a NMEA data stream

  License: MIT. Please see LICENSE.md for more details
*/

#include <SparkFun_Extensible_Message_Parser.h> //http://librarymanager/All#SparkFun_Extensible_Message_Parser

//----------------------------------------
// Constants
//----------------------------------------

// Build the table listing all of the parsers
SEMP_PARSE_ROUTINE const parserTable[] =
{
    sempNmeaPreamble
};
const int parserCount = sizeof(parserTable) / sizeof(parserTable[0]);

const char * const parserNames[] =
{
    "NMEA parser"
};
const int parserNameCount = sizeof(parserNames) / sizeof(parserNames[0]);


// Provide some valid and invalid NMEA sentences
const uint8_t rawDataStream[] =
{
    // Valid NMEA sentences, with or without CR or LF
    "$GPRMC,210230,A,3855.4487,N,09446.0071,W,0.0,076.2,130495,003.8,E*69"      //   0
    "$GPRMC,210230,A,3855.4487,N,09446.0071,W,0.0,076.2,130495,003.8,E*69\r"    //  68
    "$GPRMC,210230,A,3855.4487,N,09446.0071,W,0.0,076.2,130495,003.8,E*69\n"    // 137

    // Invalid data, must skip over
    "abcdefg"                                                                   // 206

    // Valid NMEA sentences
    //        1         2         3         4         5         6         7         8
    //2345678901234567890123456789012345678901234567890123456789012345678901234567890
    "$GPGGA,210230,3855.4487,N,09446.0071,W,1,07,1.1,370.5,M,-29.5,M,,*7A\r\n"  // 213
    "$GPGSV,2,1,08,02,74,042,45,04,18,190,36,07,67,279,42,12,29,323,36*77\r\n"  // 283
    "$GPGSV,2,2,08,15,30,050,47,19,09,158,,26,12,281,40,27,38,173,41*7B\r\n"    // 353
    "$GPRMC,210230,A,3855.4487,N,09446.0071,W,0.0,076.2,130495,003.8,E*69\r\n"  // 421

    // Long valid name
    //123456789012345
    "$ABCDEFGHIJKLMNO,210230,A,3855.4487,N,09446.0071,W,0.0,076.2,1,00*15\r\n"  // 491

    // Name too long
    //1234567890123456
    "$ABCDEFGHIJKLMNOP,210230,A,3855.4487,N,09446.0071,W,0.0,076.2,130*6A\r\n"  // 561

    // Invalid character in the sentence name
    "$G@RMC,210230,A,3855.4487,N,09446.0071,W,0.0,076.2,130495,003.8,E*69\r\n"  // 631
    "$G[RMC,210230,A,3855.4487,N,09446.0071,W,0.0,076.2,130495,003.8,E*69\r\n"  // 701

    // Bad checksum
    "$GaRMC,210230,A,3855.4487,N,09446.0071,W,0.0,076.2,130495,003.8,E*69\r\n"  // 771
    "$GzRMC,210230,A,3855.4487,N,09446.0071,W,0.0,076.2,130495,003.8,E*69\r\n"  // 841
    "$GPRMC,210230,A,3855.4487,N,09446.0071,W,0.0,076.2,130495,003.8,E*68\r\n"  // 911

    // Sentence too long by a single byte
    //        1         2         3         4         5         6         7         8
    //2345678901234567890123456789012345678901234567890123456789012345678901234567890
    "$GPRMC,210230,A,3855.4487,N,09446.0071,W,0.0,076.2,130495,003.8,E+*69\r\n" // 981
};

// Number of bytes in the rawDataStream
#define RAW_DATA_BYTES      sizeof(rawDataStream)

// Account for the largest NMEA sentence + CR + LF + zero termination
#define BUFFER_LENGTH   68 + 1 + 1 + 1

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
    Serial.println("NMEA_Test_example sketch");
    Serial.println();

    // Initialize the parser
    parse = sempBeginParser(parserTable, parserCount,
                            parserNames, parserNameCount,
                            0, BUFFER_LENGTH, processMessage, "NMEA_Test");
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

    // Determine the raw data stream offset
    offset = dataOffset + 1 + 2 - parse->length;
    while (rawDataStream[offset] != '$')
        offset -= 1;

    // Display the raw message
    Serial.println();
        Serial.printf("Valid NMEA Sentence: %s, %d bytes at 0x%08x (%d)\r\n",
                  sempNmeaGetSentenceName(parse), parse->length, offset, offset);
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
