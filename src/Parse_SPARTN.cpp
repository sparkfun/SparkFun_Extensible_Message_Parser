/*------------------------------------------------------------------------------
Parse_SPARTN.cpp

SPARTN message parsing support routines

The parser routines within a parser module are typically placed in
reverse order within the module.  This lets the routine declaration
proceed the routine use and eliminates the need for forward declaration.
Removing the forward declaration helps reduce the exposure of the
routines to the application layer.  As such only the preamble routine
should need to be listed in SparkFun_Extensible_Message_Parser.h.

License: MIT. Please see LICENSE.md for more details
------------------------------------------------------------------------------*/

#include <stdio.h>
#include "SparkFun_Extensible_Message_Parser.h"
#include "semp_crc_spartn.h" // 4/8/16/24/32-bit cyclic redundancy checksums for SPARTN parsing

//----------------------------------------
// SPARTN parse routines
//----------------------------------------

bool sempSpartnReadTF018(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;

    scratchPad->spartn.frameCount++;
    if (scratchPad->spartn.frameCount == scratchPad->spartn.crcBytes)
    {
        uint16_t numBytes = 4 + scratchPad->spartn.TF007toTF016 + scratchPad->spartn.payloadLength + scratchPad->spartn.embeddedApplicationLengthBytes;
        uint8_t *ptr = &parse->buffer[numBytes];
        bool valid;
        switch (scratchPad->spartn.crcType)
        {
        case 0:
        {
            uint8_t expected = *ptr;
            // Don't include the preamble in the CRC
            valid =  ((semp_uSpartnCrc8(&parse->buffer[1], numBytes - 1) == expected)
                      || (parse->badCrc && (!parse->badCrc(parse))));
        }
        break;
        case 1:
        {
            uint16_t expected = *ptr++;
            expected <<= 8;
            expected |= *ptr;
            // Don't include the preamble in the CRC
            valid =  ((semp_uSpartnCrc16(&parse->buffer[1], numBytes - 1) == expected)
                      || (parse->badCrc && (!parse->badCrc(parse))));
        }
        break;
        case 2:
        {
            uint32_t expected = *ptr++;
            expected <<= 8;
            expected |= *ptr++;
            expected <<= 8;
            expected |= *ptr;
            // Don't include the preamble in the CRC
            valid =  ((semp_uSpartnCrc24(&parse->buffer[1], numBytes - 1) == expected)
                      || (parse->badCrc && (!parse->badCrc(parse))));
        }
        break;
        default:
        {
            uint32_t expected = *ptr++;
            expected <<= 8;
            expected |= *ptr++;
            expected <<= 8;
            expected |= *ptr++;
            expected <<= 8;
            expected |= *ptr;
            // Don't include the preamble in the CRC
            valid =  ((semp_uSpartnCrc32(&parse->buffer[1], numBytes - 1) == expected)
                      || (parse->badCrc && (!parse->badCrc(parse))));
        }
        break;
        }
        if (valid)
            parse->eomCallback(parse, parse->type); // Pass parser array index
        else
            sempPrintf(parse->printDebug,
                    "SEMP: %s SPARTN %d %d, 0x%04x (%d) bytes, bad CRC",
                    parse->parserName,
                    scratchPad->spartn.messageType,
                    scratchPad->spartn.messageSubtype,
                    parse->length, parse->length);
        parse->state = sempFirstByte;
        return false;
    }

    return true;
}

bool sempSpartnReadTF017(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;

    scratchPad->spartn.frameCount++;
    if (scratchPad->spartn.frameCount == scratchPad->spartn.embeddedApplicationLengthBytes)
    {
        parse->state = sempSpartnReadTF018;
        scratchPad->spartn.frameCount = 0;
    }

    return true;
}

bool sempSpartnReadTF016(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;

    scratchPad->spartn.frameCount++;
    if (scratchPad->spartn.frameCount == scratchPad->spartn.payloadLength)
    {
        if (scratchPad->spartn.embeddedApplicationLengthBytes > 0)
        {
            parse->state = sempSpartnReadTF017;
            scratchPad->spartn.frameCount = 0;
        }
        else
        {
            parse->state = sempSpartnReadTF018;
            scratchPad->spartn.frameCount = 0;
        }
    }

    return true;
}

bool sempSpartnReadTF009(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;

    scratchPad->spartn.frameCount++;
    if (scratchPad->spartn.frameCount == scratchPad->spartn.TF007toTF016)
    {
        if (scratchPad->spartn.EAF == 0)
        {
            scratchPad->spartn.authenticationIndicator = 0;
            scratchPad->spartn.embeddedApplicationLengthBytes = 0;
        }
        else
        {
            scratchPad->spartn.authenticationIndicator = (data >> 3) & 0x07;
            if (scratchPad->spartn.authenticationIndicator <= 1)
                scratchPad->spartn.embeddedApplicationLengthBytes = 0;
            else
            {
                switch (data & 0x07)
                {
                case 0:
                    scratchPad->spartn.embeddedApplicationLengthBytes = 8; // 64 bits
                    break;
                case 1:
                    scratchPad->spartn.embeddedApplicationLengthBytes = 12; // 96 bits
                    break;
                case 2:
                    scratchPad->spartn.embeddedApplicationLengthBytes = 16; // 128 bits
                    break;
                case 3:
                    scratchPad->spartn.embeddedApplicationLengthBytes = 32; // 256 bits
                    break;
                default:
                    scratchPad->spartn.embeddedApplicationLengthBytes = 64; // 512 / TBD bits
                    break;
                }
            }
        }
        scratchPad->spartn.frameCount = 0;
        parse->state = sempSpartnReadTF016;
    }

    return true;
}

bool sempSpartnReadTF007(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;

    scratchPad->spartn.messageSubtype = data >> 4;
    scratchPad->spartn.timeTagType = (data >> 3) & 0x01;
    if (scratchPad->spartn.timeTagType == 0)
        scratchPad->spartn.TF007toTF016 = 4;
    else
        scratchPad->spartn.TF007toTF016 = 6;
    if (scratchPad->spartn.EAF > 0)
        scratchPad->spartn.TF007toTF016 += 2;
    scratchPad->spartn.frameCount = 1;

    parse->state = sempSpartnReadTF009;

    return true;
}

bool sempSpartnReadTF002TF006(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;

    if (scratchPad->spartn.frameCount == 0)
    {
        scratchPad->spartn.messageType = data >> 1;
        scratchPad->spartn.payloadLength = data & 0x01;
    }
    if (scratchPad->spartn.frameCount == 1)
    {
        scratchPad->spartn.payloadLength <<= 8;
        scratchPad->spartn.payloadLength |= data;
    }
    if (scratchPad->spartn.frameCount == 2)
    {
        scratchPad->spartn.payloadLength <<= 1;
        scratchPad->spartn.payloadLength |= data >> 7;
        scratchPad->spartn.EAF = (data >> 6) & 0x01;
        scratchPad->spartn.crcType = (data >> 4) & 0x03;
        switch (scratchPad->spartn.crcType)
        {
        case 0:
            scratchPad->spartn.crcBytes = 1;
            break;
        case 1:
            scratchPad->spartn.crcBytes = 2;
            break;
        case 2:
            scratchPad->spartn.crcBytes = 3;
            break;
        default:
            scratchPad->spartn.crcBytes = 4;
            break;
        }
        scratchPad->spartn.frameCRC = data & 0x0F;
        parse->buffer[3] = parse->buffer[3] & 0xF0; // Zero the 4 LSBs before calculating the CRC
        if (semp_uSpartnCrc4(&parse->buffer[1], 3) == scratchPad->spartn.frameCRC)
        {
            parse->buffer[3] = data; // Restore TF005 and TF006 now we know the data is valid
            parse->state = sempSpartnReadTF007;
        }
        else
        {
            // Invalid header CRC
            parse->buffer[3] = data; // Restore the byte now we know the data is invalid
            parse->state = sempFirstByte;

            sempPrintf(parse->printDebug,
                    "SEMP: %s SPARTN %d, 0x%04x (%d) bytes, bad header CRC",
                    parse->parserName,
                    scratchPad->spartn.messageType,
                    parse->length, parse->length);

            return false;
        }
    }

    scratchPad->spartn.frameCount++;
    return true;
}

// Check for the preamble
bool sempSpartnPreamble(SEMP_PARSE_STATE *parse, uint8_t data)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;

    if (data == 0x73)
    {
        scratchPad->spartn.frameCount = 0;
        parse->state = sempSpartnReadTF002TF006;
        return true;
    }
    return false;
}

// Translates state value into an string, returns nullptr if not found
const char * sempSpartnGetStateName(const SEMP_PARSE_STATE *parse)
{
    if (parse->state == sempSpartnPreamble)
        return "sempSpartnPreamble";
    if (parse->state == sempSpartnReadTF002TF006)
        return "sempSpartnReadTF002TF006";
    if (parse->state == sempSpartnReadTF007)
        return "sempSpartnReadTF007";
    if (parse->state == sempSpartnReadTF009)
        return "sempSpartnReadTF009";
    if (parse->state == sempSpartnReadTF016)
        return "sempSpartnReadTF016";
    if (parse->state == sempSpartnReadTF017)
        return "sempSpartnReadTF017";
    if (parse->state == sempSpartnReadTF018)
        return "sempSpartnReadTF018";
    return nullptr;
}

// Get the message number
uint8_t sempSpartnGetMessageType(const SEMP_PARSE_STATE *parse)
{
    SEMP_SCRATCH_PAD *scratchPad = (SEMP_SCRATCH_PAD *)parse->scratchPad;
    return scratchPad->spartn.messageType;
}
