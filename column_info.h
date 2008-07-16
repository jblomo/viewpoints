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
// Modified: P. R. Gazis  16-JUL-2008
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
//   update_ascii_values_and_data() -- Update ascii_values table and data
//
//
//   index() -- Access function to get index
//
// Author: Creon Levit    2005-2006
// Modified: P. R. Gazis  16-JUL-2008
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
    int update_ascii_values_and_data( int j);

    // Acces functions
    int index() { return jvar_;}
    string ascii_value( int j);
    
    string label;
    int hasASCII;
    std::map<std::string,int> ascii_values_;
};

#endif   // COLUMN_INFO_H
