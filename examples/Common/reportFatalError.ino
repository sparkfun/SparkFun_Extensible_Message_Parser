/*
  reportFatalError.ino

  Display a the HALTED message every 15 seconds

  License: MIT. Please see LICENSE.md for more details
*/

//----------------------------------------
// Print the error message every 15 seconds
//
// Inputs:
//   errorMsg: Zero-terminated error message string to output every 15 seconds
//----------------------------------------
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
