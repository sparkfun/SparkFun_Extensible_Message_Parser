/*
  SparkFun mixed parser example sketch

  This example demonstrates how to create a mixed parser -
  here we parse u-blox UBX and NMEA using a single mixed parser

  A mixed parser lists multiple parsers in the parserTable.  This
  configuration works when the data stream contains a mix of complete
  messages.  For cases where protocol2 messages are embedded in the
  payload of protocol1, use the design of the SBF_in_SPARTN example and
  invoke the protocol2 parser in the processMessage routine.

  License: MIT. Please see LICENSE.md for more details
*/

#include <SparkFun_Extensible_Message_Parser.h> //http://librarymanager/All#SparkFun_Extensible_Message_Parser

#include "../Common/output.ino"
#include "../Common/dumpBuffer.ino"
#include "../Common/reportFatalError.ino"

//----------------------------------------
// Constants
//----------------------------------------

// Define the indexes into the parserTable array
#define NMEA_PARSER_INDEX       0
#define UBLOX_PARSER_INDEX      1

// Build the table listing all of the parsers
SEMP_PARSER_DESCRIPTION * parserTable[] =
{
    &sempNmeaParserDescription,
    &sempUbloxParserDescription,
};
const int parserCount = sizeof(parserTable) / sizeof(parserTable[0]);

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

#define DATA_STREAM_INIT(x, extraBytes)     {sizeof(x) - extraBytes, &x[0]}
const DataStream dataStream[] =
{
    DATA_STREAM_INIT(nmea_1, 1),
    DATA_STREAM_INIT(ublox_1, 0),
    DATA_STREAM_INIT(nmea_2, 1),
    DATA_STREAM_INIT(ublox_2, 0),
    DATA_STREAM_INIT(nmea_3, 1),
    DATA_STREAM_INIT(ublox_3, 0),
    DATA_STREAM_INIT(nmea_4, 1),
    DATA_STREAM_INIT(ublox_4, 0)
};

#define DATA_STREAM_ENTRIES     (sizeof(dataStream) / sizeof(dataStream[0]))

//----------------------------------------
// Locals
//----------------------------------------

uint8_t buffer[3096];
int byteOffset;
int dataIndex;
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
    int rawDataBytes;

    delay(1000);

    Serial.begin(115200);
    sempPrintLn(output);
    sempPrintStringLn(output, "Mixed_Parser example sketch");
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
    parse = sempBeginParser("Mixed_Parser", parserTable, parserCount,
                            buffer, bufferLength, processMessage, output);
    if (!parse)
        reportFatalError("Failed to initialize the parser");

    // Obtain a raw data stream from somewhere
    rawDataBytes = 0;
    for (dataIndex = 0; dataIndex < DATA_STREAM_ENTRIES; dataIndex++)
        rawDataBytes += dataStream[dataIndex].length;
    sempPrintString(output, "Raw data stream: ");
    sempPrintDecimalI32(output,rawDataBytes);
    sempPrintStringLn(output, " bytes");

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
    uint32_t byteIndex;
    uint32_t offset;

    // Display the raw message
    sempPrintLn(output);
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
            sempPrintString(output, "Valid NMEA Sentence: ");
            sempPrintString(output,sempNmeaGetSentenceName(parse));
            sempPrintString(output, ", ");
            sempPrintDecimalI32(output, parse->length);
            sempPrintString(output, " bytes at ");
            sempPrintHex0x08x(output, offset);
            sempPrintString(output, " (");
            sempPrintDecimalI32Ln(output, offset);
            break;

        case UBLOX_PARSER_INDEX:
            offset = dataOffset + 1 - parse->length;
            sempPrintString(output, "Valid u-blox message: ");
            sempPrintDecimalI32(output, parse->length);
            sempPrintString(output, " bytes at ");
            sempPrintHex0x08x(output, offset);
            sempPrintString(output, " (");
            sempPrintDecimalI32Ln(output, offset);
            break;
    }
    dumpBuffer(parse->buffer, parse->length);

    // Display the parser state
    if (displayOnce)
    {
        displayOnce = false;
        sempPrintLn(output);
        sempPrintParserConfiguration(parse, &Serial);
    }
}
