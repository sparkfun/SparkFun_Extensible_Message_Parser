/*
  Justify_Test.ino

  Display two boxes and output justified text within the boxes.  If the
  box is not distorted then the justification is successful and if either
  box distorts then the test fails!

  License: MIT. Please see LICENSE.md for more details
*/

#include <SparkFun_Extensible_Message_Parser.h> //http://librarymanager/All#SparkFun_Extensible_Message_Parser

//----------------------------------------
// Output a dashed line for the bounding box
//----------------------------------------
void outputLine(int fieldWidth, char corner)
{
    output(corner);
    while (fieldWidth--)
        output('-');
    sempPrintCharLn(output, corner);
}

//----------------------------------------
// Justify values and strings within an 11 character box
//----------------------------------------
void justifyTest(int fieldWidth)
{
    // Right justification
    outputLine(fieldWidth, '.');

    output('|');
    sempPrintDecimalI32(output, 0, fieldWidth);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintDecimalI32(output, 1, fieldWidth);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintDecimalI32(output, -1, fieldWidth);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintDecimalI32(output, 0x7fffffff, fieldWidth);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintDecimalI32(output, (int32_t)0x80000000, fieldWidth);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintHex02x(output, 0xff, fieldWidth);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintHex04x(output, 0xffff, fieldWidth);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintHex08x(output, 0xffffffff, fieldWidth);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintHex0x02x(output, 0xff, fieldWidth);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintHex0x04x(output, 0xffff, fieldWidth);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintHex0x08x(output, 0xffffffff, fieldWidth);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintString(output, "Test", fieldWidth);
    sempPrintCharLn(output, '|');

    // 64-bit justification
    if (fieldWidth > 11)
    {
        // Right justification
        output('|');
        sempPrintDecimalI64(output, 0, fieldWidth);
        sempPrintCharLn(output, '|');

        output('|');
        sempPrintDecimalI64(output, 1, fieldWidth);
        sempPrintCharLn(output, '|');

        output('|');
        sempPrintDecimalI64(output, -1, fieldWidth);
        sempPrintCharLn(output, '|');

        output('|');
        sempPrintDecimalI64(output, 0x7fffffffffffffff, fieldWidth);
        sempPrintCharLn(output, '|');

        output('|');
        sempPrintDecimalI64(output, (int64_t)0x8000000000000000ull, fieldWidth);
        sempPrintCharLn(output, '|');

        output('|');
        sempPrintHex016x(output, 0xffffffffffffffffull, fieldWidth);
        sempPrintCharLn(output, '|');

        output('|');
        sempPrintHex0x016x(output, 0xffffffffffffffffull, fieldWidth);
        sempPrintCharLn(output, '|');

        output('|');
        sempPrintString(output, "This is a test", fieldWidth);
        sempPrintCharLn(output, '|');
    }

    // Left justification
    output('|');
    sempPrintDecimalI32(output, 0, -fieldWidth);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintDecimalI32(output, 1, -fieldWidth);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintDecimalI32(output, -1, -fieldWidth);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintDecimalI32(output, 0x7fffffff, -fieldWidth);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintDecimalI32(output, (int32_t)0x80000000, -fieldWidth);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintHex02x(output, 0xff, -fieldWidth);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintHex04x(output, 0xffff, -fieldWidth);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintHex08x(output, 0xffffffff, -fieldWidth);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintHex0x02x(output, 0xff, -fieldWidth);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintHex0x04x(output, 0xffff, -fieldWidth);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintHex0x08x(output, 0xffffffff, -fieldWidth);
    sempPrintCharLn(output, '|');

    output('|');
    sempPrintString(output, "Test", -fieldWidth);
    sempPrintCharLn(output, '|');

    // 64-bit justification
    if (fieldWidth > 11)
    {
        // Left justification
        output('|');
        sempPrintDecimalI64(output, 0, -fieldWidth);
        sempPrintCharLn(output, '|');

        output('|');
        sempPrintDecimalI64(output, 1, -fieldWidth);
        sempPrintCharLn(output, '|');

        output('|');
        sempPrintDecimalI64(output, -1, -fieldWidth);
        sempPrintCharLn(output, '|');

        output('|');
        sempPrintDecimalI64(output, 0x7fffffffffffffffull, -fieldWidth);
        sempPrintCharLn(output, '|');

        output('|');
        sempPrintDecimalI64(output, (int64_t)0x8000000000000000ull, -fieldWidth);
        sempPrintCharLn(output, '|');

        output('|');
        sempPrintDecimalU64(output, 0xffffffffffffffffull, -fieldWidth);
        sempPrintCharLn(output, '|');

        output('|');
        sempPrintHex016x(output, 0xffffffffffffffffull, -fieldWidth);
        sempPrintCharLn(output, '|');

        output('|');
        sempPrintHex0x016x(output, 0xffffffffffffffffull, -fieldWidth);
        sempPrintCharLn(output, '|');

        output('|');
        sempPrintString(output, "This is a test", -fieldWidth);
        sempPrintCharLn(output, '|');
    }
    outputLine(fieldWidth, '\'');
}

//----------------------------------------
// Test the justification routines
//----------------------------------------
void justifyTests()
{
    justifyTest(11);
    justifyTest(20);
}
