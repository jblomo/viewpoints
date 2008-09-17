// viewpoints - interactive linked scatterplots and more.
// copyright 2005 Creon Levit and Paul Gazis, all rights reserved.
//***************************************************************************
// File name: column_info.h
//
// Class definitions:
//   Column_Info -- Colum,n information
//
// Classes referenced:
//   May require various BLITZ templates
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
// Purpose: Member class of Data_File_Manager to generate, manage, and 
//   store column information
//
// General design philosophy:
//   1) This is inteded for use as a mamber class of data_file_manager
//
// Author: Creon Levit    2005-2006
// Modified: P. R. Gazis  16-SEP-2008
//***************************************************************************

// Protection to make sure this header is not included twice
#ifndef COLUMN_INFO_H
#define COLUMN_INFO_H 1

// Include the necessary include libraries
#include "include_libraries_vp.h"

// Include globals
#include "global_definitions_vp.h"

//***************************************************************************
// Class: Column_Info
//
// Class definitions:
//   Column_Info
//
// Classes referenced: none
//
// Purpose: Member class of Data_File_Manager to generate, manage, and 
//   store column information
//
// Functions:
//   Column_Info() -- Default Constructor
//   Column_Info( sColumnInfo) -- Constructor
//   ~Column_Info() -- Destructor
//   Column_Info( Column_Info&) -- copy constructor
//   Column_Info& operator=( Column_Info &Column_Info) -- '='
//
//   free() -- Clear buffers
//   copy( inputInfo) -- Copy column info
//
//   add_value( string sToken) -- Update list of ASCII values
//   add_info_and_update_data( j, old_info) -- Add info and update data
//   update_ascii_values_and_data() -- Update ascii_values table and data
//
//   index() -- Get column index for this column
//   index( j) -- Set column index for this column
//   ascii_value( j) -- Get ASCII value for point j
//
// Author: Creon Levit    2005-2006
// Modified: P. R. Gazis  16-SEP-2008
//***************************************************************************
class Column_Info
{
  protected:
    int jvar_;

  public:
    Column_Info();
    Column_Info( string sColumnInfo);
    ~Column_Info();
    Column_Info( const Column_Info&);  // Copy constructor
    Column_Info& operator=( const Column_Info &Column_Info);  // '='

    // Functions to initialize, copy, and deallocate lists
    void free();
    void copy( const Column_Info& inputInfo);

    // Functions to update lists
    int add_value( string sToken);
    int add_info_and_update_data( int j, Column_Info &old_info);
    int update_ascii_values_and_data( int j);

    // Access functions
    int index() { return jvar_;}
    void index( int j) { jvar_ = j;}
    string ascii_value( int j);

    // Define buffers to hold label and ASCII values    
    string label;
    int hasASCII;
    std::map<std::string,int> ascii_values_;
    
    // Define buffers to hold vector information for FITS files
    string vectorLabel;
    int isVector;
    int vectorIndex;
    
    // Define buffers to hold the data
    blitz::Array<float,1> points;  // main data array
    blitz::Array<int,1> ranked_points;   // data, ranked, as needed.
    int isRanked;    // flag: 1->column is ranked, 0->not
};

#endif   // COLUMN_INFO_H
