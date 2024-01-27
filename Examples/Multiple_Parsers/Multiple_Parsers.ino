/*
  SparkFun multiple parser example sketch

  License: MIT. Please see LICENSE.md for more details
*/

#include <SparkFun_Extensible_Message_Parser.h> //http://librarymanager/All#SparkFun_Extensible_Message_Parser

//----------------------------------------
// Constants
//----------------------------------------

// Provide some valid and invalid u-blox messages
const uint8_t nmea_1[] =
{
    "$GPRMC,210230,A,3855.4487,N,09446.0071,W,0.0,076.2,130495,003.8,E*69"
    "$GPRMC,210230,A,3855.4487,N,09446.0071,W,0.0,076.2,130495,003.8,E*69\r"
    "$GPRMC,210230,A,3855.4487,N,09446.0071,W,0.0,076.2,130495,003.8,E*69\n"
};

const uint8_t ublox_1[] =
{
    0xb5, 0x62, 0x02, 0x13, 0x28, 0x00, 0x02, 0x24,
    0x01, 0x00, 0x08, 0x30, 0x02, 0x00, 0x55, 0x55,
    0x95, 0x00, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
    0x55, 0x55, 0x00, 0x40, 0xbd, 0x52, 0x45, 0x5c,
    0x9e, 0xa3, 0xea, 0x40, 0x40, 0xee, 0x75, 0x45,
    0xaa, 0xaa, 0x00, 0x40, 0x3f, 0x85, 0x20, 0xc1,

                                        0xb5, 0x62,
    0x02, 0x13, 0x28, 0x00, 0x02, 0x05, 0x05, 0x00,
    0x08, 0x49, 0x02, 0x43, 0x55, 0x55, 0x95, 0x00,
    0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
    0x00, 0x40, 0xbd, 0x52, 0x00, 0x80, 0x9e, 0xa3,
    0x2a, 0x00, 0x00, 0x00, 0x30, 0xb2, 0xaa, 0xaa,
    0x00, 0x40, 0x7f, 0x0a, 0xff, 0xb4,
};

const uint8_t nmea_2[] =
{
    "$GPGGA,210230,3855.4487,N,09446.0071,W,1,07,1.1,370.5,M,-29.5,M,,*7A\r\n"
    "$GPGSV,2,1,08,02,74,042,45,04,18,190,36,07,67,279,42,12,29,323,36*77\r\n",
};

const uint8_t ublox_2[] =
{

                                        0xb5, 0x62,
    0x02, 0x13, 0x28, 0x00, 0x02, 0x09, 0x05, 0x00,
    0x08, 0x40, 0x02, 0x43, 0x55, 0x55, 0x95, 0x00,
    0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
    0x00, 0x40, 0xbd, 0x52, 0x00, 0x80, 0x9e, 0xa3,
    0x2a, 0x00, 0x00, 0x00, 0x30, 0xb2, 0xaa, 0xaa,
    0x00, 0x40, 0x7f, 0x0a, 0xfa, 0x15,

                                              0xb5,
    0x62, 0x02, 0x13, 0x18, 0x00, 0x06, 0x02, 0x00,
    0x03, 0x04, 0x0d, 0x02, 0x00, 0x00, 0x80, 0xc1,
    0x2b, 0x00, 0x1c, 0x01, 0x00, 0x00, 0xd8, 0x11,
    0x03, 0x03, 0x00, 0xc6, 0x09, 0x92, 0x8a,
};

const uint8_t nmea_3[] =
{

    "$GPGSV,2,2,08,15,30,050,47,19,09,158,,26,12,281,40,27,38,173,41*7B\r\n"
    "$GPRMC,210230,A,3855.4487,N,09446.0071,W,0.0,076.2,130495,003.8,E*69\r\n",
};

const uint8_t ublox_3[] =
{

                                              0xb5,
    0x62, 0x02, 0x13, 0x18, 0x00, 0x06, 0x0c, 0x02,
    0x06, 0x04, 0x5b, 0x02, 0x43, 0x00, 0x80, 0xc1,
    0x2b, 0x00, 0x1c, 0x01, 0x00, 0x00, 0xd8, 0x11,
    0x03, 0x03, 0x00, 0xc6, 0x09, 0x32, 0x18,
};

const uint8_t nmea_4[] =
{

    "$ABCDEFGHIJKLMNO,210230,A,3855.4487,N,09446.0071,W,0.0,076.2,1,00*15\r\n",
};

const uint8_t ublox_4[] =
{

                                              0xb5,
    0x62, 0x02, 0x13, 0x18, 0x00, 0x06, 0x03, 0x00,
    0x0c, 0x04, 0x11, 0x02, 0x00, 0x00, 0x80, 0xc1,
    0x2b, 0x00, 0x1c, 0x01, 0x00, 0x00, 0xd8, 0x11,
    0x03, 0x03, 0x00, 0xc6, 0x09, 0xa0, 0xaa,

                                              0xb5,
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

SEMP_PARSE_ROUTINE const nmeaParserTable[] =
{
    sempNmeaPreamble
};
const int nmeaParserCount = sizeof(nmeaParserTable) / sizeof(nmeaParserTable[0]);

const char * const nmeaParserNames[] =
{
    "NMEA parser"
};
const int nmeaParserNameCount = sizeof(nmeaParserNames) / sizeof(nmeaParserNames[0]);

SEMP_PARSE_ROUTINE const ubloxParserTable[] =
{
    sempUbloxPreamble
};
const int ubloxParserCount = sizeof(ubloxParserTable) / sizeof(ubloxParserTable[0]);

const char * const ubloxParserNames[] =
{
    "U-Blox parser"
};
const int ubloxParserNameCount = sizeof(ubloxParserNames) / sizeof(ubloxParserNames[0]);

//----------------------------------------
// Locals
//----------------------------------------

uint32_t dataOffset;
SEMP_PARSE_STATE *nmeaParser;
SEMP_PARSE_STATE *ubloxParser;

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

    // Initialize the parsers
    nmeaParser = sempInitParser(nmeaParserTable, nmeaParserCount,
                                nmeaParserNames, nmeaParserNameCount,
                                0, BUFFER_LENGTH, nmeaMessage, "NMEA_Parser");
    if (!nmeaParser)
        reportFatalError("Failed to initialize the NMEA parser");
    ubloxParser = sempInitParser(ubloxParserTable, ubloxParserCount,
                                 ubloxParserNames, ubloxParserNameCount,
                                 0, BUFFER_LENGTH, ubloxMessage, "U-Blox_Parser");
    if (!ubloxParser)
        reportFatalError("Failed to initialize the U-Blox parser");

    // Obtain a raw data stream from somewhere
    rawDataBytes = 0;
    for (dataIndex = 0; dataIndex < DATA_STREAM_ENTRIES; dataIndex++)
        rawDataBytes += dataStream[dataIndex].length;
    Serial.printf("Raw data stream: %d bytes\r\n", rawDataBytes);

    // The raw data stream is passed to the parser one byte at a time
    sempSetPrintDebug(nmeaParser, &Serial);
    sempSetPrintDebug(ubloxParser, &Serial);
    for (dataIndex = 0; dataIndex < DATA_STREAM_ENTRIES; dataIndex++)
    {
        for (dataOffset = 0; dataOffset < dataStream[dataIndex].length; dataOffset++)
        {
            // Update the parser state based on the incoming byte
            sempParseNextByte(nmeaParser, dataStream[dataIndex].data[dataOffset]);
            sempParseNextByte(ubloxParser, dataStream[dataIndex].data[dataOffset]);
        }
    }

    // Done parsing the data
    sempShutdownParser(&nmeaParser);
    sempShutdownParser(&ubloxParser);
    free(nmeaParser);
    free(ubloxParser);
}

// Main loop processing after system is initialized
void loop()
{
}

// Call back from within parser, for end of message
// Process a complete message incoming from parser
void nmeaMessage(SEMP_PARSE_STATE *parse, uint8_t type)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;
    static bool displayOnce = true;
    uint32_t offset;

    // Display the raw message
    Serial.println();
    offset = dataOffset + 1 + 3 - parse->length;
    Serial.printf("Valid NMEA Message: %s, %d bytes at 0x%08x (%d)\r\n",
                  scratchPad->nmea.messageName, parse->length, offset, offset);
    dumpBuffer(parse->buffer, parse->length);

    // Display the parser state
    if (displayOnce)
    {
        displayOnce = false;
        Serial.println();
        sempPrintParserConfiguration(parse);
    }
}

// Call back from within parser, for end of message
// Process a complete message incoming from parser
void ubloxMessage(SEMP_PARSE_STATE *parse, uint8_t type)
{
    static bool displayOnce = true;
    uint32_t offset;

    // Display the raw message
    Serial.println();
    offset = dataOffset + 1 - parse->length;
    Serial.printf("Valid u-blox message: %d bytes at 0x%08x (%d)\r\n",
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
