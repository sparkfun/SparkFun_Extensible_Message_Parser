/*
  SparkFun SBF test example sketch

  This example demonstrates how to parse a Septentrio SBF data stream

  License: MIT. Please see LICENSE.md for more details
*/

#include <SparkFun_Extensible_Message_Parser.h> //http://librarymanager/All#SparkFun_Extensible_Message_Parser
#include "Arduino_Boards.h"
#include "ESP32.h"
#include "SAMD21.h"
#include "SAMD51.h"

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
