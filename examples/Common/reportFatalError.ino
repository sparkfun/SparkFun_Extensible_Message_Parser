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
        sempPrintString(output, "HALTED: ");
        sempPrintStringLn(output, errorMsg);
        delay(15 * 1000);   // sleep(15);
    }
}
