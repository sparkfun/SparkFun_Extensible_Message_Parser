/*
  output.ino

  Output a single character to the serial port

  License: MIT. Please see LICENSE.md for more details
*/

//----------------------------------------
// Output a character
//
// Inputs:
//   character: The character to output
//----------------------------------------
void output(char character)
{
    // Wait until space is available in the FIFO
    while (Serial.availableForWrite() == 0);

    // Output the character
    Serial.write(character);
}
