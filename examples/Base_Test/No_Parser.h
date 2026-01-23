/*
  No_Parser.h

  License: MIT. Please see LICENSE.md for more details

  Declare the public data structures and routines for the "No_Parser"
*/

#ifndef __NO_PARSER_H__
#define __NO_PARSER_H__

#include <SparkFun_Extensible_Message_Parser.h> //http://librarymanager/All#SparkFun_Extensible_Message_Parser

//----------------------------------------
// Constants
//----------------------------------------

// No parser externals
extern SEMP_PARSER_DESCRIPTION noParserDescription;

#define NO_PARSER_MINIMUM_BUFFER_SIZE       10  // Exposed for testing only

#endif  // __NO_PARSER_H__
