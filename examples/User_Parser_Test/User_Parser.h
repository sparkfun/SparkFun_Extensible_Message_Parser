/*
  User_Parser.h

  License: MIT. Please see LICENSE.md for more details

  Declare the public data structures and routines for the User_Parser
*/

#ifndef __USER_PARSER_H__
#define __USER_PARSER_H__

#include <SparkFun_Extensible_Message_Parser.h> //http://librarymanager/All#SparkFun_Extensible_Message_Parser

//----------------------------------------
// Constants
//----------------------------------------

extern SEMP_PARSER_DESCRIPTION userParserDescription;

//----------------------------------------
// User_Parser specific API
//----------------------------------------

// Get the message number
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//
// Outputs:
//   Returns the message number value
uint32_t userParserGetMessageNumber(const SEMP_PARSE_STATE *parse);

// Translates state value into an string, returns nullptr if not found
// Inputs:
//   parse: Address of a SEMP_PARSE_STATE structure
//
// Outputs:
//   Returns a zero-terminated state name string
//----------------------------------------
const char * userParserGetStateName(const SEMP_PARSE_STATE *parse);

#endif  // __USER_PARSER_H__
