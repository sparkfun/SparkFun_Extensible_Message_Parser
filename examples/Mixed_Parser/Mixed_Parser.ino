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
    sempPrintStringLn(output, "Mixed_Parser example sketch");
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
