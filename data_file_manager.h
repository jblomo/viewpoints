// viewpoints - interactive linked scatterplots and more.
// copyright 2005 Creon Levit, all rights reserved.
//***************************************************************************
// File name: data_file_manager.h
//
// Class definitions:
//   Data_file_manager -- Data file manager
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
// Author: Creon Levit    unknown
// Modified: P. R. Gazis  09-NOV-2006
//***************************************************************************

// Protection to make sure this header is not included twice
#ifndef DATA_FILE_MANAGER_H
#define DATA_FILE_MANAGER_H 1

// Include the necessary include libraries
#include "include_libraries_vp.h"

// Include globals
#include "global_definitions_vp.h"

//***************************************************************************
// Class: Data_file_manager
//
// Class definitions:
//   Data_file_manager -- Data file manager
//
// Classes referenced: none
//
// Purpose: Data file manager to open, read, and write data files
//
// Functions:
//   Data_file_manager() -- Constructor
//   initialize() -- Initializer
//
//   remove_trivial_columns() -- Remove identical data
//   resize_global_arrays() -- Resize global arrays
//
//   make_confirm_window() -- Manage confirmation window
//
//   findInputFile() -- Query user to open input file
//   load_data_file( *inFileSpec) -- Load and initialize data
//   load_data_file() -- Load and initialize data
//   read_ascii_file_with_headers( *inFileSpec) -- Read ASCII
//   read_binary_file_with_headers( *inFileSpec) -- Read binary
//   create_default_data( nvars_in) -- Create default data
//   write_ascii_file_with_headers() -- Write ASCII file
//   write_binary_file_with_headers() -- Write binary file
//
//   directory() -- Get pathname
//   directory( sPathnameIn) -- Set pathname
//   input_filespec() -- Get input filespec
//   input_filespec( inFileSpecIn) -- Set input filespec
//
//   inFileSpec() -- Get input filespec
//   ascii_input() -- Get ASCII input flag
//   ascii_input( i) -- Set ASCII input flag
//   ascii_output() -- Get ASCII output flag
//   ascii_output( i) -- Set ASCII output flag
//   selected_data() -- Get 'use selected data' flag
//   selected_data( i)-- Set 'use selected data' flag
//   column_major() -- Get column major flag
//   column_major( i) -- Set column major flag
//
// Author: Creon Levit   unknown
// Modified: P. R. Gazis  09-NOV-2006
//***************************************************************************
class Data_file_manager
{
  protected:
    void remove_trivial_columns();
    void resize_global_arrays();

    // Create and manage confirmation window
    void make_confirm_window( const char* output_file_name);
    enum enumConfirmResult { CANCEL_FILE = 0, NO_FILE, YES_FILE} confirmResult;

    // Buffers to hold filespec and pathname
    string inFileSpec;
    string sPathname;
    
    // State variables
    int nSkipHeaderLines;
    int isAsciiInput, isAsciiOutput;
    int useSelectedData;
    int isColumnMajor;

  public:
    Data_file_manager();
    void initialize();

    // File i/o methods
    int findInputFile();
    int load_data_file( string inFileSpecIn);
    int load_data_file();
    int read_ascii_file_with_headers();
    int read_binary_file_with_headers();
    void create_default_data( int nvars_in);
    void write_ascii_file_with_headers();
    void write_binary_file_with_headers();

    // Access methods
    string directory();
    void directory( string sPathnameIn);
    string input_filespec();
    void input_filespec( string inFileSpecIn);
    int n_skip_header_lines() { return nSkipHeaderLines;}
    void n_skip_header_lines( int i) { nSkipHeaderLines = i;}

    int ascii_input() { return isAsciiInput;}
    void ascii_input( int i) { isAsciiInput = (i==1);}
    int ascii_output() { return isAsciiOutput;}
    void ascii_output( int i) { isAsciiOutput = (i==1);}
    int selected_data() { return useSelectedData;}
    void selected_data( int i) { useSelectedData = (i==1);}
    int column_major() { return isColumnMajor;}
    void column_major( int i) { isColumnMajor = (i==1);}
    

    // Define number of points and  number of variables specified by the 
    // command line argument.  NOTE: 0 means read to EOF and/or end of line.
    int npoints_cmd_line;
    int nvars_cmd_line;

    // Define pointers to hold file confirmation window
    Fl_Window *confirm_window;

    // Define statics to hold header format
    static const int MAX_HEADER_LENGTH;
    static const int MAX_HEADER_LINES;
    
    // Define statics to hold tests for bad lines of ASCII data
    static const int MAX_NTESTCYCLES = 1000;
    static const int MAX_NUNREADABLELINES = 200;
};

#endif   // DATA_FILE_MANAGER_H
