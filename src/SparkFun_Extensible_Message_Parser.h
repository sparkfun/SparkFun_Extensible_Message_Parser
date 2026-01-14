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

// Length of the sentence name array
#define SEMP_NMEA_SENTENCE_NAME_BYTES    16

// NMEA parser scratch area
typedef struct _SEMP_NMEA_VALUES
{
    uint8_t sentenceName[SEMP_NMEA_SENTENCE_NAME_BYTES]; // Sentence name
    uint8_t sentenceNameLength; // Length of the sentence name
} SEMP_NMEA_VALUES;

// RTCM parser scratch area
typedef struct _SEMP_RTCM_VALUES
{
    uint32_t crc;            // Copy of CRC calculation before CRC bytes
    uint16_t bytesRemaining; // Bytes remaining in RTCM CRC calculation
    uint16_t message;        // Message number
} SEMP_RTCM_VALUES;

// UBLOX payload offset
#define SEMP_UBLOX_PAYLOAD_OFFSET   6

// UBLOX parser scratch area
typedef struct _SEMP_UBLOX_VALUES
{
    uint16_t bytesRemaining; // Bytes remaining in field
    uint8_t messageClass;    // Message Class
    uint8_t messageId;       // Message ID
    uint16_t payloadLength;  // Payload length
    uint8_t ck_a;            // U-blox checksum byte 1
    uint8_t ck_b;            // U-blox checksum byte 2
} SEMP_UBLOX_VALUES;

// Unicore parser scratch area
typedef struct _SEMP_UNICORE_BINARY_VALUES
{
    uint32_t crc;            // Copy of CRC calculation before CRC bytes
    uint16_t bytesRemaining; // Bytes remaining in RTCM CRC calculation
} SEMP_UNICORE_BINARY_VALUES;

// Length of the sentence name array
#define SEMP_UNICORE_HASH_SENTENCE_NAME_BYTES    16

// Unicore hash (#) parser scratch area
typedef struct _SEMP_UNICORE_HASH_VALUES
{
    uint8_t bytesRemaining;     // Bytes remaining in field
    uint8_t checksumBytes;      // Number of checksum bytes
    uint8_t sentenceName[SEMP_UNICORE_HASH_SENTENCE_NAME_BYTES]; // Sentence name
    uint8_t sentenceNameLength; // Length of the sentence name
} SEMP_UNICORE_HASH_VALUES;

// SPARTN parser scratch area
typedef struct _SEMP_SPARTN_VALUES
{
    uint16_t frameCount;
    uint16_t crcBytes;
    uint16_t TF007toTF016;

    uint8_t messageType;
    uint16_t payloadLength;
    uint16_t EAF;
    uint8_t crcType;
    uint8_t frameCRC;
    uint8_t messageSubtype;
    uint16_t timeTagType;
    uint16_t authenticationIndicator;
    uint16_t embeddedApplicationLengthBytes;
} SEMP_SPARTN_VALUES;

// SBF parser scratch area
typedef struct _SEMP_SBF_VALUES
{
    uint16_t expectedCRC;
    uint16_t computedCRC;
    uint16_t sbfID = 0;
    uint8_t sbfIDrev = 0;
    uint16_t length;
    uint16_t bytesRemaining;
    // Invalid data callback routine (parser-specific)
    SEMP_INVALID_DATA_CALLBACK invalidDataCallback = (SEMP_INVALID_DATA_CALLBACK)nullptr;
} SEMP_SBF_VALUES;

// Overlap the scratch areas since only one parser is active at a time
typedef union
{
    SEMP_NMEA_VALUES nmea;       // NMEA specific values
    SEMP_RTCM_VALUES rtcm;       // RTCM specific values
    SEMP_UBLOX_VALUES ublox;     // U-blox specific values
    SEMP_UNICORE_BINARY_VALUES unicoreBinary; // Unicore binary specific values
    SEMP_UNICORE_HASH_VALUES unicoreHash;     // Unicore hash (#) specific values
    SEMP_SPARTN_VALUES spartn;   // SPARTN specific values
    SEMP_SBF_VALUES sbf;         // SBF specific values
} SEMP_SCRATCH_PAD;

// Describe the parser
typedef const struct _SEMP_PARSER_DESCRIPTION
{
    const char * parserName;        // Name of the parser
    SEMP_PARSE_ROUTINE preamble;    // Routine to handle the preamble
} SEMP_PARSER_DESCRIPTION;

// Maintain the operating state of one or more parsers processing a raw
// data stream.
typedef struct _SEMP_PARSE_STATE
{
    const SEMP_PARSE_ROUTINE *parsers; // Table of parsers
    const char * const *parserNames;   // Table of parser names
    SEMP_PARSE_ROUTINE state;      // Parser state routine
    SEMP_EOM_CALLBACK eomCallback; // End of message callback routine
    SEMP_BAD_CRC_CALLBACK badCrc;  // Bad CRC callback routine
    SEMP_COMPUTE_CRC computeCrc;   // Routine to compute the CRC when set
    const char *parserName;        // Name of parser
    void *scratchPad;              // Parser scratchpad area
    Print *printError;             // Class to use for error output
    Print *printDebug;             // Class to use for debug output
    bool verboseDebug;             // Verbose debug output (default: false)
    uint32_t crc;                  // RTCM computed CRC
    uint8_t *buffer;               // Buffer containing the message
    uint32_t bufferLength;         // Length of the buffer in bytes
    uint16_t parserCount;          // Number of parsers
    uint16_t length;               // Message length including line termination
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

//----------------------------------------
// Support routines
//----------------------------------------

int sempAsciiToNibble(int data);

//----------------------------------------
// Public routines - Called by the application
//
// The public routines are used to locate and release the parse data
// structure and to pass data bytes to the parser.
//----------------------------------------

// Initialize the parser
//
// Inputs:
//   parseTable: Address of an array of SEMP_PARSE_ROUTINE objects
//   parserCount:  Number of entries in the parseTable
//   parserNameTable: Names of each of the parsers
//   scratchPadBytes: Number of bytes in the scratch pad area
//   buffer: Address of the buffer to be used for parser state, scratchpad
//   bufferLength: Number of bytes in the buffer
//   oemCallback: Address of a callback routine to handle the output
//   name: Address of a zero terminated parser name string
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
// Two array addresses are passed to the sempBeginParser routine along
// with the number of entries in each of these arrays.  The array
// parseTable lists the preamble routines associated with each of the
// parsers that will process the raw data stream.  The array
// parserNameTable contains a name string for each of the parsers which
// can be output during debugging.
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
SEMP_PARSE_STATE * sempBeginParser(const SEMP_PARSE_ROUTINE *parseTable,
                                   uint16_t parserCount,
                                   const char * const *parserNameTable,
                                   uint16_t parserNameCount,
                                   uint16_t scratchPadBytes,
                                   uint8_t * buffer,
                                   size_t bufferLength,
                                   SEMP_EOM_CALLBACK eomCallback,
                                   const char *name,
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

// Enable debug output
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//   print: Address of a Print object to use for output
void sempEnableDebugOutput(SEMP_PARSE_STATE *parse, Print *print = &Serial, bool verbose = false);

// Enable error output
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//   print: Address of a Print object to use for output
void sempEnableErrorOutput(SEMP_PARSE_STATE *parse, Print *print = &Serial);

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

// Compute the necessary buffer length in bytes to support the scratch pad
// and parse buffer lengths.
//
// Inputs:
//   scratchPadBytes: Desired size of the scratch pad in bytes
//   parseBufferBytes: Desired size of the parse buffer in bytes
//   printDebug: Device to output any debug messages, may be nullptr
//
// Outputs:
//    Returns the number of bytes needed for the buffer that contains
//    the SEMP parser state, a scratch pad area and the parse buffer
size_t sempGetBufferLength(size_t scratchPadBytes,
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
void sempParseNextBytes(SEMP_PARSE_STATE *parse, uint8_t *data, uint16_t len);

// Print the contents of the parser data structure
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//   print: Address of a Print object to use for output
void sempPrintParserConfiguration(SEMP_PARSE_STATE *parse, Print *print = &Serial);

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

// The routine sempStopParser frees the parse data structure and sets
// the pointer value to nullptr to prevent future references to the
// structure.
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE * structure
void sempStopParser(SEMP_PARSE_STATE **parse);

//----------------------------------------
// Parsers
//----------------------------------------

// The parser routines within a parser module are typically placed in
// reverse order within the module.  This lets the routine declaration
// proceed the routine use and eliminates the need for forward declaration.
// Removing the forward declaration helps reduce the exposure of the
// routines to the application layer.  As such only the preamble routine
// should need to be listed below.

//----------------------------------------
// NMEA
//----------------------------------------

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

// Start the parser by processing the first byte of data
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//   data: First data byte in the stream of data to parse
//
// Outputs:
//   Returns true if the NMEA parser recgonizes the input and false
//   another parser should be used
bool sempNmeaPreamble(SEMP_PARSE_STATE *parse, uint8_t data);

//----------------------------------------
// RTCM
//----------------------------------------

// RTCM parse routines
uint16_t sempRtcmGetMessageNumber(const SEMP_PARSE_STATE *parse);
int64_t sempRtcmGetSignedBits(const SEMP_PARSE_STATE *parse, uint16_t start, uint16_t width);
const char * sempRtcmGetStateName(const SEMP_PARSE_STATE *parse);
uint64_t sempRtcmGetUnsignedBits(const SEMP_PARSE_STATE *parse, uint16_t start, uint16_t width);

// Start the parser by processing the first byte of data
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//   data: First data byte in the stream of data to parse
//
// Outputs:
//   Returns true if the RTCM parser recgonizes the input and false
//   another parser should be used
bool sempRtcmPreamble(SEMP_PARSE_STATE *parse, uint8_t data);

//----------------------------------------
// SBF
//----------------------------------------

// SBF parse routines
uint16_t sempSbfGetBlockNumber(const SEMP_PARSE_STATE *parse);
uint8_t sempSbfGetBlockRevision(const SEMP_PARSE_STATE *parse);
const uint8_t *sempSbfGetEncapsulatedPayload(const SEMP_PARSE_STATE *parse);
uint16_t sempSbfGetEncapsulatedPayloadLength(const SEMP_PARSE_STATE *parse);
float sempSbfGetF4(const SEMP_PARSE_STATE *parse, uint16_t offset);
double sempSbfGetF8(const SEMP_PARSE_STATE *parse, uint16_t offset);
int8_t sempSbfGetI1(const SEMP_PARSE_STATE *parse, uint16_t offset);
int16_t sempSbfGetI2(const SEMP_PARSE_STATE *parse, uint16_t offset);
int32_t sempSbfGetI4(const SEMP_PARSE_STATE *parse, uint16_t offset);
int64_t sempSbfGetI8(const SEMP_PARSE_STATE *parse, uint16_t offset);
const char * sempSbfGetStateName(const SEMP_PARSE_STATE *parse);
const char *sempSbfGetString(const SEMP_PARSE_STATE *parse, uint16_t offset);
uint8_t sempSbfGetU1(const SEMP_PARSE_STATE *parse, uint16_t offset);
uint16_t sempSbfGetU2(const SEMP_PARSE_STATE *parse, uint16_t offset);
uint32_t sempSbfGetU4(const SEMP_PARSE_STATE *parse, uint16_t offset);
uint64_t sempSbfGetU8(const SEMP_PARSE_STATE *parse, uint16_t offset);
bool sempSbfIsEncapsulatedNMEA(const SEMP_PARSE_STATE *parse);
bool sempSbfIsEncapsulatedRTCMv3(const SEMP_PARSE_STATE *parse);

// Start the parser by processing the first byte of data
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//   data: First data byte in the stream of data to parse
//
// Outputs:
//   Returns true if the SBF parser recgonizes the input and false
//   another parser should be used
bool sempSbfPreamble(SEMP_PARSE_STATE *parse, uint8_t data);
void sempSbfSetInvalidDataCallback(const SEMP_PARSE_STATE *parse, SEMP_INVALID_DATA_CALLBACK invalidDataCallback);

//----------------------------------------
// SPARTN
//----------------------------------------

// SPARTN parse routines
uint8_t sempSpartnGetMessageType(const SEMP_PARSE_STATE *parse);
const char * sempSpartnGetStateName(const SEMP_PARSE_STATE *parse);

// Start the parser by processing the first byte of data
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//   data: First data byte in the stream of data to parse
//
// Outputs:
//   Returns true if the SPARTN parser recgonizes the input and false
//   another parser should be used
bool sempSpartnPreamble(SEMP_PARSE_STATE *parse, uint8_t data);

//----------------------------------------
// u-blox
//----------------------------------------

// Get the 8-bit integer from the offset
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//   offset: Offset from the message header
//
// Outputs:
//   Returns the integer value
int8_t sempUbloxGetI1(const SEMP_PARSE_STATE *parse, uint16_t offset);
int16_t sempUbloxGetI2(const SEMP_PARSE_STATE *parse, uint16_t offset);
int32_t sempUbloxGetI4(const SEMP_PARSE_STATE *parse, uint16_t offset);
int64_t sempUbloxGetI8(const SEMP_PARSE_STATE *parse, uint16_t offset);
uint8_t sempUbloxGetMessageClass(const SEMP_PARSE_STATE *parse);
uint8_t sempUbloxGetMessageId(const SEMP_PARSE_STATE *parse);
uint16_t sempUbloxGetMessageNumber(const SEMP_PARSE_STATE *parse); // |- Class (8 bits) -||- ID (8 bits) -|
uint16_t sempUbloxGetPayloadLength(const SEMP_PARSE_STATE *parse);
float sempUbloxGetR4(const SEMP_PARSE_STATE *parse, uint16_t offset);
double sempUbloxGetR8(const SEMP_PARSE_STATE *parse, uint16_t offset);
const char * sempUbloxGetStateName(const SEMP_PARSE_STATE *parse);
const char *sempUbloxGetString(const SEMP_PARSE_STATE *parse, uint16_t offset);
uint8_t sempUbloxGetU1(const SEMP_PARSE_STATE *parse, uint16_t offset); // offset is the Payload offset
uint16_t sempUbloxGetU2(const SEMP_PARSE_STATE *parse, uint16_t offset);
uint32_t sempUbloxGetU4(const SEMP_PARSE_STATE *parse, uint16_t offset);
uint64_t sempUbloxGetU8(const SEMP_PARSE_STATE *parse, uint16_t offset);

// Start the parser by processing the first byte of data
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//   data: First data byte in the stream of data to parse
//
// Outputs:
//   Returns true if the UBLOX parser recgonizes the input and false
//   another parser should be used
bool sempUbloxPreamble(SEMP_PARSE_STATE *parse, uint8_t data);

//----------------------------------------
// Unicore Binary
//----------------------------------------

// Unicore binary parse routines
const char * sempUnicoreBinaryGetStateName(const SEMP_PARSE_STATE *parse);

// Start the parser by processing the first byte of data
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//   data: First data byte in the stream of data to parse
//
// Outputs:
//   Returns true if the Unicore Binary parser recgonizes the input and
//   false another parser should be used
bool sempUnicoreBinaryPreamble(SEMP_PARSE_STATE *parse, uint8_t data);

void sempUnicoreBinaryPrintHeader(SEMP_PARSE_STATE *parse);

//----------------------------------------
// Unicore Hash (#)
//----------------------------------------

// Abort Unicore hash on a non-printable char
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//   abort: Set true to abort or false to continue when detecting a
//          non-printable character in the input stream
void sempUnicoreHashAbortOnNonPrintable(SEMP_PARSE_STATE *parse, bool abort = true);

// Unicore hash (#) parse routines
const char * sempUnicoreHashGetSentenceName(const SEMP_PARSE_STATE *parse);
const char * sempUnicoreHashGetStateName(const SEMP_PARSE_STATE *parse);

// Start the parser by processing the first byte of data
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//   data: First data byte in the stream of data to parse
//
// Outputs:
//   Returns true if the Unicore Hash parser recgonizes the input and
//   false another parser should be used
bool sempUnicoreHashPreamble(SEMP_PARSE_STATE *parse, uint8_t data);

void sempUnicoreHashPrintHeader(SEMP_PARSE_STATE *parse);

#endif  // __SPARKFUN_EXTENSIBLE_MESSAGE_PARSER_H__
