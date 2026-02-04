/*
  Access_Tests.ino

  Test access for different field sizes

  License: MIT. Please see LICENSE.md for more details
*/

#include <SparkFun_Extensible_Message_Parser.h> //http://librarymanager/All#SparkFun_Extensible_Message_Parser

extern uint8_t bufferArea[];

//----------------------------------------
// Return the 8-bit value at the beginning of the buffer
//----------------------------------------
uint8_t access_8_bits(uint8_t * buffer)
{
    sempPrintStringLn(output, "access_8_bits");
    return *buffer;
}

//----------------------------------------
// Return the 16-bit value at the beginning of the buffer
//----------------------------------------
uint16_t access_16_bits(uint8_t * buffer)
{
    sempPrintStringLn(output, "access_16_bits");
    return *(uint16_t *)buffer;
}

//----------------------------------------
// Return the 32-bit value at the beginning of the buffer
//----------------------------------------
uint32_t access_32_bits(uint8_t * buffer)
{
    sempPrintStringLn(output, "access_32_bits");
    return *(uint32_t *)buffer;
}

//----------------------------------------
// Return the 64-bit value at the beginning of the buffer
//----------------------------------------
uint64_t access_64_bits(uint8_t * buffer)
{
    sempPrintStringLn(output, "access_64_bits");
    return *(uint64_t *)buffer;
}

//----------------------------------------
// Test the justification routines
//----------------------------------------
void accessTests(uint8_t * buffer)
{
    access_8_bits(buffer);
    access_16_bits(buffer);
    access_32_bits(buffer);
    access_64_bits(buffer);
}
