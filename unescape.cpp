// viewpoints - interactive linked scatterplots and more.
// copyright 2005 Creon Levit, all rights reserved.
//***************************************************************************
// File name: unescape.cpp
//
// Class definitions: none
//
// Classes referenced:
//   Various FLTK classes
//
// Required packages: none
//
// Compiler directives:
//   May require D__WIN32__ for the C++ compiler
//
// Purpose: Source code for <unescape.h>
//
// Author: Creon Levit    2007
// Modified: P. R. Gazis  12-JUL-2007
//***************************************************************************

#include <ctype.h>
#include <locale.h>
#include <string.h>
#include <unistd.h>

// Define script to identify ISO digit
#define ISODIGIT(c)  ((int)(c) >= '0' && (int)(c) <= '7')

//*****************************************************************************
// unescape( orig) - Return a string with escaped character sequences replaced 
// by the actual characters that the escape codes refer to.
char* unescape( char *orig)
{
  // Loop: Examine pointers to successive words of the string
  char c, *cp, *result = orig;
  int i;
  for( cp = orig; (*orig = *cp); cp++, orig++) {

    // If this is not an escape sequence, keep going
    if (*cp != '\\') continue;

    // Check for different escape sequences
    switch (*++cp) {
    case 'a':  /* alert (bell) */
      *orig = '\a';
      continue;
    case 'b':  /* backspace */
      *orig = '\b';
      continue;
    case 'e':  /* escape */
      *orig = '\e';
      continue;
    case 'f':  /* formfeed */
      *orig = '\f';
      continue;
    case 'n':  /* newline */
      *orig = '\n';
      continue;
    case 'r':  /* carriage return */
      *orig = '\r';
      continue;
    case 't':  /* horizontal tab */
      *orig = '\t';
      continue;
    case 'v':  /* vertical tab */
      *orig = '\v';
      continue;
    case '\\':  /* backslash */
      *orig = '\\';
      continue;
    case '\'':  /* single quote */
      *orig = '\'';
      continue;
    case '\"':  /* double quote */
      *orig = '"';
      continue;
    case '0':
    case '1':
    case '2':
    case '3':  /* octal */
    case '4':
    case '5':
    case '6':
    case '7':  /* number */
      for( i = 0, c = 0;
           ISODIGIT((unsigned char)*cp) && i < 3;
           i++, cp++) {
        c <<= 3;
        c |= (*cp - '0');
      }
      *orig = c;
      continue;
    case 'x':  /* hexidecimal number */
      cp++;  /* skip 'x' */
      for (i = 0, c = 0;
           isxdigit((unsigned char)*cp) && i < 2;
           i++, cp++) {
        c <<= 4;
        if (isdigit((unsigned char)*cp))
          c |= (*cp - '0');
        else
          c |= ((toupper((unsigned char)*cp) -
              'A') + 10);
      }
      *orig = c;
      continue;
    default:
      --cp;
      break;
    }
  }

  // Return result and quit
  return (result);
}

