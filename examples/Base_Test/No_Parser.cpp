/*
  No_Parser.cpp
  SparkFun base test example sketch, verify it builds

  License: MIT. Please see LICENSE.md for more details

  Implement the "No_Parser"
*/

#include "No_Parser.h"

//------------------------------------------------------------------------------
// No_Parser routines
//
// The parser routines are placed in reverse order to define the routine before
// its use and eliminate forward declarations.  Removing the forward declaration
// helps reduce the exposure of the routines to the application layer.  The public
// data structures and routines are listed at the end of the file.
//------------------------------------------------------------------------------

//----------------------------------------
// Check for the preamble
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//   data: First data byte in the stream of data to parse
//
// Outputs:
//   Returns true if the No_Parser recgonizes the input and false
//   if another parser should be used
//----------------------------------------
bool noParserPreamble(SEMP_PARSE_STATE *parse, uint8_t data)
{
    if (data == 2)
    {
        parse->eomCallback(parse, parse->type);
        parse->state = sempFirstByte;
        return true;
    }
    // Preamble not found
    return false;
}

//----------------------------------------
// Translate the state into a zero terminated state name string
//
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//
// Outputs
//   Returns the address of the zero terminated state name string
//----------------------------------------
const char * noParserGetStateName(const SEMP_PARSE_STATE *parse)
{
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
SEMP_PARSER_DESCRIPTION noParserDescription =
{
    "No parser",            // parserName
    noParserPreamble,       // preamble
    0,                      // scratchPadBytes
};
