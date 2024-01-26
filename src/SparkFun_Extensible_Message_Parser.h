/*------------------------------------------------------------------------------
SparkFun_Extensible_Message_Parser.h

  Constant and routine declarations for the extensible message parser.
------------------------------------------------------------------------------*/

#ifndef __SPARKFUN_EXTENSIBLE_MESSAGE_PARSER_H__
#define __SPARKFUN_EXTENSIBLE_MESSAGE_PARSER_H__

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

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

// End of message callback routine
typedef void (*SEMP_EOM_CALLBACK)(P_SEMP_PARSE_STATE parse, // Parser state
                                  uint8_t type); // Message type

// Length of the message name array
#define SEMP_NMEA_MESSAGE_NAME_BYTES    16

// NMEA parser scratch area
typedef struct _SEMP_NMEA_VALUES
{
    uint8_t messageName[SEMP_NMEA_MESSAGE_NAME_BYTES]; // Message name
    uint8_t messageNameLength; // Length of the message name
} SEMP_NMEA_VALUES;

// UBLOX parser scratch area
typedef struct _SEMP_UBLOX_VALUES
{
    uint16_t bytesRemaining; // Bytes remaining in field
    uint16_t message;        // Message number
    uint8_t ck_a;            // U-blox checksum byte 1
    uint8_t ck_b;            // U-blox checksum byte 2
} SEMP_UBLOX_VALUES;

// Overlap the scratch areas since only one parser is active at a time
typedef union
{
    SEMP_NMEA_VALUES nmea;   // NMEA specific values
    SEMP_UBLOX_VALUES ublox; // U-blox specific values
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
    uint32_t crc;                  // RTCM computed CRC
    uint8_t *buffer;               // Buffer containing the message
    uint32_t bufferLength;         // Length of the buffer in bytes
    uint16_t parserCount;          // Number of parsers
    uint16_t length;               // Message length including line termination
    uint8_t type;                  // Active parser type
} SEMP_PARSE_STATE;

//----------------------------------------
// Application defined routines
//----------------------------------------

// The sempExtPrintLineOfText routine must be implemented by the
// application layer.  A typical implementation is Serial.println(string).
void sempExtPrintLineOfText(const char *string);

//----------------------------------------
// Globals values
//----------------------------------------

// sempPrintErrorMessages controls when parse error messages may be
// displayed.  This value enables message display during sempInitParser.
extern bool sempPrintErrorMessages;

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
// The routine sempInitParser verifies the parameters and allocates an
// SEMP_PARSE_STATE data structure, returning the pointer when successful
// or nullptr upon failure.
//
// Two array addresses are passed to the sempInitParser routine along
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
// Allocate and initialize a parse data structure
SEMP_PARSE_STATE * sempInitParser(const SEMP_PARSE_ROUTINE *parseTable, \
                                  uint16_t parserCount, \
                                  const char * const *parserNameTable, \
                                  uint16_t parserNameCount, \
                                  uint16_t scratchPadBytes, \
                                  size_t bufferLength, \
                                  SEMP_EOM_CALLBACK eomCallback, \
                                  const char *name);

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

// The routine sempShutdownParser frees the parse data structure and
// sets the pointer value to nullptr to prevent future references to
// the freed structure.
void sempShutdownParser(SEMP_PARSE_STATE **parse);

// Print the contents of the parser data structure
void sempPrintParserConfiguration(SEMP_PARSE_STATE *parse);

// The parser routines within a parser module are typically placed in
// reverse order within the module.  This lets the routine declaration
// proceed the routine use and eliminates the need for forward declaration.
// Removing the forward declaration helps reduce the exposure of the
// routines to the application layer.  As such only the preamble routine
// should need to be listed below.

// NMEA parse routines
bool sempNmeaPreamble(SEMP_PARSE_STATE *parse, uint8_t data);

// u-blox parse routines
bool sempUbloxPreamble(SEMP_PARSE_STATE *parse, uint8_t data);

#endif  // __SPARKFUN_EXTENSIBLE_MESSAGE_PARSER_H__
