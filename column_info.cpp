// viewpoints - interactive linked scatterplots and more.
// copyright 2005 Creon Levit and Paul Gazis, all rights reserved.
//***************************************************************************
// File name: column_info.cpp
//
// Class definitions:
//   Column_Info -- Column info
//
// Classes referenced: none
//
// Required packages
//    FLTK 1.1.6 -- Fast Light Toolkit graphics package
//    FLEWS 0.3 -- Extensions to FLTK 
//    OGLEXP 1.2.2 -- Access to OpenGL extension under Windows
//    GSL 1.6 -- Gnu Scientific Library package for Windows
//    Blitz++ 0.9 -- Various math routines
//
// Compiler directives:
//   May require D__WIN32__ for the C++ compiler
//
// Purpose: Source code for <column_info.h>
//
// Author: Creon Levit    2005-2006
// Modified: P. R. Gazis  11-JUL-2008
//***************************************************************************

// Include the necessary include libraries
#include "include_libraries_vp.h"

// Include globals
#include "global_definitions_vp.h"

// Include associated headers and source code
#include "column_info.h"

//***************************************************************************
// Column_Info::Column_Info() --  Default constructor clears everything.
Column_Info::Column_Info() : 
  label( ""), hasASCII( 0)
{
  ascii_values.erase( ascii_values.begin(), ascii_values.end());
}

//***************************************************************************
// Column_Info::Column_Info( sColumnInfo) --  Invoke default constructor to
// clears everything, then parse strung to load column info.
Column_Info::Column_Info( string sColumnInfo) : 
  label( ""), hasASCII( 0)
{
  ascii_values.erase( ascii_values.begin(), ascii_values.end());
}

//***************************************************************************
// Column_Info::~Column_Info() --  Default destructor clears everything.
Column_Info::~Column_Info()
{
  free();
}

//*****************************************************************
// Column_Info::Column_Info( Column_Info&) -- Copy constructor.
Column_Info::Column_Info( const Column_Info &inputInfo)
{
  this->copy( inputInfo);
}

//***************************************************************************
// Column_Info::~Column_Info() --  Default destructor clears everything.
void Column_Info::free()
{
  label = "";
  hasASCII = 0;
  ascii_values.erase( ascii_values.begin(), ascii_values.end());
}

//*****************************************************************
// Column_Info::copy( inputInfo) -- Copy inputInfo
void Column_Info::copy( const Column_Info &inputInfo)
{
  label = inputInfo.label;
  hasASCII = inputInfo.hasASCII;
  if( ascii_values.size() <= 0)
    ascii_values.erase( ascii_values.begin(), ascii_values.end());
  ascii_values = inputInfo.ascii_values;
}

//*****************************************************************
// Column_Info::operator=() -- Overload the '=' operator as a
// non-static member function.  Deallocate any storage for this
// object, then copy values for the input element.
Column_Info& Column_Info::operator=( const Column_Info &inputInfo)
{
  free();
  this->copy( inputInfo);
  return *this;
}
