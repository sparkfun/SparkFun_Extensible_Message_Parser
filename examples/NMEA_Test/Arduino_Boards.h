/*
  Arduino_Boards.h

  Constants and routines for Arduino platforms

  License: MIT. Please see LICENSE.md for more details
*/

#if defined(ARDUINO_ARCH_AVR)
// ARDUINO_AVR_UNO

//----------------------------------------
// Chip specific routines
//----------------------------------------

void initUart()
{
    Serial.begin(115200);

    // Delay to let the hardware initialize
    delay(1000);
}

//----------------------------------------
// Touch the watch dog timer to prevent a reboot
//----------------------------------------
#define petWDT()

//----------------------------------------
// Output a buffer of data
//----------------------------------------
void output(uint8_t * buffer, size_t length)
{
    size_t bytesWritten;

    if (Serial)
    {
        while (length)
        {
            // Wait until space is available in the FIFO
            while (Serial.availableForWrite() == 0);

            // Output the character
            bytesWritten = Serial.write(buffer, length);
            buffer += bytesWritten;
            length -= bytesWritten;
        }
    }
}

//----------------------------------------
// Delay for a while
//
// Inputs:
//   seconds: The number of seconds to delay
//----------------------------------------
void sleep(int seconds)
{
    int count;

    // Determine how many 100mSec intervals are in the specified seconds
    count = seconds * 10;
    while (count-- > 0)
    {
        petWDT();
        delay(100);
    }
}

#endif  // ARDUINO_ARCH_AVR
