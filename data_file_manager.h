// viewpoints - interactive linked scatterplots and more.
// copyright 2005 Creon Levit and Paul Gazis, all rights reserved.
//***************************************************************************
// File name: data_file_manager.h
//
// Class definitions:
//   Data_File_Manager -- Data file manager
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
// Author: Creon Levit    2005-2006
// Modified: P. R. Gazis  14-DEC-2007
//***************************************************************************

// Protection to make sure this header is not included twice
#ifndef DATA_FILE_MANAGER_H
#define DATA_FILE_MANAGER_H 1

// Include the necessary include libraries
#include "include_libraries_vp.h"

// Include globals
#include "global_definitions_vp.h"

//***************************************************************************
// Class: Data_File_Manager
//
// Class definitions:
//   Data_File_Manager -- Data file manager
//
// Classes referenced: none
//
// Purpose: Data file manager to open, read, and write data files
//
// Functions:
//   Data_File_Manager() -- Constructor
//   initialize() -- Initializer
//
//   serialize( &ar, iFileVersion) -- Perform serialization
//   remove_trivial_columns() -- Remove identical data
//   resize_global_arrays() -- Resize global arrays
//
//   findInputFile() -- Query user to find input file
//   load_data_file( inFileSpec) -- Load and initialize data
//   load_data_file() -- Load and initialize data
//   extract_column_labels( sLine, doDefault) -- Extract column labels
//   read_ascii_file_with_headers() -- Read ASCII
//   read_binary_file_with_headers() -- Read binary
//   create_default_data( nvars_in) -- Create default data
//
//   save_data_file( *outFileSpec) -- Save data
//   save_data_file() -- Save data
//   write_ascii_file_with_headers() -- Write ASCII file
//   write_binary_file_with_headers() -- Write binary file
//
//   edit_column_labels_i( *o) -- Static wrapper for edit_column_labels
//   edit_column_labels( *o) -- Maintain Edit Column Labels window
//   refresh_edit_column_labels() -- Refresh labels of edit window
//   delete_labels( *o, *u) -- Static callback to delete labels
//   close_edit_labels_window( *o, *u) -- Static callback to close window
//
//   findOutputFile() -- Query user to find output file
//   directory() -- Get pathname
//   directory( sDirectory_in) -- Set pathname
//   input_filespec() -- Get input filespec
//   input_filespec( inFileSpecIn) -- Set input filespec
//   output_filespec() -- Get output filespec
//   output_filespec( outFileSpecIn) -- Set output filespec
//
//   bad_value_proxy() -- Get bad value proxy
//   bad_value_proxy( f) -- Set bad value proxy
//   delimiter_char() -- Get delimiter character
//   delimiter_char( c) -- Set delimiter character
//   inFileSpec() -- Get input filespec
//   ascii_input() -- Get ASCII input flag
//   ascii_input( i) -- Set ASCII input flag
//   ascii_output() -- Get ASCII output flag
//   ascii_output( i) -- Set ASCII output flag
//   column_major() -- Get column major flag
//   column_major( i) -- Set column major flag
//   do_append() -- Get append flag
//   do_append( i) -- Set append flag
//   do_merge() -- Get append flag
//   do_merge( i) -- Set merge flag
//   needs_restore_panels() -- Get flag
//   needs_restore_panels( i) -- Set flag
//   write_all_data() -- Get the 'write all data' flag
//   selected_data( i)-- Set the 'write all data' flag
//
// Author: Creon Levit    2005-2006
// Modified: P. R. Gazis  14-DEC-2007
//***************************************************************************
class Data_File_Manager
{
  protected:
    // Need this to grant the serialization library access to private member 
    // variables and functions.
    friend class boost::serialization::access;
       
    // When the class Archive corresponds to an output archive, the &
    // operator is defined similar to <<.  Likewise, when the class Archive 
    // is a type of input archive the & operator is defined similar to >>.
    // It is easiest to define this method inline.
    template<class Archive>
    void serialize( Archive & ar, const unsigned int /* file_version */)
    {
      // Embed serialization in a try-catch loop so we can pass exceptions
      try{
        ar & boost::serialization::make_nvp( "dataFileSpec", dataFileSpec);
        ar & boost::serialization::make_nvp( "is_ASCII_data", isAsciiData);
        ar & boost::serialization::make_nvp( "delimiter_char_", delimiter_char_);
        ar & boost::serialization::make_nvp( "doCommentedLabels_", doCommentedLabels_);
        ar & boost::serialization::make_nvp( "nSkipHeaderLines", nSkipHeaderLines);
        ar & boost::serialization::make_nvp( "isColumnMajor", isColumnMajor);
        ar & boost::serialization::make_nvp( "bad_value_proxy_", bad_value_proxy_);
      }
      catch( exception &e) {}
      inFileSpec = dataFileSpec;
      isAsciiInput = isAsciiData;
    }

    void remove_trivial_columns();
    void resize_global_arrays();

    // Buffers to hold filespec, pathname, and selection information
    string sDirectory_, inFileSpec, outFileSpec, dataFileSpec;
    blitz::Array<int,1> read_selected;
    
    // Define pointers to hold Edit Column Labels window.  These must be
    // static to prevent the class from defining new pointers
    static Fl_Window *edit_labels_window;
    static Fl_Check_Browser *edit_labels_widget;
    static int needs_restore_panels_;

    // Delimiter for files, e.g. ',' for CSV.  Default is whitespace.  Note 
    // that missing values can be specified in asci input file as long as the 
    // delimiter is not whitespace, e.g. 1,2,3,,5,,,8,9,10
    char delimiter_char_;

    // Value assigned to unreadable/nonnumeric/empty/missing values:
    float bad_value_proxy_;

    // State variables
    int nSkipHeaderLines;
    int isAsciiInput, isAsciiOutput, isAsciiData;
    int doAppend, doMerge, writeAllData_;
    int readSelectionInfo_, writeSelectionInfo_;
    int doCommentedLabels_;
    int isColumnMajor;
    int isSavedFile_;

  public:
    Data_File_Manager();
    void initialize();

    // File i/o methods
    int findInputFile();
    int load_data_file( string inFileSpec);
    int load_data_file();
    int extract_column_labels( string sLine, int doDefault);
    int read_ascii_file_with_headers();
    int read_binary_file_with_headers();
    void create_default_data( int nvars_in);

    int findOutputFile();
    int save_data_file( string outFileSpec);
    int save_data_file();
    int write_ascii_file_with_headers();
    int write_binary_file_with_headers();

    // Column label edit window methods
    static void edit_column_labels( Fl_Widget *o);
    void edit_column_labels_i( Fl_Widget *o);
    void refresh_edit_column_labels();
    static void delete_labels( Fl_Widget *o, void* user_data);
    static void close_edit_labels_window( Fl_Widget *o, void* user_data);
    
    // Access methods
    string directory();
    void directory( string sDirectory_in);
    string input_filespec();
    void input_filespec( string inFileSpecIn);
    int n_skip_lines() { return nSkipHeaderLines;}
    void n_skip_lines( int i) { nSkipHeaderLines = i;}
    string output_filespec();
    void output_filespec( string outFileSpecIn);

    float bad_value_proxy() { return bad_value_proxy_;}
    void bad_value_proxy( float f) { bad_value_proxy_ = f;}
    char delimiter_char() { return delimiter_char_;}
    void delimiter_char( char c) { delimiter_char_ = c;}
    int ascii_input() { return isAsciiInput;}
    void ascii_input( int i) { isAsciiInput = (i==1);}
    int ascii_output() { return isAsciiOutput;}
    void ascii_output( int i) { isAsciiOutput = (i==1);}
    int column_major() { return isColumnMajor;}
    void column_major( int i) { isColumnMajor = (i==1);}
    int do_append() { return doAppend;}
    void do_append( int i) { doAppend = (i==1);}
    int do_merge() { return doMerge;}
    void do_merge( int i) { doMerge = (i==1);}
    int is_saved_file() { return isSavedFile_;}
    void is_saved_file( int i) { isSavedFile_ = i;}
    int needs_restore_panels() { return needs_restore_panels_;}
    void needs_restore_panels( int i) {needs_restore_panels_ = i;}
    int write_all_data() { return writeAllData_;}
    void write_all_data( int i) { writeAllData_ = (i==1);}
        
    // Define number of points and  number of variables specified by the 
    // command line argument.  NOTE: 0 means read to EOF and/or end of line.
    int npoints_cmd_line;
    int nvars_cmd_line;

    // Define statics to hold header format
    static const int MAX_HEADER_LENGTH;
    static const int MAX_HEADER_LINES;
    static string SELECTION_LABEL;
    
    // Define statics to hold tests for bad lines of ASCII data
    static const int MAX_NTESTCYCLES = 1000;
    static const int MAX_NUNREADABLELINES = 200;
};

#endif   // DATA_FILE_MANAGER_H
