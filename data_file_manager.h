// viewpoints - interactive linked scatterplots and more.
// copyright 2005 Creon Levit, all rights reserved.
//*****************************************************************
// File name: data_file_manager.h
//
// Class definitions:
//   data_file_manager -- Data file manager
//
// Classes referenced:
//   Various BLITZ templates
//
// Required packages
//   GSL 1.6 -- Gnu Scientific Library package for Windows
//   Blitz++ 0.9 -- Various math routines
//
// Compiler directives:
//   May require D__WIN32__ for the C++ compiler
//
// Purpose: Data file manager for Creon Levit's viewpoints
//
// General design philosophy:
//   1) Data are read into global variables
//
// Author: Creon Levitt   unknown
// Modified: P. R. Gazis  24-APR-2006
//*****************************************************************

// Protection to make sure this header is not included twice
#ifndef DATA_FILE_MANAGER_H
#define DATA_FILE_MANAGER_H 1

// Include the necessary include libraries
#include "include_libraries_vp.h"

// Include globals
#include "global_definitions_vp.h"

//*****************************************************************
// Class: data_file_manager
//
// Class definitions:
//   data_file_manager -- Data file manager
//
// Classes referenced: none
//
// Purpose: Data file manager to open, read, and write data files
//
// Functions:
//   data_file_manager() -- Constructor
//   initialize() -- Initializer
//
//   remove_trivial_columns() -- Remove identical data
//   resize_global_arrays() -- Resize global arrays
//
//   load_data_file( *inFileSpec) -- Load and initialize data
//   read_ascii_file_with_headers( *inFileSpec) -- Read ASCII
//   read_binary_file_with_headers( *inFileSpec) -- Read binary
//   create_default_data( nvars_in) -- Create default data
//   write_binary_file_with_headers() -- Write binary file
//
// Author: Creon Levitt   unknown
// Modified: P. R. Gazis  25-APR-2006
//*****************************************************************
class data_file_manager
{
  protected:
    void remove_trivial_columns();
    void resize_global_arrays();

  public:
    data_file_manager();
    void initialize();

    // I/O methods    
    int load_data_file( char* inFileSpec);
    int read_ascii_file_with_headers( char* inFileSpec);
    int read_binary_file_with_headers( char* inFileSpec);
    void create_default_data( int nvars_in);
    void write_binary_file_with_headers();

    // Define values for file reads.
    int format;  // ASCII or binary
    int ordering;  // Input data ordering
    int nSkipHeaderLines;  // Number of header lines to skip

    // Define number of points specified by the command line 
    // argument.  NOTE: 0 means read to EOF.
    int npoints_cmd_line;

    // Define statics to hold header format
    static const int MAX_HEADER_LENGTH;
    static const int MAX_HEADER_LINES;
};

// Define and set the maximum number of lines and line length for 
// the header block
// const int data_file_manager::MAX_HEADER_LENGTH = 2000;
// const int data_file_manager::MAX_HEADER_LINES = 2000;

#endif   // DATA_FILE_MANAGER_H