/*
  SparkFun Unicore Hash (#) test example sketch

  License: MIT. Please see LICENSE.md for more details
*/

#include <SparkFun_Extensible_Message_Parser.h> //http://librarymanager/All#SparkFun_Extensible_Message_Parser

//----------------------------------------
// Constants
//----------------------------------------

// Build the table listing all of the parsers
SEMP_PARSE_ROUTINE const parserTable[] =
{
    sempUnicoreHashPreamble
};
const int parserCount = sizeof(parserTable) / sizeof(parserTable[0]);

const char * const parserNames[] =
{
    "Unicore hash parser"
};
const int parserNameCount = sizeof(parserNames) / sizeof(parserNames[0]);


// Provide some valid and invalid Unicore hash (#) sentences
const uint8_t rawDataStream[] =
{
    // Valid Unicore hash (#) sentences, with or without CR or LF
    "#MODE,0,GPS,UNKNOWN,0,0,0,0,0,653;MODE ROVER SURVEY,*66"      //   0
    "#MODE,0,GPS,UNKNOWN,0,0,0,0,0,653;MODE ROVER SURVEY,*66\r"    //  55
    "#MODE,0,GPS,UNKNOWN,0,0,0,0,0,653;MODE ROVER SURVEY,*66\n"    // 111

    // Invalid data, must skip over
    "abcdefg"                                                      // 167

    // Valid Unicore hash (#) sentences
    //        1         2         3         4         5         6         7         8
    //2345678901234567890123456789012345678901234567890123456789012345678901234567890
    "#MODE,0,GPS,UNKNOWN,0,0,0,0,0,653;MODE ROVER SURVEY,*66\r\n"  // 174

    // Valid Unicore hash (#) sentence with an 8 byte checksum
    "#VERSION,40,GPS,UNKNOWN,1,1000,0,0,18,15;UM980,R4.10Build7923,HRPT00-S10C-P,2310415000001-MD22B1225023842,ff3b1e9611b3b07b,2022/09/28*b164c965\r\n"

    // Long valid name
    //123456789012345
    "#ABCDEFGHIJKLMNO,210230,A,3855.4487,N,09446.0071,W,0.0,076.2,1,00*15\r\n"  // 231

    // Name too long
    //1234567890123456
    "#ABCDEFGHIJKLMNOP,210230,A,3855.4487,N,09446.0071,W,0.0,076.2,130*6A\r\n"  // 301

    // Invalid character in the sentence name
    "#M@DE,0,GPS,UNKNOWN,0,0,0,0,0,653;MODE ROVER SURVEY,*66\r\n"  //

    // Bad checksum
    "#MODE,0,GPS,UNKNOWN,0,0,0,0,0,653;MODE ROVER SURVEY,*67\r\n"  //

    // Sentence too long by a single byte
    "#VERSION,40,GPS,UNKNOWN,1,1000,0,0,18,15;UM980,R4.10Build7923,HRPT00-S10C-P,2310415000001-MD22B1225023842,ff3b1e9611b3b07b,2022/09/28.*e1bffcd1\r\n"

    // Sentence too long by a two bytes
    "#VERSION,40,GPS,UNKNOWN,1,1000,0,0,18,15;UM980,R4.10Build7923,HRPT00-S10C-P,2310415000001-MD22B1225023842,ff3b1e9611b3b07b,2022/09/28..*2de35071\r\n"

    // Sentence too long by a three bytes
    "#VERSION,40,GPS,UNKNOWN,1,1000,0,0,18,15;UM980,R4.10Build7923,HRPT00-S10C-P,2310415000001-MD22B1225023842,ff3b1e9611b3b07b,2022/09/28...*fbf9af35\r\n"
};

// Number of bytes in the rawDataStream
#define RAW_DATA_BYTES      sizeof(rawDataStream)

// Account for the largest Unicore hash (#) sentence + zero termination
#define BUFFER_LENGTH   144 + 1

//----------------------------------------
// Locals
//----------------------------------------

uint32_t dataOffset;
SEMP_PARSE_STATE *parse;

//----------------------------------------
// Test routine
//----------------------------------------

// Initialize the system
void setup()
{
    delay(1000);

    Serial.begin(115200);
    Serial.println();
    Serial.println("Unicore hash (#)_Test_example sketch");
    Serial.println();

    // Initialize the parser
    parse = sempBeginParser(parserTable, parserCount,
                            parserNames, parserNameCount,
                            0, BUFFER_LENGTH, processMessage, "Unicore_Hash_Test",
                            &Serial, nullptr, badUnicoreHashChecksum);
    if (!parse)
        reportFatalError("Failed to initialize the parser");

    // Obtain a raw data stream from somewhere
    Serial.printf("Raw data stream: %d bytes\r\n", RAW_DATA_BYTES);

    // The raw data stream is passed to the parser one byte at a time
    sempEnableDebugOutput(parse);
    for (dataOffset = 0; dataOffset < RAW_DATA_BYTES; dataOffset++)
        // Update the parser state based on the incoming byte
        sempParseNextByte(parse, rawDataStream[dataOffset]);

    // Done parsing the data
    sempStopParser(&parse);
}

// Main loop processing after system is initialized
void loop()
{
}

// Handle the bad checksum calculation
// See Unicore NebulasIV, High Precision Products, HIGH PRECISION
// COMMANDS AND LOGS Manual
// Section 7.3, page 112
bool badUnicoreHashChecksum(SEMP_PARSE_STATE *parse)
{
    int alternateChecksum;
    bool badChecksum;
    int checksum;

    // Older UM980 firmware during setup is improperly adding the first
    // character into the checksum calculation.  Convert the received
    // checksum characters into binary.
    checksum = sempAsciiToNibble(parse->buffer[parse->length - 1]);
    checksum |= sempAsciiToNibble(parse->buffer[parse->length - 2]) << 4;

    // Determine if the checksum also includes the first character
    alternateChecksum = parse->crc ^ parse->buffer[0];
    badChecksum = (alternateChecksum != checksum);

    // Display bad checksums
    if (!badChecksum)
    {
        Serial.printf("UM980: Message improperly includes %c in checksum\r\n", parse->buffer[0]);
        dumpBuffer(parse->buffer, parse->length);
    }
    return badChecksum;
}

// Call back from within parser, for end of message
// Process a complete message incoming from parser
void processMessage(SEMP_PARSE_STATE *parse, uint16_t type)
{
    uint32_t offset;

    // Determine the raw data stream offset
    offset = dataOffset + 1 + 2 - parse->length;
    while (rawDataStream[offset] != '#')
        offset -= 1;

    // Display the raw message
    Serial.println();
    Serial.printf("Valid Unicore Hash (#) Sentence: %s, %d bytes at 0x%08x (%d)\r\n",
              sempUnicoreHashGetSentenceName(parse), parse->length, offset, offset);
    dumpBuffer(parse->buffer, parse->length);
}

// Display the contents of a buffer
void dumpBuffer(const uint8_t *buffer, uint16_t length)
{
    int bytes;
    const uint8_t *end;
    int index;
    uint16_t offset;

    end = &buffer[length];
    offset = 0;
    while (buffer < end)
    {
        // Determine the number of bytes to display on the line
        bytes = end - buffer;
        if (bytes > (16 - (offset & 0xf)))
            bytes = 16 - (offset & 0xf);

        // Display the offset
        Serial.printf("0x%08lx: ", offset);

        // Skip leading bytes
        for (index = 0; index < (offset & 0xf); index++)
            Serial.printf("   ");

        // Display the data bytes
        for (index = 0; index < bytes; index++)
            Serial.printf("%02x ", buffer[index]);

        // Separate the data bytes from the ASCII
        for (; index < (16 - (offset & 0xf)); index++)
            Serial.printf("   ");
        Serial.printf(" ");

        // Skip leading bytes
        for (index = 0; index < (offset & 0xf); index++)
            Serial.printf(" ");

        // Display the ASCII values
        for (index = 0; index < bytes; index++)
            Serial.printf("%c", ((buffer[index] < ' ') || (buffer[index] >= 0x7f)) ? '.' : buffer[index]);
        Serial.printf("\r\n");

        // Set the next line of data
        buffer += bytes;
        offset += bytes;
    }
}

// Print the error message every 15 seconds
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
