/*
  SAMD51.h

  Constants and routines for SAMD51 platforms

  License: MIT. Please see LICENSE.md for more details
*/

#if defined(ARDUINO_SAMD51_THING_PLUS) || defined(ARDUINO_SAMD51_MICROMOD)

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Includes
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//----------------------------------------
// Constants
//----------------------------------------

const int baudRate = 115200;

//----------------------------------------
// Chip specific routines
//----------------------------------------
void initUart()
{
    // Start watchdog timer
//    myWatchDog.setup(WDT_HARDCYCLE2S);  // Initialize WDT with 2s timeout

    // Start the serial port
    Serial.begin(baudRate);

    // Delay to let the hardware initialize
    delay(1000);

    // Wait for USB serial to become available
    while (Serial == false);
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
            while (Serial.availableForWrite() == 0)
                petWDT();

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

#endif  // ARDUINO_SAMD51_THING_PLUS
