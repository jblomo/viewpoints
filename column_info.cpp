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
// Modified: P. R. Gazis  17-JUL-2008
//***************************************************************************

// Include the necessary include libraries
#include "include_libraries_vp.h"

// Include globals
#include "global_definitions_vp.h"

// Include associated headers and source code
#include "column_info.h"

//***************************************************************************
// Column_Info::Column_Info() --  Default constructor clears everything.
Column_Info::Column_Info() : jvar_( 0), label( ""), hasASCII( 0)
{
  ascii_values_.erase( ascii_values_.begin(), ascii_values_.end());
}

//***************************************************************************
// Column_Info::Column_Info( sColumnInfo) --  Invoke default constructor to
// clear everything, then parse header string from binary file to load 
// column info.
Column_Info::Column_Info( string sColumnInfo) : 
  jvar_( 0), label( ""), hasASCII( 0)
{
  ascii_values_.erase( ascii_values_.begin(), ascii_values_.end());
  // Code to parse string has yet to be written
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
  jvar_ = 0;
  label = "";
  hasASCII = 0;
  ascii_values_.erase( ascii_values_.begin(), ascii_values_.end());
}

//***************************************************************************
// Column_Info::copy( inputInfo) -- Copy inputInfo
void Column_Info::copy( const Column_Info &inputInfo)
{
  jvar_ = inputInfo.jvar_;
  label = inputInfo.label;
  hasASCII = inputInfo.hasASCII;
  if( ascii_values_.size() <= 0)
    ascii_values_.erase( ascii_values_.begin(), ascii_values_.end());
  ascii_values_ = inputInfo.ascii_values_;
}

//***************************************************************************
// Column_Info::operator=() -- Overload the '=' operator as a non-static 
// member function.  Deallocate any storage for this object, then copy 
// values for the input element.
Column_Info& Column_Info::operator=( const Column_Info &inputInfo)
{
  free();
  this->copy( inputInfo);
  return *this;
}

//***************************************************************************
// Column_Info::add_value( sToken) -- Update list of ASCII values and return 
// the order in which a token appeared.
int Column_Info::add_value( string sToken)
{
  // MCL XXX Can't we do this whole thing with operator[] ? 

 // Determine if this value has occurred before
  map<string,int>::iterator iter = ascii_values_.find( sToken);

  // Insert it if it's new.  Does insert know how to order strings?
  if( iter != ascii_values_.end()) {
    return iter->second;
  }
  else {
    int nValues = ascii_values_.size();
    //ascii_values_.insert(
    //  ascii_values_.end(), 
    //  map<string,int>::value_type(sToken,nValues));
    ascii_values_.insert( map<string,int>::value_type(sToken,nValues));
    return nValues;
  }
}

//***************************************************************************
// Column_Info::update_ascii_values_and_data( int j) -- Update the lookup 
// table to index ascii_values in alphabetical order and update the common 
// data array for this column.
int Column_Info::update_ascii_values_and_data( int j)
{
  // Make sure we have the right index and look-up table
  jvar_ = j;
  if( hasASCII == 0) return -1;

  // Loop: Create and load a map to do the conversion
  map<int,int> conversion;
  int iAlpha = 0;
  for(
    map<string,int>::iterator iter = ascii_values_.begin();
    iter != ascii_values_.end(); iter++)
  {
    int iOrder = iter->second;
    conversion.insert( map<int,int>::value_type( iOrder, iAlpha));
    iter->second = iAlpha;
    iAlpha++;
  }

  // Loop: Do the conversion for this column.  WARNING: It is the 
  // user's responsibility to make sure the column index is correct!
  for( int i=0; i<npoints; i++) { 
    points(jvar_,i) = (conversion.find( (int) points(jvar_,i)))->second;
  }
  
  // Report success
  return jvar_;
}

//***************************************************************************
// Column_Info::add_info_and_update_data( j, old_info) -- Add old column 
// info and update the common data array for this column.  Note that this
// is Way Tricky!  The current column info and common data array will be 
// associated with new data that mst be modified while the old data remains 
// the same.
int Column_Info::add_info_and_update_data( int j, Column_Info &old_info)
{
  // If this is not an ASCII column then quit
  if( hasASCII == 0 || old_info.hasASCII == 0) return 0;
  
  // Define a map to convert values
  map<int,int> conversion_table;

  // Loop: Examine successive keys (ASCII values) in the current (e.g., new) 
  // Column_Info object.  If this key occurs in the old object, change the
  // associated value to the old value.  If it doesn't occur, increment the 
  // number of keys and use this as its value.
  int nAllKeys = (old_info.ascii_values_).size();
  for(
    map<string,int>::iterator iter = ascii_values_.begin();
    iter != ascii_values_.end(); iter++)
  {
    string sThisKey = iter->first;
    int iThisValue = iter->second;
    if( (old_info.ascii_values_).find(sThisKey) != 
        (old_info.ascii_values_).end()) {
      int iOldValue = old_info.ascii_values_[iter->first];
      iter->second = iOldValue;
      ascii_values_[sThisKey] = iOldValue;
      conversion_table.insert( map<int,int>::value_type( iThisValue, iOldValue));
    }
    else {
      iter->second = nAllKeys;
      ascii_values_[sThisKey] = nAllKeys;
      conversion_table.insert( map<int,int>::value_type( iThisValue, nAllKeys));
      nAllKeys++;
    }
  }

  // Loop: Go through the old Column_Info object one more time to make sure 
  // we got everything
  for(
    map<string,int>::iterator old_iter = (old_info.ascii_values_).begin();
    old_iter != (old_info.ascii_values_).end(); old_iter++)
  {
    string sOldKey = old_iter->first;
    int iOldValue = old_iter->second;
    ascii_values_[sOldKey] = iOldValue;
  }

  // Loop: Do the conversion for this column.  WARNING: It is the 
  // user's responsibility to make sure the column index is correct!
  for( int i=0; i<npoints; i++) { 
    points(jvar_,i) = conversion_table[ (int) points(jvar_,i)];
  }
  
  // Return number of ASCII vales
  return ascii_values_.size();
}

//***************************************************************************
// Column_Info::ascii_value( iValue) -- Protect against bad indices, then
// loop through map of ASCII values to get value for this index.  
string Column_Info::ascii_value( int iValue)
{
  if( 0>iValue || iValue >= ascii_values_.size()) return string( "BAD_INDEX_VP");
  map<string,int>::iterator iter = ascii_values_.begin();
  for( int i=0; i<iValue; i++) {
    iter++;
    if( iter == ascii_values_.end()) break;
  }
  return iter->first;
}
