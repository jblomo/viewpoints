// viewpoints - interactive linked scatterplots and more.
// copyright 2005 Creon Levit and Paul Gazis, all rights reserved.
//***************************************************************************
// File name: unescape.h
//
// Class definitions: none
//
// Classes referenced:
//   Various FLTK classes
//
// Required packages: none
//
// Compiler directives:
//   Requires WIN32 to be defined
//
// Purpose: Global member function to that returns a string with escaped 
//   character sequences replaced by the actual characters that the escape 
//   codes refer to.
//
// General design philosophy:
//   1) The binary and ASCII read operations are sufficiently different that 
//      they should be handled by entirely separate methods.
//   2) Data are read into global variables.
//   3) Arguments are passed as strings rather than const char* under the 'if 
//      only it were JAVA' approach to C++ style and to take advantage of 
//      STL's powerful string manipulation tools.  Unfortunately, this means 
//      that c_str() must be used to pass some of these strings on to other 
//      methods.
//   4) NOTE: There is considerable duplicate code here.  In particular, the 
//      code to read headers and the calls to Fl_File_Chooser here and in 
//      vp.cpp could be consolidated.
//
// Author: Creon Levit    2007
// Modified: P. R. Gazis  13-JUL-2007
//***************************************************************************

// Protection to make sure this header is not included twice
#ifndef UNESCAPE_H
#define UNESCAPE_H 1

// unescape - return string with escaped character sequences replaced by the 
// actual characters that the escape codes refer to.
char* unescape( char *orig);

#endif   // UNESCAPE_H

