/*
  SparkFun mixed parser example sketch

  This example demonstrates how to create a mixed parser -
  here we parse u-blox UBX and NMEA using a single mixed parser

  License: MIT. Please see LICENSE.md for more details
*/

#include <SparkFun_Extensible_Message_Parser.h> //http://librarymanager/All#SparkFun_Extensible_Message_Parser

//----------------------------------------
// Constants
//----------------------------------------

// Define the indexes into the parserTable array
#define NMEA_PARSER_INDEX       0
#define UBLOX_PARSER_INDEX      1

// Build the table listing all of the parsers
SEMP_PARSE_ROUTINE const parserTable[] =
{
    sempNmeaPreamble,
    sempUbloxPreamble,
};
const int parserCount = sizeof(parserTable) / sizeof(parserTable[0]);

const char * const parserNames[] =
{
    "NMEA parser",
    "U-Blox parser",
};
const int parserNameCount = sizeof(parserNames) / sizeof(parserNames[0]);

// Provide a mix of NMEA sentences and u-blox messages
const uint8_t nmea_1[] =
{
    "$GPRMC,210230,A,3855.4487,N,09446.0071,W,0.0,076.2,130495,003.8,E*69"   //0
    "$GPRMC,210230,A,3855.4487,N,09446.0071,W,0.0,076.2,130495,003.8,E*69\r" //  68
    "$GPRMC,210230,A,3855.4487,N,09446.0071,W,0.0,076.2,130495,003.8,E*69\n" // 137
};

const uint8_t ublox_1[] =
{
    0xb5, 0x62, 0x02, 0x13, 0x28, 0x00, 0x02, 0x24, // 207
    0x01, 0x00, 0x08, 0x30, 0x02, 0x00, 0x55, 0x55,
    0x95, 0x00, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
    0x55, 0x55, 0x00, 0x40, 0xbd, 0x52, 0x45, 0x5c,
    0x9e, 0xa3, 0xea, 0x40, 0x40, 0xee, 0x75, 0x45,
    0xaa, 0xaa, 0x00, 0x40, 0x3f, 0x85, 0x20, 0xc1,

                                        0xb5, 0x62, // 255
    0x02, 0x13, 0x28, 0x00, 0x02, 0x05, 0x05, 0x00,
    0x08, 0x49, 0x02, 0x43, 0x55, 0x55, 0x95, 0x00,
    0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
    0x00, 0x40, 0xbd, 0x52, 0x00, 0x80, 0x9e, 0xa3,
    0x2a, 0x00, 0x00, 0x00, 0x30, 0xb2, 0xaa, 0xaa,
    0x00, 0x40, 0x7f, 0x0a, 0xff, 0xb4,
};

const uint8_t nmea_2[] =
{
    "$GPGGA,210230,3855.4487,N,09446.0071,W,1,07,1.1,370.5,M,-29.5,M,,*7A\r\n"  // 303
    "$GPGSV,2,1,08,02,74,042,45,04,18,190,36,07,67,279,42,12,29,323,36*77\r\n", // 373
};

const uint8_t ublox_2[] =
{

                                        0xb5, 0x62, // 444
    0x02, 0x13, 0x28, 0x00, 0x02, 0x09, 0x05, 0x00,
    0x08, 0x40, 0x02, 0x43, 0x55, 0x55, 0x95, 0x00,
    0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
    0x00, 0x40, 0xbd, 0x52, 0x00, 0x80, 0x9e, 0xa3,
    0x2a, 0x00, 0x00, 0x00, 0x30, 0xb2, 0xaa, 0xaa,
    0x00, 0x40, 0x7f, 0x0a, 0xfa, 0x15,

                                              0xb5, // 492
    0x62, 0x02, 0x13, 0x18, 0x00, 0x06, 0x02, 0x00,
    0x03, 0x04, 0x0d, 0x02, 0x00, 0x00, 0x80, 0xc1,
    0x2b, 0x00, 0x1c, 0x01, 0x00, 0x00, 0xd8, 0x11,
    0x03, 0x03, 0x00, 0xc6, 0x09, 0x92, 0x8a,
};

const uint8_t nmea_3[] =
{

    "$GPGSV,2,2,08,15,30,050,47,19,09,158,,26,12,281,40,27,38,173,41*7B\r\n"    // 524
    "$GPRMC,210230,A,3855.4487,N,09446.0071,W,0.0,076.2,130495,003.8,E*69\r\n", // 592
};

const uint8_t ublox_3[] =
{

                                              0xb5, // 663
    0x62, 0x02, 0x13, 0x18, 0x00, 0x06, 0x0c, 0x02,
    0x06, 0x04, 0x5b, 0x02, 0x43, 0x00, 0x80, 0xc1,
    0x2b, 0x00, 0x1c, 0x01, 0x00, 0x00, 0xd8, 0x11,
    0x03, 0x03, 0x00, 0xc6, 0x09, 0x32, 0x18,
};

const uint8_t nmea_4[] =
{

    "$ABCDEFGHIJKLMNO,210230,A,3855.4487,N,09446.0071,W,0.0,076.2,1,00*15\r\n", // 695
};

const uint8_t ublox_4[] =
{

                                              0xb5, // 766
    0x62, 0x02, 0x13, 0x18, 0x00, 0x06, 0x03, 0x00,
    0x0c, 0x04, 0x11, 0x02, 0x00, 0x00, 0x80, 0xc1,
    0x2b, 0x00, 0x1c, 0x01, 0x00, 0x00, 0xd8, 0x11,
    0x03, 0x03, 0x00, 0xc6, 0x09, 0xa0, 0xaa,

                                              0xb5, // 798
    0x62, 0x02, 0x13, 0x28, 0x00, 0x02, 0x09, 0x05,
    0x00, 0x08, 0x40, 0x02, 0x43, 0x55, 0x55, 0x95,
    0x00, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
    0x55, 0x00, 0x40, 0xbd, 0x52, 0x00, 0x00, 0xe5,
    0xa3, 0x2a, 0x00, 0x00, 0x00, 0xf6, 0x8e, 0xaa,
    0xaa, 0x00, 0x40, 0x3f, 0x50, 0x69, 0x71,
};

typedef struct _DataStream
{
    size_t length;
    const uint8_t *data;
} DataStream;

#define DATA_STREAM_INIT(x)     {sizeof(x), &x[0]}
const DataStream dataStream[] =
{
    DATA_STREAM_INIT(nmea_1),
    DATA_STREAM_INIT(ublox_1),
    DATA_STREAM_INIT(nmea_2),
    DATA_STREAM_INIT(ublox_2),
    DATA_STREAM_INIT(nmea_3),
    DATA_STREAM_INIT(ublox_3),
    DATA_STREAM_INIT(nmea_4),
    DATA_STREAM_INIT(ublox_4)
};

#define DATA_STREAM_ENTRIES     (sizeof(dataStream) / sizeof(dataStream[0]))

// Account for the largest u-blox messages
#define BUFFER_LENGTH   3000

//----------------------------------------
// Locals
//----------------------------------------

int byteOffset;
int dataIndex;
uint32_t dataOffset;
SEMP_PARSE_STATE *parse;

//----------------------------------------
// Test routine
//----------------------------------------

// Initialize the system
void setup()
{
    int rawDataBytes;

    delay(1000);

    Serial.begin(115200);
    Serial.println();
    Serial.println("Mixed_Parser example sketch");
    Serial.println();

    // Initialize the parser
    parse = sempBeginParser(parserTable, parserCount,
                            parserNames, parserNameCount,
                            0, BUFFER_LENGTH, processMessage, "Mixed_Parser");
    if (!parse)
        reportFatalError("Failed to initialize the parser");

    // Obtain a raw data stream from somewhere
    rawDataBytes = 0;
    for (dataIndex = 0; dataIndex < DATA_STREAM_ENTRIES; dataIndex++)
        rawDataBytes += dataStream[dataIndex].length;
    Serial.printf("Raw data stream: %d bytes\r\n", rawDataBytes);

    // The raw data stream is passed to the parser one byte at a time
    sempEnableDebugOutput(parse);
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

// Main loop processing after system is initialized
void loop()
{
}

// Call back from within parser, for end of message
// Process a complete message incoming from parser
void processMessage(SEMP_PARSE_STATE *parse, uint16_t type)
{
    static bool displayOnce = true;
    uint32_t byteIndex;
    uint32_t offset;

    // Display the raw message
    Serial.println();
    switch (type) // Index into parserTable array
    {
        case NMEA_PARSER_INDEX:
            // Determine the raw data stream offset
            offset = dataOffset + 2 - parse->length;
            byteIndex = byteOffset + 2 - parse->length;
            while (dataStream[dataIndex].data[byteIndex] != '$')
            {
                offset -= 1;
                byteIndex -= 1;
            }
            Serial.printf("Valid NMEA Sentence: %s, %d bytes at 0x%08x (%d)\r\n",
                          sempNmeaGetSentenceName(parse), parse->length, offset, offset);
            break;

        case UBLOX_PARSER_INDEX:
            offset = dataOffset + 1 - parse->length;
            Serial.printf("Valid u-blox message: %d bytes at 0x%08x (%d)\r\n",
                          parse->length, offset, offset);
            break;
    }
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
