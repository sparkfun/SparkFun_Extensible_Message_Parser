/*------------------------------------------------------------------------------
SparkFun_Extensible_Message_Parser.h

  Constant and routine declarations for the extensible message parser.
------------------------------------------------------------------------------*/

#ifndef __SPARKFUN_EXTENSIBLE_MESSAGE_PARSER_H__
#define __SPARKFUN_EXTENSIBLE_MESSAGE_PARSER_H__

#include <Arduino.h>

//----------------------------------------
// Constants
//----------------------------------------

#define SEMP_MINIMUM_BUFFER_LENGTH      32

//----------------------------------------
// Types
//----------------------------------------

// Forward type declaration
typedef struct _SEMP_PARSE_STATE *P_SEMP_PARSE_STATE;

// Parse routine
typedef bool (*SEMP_PARSE_ROUTINE)(P_SEMP_PARSE_STATE parse, // Parser state
                                   uint8_t data); // Incoming data byte

// CRC callback routine
typedef uint32_t (*SEMP_CRC_CALLBACK)(P_SEMP_PARSE_STATE parse, // Parser state
                                      uint8_t dataByte); // Data byte

// Call the application back at specified routine address.  Pass in the
// parse data structure containing the buffer containing the address of
// the message data and the length field containing the number of valid
// data bytes in the buffer.
//
// The type field contains the index into the parseTable which specifies
// the parser that successfully processed the incoming data.
//
// End of message callback routine
typedef void (*SEMP_EOM_CALLBACK)(P_SEMP_PARSE_STATE parse, // Parser state
                                  uint16_t type); // Index into parseTable

// Length of the message name array
#define SEMP_NMEA_MESSAGE_NAME_BYTES    16

// NMEA parser scratch area
typedef struct _SEMP_NMEA_VALUES
{
    uint8_t messageName[SEMP_NMEA_MESSAGE_NAME_BYTES]; // Message name
    uint8_t messageNameLength; // Length of the message name
} SEMP_NMEA_VALUES;

// RTCM parser scratch area
typedef struct _SEMP_RTCM_VALUES
{
    uint32_t crc;            // Copy of CRC calculation before CRC bytes
    uint16_t bytesRemaining; // Bytes remaining in RTCM CRC calculation
    uint16_t message;        // Message number
} SEMP_RTCM_VALUES;

// UBLOX parser scratch area
typedef struct _SEMP_UBLOX_VALUES
{
    uint16_t bytesRemaining; // Bytes remaining in field
    uint16_t message;        // Message number
    uint8_t ck_a;            // U-blox checksum byte 1
    uint8_t ck_b;            // U-blox checksum byte 2
} SEMP_UBLOX_VALUES;

// Unicore parser scratch area
typedef struct _SEMP_UNICORE_VALUES
{
    uint32_t crc;            // Copy of CRC calculation before CRC bytes
    uint16_t bytesRemaining; // Bytes remaining in RTCM CRC calculation
} SEMP_UNICORE_VALUES;

// Overlap the scratch areas since only one parser is active at a time
typedef union
{
    SEMP_NMEA_VALUES nmea;       // NMEA specific values
    SEMP_RTCM_VALUES rtcm;       // RTCM specific values
    SEMP_UBLOX_VALUES ublox;     // U-blox specific values
    SEMP_UNICORE_VALUES unicore; // Unicore specific values
} SEMP_SCRATCH_PAD;

// Maintain the operating state of one or more parsers processing a raw
// data stream.
typedef struct _SEMP_PARSE_STATE
{
    const SEMP_PARSE_ROUTINE *parsers; // Table of parsers
    const char * const *parserNames;   // Table of parser names
    SEMP_PARSE_ROUTINE state;      // Parser state routine
    SEMP_EOM_CALLBACK eomCallback; // End of message callback routine
    SEMP_CRC_CALLBACK computeCrc;  // Routine to compute the CRC when set
    const char *parserName;        // Name of parser
    void *scratchPad;              // Parser scratchpad area
    Print *printError;             // Class to use for error output
    Print *printDebug;             // Class to use for debug output
    uint32_t crc;                  // RTCM computed CRC
    uint8_t *buffer;               // Buffer containing the message
    uint32_t bufferLength;         // Length of the buffer in bytes
    uint16_t parserCount;          // Number of parsers
    uint16_t length;               // Message length including line termination
    uint16_t type;                 // Active parser type, a value of
                                   // parserCount means searching for preamble
} SEMP_PARSE_STATE;

//----------------------------------------
// Support routines
//----------------------------------------

int sempAsciiToNibble(int data);

//----------------------------------------
// Public routines - Called by the application
//----------------------------------------

// The general routines are used to allocate and free the parse data
// structure and to pass data bytes to the parser.
//
// The routine sempBeginParser verifies the parameters and allocates an
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
//
// Allocate and initialize a parse data structure
SEMP_PARSE_STATE * sempBeginParser(const SEMP_PARSE_ROUTINE *parseTable, \
                                   uint16_t parserCount, \
                                   const char * const *parserNameTable, \
                                   uint16_t parserNameCount, \
                                   uint16_t scratchPadBytes, \
                                   size_t bufferLength, \
                                   SEMP_EOM_CALLBACK eomCallback, \
                                   const char *name, \
                                   Print *printError = &Serial);

// Only parsers should call the routine sempFirstByte when an unexpected
// byte is found in the data stream.  Parsers will also set the state
// value to sempFirstByte after successfully parsing a message.  The
// sempFirstByte routine calls each of the parsers' preamble routine to
// determine if the parser recognizes the data byte as the preamble for
// a message.  The first parser to acknowledge the preamble byte by
// returning true is the parser that gets called for the following data.
bool sempFirstByte(SEMP_PARSE_STATE *parse, uint8_t data);

// The routine sempParseNextByte is used to parse the next data byte
// from a raw data stream.
void sempParseNextByte(SEMP_PARSE_STATE *parse, uint8_t data);

// The routine sempStopParser frees the parse data structure and sets
// the pointer value to nullptr to prevent future references to the
// freed structure.
void sempStopParser(SEMP_PARSE_STATE **parse);

// Print the contents of the parser data structure
void sempPrintParserConfiguration(SEMP_PARSE_STATE *parse);

// Format and print a line of text
void sempPrintf(Print *print, const char *format, ...);

// Print a line of text
void sempPrintln(Print *print, const char *string = "");

// Translates state value into an ASCII state name
const char * sempGetStateName(const SEMP_PARSE_STATE *parse);

// Translate the type value into an ASCII type name
const char * sempGetTypeName(SEMP_PARSE_STATE *parse, uint16_t type);

// Enable or disable debug output.  Specify nullptr to disable output.
void sempSetPrintDebug(SEMP_PARSE_STATE *parse, Print *printDebug);

// Enable or disable error output.  Specify nullptr to disable output.
void sempSetPrintError(SEMP_PARSE_STATE *parse, Print *printError);

// The parser routines within a parser module are typically placed in
// reverse order within the module.  This lets the routine declaration
// proceed the routine use and eliminates the need for forward declaration.
// Removing the forward declaration helps reduce the exposure of the
// routines to the application layer.  As such only the preamble routine
// should need to be listed below.

// NMEA parse routines
bool sempNmeaPreamble(SEMP_PARSE_STATE *parse, uint8_t data);
bool sempNmeaFindFirstComma(SEMP_PARSE_STATE *parse, uint8_t data);
const char * sempNmeaGetStateName(const SEMP_PARSE_STATE *parse);

// RTCM parse routines
bool sempRtcmPreamble(SEMP_PARSE_STATE *parse, uint8_t data);
const char * sempRtcmGetStateName(const SEMP_PARSE_STATE *parse);

// u-blox parse routines
bool sempUbloxPreamble(SEMP_PARSE_STATE *parse, uint8_t data);
const char * sempUbloxGetStateName(const SEMP_PARSE_STATE *parse);

// Unicore parse routines
bool sempUnicorePreamble(SEMP_PARSE_STATE *parse, uint8_t data);
const char * sempUnicoreGetStateName(const SEMP_PARSE_STATE *parse);
void sempUnicorePrintHeader(SEMP_PARSE_STATE *parse);

#endif  // __SPARKFUN_EXTENSIBLE_MESSAGE_PARSER_H__
