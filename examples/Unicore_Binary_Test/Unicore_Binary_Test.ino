/*
  SparkFun Unicore binary test example sketch

  This example demonstrates how to parse a Unicore Binary data stream

  License: MIT. Please see LICENSE.md for more details
*/

#include <SparkFun_Extensible_Message_Parser.h> //http://librarymanager/All#SparkFun_Extensible_Message_Parser
#include "ESP32.h"

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
    sempPrintStringLn(output, "Unicore_Test example sketch");
    sempPrintLn(output);

    // Run the tests
    runTests = true;
}

//----------------------------------------
// Main loop processing, repeatedly called after system is initialized by setup
//----------------------------------------
void loop()
{
    // Determine if a character was input
    if (Serial)
    {
        if (Serial.available())
        {
            Serial.read();

            // If so, run the tests again
            runTests = true;
        }
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
