/*
  SAMD21.h

  Constants and routines for SAMD platforms

  License: MIT. Please see LICENSE.md for more details
*/

#if defined(ARDUINO_SAMD_ZERO)

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Includes
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "SAMDTimerInterrupt.h" //http://librarymanager/All#SAMD_TimerInterrupt v1.10.1 by Koi Hang
#include <WDTZero.h> //https://github.com/javos65/WDTZero

//----------------------------------------
// Constants
//----------------------------------------

const int baudRate = 115200;
const int pinCTS = 30;
const int pinRTS = 38;

//----------------------------------------
// Locals
//----------------------------------------

WDTZero myWatchDog;

//----------------------------------------
// Chip specific routines
//----------------------------------------
void initUart()
{
    // Start watchdog timer
    myWatchDog.setup(WDT_HARDCYCLE2S);  // Initialize WDT with 2s timeout

    // Start the serial port
    Serial.begin(baudRate);

    // Flow control
    pinMode(pinCTS, INPUT_PULLUP);
    pinMode(pinRTS, OUTPUT);
    digitalWrite(pinRTS, 1);

    // Delay to let the hardware initialize
    delay(1000);

    // Wait for USB serial to become available
    while (Serial == false);
}

//----------------------------------------
// Touch the watch dog timer to prevent a reboot
//----------------------------------------
void petWDT()
{
    static uint32_t lastPet;

    // Petting the dog takes a long time (~4.5ms on SAMD21) so it's only
    // done just before the 2 seccond limit
    if ((millis() - lastPet) > 1800)
    {
        lastPet = millis();

        // This takes 4-5ms to complete
        myWatchDog.clear();
    }
}

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
            petWDT();
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

#endif  // ARDUINO_SAMD_ZERO
