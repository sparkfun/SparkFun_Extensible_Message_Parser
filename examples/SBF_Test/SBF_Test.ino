/*
  SparkFun SBF test example sketch

  This example demonstrates how to parse a Septentrio SBF data stream

  License: MIT. Please see LICENSE.md for more details
*/

#include <SparkFun_Extensible_Message_Parser.h> //http://librarymanager/All#SparkFun_Extensible_Message_Parser

#include "../Common/output.ino"
#include "../Common/dumpBuffer.ino"
#include "../Common/reportFatalError.ino"

//----------------------------------------
// Constants
//----------------------------------------

// Build the table listing all of the parsers
SEMP_PARSER_DESCRIPTION * parserTable[] =
{
    &sempSbfParserDescription
};
const int parserCount = sizeof(parserTable) / sizeof(parserTable[0]);

const char * const parserNames[] =
{
    "SBF parser"
};
const int parserNameCount = sizeof(parserNames) / sizeof(parserNames[0]);

// Provide some valid and invalid SBF messages
const uint8_t rawDataStream[] =
{
    // Invalid data - must skip over
    0, 1, 2, 3, 4, 5, 6, 7,                         //     0

    // SBF Block 5914 (ReceiverTime)
    0x24, 0x40, 0x06, 0xFE, 0x1A, 0x17, 0x18, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x12, 0x00, 0x00, 0x00,

    // SBF Block 5914 (ReceiverTime) - invalid length
    0x24, 0x40, 0x06, 0xFE, 0x1A, 0x17, 0x17, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x12, 0x00, 0x00, 0x00,

    // SBF Block 5914 (ReceiverTime) - invalid CRC
    0x24, 0x40, 0x06, 0xFE, 0x1A, 0x17, 0x18, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x80, 0x80,
    0xFF, 0x80, 0x80, 0x80, 0x12, 0x00, 0x00, 0x00,

    // Invalid data - must skip over
    0, 1, 2, 3, 4, 5, 6, 7,                         //     0

    // SBF Block 4007 (PVTGeodetic)
    // Stonehenge : 0.8932412891, -0.03187402625 radians (51.17895595, -1.82624718 degrees)
    0x24, 0x40, 0xC3, 0xF2, 0xa7, 0x4f, 0x60, 0x00, 0x80, 0x43, 0x6c, 0x1b, 0x63, 0x09, 0x03, 0x00,
    0x5b, 0xe2, 0x83, 0xc1, 0x6e, 0x95, 0xec, 0x3f, 0x2d, 0xd3, 0xab, 0xd8, 0xca, 0x51, 0xa0, 0xbf,
    0x98, 0x52, 0x34, 0xb8, 0xba, 0x79, 0x63, 0x40, 0x21, 0x72, 0x3e, 0x42, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf9, 0x02, 0x95, 0xd0, 0x46, 0x2d, 0x48, 0xd5,
    0xdb, 0xce, 0xad, 0x3f, 0x07, 0x14, 0xab, 0x3d, 0x00, 0x00, 0x26, 0x00, 0xff, 0xff, 0xff, 0xff,
    0x01, 0x01, 0x22, 0x10, 0x01, 0x00, 0x00, 0x00, 0x34, 0x00, 0xff, 0xff, 0xff, 0xff, 0x60, 0x00,

};

// Number of bytes in the rawDataStream
#define RAW_DATA_BYTES      (sizeof(rawDataStream) / sizeof(rawDataStream[0]))

//----------------------------------------
// Locals
//----------------------------------------

bool runTests;

//----------------------------------------
// Application entry point used to initialize the system
//----------------------------------------
void setup()
{
    initUart();
    sempPrintLn(output);
    sempPrintStringLn(output, "SBF_Test example sketch");
    sempPrintLn(output);

    // Run the tests
    runTests = true;
}

//----------------------------------------
// Main loop processing, repeatedly called after system is initialized by setup
//----------------------------------------
void loop()
{
    // Keep the system running
    petWDT();

    // Determine if a character was input
    if (Serial)
    {
        sempPrintString(output, "TOW: ");
        sempPrintDecimalI32Ln(output, sempSbfGetU4(parse, 8));
        sempPrintString(output, "Mode: ");
        sempPrintDecimalI32Ln(output, sempSbfGetU1(parse, 14));
        sempPrintString(output, "Error: ");
        sempPrintDecimalI32Ln(output, sempSbfGetU1(parse, 15));
        Serial.printf("Latitude: %.8g\r\n", sempSbfGetF8(parse, 16) * 180.0 / PI);
        Serial.printf("Longitude: %.8g\r\n", sempSbfGetF8(parse, 24) * 180.0 / PI);
    }

    // Run the tests when requested
    if (runTests)
    {
        runTests = false;

        // Run the tests
        parserTests();
        sempPrintStringLn(output, "All done");
    }
}
