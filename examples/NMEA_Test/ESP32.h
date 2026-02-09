/*
  ESP32.h

  Constants and routines for ESP32 platforms

  License: MIT. Please see LICENSE.md for more details
*/

#if defined(ESP32)

//----------------------------------------
// Constants
//----------------------------------------

const int baudRate = 115200;

//----------------------------------------
// Chip specific routines
//----------------------------------------

void initUart()
{
    Serial.begin(baudRate);

    // Delay to let the hardware initialize
    delay(1000);
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
            while (Serial.availableForWrite() == 0);

            // Output the character
            bytesWritten = Serial.write(buffer, length);
            buffer += bytesWritten;
            length -= bytesWritten;
        }
    }
}

//----------------------------------------
// Touch the watch dog timer to prevent a reboot
//----------------------------------------
#define petWDT()

#endif  // ESP32
