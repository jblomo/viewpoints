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
// Future plans:
//   1) Extend the Column_Info objects to contain label, a 'contains ASCII 
//      values' flag, and a lookup table to relate indices to ASCII values.
//
// Author: Creon Levit    2005-2006
// Modified: P. R. Gazis  23-SEP-2008
//***************************************************************************

// Protection to make sure this header is not included twice
#ifndef DATA_FILE_MANAGER_H
#define DATA_FILE_MANAGER_H 1

// Include the necessary include libraries
#include "include_libraries_vp.h"

// Include globals
#include "global_definitions_vp.h"

// Need access to Column_Info class definitions here so we can declare it as
// a member variable rather than just a pointer
#include "column_info.h"

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
//   copy_state( *dfm) -- Copy state parameters from another object
//
//   serialize( &ar, iFileVersion) -- Perform serialization
//   remove_trivial_columns() -- Remove identical data
//   resize_global_arrays() -- Resize global arrays
//
//   findInputFile() -- Query user to find input file
//   load_data_file( inFileSpec) -- Load and initialize data
//   load_data_file() -- Load and initialize data
//   extract_column_labels( sLine, doDefault) -- Extract column labels
//   extract_column_type( sLine) -- Extract column types
//   remove_column_of_selection_info() -- Remove column of selection info
//   reorder_ascii_values() -- Alphabetize ascii values
//   read_ascii_file_with_headers() -- Read ASCII
//   read_binary_file_with_headers() -- Read binary
//   read_table_from_fits_file() -- Read FITS table extension
//   create_default_data( nvars_in) -- Create default data
//
//   save_data_file( *outFileSpec) -- Save data
//   save_data_file() -- Save data
//   write_ascii_file_with_headers() -- Write ASCII file
//   write_binary_file_with_headers() -- Write binary file
//   write_table_to_fits_file() -- Write FITS table extension
//
//   edit_column_info_i( *o) -- Static wrapper for edit_column_info
//   edit_column_info( *o) -- Maintain Edit Column Labels window
//   refresh_edit_column_info() -- Refresh labels of edit window
//   delete_labels( *o, *u) -- Static callback to delete labels
//   close_edit_labels_window( *o, *u) -- Static callback to close window
//
//   findOutputFile() -- Query user to find output file
//   directory() -- Get pathname
//   directory( sDirectory_in) -- Set pathname
//   input_filespec() -- Get input filespec
//   input_filespec( inFileSpecIn) -- Set input filespec
//   inputFileType() -- Get input file type
//   inputFileType( i) -- Set input file type
//   n_skip_lines() -- Gte number of header lines to be skipped
//   n_skip_lines( i) -- Set number of header lines to be skipped
//   output_filespec() -- Get output filespec
//   output_filespec( outFileSpecIn) -- Set output filespec
//   outputFileType() -- Get output file type
//   outputFileType( i) -- Set output file type
//
//   bad_value_proxy() -- Get bad value proxy
//   bad_value_proxy( f) -- Set bad value proxy
//   column_label( i) -- Get column label
//   delimiter_char() -- Get delimiter character
//   delimiter_char( c) -- Set delimiter character
//   inFileSpec() -- Get input filespec
//   ascii_value( jcol, ivalue) -- ASCII value ival for column jcol
//   ascii_value_index( jcol, sToken) -- Index is sToken in clumn jcol
//   column_major() -- Get column major flag
//   column_major( i) -- Set column major flag
//   do_append() -- Get append flag
//   do_append( i) -- Set append flag
//   do_commented_labels() -- Get 'commented labels' flag
//   do_commented_labels( i) -- Set 'commented labels' flag
//   do_merge() -- Get append flag
//   do_merge( i) -- Set merge flag
//   do_save_old_data() -- Get save old data flag
//   do_save_old_data( i) -- Set save old data flag
//   is_ascii_column( jcol) -- Does this column have ASCII values?
//   is_saved_file() -- Get the 'saved file' flag
//   is_saved_file( i) -- Set the 'saved file' flag
//   maxpoints() -- Get maximum number of rows
//   maxpoints( i) -- Set maximum number of rows
//   maxvars() -- Get maximum number of columns
//   maxvars( i)  -- Get maximum number of columnss
//   n_ascii_columns() -- Get number of ascii columns
//   n_ascii_values( jcol) -- Get number of ASCII values in colum jcol
//   n_data_columns() -- Get number of columns in data file
//   n_data_rows() -- Get number of rows in data file
//   n_vars() -- Get number of columns of data (variables)
//   n_points() -- Get number of rows of data (points)
//   needs_restore_panels() -- Get flag
//   needs_restore_panels( i) -- Set flag
//   read_selection_info() -- Get the 'read selections' flag
//   write_all_data() -- Get the 'write all data' flag
//   selected_data( i)-- Set the 'write all data' flag
//
// Author: Creon Levit    2005-2006
// Modified: P. R. Gazis  23-SEP-2008
//***************************************************************************
class Data_File_Manager
{
  protected:
    // Need this declaration to grant the serialization library access to 
    // private member variables and functions.
#ifdef SERIALIZATION
    friend class boost::serialization::access;
#endif // SERIALIZATION
       
    // When the class Archive corresponds to an output archive, the &
    // operator is defined similar to <<.  Likewise, when the class Archive 
    // is a type of input archive the & operator is defined similar to >>.
    // It is easiest to define this serialize method inline.
#ifdef SERIALIZATION
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
      // inputFileType_ = (isAsciiData==0);
      inputFileType_ = 1-isAsciiData;  // 0,1,2 -> ASCII, binary, FITS
    }
#endif // SERIALIZATION

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
    // that missing values can be specified in asci input file as long as 
    // the delimiter is not whitespace, e.g. 1,2,3,,5,,,8,9,10
    char delimiter_char_;

    // Value assigned to unreadable/nonnumeric/empty/missing values:
    float bad_value_proxy_;

    // I/O parameters and state variables
    int nSkipHeaderLines;
    int inputFileType_, outputFileType_, isAsciiData;
    int doAppend, doMerge, writeAllData_;
    int readSelectionInfo_, writeSelectionInfo_;
    int doCommentedLabels_;
    int isColumnMajor;
    int isSavedFile_;
    
    // Size information
    int maxpoints_, maxvars_;
    int nDataRows_, nDataColumns_;

  public:
    // Member variable to hold column information must be declared static
    // for use with static member functions
    static std::vector<Column_Info> column_info;
    
    Data_File_Manager();
    void initialize();
    void copy_state( Data_File_Manager* dfm);

    // File i/o methods
    int findInputFile();
    int load_data_file( string inFileSpec);
    int load_data_file();
    int extract_column_labels( string sLine, int doDefault);
    void extract_column_types( string sLine);
    int remove_column_of_selection_info();
    int read_ascii_file_with_headers();
    int read_binary_file_with_headers();
    int read_table_from_fits_file();
    void create_default_data( int nvars_in);

    int findOutputFile();
    int save_data_file( string outFileSpec);
    int save_data_file();
    int write_ascii_file_with_headers();
    int write_binary_file_with_headers();
    int write_table_to_fits_file();

    // Column label edit window methods
    static void edit_column_info( Fl_Widget *o);
    void edit_column_info_i( Fl_Widget *o);
    void refresh_edit_column_info();
    static void delete_labels( Fl_Widget *o, void* user_data);
    static void close_edit_labels_window( Fl_Widget *o, void* user_data);
    
    // Access methods
    string directory();
    void directory( string sDirectory_in);
    string input_filespec();
    void input_filespec( string inFileSpecIn);
    int inputFileType();
    void inputFileType( int i);
    int n_skip_lines() { return nSkipHeaderLines;}
    void n_skip_lines( int i) { nSkipHeaderLines = i;}
    string output_filespec();
    void output_filespec( string outFileSpecIn);
    int outputFileType();
    void outputFileType( int i);

    float bad_value_proxy() { return bad_value_proxy_;}
    void bad_value_proxy( float f) { bad_value_proxy_ = f;}
    string column_label( int i) { return column_info[i].label;}
    char delimiter_char() { return delimiter_char_;}
    void delimiter_char( char c) { delimiter_char_ = c;}
    string ascii_value( int jcol, int ival);
    int ascii_value_index( int jcol, string &sToken);
    int column_major() { return isColumnMajor;}
    void column_major( int i) { isColumnMajor = (i==1);}
    int do_append() { return doAppend;}
    void do_append( int i) { doAppend = (i==1);}
    int do_commented_labels() { return doCommentedLabels_;}
    void do_commented_labels( int i) { doCommentedLabels_ = (i==1);}
    int do_merge() { return doMerge;}
    void do_merge( int i) { doMerge = (i==1);}
    int is_ascii_column( int jcol) { return column_info[jcol].hasASCII;}
    int is_saved_file() { return isSavedFile_;}
    void is_saved_file( int i) { isSavedFile_ = i;}
    int maxpoints() { return maxpoints_;}
    void maxpoints( int i);
    int maxvars() { return maxvars_;}
    void maxvars( int i);
    int n_ascii_columns();
    int n_ascii_values( int jcol) { return (column_info[jcol].ascii_values_).size();}
    int n_data_columns() { return nDataColumns_;}
    int n_data_rows() { return nDataRows_;}
    int n_vars();
    int n_points();
    int needs_restore_panels() { return needs_restore_panels_;}
    void needs_restore_panels( int i) {needs_restore_panels_ = i;}
    int read_selection_info() { return readSelectionInfo_;}
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
    static string BINARY_FILE_WITH_ASCII_VALUES_LABEL;
    
    // Define statics to hold tests for bad lines of ASCII data
    static const int MAX_NTESTCYCLES = 1000;
    static const int MAX_NUNREADABLELINES = 200;
};

#endif   // DATA_FILE_MANAGER_H
