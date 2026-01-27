/*
  User_Parser.ino

  License: MIT. Please see LICENSE.md for more details

  Implement the User_Parser
*/

#include "User_Parser.h"

//----------------------------------------
// Types
//----------------------------------------

typedef struct _USER_SCRATCH_PAD
{
    uint32_t messageNumber; // Count the valid messages
} USER_SCRATCH_PAD;

//------------------------------------------------------------------------------
// User Parser
//
// The parser routines are placed in reverse order to define the routine before
// its use and eliminate forward declarations.  Removing the forward declaration
// helps reduce the exposure of the routines to the application layer.  The public
// data structures and routines are listed at the end of the file.
//------------------------------------------------------------------------------

//----------------------------------------
// Check for a number
//----------------------------------------
bool userFindNumber(SEMP_PARSE_STATE *parse, uint8_t data)
{
    USER_SCRATCH_PAD *scratchPad = (USER_SCRATCH_PAD *)parse->scratchPad;
    if ((data < '0') || (data > '9'))
        return sempFirstByte(parse, data);

    // Account for the valid message
    scratchPad->messageNumber += 1;

    // Pass the valid message to the end-of-message handler
    parse->eomCallback(parse, parse->type);

    // Start searching for a preamble byte
    parse->state = sempFirstByte;
    return true;
}

//----------------------------------------
// Check for the second preamble byte
//----------------------------------------
bool userSecondPreambleByte(SEMP_PARSE_STATE *parse, uint8_t data)
{
    USER_SCRATCH_PAD *scratchPad = (USER_SCRATCH_PAD *)parse->scratchPad;

    // Validate the second preamble byte
    if (data != 'B')
    {
        // Print the error
        sempPrintf(parse->printDebug,
                   "USER_Parser: Bad second preamble byte after message %d",
                   scratchPad->messageNumber);

        // Start searching for a preamble byte
        return sempFirstByte(parse, data);
    }

    // Valid preamble, search for a number
    parse->state = userFindNumber;
    return true;
}

//----------------------------------------
// Check for the first preamble byte
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//   data: First data byte in the stream of data to parse
//
// Outputs:
//   Returns true if the User_Parser recgonizes the input and false
//   if another parser should be used
//----------------------------------------
bool userPreamble(SEMP_PARSE_STATE *parse, uint8_t data)
{
    // Validate the first preamble byte
    if (data != 'A')
        // Start searching for a preamble byte
        return false;

    // Valid preamble byte, search for the next preamble byte
    parse->state = userSecondPreambleByte;
    return true;
}

//----------------------------------------
// Translates state value into an string, returns nullptr if not found
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//
// Outputs
//   Returns the address of the zero terminated state name string
//----------------------------------------
const char * userParserGetStateName(const SEMP_PARSE_STATE *parse)
{
    if (parse->state == userPreamble)
        return "userPreamble";
    if (parse->state == userSecondPreambleByte)
        return "userSecondPreambleByte";
    if (parse->state == userFindNumber)
        return "userFindNumber";
    return nullptr;
}

//------------------------------------------------------------------------------
// Public data and routines
//
// The following data structures and routines are listed in the .h file and are
// exposed to the SEMP routine and application layer.
//------------------------------------------------------------------------------

//----------------------------------------
// Describe the parser
//----------------------------------------
SEMP_PARSER_DESCRIPTION userParserDescription =
{
    "User parser",              // parserName
    userPreamble,               // preamble
    userParserGetStateName,     // State to state name translation routine
    3,                          // minimumParseAreaBytes
    sizeof(USER_SCRATCH_PAD),   // scratchPadBytes
    0,                          // payloadOffset
};

//----------------------------------------
// Get the message number
//----------------------------------------
uint32_t userParserGetMessageNumber(const SEMP_PARSE_STATE *parse)
{
    USER_SCRATCH_PAD *scratchPad = (USER_SCRATCH_PAD *)parse->scratchPad;
    return scratchPad->messageNumber;
}
