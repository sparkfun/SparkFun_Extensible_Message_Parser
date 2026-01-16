/*------------------------------------------------------------------------------
SparkFun_Extensible_Message_Parser.h

Constant and routine declarations for the extensible message parser.

License: MIT. Please see LICENSE.md for more details
------------------------------------------------------------------------------*/

#ifndef __SPARKFUN_EXTENSIBLE_MESSAGE_PARSER_H__
#define __SPARKFUN_EXTENSIBLE_MESSAGE_PARSER_H__

#include <Arduino.h>

//----------------------------------------
// Constants
//----------------------------------------

#define SEMP_MINIMUM_BUFFER_LENGTH      32

//----------------------------------------
// Externals
//----------------------------------------

extern const int unsigned semp_crc24qTable[256];
extern const unsigned long semp_crc32Table[256];
extern const uint8_t semp_u8Crc4Table[];
extern const uint8_t semp_u8Crc8Table[];
extern const uint16_t semp_u16Crc16Table[];
extern const uint32_t semp_u32Crc24Table[];
extern const uint32_t semp_u32Crc32Table[];

//----------------------------------------
// Types
//----------------------------------------

// Forward type declaration
typedef struct _SEMP_PARSE_STATE *P_SEMP_PARSE_STATE;

// Bad CRC callback
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//
// Outputs:
//   Returns true when the CRC calculation fails otherwise returns false
//   if the alternate CRC or checksum calculation is successful.
//
// Normally this routine pointer is set to nullptr and sempInvalidData is
// called.  The parser calls the badCrcCallback routine when the default
// CRC or checksum calculation fails.  This allows an upper layer to adjust
// the CRC calculation if necessary.  Alternatively the upper layer can call
// sempInvalidData.
typedef bool (*SEMP_BAD_CRC_CALLBACK)(P_SEMP_PARSE_STATE parse);

// CRC callback routine
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//   data: Next byte of data from the data strream
//
// Outputs:
//   Returns true if the parser accepted and processed the incoming data
//   and false if this data is for another parser
typedef uint32_t (*SEMP_COMPUTE_CRC)(P_SEMP_PARSE_STATE parse, uint8_t dataByte);

// End of message callback
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//   type: Index into parseTable designating the type of parser
//
// Call the application back at specified routine address.  Pass in the
// parse data structure containing the buffer containing the address of
// the message data and the length field containing the number of valid
// data bytes in the buffer.
typedef void (*SEMP_EOM_CALLBACK)(P_SEMP_PARSE_STATE parse, uint16_t type);

// Invalid data callback
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//
// Normally this routine pointer is set to nullptr.  When this routine is
// specified, it is called when ever the parser detects an invalid data
// stream.  All invalid data is passed to this routine and the parser
// goes back to scanning for the first preamble byte.
typedef void (*SEMP_INVALID_DATA_CALLBACK)(P_SEMP_PARSE_STATE parse);

// Parse routine
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//   data: Next byte of data from the data strream
//
// Outputs:
//   Returns true if the parser accepted and processed the incoming data
//   and false if this data is for another parser
typedef bool (*SEMP_PARSE_ROUTINE)(P_SEMP_PARSE_STATE parse, uint8_t data);

// Describe the parser
typedef const struct _SEMP_PARSER_DESCRIPTION
{
    const char * parserName;        // Name of the parser
    SEMP_PARSE_ROUTINE preamble;    // Routine to handle the preamble
    size_t scratchPadBytes;         // Required scratch pad size
    size_t payloadOffset;           // Offset to the first byte of the payload
} SEMP_PARSER_DESCRIPTION;

// Maintain the operating state of one or more parsers processing a raw
// data stream.
typedef struct _SEMP_PARSE_STATE
{
    SEMP_PARSER_DESCRIPTION **parsers; // Table of parsers
    SEMP_PARSE_ROUTINE state;      // Parser state routine
    SEMP_EOM_CALLBACK eomCallback; // End of message callback routine
    SEMP_BAD_CRC_CALLBACK badCrc;  // Bad CRC callback routine
    SEMP_COMPUTE_CRC computeCrc;   // Routine to compute the CRC when set
    const char *parserName;        // Name of parser table
    void *scratchPad;              // Parser scratchpad area
    Print *printError;             // Class to use for error output
    Print *printDebug;             // Class to use for debug output
    bool verboseDebug;             // Verbose debug output (default: false)
    uint32_t crc;                  // RTCM computed CRC
    uint8_t *buffer;               // Buffer containing the message
    size_t bufferLength;           // Length of the buffer in bytes
    uint16_t parserCount;          // Number of parsers
    size_t length;                 // Message length including line termination
    uint16_t type;                 // Active parser type, a value of
                                   // parserCount means searching for preamble
    bool nmeaAbortOnNonPrintable;  // Abort NMEA parsing on the arrival of a non-printable char
    bool unicoreHashAbortOnNonPrintable; // Abort Unicore hash parsing on the arrival of a non-printable char
} SEMP_PARSE_STATE;

//----------------------------------------
// Protocol specific types
//----------------------------------------

// Define the Unicore message header
typedef struct _SEMP_UNICORE_HEADER
{
    uint8_t syncA;            // 0xaa
    uint8_t syncB;            // 0x44
    uint8_t syncC;            // 0xb5
    uint8_t cpuIdlePercent;   // CPU Idle Percentage 0-100
    uint16_t messageId;       // Message ID
    uint16_t messageLength;   // Message Length
    uint8_t referenceTime;    // Reference time（GPST or BDST)
    uint8_t timeStatus;       // Time status
    uint16_t weekNumber;      // Reference week number
    uint32_t secondsOfWeek;   // GPS seconds from the beginning of the
                              // reference week, accurate to the millisecond
    uint32_t RESERVED;

    uint8_t releasedVersion;  // Release version
    uint8_t leapSeconds;      // Leap sec
    uint16_t outputDelayMSec; // Output delay time, ms
} SEMP_UNICORE_HEADER;

//------------------------------------------------------------------------------
// SparkFun Extensible Message Parser API routines - Called by the applications
//
// The general API routines are used to locate and release the parse data
// structure, pass data bytes to the parser and control output.
//------------------------------------------------------------------------------

// Initialize the parser
//
// Inputs:
//   parserTableName: Address of a zero terminated parserTable name
//   parseTable: Address of an array of SEMP_PARSER_DESCRIPTION addresses
//   parserCount:  Number of entries in the parseTable
//   buffer: Address of the buffer to be used for parser state, scratchpad
//   bufferLength: Number of bytes in the buffer
//   oemCallback: Address of a callback routine to handle the output
//   printError: Addess of a routine used to output error messages
//   printDebug: Addess of a routine used to output debug messages
//   badCrcCallback: Address of a routine to handle bad CRC messages
//
// Outputs:
//   Returns the address of a SEMP_PARSE_STATE structure when successful
//   or nullptr upon error
//
// The routine sempBeginParser verifies the parameters and locates an
// SEMP_PARSE_STATE data structure, returning the pointer when successful
// or nullptr upon failure.
//
// An array addresses are passed to the sempBeginParser routine along
// with the number of entries in the arrays.  The array parseTable lists
// the descriptions for each of the parsers that will process the raw data
// stream.  The parser name in the description contains a name string
// which can be output during debugging.
//
// Some of the parsers require additional storage to successfully parse
// the data stream.  The value scratchPadBytes must contain the maximum
// size necessary for all user parser implementations that are included
// in the parse table.  The scratchPadBytes value gets increased if it
// is less than the requirements for the SparkFun parsers included in
// this library.  When no user parsers are specified a value of zero (0)
// may be used for scratchPadBytes.
//
// The value bufferLength must specify the number of bytes in the
// largest message that will be parsed.  As parsing proceeds raw data
// bytes are placed into the local parse buffer which gets passed to
// the end-of-message callback routine.
//
// The parser calls the eomCallback routine after successfully parsing
// a message.  The end-of-message callback routine accesses the message
// at the beginning of the parse buffer.  The message length is found
// in the length field of the parse data structure.
//
// The name string is a name for the parse data structure.  When errors
// are detected by a parser the name string value gets displayed to
// identify the parse data structure associated with the error.  This
// is helpful when multiple parsers are running at the same time.
//
// The printError value specifies a class address to use when printing
// errors.  A nullptr value prevents any error from being output.  It is
// possible to call sempSetPrintError later to enable or disable error
// output.
SEMP_PARSE_STATE * sempBeginParser(const char *parserTableName,
                                   SEMP_PARSER_DESCRIPTION **parseTable,
                                   uint16_t parserCount,
                                   uint8_t * buffer,
                                   size_t bufferLength,
                                   SEMP_EOM_CALLBACK eomCallback,
                                   Print *printError = &Serial,
                                   Print *printDebug = (Print *)nullptr,
                                   SEMP_BAD_CRC_CALLBACK badCrcCallback = (SEMP_BAD_CRC_CALLBACK)nullptr);

// Disable debug output
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
void sempDisableDebugOutput(SEMP_PARSE_STATE *parse);

// Disable error output
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
void sempDisableErrorOutput(SEMP_PARSE_STATE *parse);

// Compute the necessary buffer length in bytes to support the scratch pad
// and parse buffer lengths.
//
// Inputs:
//   parseTable: Address of an array of SEMP_PARSER_DESCRIPTION addresses
//   parserCount:  Number of entries in the parseTable
//   parseBufferBytes: Desired size of the parse buffer in bytes
//   printDebug: Device to output any debug messages, may be nullptr
//
// Outputs:
//    Returns the number of bytes needed for the buffer that contains
//    the SEMP parser state, a scratch pad area and the parse buffer
size_t sempGetBufferLength(SEMP_PARSER_DESCRIPTION **parserTable,
                           uint16_t parserCount,
                           size_t parserBufferBytes,
                           Print *printDebug = &Serial);

// Translates state value into an ASCII state name
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//
// Outputs:
//   Returns the address of a zero terminated state name string
const char * sempGetStateName(const SEMP_PARSE_STATE *parse);

// Translate the type value into an ASCII type name
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//
// Outputs:
//   Returns the address of a zero terminated type name string
const char * sempGetTypeName(SEMP_PARSE_STATE *parse, uint16_t type);

// The routine sempParseNextByte is used to parse the next data byte
// from a raw data stream.
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//   data: Next data byte in the stream of data to parse
void sempParseNextByte(SEMP_PARSE_STATE *parse, uint8_t data);

// The routine sempParseNextBytes is used to parse the next bytes
// from a raw data stream.
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//   data: Address of a buffr containing the next data bytes in the
//         stream of data to parse
//   len: Number of data bytes to parse
void sempParseNextBytes(SEMP_PARSE_STATE *parse,
                        const uint8_t *data,
                        size_t len);

// The routine sempStopParser frees the parse data structure and sets
// the pointer value to nullptr to prevent future references to the
// structure.
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE * structure
void sempStopParser(SEMP_PARSE_STATE **parse);

//------------------------------------------------------------------------------
// Payload access routines - Called by the applications
//
// The payload access routines are used by the application to extract
// values from the payload.
//------------------------------------------------------------------------------

// Get a 32-bit floating point value
// Inputs:
//   parseTable: Address of an array of SEMP_PARSER_DESCRIPTION addresses
//   offset:  Offsets from the packetNumber of entries in the parseTable
//
// Outputs:
//    Returns the floating point value
float sempGetF4(const SEMP_PARSE_STATE *parse, size_t offset);

// Get a 32-bit floating point value
// Inputs:
//   parseTable: Address of an array of SEMP_PARSER_DESCRIPTION addresses
//   offset:  Offsets from the packetNumber of entries in the parseTable
//
// Outputs:
//    Returns the floating point value
float sempGetF4NoOffset(const SEMP_PARSE_STATE *parse, size_t offset);

// Get a 64-bit floating point (double) value
// Inputs:
//   parseTable: Address of an array of SEMP_PARSER_DESCRIPTION addresses
//   offset:  Offsets from the packetNumber of entries in the parseTable
//
// Outputs:
//    Returns the floating point value
double sempGetF8(const SEMP_PARSE_STATE *parse, size_t offset);

// Get a 64-bit floating point (double) value
// Inputs:
//   parseTable: Address of an array of SEMP_PARSER_DESCRIPTION addresses
//   offset:  Offsets from the packetNumber of entries in the parseTable
//
// Outputs:
//    Returns the floating point value
double sempGetF8NoOffset(const SEMP_PARSE_STATE *parse, size_t offset);

// Get an 8-bit integer value
// Inputs:
//   parseTable: Address of an array of SEMP_PARSER_DESCRIPTION addresses
//   offset:  Offsets from the packetNumber of entries in the parseTable
//
// Outputs:
//    Returns the integer value
int8_t sempGetI1(const SEMP_PARSE_STATE *parse, size_t offset);

// Get an 8-bit integer value
// Inputs:
//   parseTable: Address of an array of SEMP_PARSER_DESCRIPTION addresses
//   offset:  Offsets from the packetNumber of entries in the parseTable
//
// Outputs:
//    Returns the integer value
int8_t sempGetI1NoOffset(const SEMP_PARSE_STATE *parse, size_t offset);

// Get an 16-bit integer value
// Inputs:
//   parseTable: Address of an array of SEMP_PARSER_DESCRIPTION addresses
//   offset:  Offsets from the packetNumber of entries in the parseTable
//
// Outputs:
//    Returns the integer value
int16_t sempGetI2(const SEMP_PARSE_STATE *parse, size_t offset);

// Get an 16-bit integer value
// Inputs:
//   parseTable: Address of an array of SEMP_PARSER_DESCRIPTION addresses
//   offset:  Offsets from the packetNumber of entries in the parseTable
//
// Outputs:
//    Returns the integer value
int16_t sempGetI2NoOffset(const SEMP_PARSE_STATE *parse, size_t offset);

// Get an 32-bit integer value
// Inputs:
//   parseTable: Address of an array of SEMP_PARSER_DESCRIPTION addresses
//   offset:  Offsets from the packetNumber of entries in the parseTable
//
// Outputs:
//    Returns the integer value
int32_t sempGetI4(const SEMP_PARSE_STATE *parse, size_t offset);

// Get an 32-bit integer value
// Inputs:
//   parseTable: Address of an array of SEMP_PARSER_DESCRIPTION addresses
//   offset:  Offsets from the packetNumber of entries in the parseTable
//
// Outputs:
//    Returns the integer value
int32_t sempGetI4NoOffset(const SEMP_PARSE_STATE *parse, size_t offset);

// Get an 64-bit integer value
// Inputs:
//   parseTable: Address of an array of SEMP_PARSER_DESCRIPTION addresses
//   offset:  Offsets from the packetNumber of entries in the parseTable
//
// Outputs:
//    Returns the integer value
int64_t sempGetI8(const SEMP_PARSE_STATE *parse, size_t offset);

// Get an 64-bit integer value
// Inputs:
//   parseTable: Address of an array of SEMP_PARSER_DESCRIPTION addresses
//   offset:  Offsets from the packetNumber of entries in the parseTable
//
// Outputs:
//    Returns the integer value
int64_t sempGetI8NoOffset(const SEMP_PARSE_STATE *parse, size_t offset);

// Get a zero terminated string address
// Inputs:
//   parseTable: Address of an array of SEMP_PARSER_DESCRIPTION addresses
//   offset:  Offsets from the packetNumber of entries in the parseTable
//
// Outputs:
//    Returns the address of the string
const char * sempGetString(const SEMP_PARSE_STATE *parse, size_t offset);

// Get a zero terminated string address
// Inputs:
//   parseTable: Address of an array of SEMP_PARSER_DESCRIPTION addresses
//   offset:  Offsets from the packetNumber of entries in the parseTable
//
// Outputs:
//    Returns the address of the string
const char * sempGetStringNoOffset(const SEMP_PARSE_STATE *parse, size_t offset);

// Get an 8-bit unsigned integer value
// Inputs:
//   parseTable: Address of an array of SEMP_PARSER_DESCRIPTION addresses
//   offset:  Offsets from the packetNumber of entries in the parseTable
//
// Outputs:
//    Returns the unsigned integer value
uint8_t sempGetU1(const SEMP_PARSE_STATE *parse, size_t offset);

// Get an 8-bit unsigned integer value
// Inputs:
//   parseTable: Address of an array of SEMP_PARSER_DESCRIPTION addresses
//   offset:  Offsets from the packetNumber of entries in the parseTable
//
// Outputs:
//    Returns the unsigned integer value
uint8_t sempGetU1NoOffset(const SEMP_PARSE_STATE *parse, size_t offset);

// Get an 16-bit unsigned integer value
// Inputs:
//   parseTable: Address of an array of SEMP_PARSER_DESCRIPTION addresses
//   offset:  Offsets from the packetNumber of entries in the parseTable
//
// Outputs:
//    Returns the unsigned integer value
uint16_t sempGetU2(const SEMP_PARSE_STATE *parse, size_t offset);

// Get an 16-bit unsigned integer value
// Inputs:
//   parseTable: Address of an array of SEMP_PARSER_DESCRIPTION addresses
//   offset:  Offsets from the packetNumber of entries in the parseTable
//
// Outputs:
//    Returns the unsigned integer value
uint16_t sempGetU2NoOffset(const SEMP_PARSE_STATE *parse, size_t offset);

// Get an 32-bit unsigned integer value
// Inputs:
//   parseTable: Address of an array of SEMP_PARSER_DESCRIPTION addresses
//   offset:  Offsets from the packetNumber of entries in the parseTable
//
// Outputs:
//    Returns the unsigned integer value
uint32_t sempGetU4(const SEMP_PARSE_STATE *parse, size_t offset);

// Get an 32-bit unsigned integer value
// Inputs:
//   parseTable: Address of an array of SEMP_PARSER_DESCRIPTION addresses
//   offset:  Offsets from the packetNumber of entries in the parseTable
//
// Outputs:
//    Returns the unsigned integer value
uint32_t sempGetU4NoOffset(const SEMP_PARSE_STATE *parse, size_t offset);

// Get an 64-bit unsigned integer value
// Inputs:
//   parseTable: Address of an array of SEMP_PARSER_DESCRIPTION addresses
//   offset:  Offsets from the packetNumber of entries in the parseTable
//
// Outputs:
//    Returns the unsigned integer value
uint64_t sempGetU8(const SEMP_PARSE_STATE *parse, size_t offset);

// Get an 64-bit unsigned integer value
// Inputs:
//   parseTable: Address of an array of SEMP_PARSER_DESCRIPTION addresses
//   offset:  Offsets from the packetNumber of entries in the parseTable
//
// Outputs:
//    Returns the unsigned integer value
uint64_t sempGetU8NoOffset(const SEMP_PARSE_STATE *parse, size_t offset);

//------------------------------------------------------------------------------
// SparkFun Extensible Message Parser API routines - Called by parsers
//
// These API routines should only be called by parsers when processing
// the incoming data stream
//------------------------------------------------------------------------------

// Only parsers should call the routine sempFirstByte when an unexpected
// byte is found in the data stream.  Parsers will also set the state
// value to sempFirstByte after successfully parsing a message.  The
// sempFirstByte routine calls each of the parsers' preamble routine to
// determine if the parser recognizes the data byte as the preamble for
// a message.  The first parser to acknowledge the preamble byte by
// returning true is the parser that gets called for the following data.
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//   data: First data byte in the stream of data to parse
//
// Outputs:
//   Returns true if a parser was found to process this data and false
//   when none of the parsers recgonize the input data
bool sempFirstByte(SEMP_PARSE_STATE *parse, uint8_t data);

//------------------------------------------------------------------------------
// Print support routines - Called by parsers and some applications
//
// The print support routines are used to output debug and error messages.
//------------------------------------------------------------------------------

// Convert an ASCII character (0-9, A-F, or a-f) into a 4-bit binary value
//
// Inputs:
//   data: An ASCII character (0-9, A-F, or a-f)
//
// Outputs:
//   If successful, returns the 4-bit binary value matching the character
//   or -1 upon failure for invalid characters
int sempAsciiToNibble(int data);

// Print the contents of the parser data structure
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//   print: Address of a Print object to use for output
void sempPrintParserConfiguration(SEMP_PARSE_STATE *parse, Print *print = &Serial);

//------------------------------------------------------------------------------
// Parser notes
//------------------------------------------------------------------------------

// The parser routines are placed in reverse order to define the routine before
// its use and eliminate forward declarations.  Removing the forward declaration
// helps reduce the exposure of the routines to the application layer.  Typically
// only the parser description is made public.

//------------------------------------------------------------------------------
// NMEA
//------------------------------------------------------------------------------

// Length of the sentence name array
#define SEMP_NMEA_SENTENCE_NAME_BYTES    16

// Data structure to list in the parserTable passed to sempBeginParser
extern SEMP_PARSER_DESCRIPTION sempNmeaParserDescription;

// Abort NMEA on a non-printable char
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//   abort: Set true to abort or false to continue when detecting a
//          non-printable character in the input stream
void sempNmeaAbortOnNonPrintable(SEMP_PARSE_STATE *parse, bool abort = true);

// Get the sentence name
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//
// Outputs:
//   Returns the address of a zero terminated sentence name string
const char * sempNmeaGetSentenceName(const SEMP_PARSE_STATE *parse);

// Get the state name
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//
// Outputs:
//   Returns the address of a zero terminated state name string
const char * sempNmeaGetStateName(const SEMP_PARSE_STATE *parse);

//------------------------------------------------------------------------------
// RTCM
//------------------------------------------------------------------------------

// Data structure to list in the parserTable passed to sempBeginParser
extern SEMP_PARSER_DESCRIPTION sempRtcmParserDescription;

// RTCM parse routines
uint16_t sempRtcmGetMessageNumber(const SEMP_PARSE_STATE *parse);
int64_t sempRtcmGetSignedBits(const SEMP_PARSE_STATE *parse, size_t start, size_t width);
const char * sempRtcmGetStateName(const SEMP_PARSE_STATE *parse);
uint64_t sempRtcmGetUnsignedBits(const SEMP_PARSE_STATE *parse, size_t start, size_t width);

//------------------------------------------------------------------------------
// SBF
//------------------------------------------------------------------------------

// Data structure to list in the parserTable passed to sempBeginParser
extern SEMP_PARSER_DESCRIPTION sempSbfParserDescription;

// SBF parse routines
uint16_t sempSbfGetBlockNumber(const SEMP_PARSE_STATE *parse);
uint8_t sempSbfGetBlockRevision(const SEMP_PARSE_STATE *parse);
const uint8_t *sempSbfGetEncapsulatedPayload(const SEMP_PARSE_STATE *parse);
uint16_t sempSbfGetEncapsulatedPayloadLength(const SEMP_PARSE_STATE *parse);

// Get the ID value
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//
// Outputs:
//    Returns the ID value
uint16_t sempSbfGetId(const SEMP_PARSE_STATE *parse);
const char * sempSbfGetStateName(const SEMP_PARSE_STATE *parse);
bool sempSbfIsEncapsulatedNMEA(const SEMP_PARSE_STATE *parse);
bool sempSbfIsEncapsulatedRTCMv3(const SEMP_PARSE_STATE *parse);
void sempSbfSetInvalidDataCallback(const SEMP_PARSE_STATE *parse, SEMP_INVALID_DATA_CALLBACK invalidDataCallback);

// Deprecated duplicate routines
#define sempSbfGetF4        sempGetF4
#define sempSbfGetF8        sempGetF8
#define sempSbfGetI1        sempGetI1
#define sempSbfGetI2        sempGetI2
#define sempSbfGetI4        sempGetI4
#define sempSbfGetI8        sempGetI8
#define sempSbfGetString    sempGetString
#define sempSbfGetU1        sempGetU1
#define sempSbfGetU2        sempGetU2
#define sempSbfGetU4        sempGetU4
#define sempSbfGetU8        sempGetU8

//------------------------------------------------------------------------------
// SPARTN
//------------------------------------------------------------------------------

// Data structure to list in the parserTable passed to sempBeginParser
extern SEMP_PARSER_DESCRIPTION sempSpartnParserDescription;

// Get the message subtype number
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//
// Outputs:
//    Returns the message subtype number
uint8_t sempSpartnGetMessageSubType(const SEMP_PARSE_STATE *parse);

// Get the message number
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//
// Outputs:
//    Returns the message type number
uint8_t sempSpartnGetMessageType(const SEMP_PARSE_STATE *parse);

// Translates state value into an string, returns nullptr if not found
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//
// Outputs:
//    Returns a zero terminated string of characters
const char * sempSpartnGetStateName(const SEMP_PARSE_STATE *parse);

//------------------------------------------------------------------------------
// u-blox
//------------------------------------------------------------------------------

// Data structure to list in the parserTable passed to sempBeginParser
extern SEMP_PARSER_DESCRIPTION sempUbloxParserDescription;

uint8_t sempUbloxGetMessageClass(const SEMP_PARSE_STATE *parse);
uint8_t sempUbloxGetMessageId(const SEMP_PARSE_STATE *parse);
uint16_t sempUbloxGetMessageNumber(const SEMP_PARSE_STATE *parse); // |- Class (8 bits) -||- ID (8 bits) -|
size_t sempUbloxGetPayloadLength(const SEMP_PARSE_STATE *parse);
const char * sempUbloxGetStateName(const SEMP_PARSE_STATE *parse);

// Deprecated duplicate routines
#define sempUbloxGetI1      sempGetI1
#define sempUbloxGetI2      sempGetI2
#define sempUbloxGetI4      sempGetI4
#define sempUbloxGetI8      sempGetI8
#define sempUbloxGetR4      sempGetF4
#define sempUbloxGetR8      sempGetF8
#define sempUbloxGetString  sempGetStringNoOffset
#define sempUbloxGetU1      sempGetU1
#define sempUbloxGetU2      sempGetU2
#define sempUbloxGetU4      sempGetU4
#define sempUbloxGetU8      sempGetU8

//------------------------------------------------------------------------------
// Unicore Binary
//------------------------------------------------------------------------------

// Data structure to list in the parserTable passed to sempBeginParser
extern SEMP_PARSER_DESCRIPTION sempUnicoreBinaryParserDescription;

// Unicore binary parse routines
const char * sempUnicoreBinaryGetStateName(const SEMP_PARSE_STATE *parse);
void sempUnicoreBinaryPrintHeader(SEMP_PARSE_STATE *parse);

//------------------------------------------------------------------------------
// Unicore Hash (#)
//------------------------------------------------------------------------------

// Data structure to list in the parserTable passed to sempBeginParser
extern SEMP_PARSER_DESCRIPTION sempUnicoreHashParserDescription;

// Abort Unicore hash on a non-printable char
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//   abort: Set true to abort or false to continue when detecting a
//          non-printable character in the input stream
void sempUnicoreHashAbortOnNonPrintable(SEMP_PARSE_STATE *parse, bool abort = true);

// Unicore hash (#) parse routines
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//
// Outputs:
//   Returns the address of a zero terminated sentence name string
const char * sempUnicoreHashGetSentenceName(const SEMP_PARSE_STATE *parse);

const char * sempUnicoreHashGetStateName(const SEMP_PARSE_STATE *parse);

//------------------------------------------------------------------------------
// V1 routines to be eliminated
//------------------------------------------------------------------------------

// Enable debug output
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//   print: Address of a Print object to use for output
void sempEnableDebugOutput(SEMP_PARSE_STATE *parse,
                           Print *print = &Serial,
                           bool verbose = false);

// Enable error output
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//   print: Address of a Print object to use for output
void sempEnableErrorOutput(SEMP_PARSE_STATE *parse,
                           Print *print = &Serial);

// Format and print a line of text
//
// Inputs:
//   print: Address of a Print object to use for output
//   format: Address of a zero terminated string of format characters
//   ...: Parameters needed for the format
void sempPrintf(Print *print, const char *format, ...);

// Print a line of text
//
// Inputs:
//   print: Address of a Print object to use for output
//   string: Address of a zero terminated string of characters to output
void sempPrintln(Print *print, const char *string = "");

#endif  // __SPARKFUN_EXTENSIBLE_MESSAGE_PARSER_H__
