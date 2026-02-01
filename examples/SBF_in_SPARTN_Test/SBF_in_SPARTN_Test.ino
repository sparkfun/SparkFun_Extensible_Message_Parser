/*
  SparkFun SBF-in-SPARTN test example sketch

  The Septentrio mosaic-X5 can output raw L-Band (LBandBeam1) data, interspersed with SBF messages

  A dual parser is required when two protocols are mixed in the raw data
  stream.  At least one of the protocols must have contigious elements
  within the raw data stream.  The elements may be messages, packets or
  tag-length-value tuples so that the invalid data is easily identified.

  This example uses two parsers to separate SBF from SPARTN in the raw
  L-Band data stream.  The invalid data callback is specified for the
  outer layer SBF parser and calls the SPARTN parser with the invalid data.

  License: MIT. Please see LICENSE.md for more details
*/

#include <SparkFun_Extensible_Message_Parser.h> //http://librarymanager/All#SparkFun_Extensible_Message_Parser
#include "ESP32.h"
#include "SAMD21.h"

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
    sempPrintStringLn(output, "SBF_in_SPARTN_Test example sketch");
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
