// viewpoints - interactive linked scatterplots and more.
// copyright 2005 Creon Levit and Paul Gazis, all rights reserved.
//***************************************************************************
// File name: data_file_manager.cpp
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
// Purpose: Source code for <data_file_manager.h>
//
// Author: Creon Levit    2005-2006
// Modified: P. R. Gazis  24-JUL-2008
//***************************************************************************

// Include the necessary include libraries
#include "include_libraries_vp.h"

// Include globals
#include "global_definitions_vp.h"

// Include associated headers and source code
#include "data_file_manager.h"
#include "column_info.h"
#include "plot_window.h"

// These includes should not be necessary and have been commented out
// #include "Vp_File_Chooser.H"   // PRG's new file chooser
// #include "Vp_File_Chooser.cpp"   // PRG's new file chooser

// Be sure to define the static data member to hold column info
std::vector<Column_Info> Data_File_Manager::column_info;

// Set static data members for class Data_File_Manager::
string Data_File_Manager::SELECTION_LABEL = "SELECTION_BY_VP";
Fl_Window* Data_File_Manager::edit_labels_window = NULL;
Fl_Check_Browser* Data_File_Manager::edit_labels_widget = NULL;
int Data_File_Manager::needs_restore_panels_ = 0;

// Define and set maximums length of header lines and number of lines in the 
// header block
const int Data_File_Manager::MAX_HEADER_LENGTH = MAXVARS*100;
const int Data_File_Manager::MAX_HEADER_LINES = 2000;

const bool include_line_number = false; // MCL XXX This should be an option

//***************************************************************************
// Data_File_Manager::Data_File_Manager() -- Default constructor, calls the
// initializer.
Data_File_Manager::Data_File_Manager() : delimiter_char_( ' '), 
  bad_value_proxy_( 0.0), isAsciiInput( 1), isAsciiOutput( 0), 
  readSelectionInfo_( 0), doAppend( 0), doMerge( 0), 
  writeAllData_( 1), writeSelectionInfo_( 0), doCommentedLabels_( 0),
  isColumnMajor( 0), isSavedFile_( 0)
{
  sDirectory_ = ".";  // Default pathname
  initialize();
}

//***************************************************************************
// Data_File_Manager::initialize() -- Reset control parameters.
void Data_File_Manager::initialize()
{
  // Set default values for file reads.
  delimiter_char_ = ' ';
  bad_value_proxy_ = 0.0;
  isAsciiInput = 1;
  isAsciiOutput = 0;
  isAsciiData = isAsciiInput;
  doAppend = 0;
  doMerge = 0;
  readSelectionInfo_ = 1;
  writeAllData_ = 1;
  writeSelectionInfo_ = 0;
  isSavedFile_ = 0;
  needs_restore_panels_ = 0;

  isColumnMajor = 1;
  nSkipHeaderLines = 0;  // Number of header lines to skip
  // sDirectory_ = ".";  // Default pathname -- NOT NEEDED!
  inFileSpec = "";  // Default input filespec
  outFileSpec = "";
  dataFileSpec = "";

  // Initialize the number of points and variables specified by the command 
  // line arguments.  NOTE: 0 means read to EOF or end of line.
  npoints_cmd_line = 0;
  nvars_cmd_line = 0;
  
  // Initialize number of points and variables
  npoints = MAXPOINTS;
  nvars = MAXVARS;
}

//***************************************************************************
// Data_File_Manager::findInputFile() -- Query user to find the input file.
// Class Vp_File_Chooser is used in preference to a Vp_File_Chooser method 
// to obtain access to member functions such as directory() and to allow the 
// possibility of a derived class with additional controls in the window.  
// Returns 0 if successful.  
int Data_File_Manager::findInputFile()
{
  // Print empty line to console for reasons of aesthetics
  cout << endl;

  // Generate text, file extensions, etc, for this file type.  XXX PRG: Most 
  // of this should be moved to Vp_File_Chooser
  string title;
  string pattern;
  if( isAsciiInput) {
    title = "Open data file";
    pattern = "*.{txt,lis,asc}\tAll Files (*)";
  }
  else {
    title = "Open data file";
    pattern = "*.bin\tAll Files (*)";
  }

  // Initialize read status and filespec.  NOTE: cInFileSpec is defined as
  // const char* for use with Vp_File_Chooser, which means it could be 
  // destroyed by the relevant destructors!
  const char *cInFileSpec = sDirectory_.c_str();
  
  // Instantiate and show an Vp_File_Chooser widget.  NOTE: The pathname must
  // be passed as a variable or the window will begin in some root directory.
  Vp_File_Chooser* file_chooser =
    new Vp_File_Chooser( 
      cInFileSpec, pattern.c_str(), Vp_File_Chooser::SINGLE, title.c_str());
  file_chooser->isAscii( isAsciiInput);
  
  // Comment this out to use the value file_chooser provides
  // file_chooser->doCommentedLabels( doCommentedLabels_);

  // Loop: Select fileSpecs until a non-directory is obtained.  NOTE: If all
  // goes well, this should be handled by the file_chooser object
  while( 1) {
    if( cInFileSpec != NULL) file_chooser->directory( cInFileSpec);

    // Loop: wait until the file selection is done
    file_chooser->show();
    while( file_chooser->shown()) Fl::wait();
    cInFileSpec = file_chooser->value();   

    // If no file was specified then quit
    if( cInFileSpec == NULL) {
      cerr << "Data_File_Manager::findInputFile: "
           << "No input file was specified" << endl;
      break;
    }

    // In FLTK 1.1.7 under Windows, the fl_filename_isdir method doesn't work, 
    // so try to open this file to see if it is a directory.  If it is, set 
    // the pathname and continue.  Otherwise merely update the pathname.  
    FILE* pFile = fopen( cInFileSpec, "r");
    if( pFile == NULL) {
      file_chooser->directory( cInFileSpec);
      directory( (string) cInFileSpec);
      continue;
    }
    else {
      directory( (string) file_chooser->directory());
    }

    fclose( pFile);
    break;         
  } 

  // If no file was specified then report, deallocate the Vp_File_Chooser 
  // object, and quit.
  if( cInFileSpec == NULL) {
    cerr << "Data_File_Manager::findInputFile: "
         << "No input file was specified" << endl;
    delete file_chooser;  // WARNING! Destroys cInFileSpec!
    return -1;
  }

  // Query Vp_File_Chooser object to get the file type, delimiter character,
  // and usage of a comment character with the column label information
  if( file_chooser->isAscii() != 0) isAsciiInput = 1;
  else isAsciiInput = 0;
  delimiter_char_ = file_chooser->delimiter_char();
  if( file_chooser->doCommentedLabels() != 0) doCommentedLabels_ = 1;
  else doCommentedLabels_ = 0;
  
  // Load the inFileSpec string
  inFileSpec.assign( (string) cInFileSpec);
  if( isAsciiInput == 1) 
    cout << "Data_File_Manager::inputFile: Reading ASCII data from <";
  else 
    cout << "Data_File_Manager::findInputFile: Reading binary data from <";
  cout << inFileSpec.c_str() << ">" << endl;

  // Deallocate the file_chooser object
  delete file_chooser;  // WARNING! This destroys cInFileSpec!

  // Perform partial initialization and return success
  // nSkipHeaderLines = 1;
  nSkipHeaderLines = 0;
  npoints_cmd_line = 0;
  nvars_cmd_line = 0;
  npoints = MAXPOINTS;
  nvars = MAXVARS;

  return 0;
}

//***************************************************************************
// Data_File_Manager::load_data_file( inFileSpec) -- Copy the input filespec, 
// then invoke load_data_file to load this file.
int Data_File_Manager::load_data_file( string inFileSpec) 
{
  input_filespec( inFileSpec);
  return load_data_file();
}

//***************************************************************************
// Data_File_Manager::load_data_file() -- Read an ASCII or binary data file, 
// resize arrays to allocate meomory.  Returns 0 if successful.
int Data_File_Manager::load_data_file() 
{
  // PRG XXX: Would it be possible or desirable to examine the file directly 
  // here to determine or verify its format?
  if( inFileSpec.length() <= 0) {
    cout << "Data_File_Manager::load_data_file: "
         << "No input file was specified" << endl;
    return -1;
  }
  
  // If this is an append or merge operation, save the existing data and 
  // column labels in temporary buffers
  int old_npoints=0, old_nvars=0;
  blitz::Array<float,2> old_points;
  std::vector<Column_Info> old_column_info; 
  if( doAppend > 0 || doMerge > 0) {
    old_nvars = points.rows();
    old_npoints = points.columns();
    old_points.resize( points.shape());
    old_points = points;
    old_column_info = column_info;
  }

  // Initialize READ_SELECTED here
  // read_selected.resize( npoints);
  read_selected.resize( MAXPOINTS);
  read_selected = 0;
  
  // Read data file.If there was a problem, create default data to prevent 
  // a crash, then quit before something terrible happens!  NOTE: The read
  // methods load selection information, but don't resize read_selected;
  cout << "Data_File_Manager::load_data_file: Reading input data from <"
       << inFileSpec.c_str() << ">" << endl;
  int iReadStatus = 0;
  if( isAsciiInput == 0) iReadStatus = read_binary_file_with_headers();
  else iReadStatus = read_ascii_file_with_headers();
  if( iReadStatus != 0) {
    cout << "Data_File_Manager::load_data_file: "
         << "Problems reading file <" << inFileSpec.c_str() << ">" << endl;
    create_default_data( 4);
    return -1;
  }
  else
    cout << "Data_File_Manager::load_data_file: Finished reading file <" 
         << inFileSpec.c_str() << ">" << endl;
  
  // Resize the READ_SELECTED array here
  read_selected.resizeAndPreserve( npoints);  

  // Remove trivial columns
  // XXX this should be a command line and Tool menu option with default 
  // OFF, since it can take both time and memory
  remove_trivial_columns();

  // If only one or fewer records are available, generate default data to
  // prevent a crash, then quit before something terrible happens!
  if( nvars <= 1 || npoints <= 1) {
    cerr << " -WARNING: Insufficient data, " << nvars << "x" << npoints
         << " samples.\nCheck delimiter character." << endl;
    string sWarning = "";
    sWarning.append( "WARNING: Insufficient number of attributes or samples\n.");
    sWarning.append( "Check delimiter setting.  Generating default data");
    make_confirmation_window( sWarning.c_str(), 1);
    create_default_data( 4);
    return -1;
  }
  else {
    cout << "Data_File_Manager::load_data_file: Loaded " << npoints
         << " samples with " << nvars << " fields" << endl;
  }
  
  // MCL XXX Now that we're done reading, we can update nvars to count possible
  // additional program-generated variables (presently only the line number).
  if( include_line_number) nvars = nvars+1;

  // Compare array sizes and finish the append or merge operation
  if( doAppend > 0 | doMerge > 0) {

    // If array sizes aren't consistent, restore the old data and column 
    // labels.  Otherwise, reverse old and new arrays along the relevant
    // dimensions, copy the old data to the new array, reverse the new array
    // again, and copy new column labels if this was a merge operation.
    if( ( doAppend > 0 && nvars != old_nvars) ||
        ( doMerge > 0 && npoints != old_npoints)) {
      make_confirmation_window( "Array sizes don't match.\nRestore old data", 1);

      points.resize( old_points.shape());
      points = old_points;
      nvars = points.rows();
      npoints = points.columns();
      column_info = old_column_info;
    }
    else if( doAppend > 0) {
         
      // Current (e.g., new) column info and lookup table indices in the
      // current (new) common data array must be revised first, before the 
      // current and old data arrays are appended.
      for( int j=0; j<nvars; j++) {
        column_info[j].add_info_and_update_data( j, old_column_info[j]);
      }

      // Enlarge buffer with old data to make space for current data
      int all_npoints = npoints + old_npoints;
      points.resizeAndPreserve( nvars, npoints);
      old_points.resizeAndPreserve( nvars, all_npoints);

      // Append current data to old data
      old_points(
        blitz::Range( 0, nvars-1), 
        blitz::Range( old_npoints, all_npoints-1)) = points;
      points.resize( old_points.shape());

      // Move combined data set back to the current data buffer
      points = old_points;
      npoints = all_npoints;
      
      // Loop: Examine the vector of Column_Info objects to alphabetize 
      // ASCII values and renumber the data.
      int nReordered = 0;
      for( int j=0; j<nvars; j++) {
        if( column_info[j].update_ascii_values_and_data(j) >=0) nReordered++;
      }
    }
    else {
      int all_nvars = nvars + old_nvars;
      points.reverseSelf( blitz::firstDim);
      old_points.reverseSelf( blitz::firstDim);
      points.resizeAndPreserve( all_nvars, npoints);

      points( 
        blitz::Range( nvars, all_nvars-1), 
        blitz::Range( 0, npoints-1)) = old_points;
      points.reverseSelf( blitz::firstDim);
      nvars = all_nvars;

      // Add new columns to list of column labels
      old_column_info.pop_back();
      for( unsigned int i=0; i<column_info.size(); i++) {
        old_column_info.push_back( column_info[ i]);
      }
      column_info = old_column_info;
    }

    // Free memory in case this isn't handled by the compiler.
    old_points.free();
    old_column_info.erase( old_column_info.begin(), old_column_info.end());
  }

  // If we read a different number of points then we anticipated, we resize 
  // and preserve the main data array.  Note this can take lot of time and memory
  // temporarily.  XXX it would be better to handle the growth/shrinkage of this
  // array while reading.
  if( npoints != npoints_cmd_line)
    points.resizeAndPreserve( nvars, npoints);

  // KLUDGE: If this is a merge, save current selection information in 
  // READ_SELECTED so it won't be destroyed by the resize_global_arrays().
  if( doMerge) {
    read_selected( blitz::Range( 0, npoints-1)) = 
      selected( blitz::Range( 0, npoints-1));
  }

  // Now that we know the number of variables and points we've read, we can
  // allocate and/or reallocateResize the other global arrays.  NOTE: This 
  // will invoke reset_selection_arrays()
  resize_global_arrays();

  // If this selection information was found or this was a merge operation, 
  // load saved selections in READ_SELECTED into the SELECTED array.
  if( readSelectionInfo_ || doMerge) {
    selected( blitz::Range( 0, npoints-1)) = 
      read_selected( blitz::Range( 0, npoints-1));
  }
  read_selected.free();

  // If selection information was found, edit the vector of column labels to
  // remove the selection label, SELECTION_LABEL.
  if( readSelectionInfo_) {
    vector<Column_Info>::iterator pTarget = column_info.end();
    pTarget--;
    pTarget--;
    if( include_line_number) pTarget--;
    column_info.erase( pTarget);
  }

  // Refresh edit window, if it exists.
  refresh_edit_column_info();

  // Update dataFileSpec, set saved file flag, and report success
  dataFileSpec = inFileSpec;
  isAsciiData = isAsciiInput;
  if( doAppend > 0 | doMerge > 0) isSavedFile_ = 0;
  else isSavedFile_ = 1;
  return 0;
}

//***************************************************************************
// Data_File_Manager::extract_column_labels( sLine, doDefault) -- Generate 
// or extract column labels and selection flag and store them in the static 
// member vector of Column_Info objects
int Data_File_Manager::extract_column_labels( string sLine, int doDefault)
{
  // Initialize the satic member vector of Column_Info objects
  int nLabels = 0;
  nvars = 0;
  column_info.erase( column_info.begin(), column_info.end());

  // If requested, examine the line, count the number of values, and use this
  // information to generate a set of default column labels.
  if( doDefault != 0) {
    
    // If the delimiter character is not a tab, replace all tabs in the
    // LINE string with spaces
    if( delimiter_char_ != '\t') replace( sLine.begin(), sLine.end(), '\t', ' ');

    // Loop: Insert the LINE string into a stream, define a buffer, read and
    // count successive tokens, generate default column labels and load the 
    // vector of Column_Info objects, and report results.  NOTE: whitespace-
    // and character-delimited files must be handled differently
    std::stringstream ss( sLine);
    std::string buf;
    Column_Info column_info_buf;
    if( delimiter_char_ == ' ') {
      while( ss >> buf) {
        nvars++;
        char cbuf[ 80];
        (void) sprintf( cbuf, "%d", nvars);
        buf = "Column_";
        buf.append( cbuf);
        column_info_buf.label = string( buf);
        column_info.push_back( column_info_buf);
      }
    }
    else { 
      while( getline( ss, buf, delimiter_char_)) {
        nvars++;
        char cbuf[ 80];
        (void) sprintf( cbuf, "%d", nvars);
        buf = "Column_";
        buf.append( cbuf);
        column_info_buf.label = string( buf);
        column_info.push_back( column_info_buf);
      }
    }
    nvars = column_info.size();
    cout << " -Generated " << nvars 
         << " default column labels." << endl;
  }

  // ...otherwise, examine the input line to extract column labels.
  else {

    // Discard the leading comment character, if any, of the SLINE string.
    // The rest of the line is assumed to contain column labels
    if( sLine.find_first_of( "!#%") == 0) sLine.erase( 0, 1);

    // Loop: Insert the SLINE string into a stream, define a buffer, read 
    // successive labels into the buffer, then load them into the vector of 
    // Column_Info objects, and report results.  NOTE: whitespace- and 
    // character-delimited labels must be handled differently.  Also, it is
    // necessary to trim whitespace and verify character-delimited labels.
    std::stringstream ss( sLine);
    std::string buf;
    Column_Info column_info_buf;
    if( delimiter_char_ == ' ')
      while( ss >> buf) {
        column_info_buf.label = string( buf);
        column_info.push_back( column_info_buf);
      }
    else {
      while( getline( ss, buf, delimiter_char_)) {
        string::size_type notwhite = buf.find_first_not_of( " ");
        buf.erase( 0, notwhite);
        notwhite = buf.find_last_not_of( " ");
        buf.erase( notwhite+1);
        if( buf.size() <= 0) buf = "Dummy";
        column_info_buf.label = string( buf);
        column_info.push_back( column_info_buf);
      }
    }
    nvars = column_info.size();
    cout << " -Extracted " << nvars << " column labels." << endl;
  }

  // If there were more than NVARS_CMD_LINE variables in the file, truncate 
  // the vector of Column_Info objects, reset NVARS, and warn the user.
  if( nvars_cmd_line > 0 && nvars > nvars_cmd_line) {
    column_info.erase( column_info.begin()+nvars_cmd_line, column_info.end());
    nvars = column_info.size();
    cerr << " -WARNING: Too many variables, truncated list to " << nvars 
         << " column labels." << endl;
  }

  // Examine the number of Column_Info objects that remain.  If it is too 
  // low or high, report error, close input file, and quit.  Otherwise 
  // report success.
  if( nvars <= 1) {
    cerr << " -WARNING, insufficient number of columns, "
         << "check for correct delimiter character"
         << endl;
    make_confirmation_window( 
      "WARNING: Insufficient number of columns.  Check delimiter setting.", 1);
    return -1;
  }
  if( nvars > MAXVARS) {
    cerr << " -WARNING, too many data columns, "
         << "increase MAXVARS and recompile"
         << endl;
    make_confirmation_window( 
      "WARNING: Too many data columns.", 1);
    return -1;
  }
  cout << " -Examined header of <" << inFileSpec.c_str() << ">," << endl
       << "  There should be " << nvars 
       << " fields (columns) per record (row)" << endl;

  // If requested, add a column to contain line numbers
  Column_Info column_info_buf;
  if( include_line_number) {
    column_info_buf.label = string( "-line number-");
    column_info.push_back( column_info_buf);
  }
  
  // Add a final column label that says 'nothing'.
  column_info_buf.label = string( "-nothing-");
  column_info.push_back( column_info_buf);

  // Report label information
  nLabels = column_info.size();
  cout << " -Read " << nLabels << "/" << nLabels;
  if( delimiter_char_ == ' ') cout << " whitespace-delimited ";
  else if( delimiter_char_ == ',') cout << " comma-delimited ";
  else cout << " custom-delimited ";
  cout << "column_labels:" << endl;

  // Clever output formatting to keep line lengths under control.  NOTE: 
  // This will misbehave if some label is more than 80 characters long.
  cout << "  ";
  int nLineLength = 4;
  for( unsigned int i=0; i < column_info.size(); i++ ) {
    nLineLength += 2+(column_info[ i].label).length();
    if( nLineLength > 80) {
      cout << endl << "  ";
      nLineLength = 4 + (column_info[ i].label).length();
    }
    cout << "  " << column_info[ i].label;
  }
  cout << endl;

  // Check last column to see if it is the selection label, SELECTION_LABEL,
  // defined in <global_definitions.vp.h>
  readSelectionInfo_ = 0;
  if( (column_info[nvars-1].label).compare( 0, SELECTION_LABEL.size(), SELECTION_LABEL) == 0) {
    readSelectionInfo_ = 1;
    cout << "   -Read selection info-" << endl;
  }
  
  // Return number of labels
  return nLabels;
}

//***************************************************************************
// Data_File_Manager::extract_column_types( sLine) -- Examine a line of data
// to determine which columns contain ASCII values.  NOTE: This should be 
// consolidated with some other method to avoid duplicate code.
void Data_File_Manager::extract_column_types( string sLine)
{
  // If the delimiter character is not a tab, replace tabs with spaces
  if( delimiter_char_ != '\t')
    replace( sLine.begin(), sLine.end(), '\t', ' ');

  // Loop: Insert the string into a stream and read it
  std::stringstream ss( sLine); 
  unsigned isBadData = 0;
  string sToken;
  // double xValue;
  for( int j=0; j<nvars; j++) {
    
    // Get the next word.  NOTE: whitespace-delimited and character-
    // delimited files must be handled differently.  PROBLEM: This may not 
    // handle missing values correctly, if at all.
    if( delimiter_char_ == ' ') ss >> sToken;
    else {
      std::string buf;
      getline( ss, buf, delimiter_char_);
        
      // Check for missing data
      string::size_type notwhite = buf.find_first_not_of( " ");
      buf.erase( 0, notwhite);
      notwhite = buf.find_last_not_of( " ");
      buf.erase( notwhite+1);
      if( buf.size() <= 0) sToken = string( "BAD_VALUE_PROXY");
      else {
        stringstream bufstream;
        bufstream << buf;
        bufstream >> sToken;
      }
    }

    // Issue warning and quit if this line doesn't contain enough data
    if( ss.eof() && j<nvars-1) {
      cerr << " -WARNING, extract_column_types reports "
           << "not enough data on first line!" << endl
           << "  skipping entire line." << endl;
      isBadData = 1;
      break;
    }

    // Issue warning and quit if this line contains unreadable data.  NOTE:
    // This should never happen, because error flags were cleared above.
    if( !ss.good() && j<nvars-1) {
      cerr << " -WARNING, extract_column_types reports unreadable data "
           << "(binary or ASCII?) on first line at column " << j+1
           << "," << endl
           << "  skipping entire line." << endl;
      isBadData = 1;
      break;
    }
    
    // DIAGNOSTIC
    // cout << " (" << xToken.c_str() << ")";

    // Attempt to use strod and examine values and pointers to determine if 
    // token can be parsed as a double.  This code doesn't work
    // int hasASCII = 0;
    // char **cEnd;
    // char cToken[80];
    // double xTest = std::strtod( sToken.c_str(), cEnd);
    // if( xValue == 0 && cEnd != &(sToken.c_str())) hasASCII = 1;
    // else hasASCII = 0;
    
    // Convert token to a stringstream and try to read it to determine if it
    // can be parsed as a double.  If the stringstream >> operator returns 0, 
    // the read failed and the token couldn't be parsed.  WARNING: It remains
    // to be determined if this works on all compilers!
    int hasASCII = 0;
    std::istringstream inpStream( sToken);
    double inpValue = 0.0;
    if( inpStream >> inpValue) hasASCII = 0;
    else hasASCII = 1;
    
    // Treat the string 'NaN' as numerical.  Yes, it might be better to 
    // convert sToken to uppercase, but I'm lazy
    if( sToken.compare( "NAN") == 0 || 
        sToken.compare( "NaN") == 0 ||
        sToken.compare( "nan") == 0) hasASCII = 0;

    // Load information into the vector of Column_Info objects
    column_info[ j].hasASCII = hasASCII;
  }

  // DIAGNOSTIC
  // cout << endl;
  
  // DIAGNOSTIC
  // cout << "data_file_manager::extract_column_types: " << endl;
  // for( int j=0; j<nvars; j++) cout << " " << setw( 6) << j;
  // cout << endl;
  // for( int j=0; j<nvars; j++) {
  //   if( column_info[ j].hasASCII != 0) cout << "  ASCII";
  //   else cout << "    Num";
  // }
  // cout << endl;
}

//***************************************************************************
// Data_File_Manager::read_ascii_file_with_headers() -- Reads and ASCII file.
// Step 1: Open an ASCII file for input.  Step 2: Read and discard the header
// block.  Step 3: Generate column labels.  Step 4: Read the data block.  
// Step 5:  Close the file.  Returns 0 if successful.
int Data_File_Manager::read_ascii_file_with_headers() 
{
  // STEP 1: Attempt to open input file and make sure it exists
  ifstream inFile;
  inFile.open( inFileSpec.c_str(), ios::in);
  if( inFile.bad() || !inFile.is_open()) {
    cerr << "read_ascii_file_with_headers:" << endl
         << " -ERROR, couldn't open <" << inFileSpec.c_str()
         << ">" << endl;
    return 1;
  }
  else {
    cout << "read_ascii_file_with_headers:" << endl
         << " -Opening <" << inFileSpec.c_str() << ">" << endl;
  }


  // STEP 2: Read and discard the header block, but save the last line of 
  // the header in the LASTHEADERLINE buffer

  // Loop: Read successive lines to find and store the last line of the 
  // header block. NOTE: Since tellg() and seekg() don't seem to work 
  // properly with getline() with all compilers, this must be accomplished 
  // by reading and counting each line explicitly.
  std::string line = "";
  std::string lastHeaderLine = "";
  int nRead = 0, nHeaderLines = 0;
  for( int iLine = 0; iLine < MAX_HEADER_LINES; iLine++) {
    if( inFile.eof() != 0) break;
    (void) getline( inFile, line, '\n');
    nRead++;

    // Skip empty lines without updating the LASTHEADERLINE buffer
    if( line.length() == 0) {
      nHeaderLines++;
      continue;
    }
    
    // If this line is supposed to be skipped or if it begins with a comment 
    // character, skip it and update the LASTHEADERLINE buffer
    if( iLine < nSkipHeaderLines || 
        line.length() == 0 || line.find_first_of( "!#%") == 0) {
      lastHeaderLine = line;
      nHeaderLines++;
      continue;
    }
    break;
  }
  cout << " -Header block contains " << nHeaderLines 
       << " header lines." << endl;


  // STEP 3: Get column labels.  If LASTHEADERLINE is full, use this to 
  // generate column labels.  Otherwise generate default column labels.

  // If no header lines were found or the LASTHEADERLINE buffer is empty, 
  // count the number of values in the first line of data to generate a set 
  // of default column labels.  Otherwise examine the LASTHEADERLINE buffer 
  // to extract column labels.
  int nLabels = 0;
  if( doCommentedLabels_ == 0) nLabels = extract_column_labels( line, 0);
  else {
    if( nHeaderLines == 0 || lastHeaderLine.length() == 0)
      nLabels = extract_column_labels( line, 1);
    else nLabels = extract_column_labels( lastHeaderLine, 0);
  }

  // If there were problems, close file and quit
  if( nLabels < 0) {
    inFile.close();
    return 1;
  }


  // STEP 4: Read the data block
  
  // Now we know the number of variables, NVARS, so if we also know the 
  // number of points (e.g. from the command line, we can size the main 
  // points array once and for all, and not waste memory.
  if( npoints_cmd_line > 0) npoints = npoints_cmd_line;
  else npoints = MAXPOINTS;
  if( include_line_number) {
    if( !readSelectionInfo_) points.resize( nvars+1, npoints);
    else points.resize( nvars, npoints);
  }
  else {
    if( !readSelectionInfo_) points.resize( nvars, npoints);
    else points.resize( nvars-1, npoints);
  }
  
  // Loop: Read successive lines from the file
  int nSkip = 0;
  int nLines = 0;
  unsigned uFirstLine = 0;
  if( doCommentedLabels_) uFirstLine = 1;
  int nTestCycle = 0, nUnreadableData = 0;
  while( !inFile.eof() && nLines<npoints) {
  
    // Get the next line, check for EOF, and increment accounting information
    if( !uFirstLine) {
      (void) getline( inFile, line, '\n');
      if( inFile.eof()) break;  // Break here to make accounting work right
      nRead++;
    }

    // Skip blank lines and comment lines
    if( line.length() == 0 || line.find_first_of( "!#%") == 0) {
      nSkip++;
      uFirstLine = 0;
      continue;
    }
    uFirstLine = 0;
    nTestCycle++;
    
    // Invoke member function to examine the first line of data to identify 
    // columns that contain ASCII values and load this information into the 
    // vector of column_info objects
    if( nLines == 0) extract_column_types( line);
    
    // If the delimiter character is not a tab, replace tabs with spaces
    if( delimiter_char_ != '\t')
      replace( line.begin(), line.end(), '\t', ' ');

    // Loop: Insert the string into a stream and read it
    std::stringstream ss( line); 
    unsigned isBadData = 0;
    double xValue;
    string sToken;
    for( int j=0; j<nvars; j++) {
    
      // Read the next word as double or a string, depending on whether or
      // not this column contains ASCII values.  NOTE: whitespace-delimited 
      // and character-delimited files must be handled differently.
      // PROBLEM: This isn't handling missing values correctly
      if( delimiter_char_ == ' ') {
        if( column_info[j].hasASCII == 0) ss >> xValue;
        else ss >> sToken;
      }
      else {
        std::string buf;
        getline( ss, buf, delimiter_char_);
        
        // Check for missing data
        string::size_type notwhite = buf.find_first_not_of( " ");
        buf.erase( 0, notwhite);
        notwhite = buf.find_last_not_of( " ");
        buf.erase( notwhite+1);
        if( buf.size() <= 0) xValue = bad_value_proxy_;
        else {
          stringstream bufstream;
          bufstream << buf;
          if( column_info[j].hasASCII == 0) bufstream >> xValue;
          else bufstream >> sToken;
        }
      }

      // Skip lines that don't appear to contain enough data
      if( ss.eof() && j<nvars-1) {
        cerr << " -WARNING, not enough data on line " << nRead
             << ", skipping this line!" << endl;
        isBadData = 1;
        break;
      }

      // If this was selection information, load it into the READ_SELECTED
      // vector, otherwise load it into the POINTS array.  In both cases 
      // inspect the SS stringstream to check for values of NULL that imply
      // certain types of bad data and/or missing values, and if necessary,
      // replace these with a default value and clear the error flags.
      if( !readSelectionInfo_ || j < nvars-1) {
        if( !ss) {
          points(j,nLines) = bad_value_proxy_;
          ss.clear();
        }
        else {
             
          // If this is numerical data, load it directly, otherwise invoke
          // the member function of Column_Info to determine the order in
          // which ASCII values appeared and load that order as data.
          if( column_info[j].hasASCII == 0) points(j,nLines) = (float) xValue;
          else points(j,nLines) = column_info[j].add_value( sToken);
        }
      }
      else {
        if( !ss) ss.clear();
        else read_selected( nLines) = (int) xValue;
      }
      
      // Check for unreadable data and flag this line to be skipped.  NOTE:
      // This should never happen, because error flags were cleared above.
      if( !ss.good() && j<nvars-1) {
        cerr << " -WARNING, unreadable data "
             << "(binary or ASCII?) at line " << nRead
             << " column " << j+1 << "," << endl
             << "  skipping entire line." << endl;
        nUnreadableData++;
        isBadData = 1;
        break;
      }
    }

    // Loop: Check for bad data flags and flag this line to be skipped
    for( int j=0; j<nvars; j++) {
      if( points(j,nLines) < -90e99) {
        cerr << " -WARNING, bad data flag (<-90e99) at line " << nRead
             << ", column " << j << " - skipping entire line\n";
        isBadData = 1;
        break;
      }
    }

    // Check for too much unreadable data
    if( nTestCycle >= MAX_NTESTCYCLES) {
      if( nUnreadableData >= MAX_NUNREADABLELINES) {
        cerr << " -ERROR: " << nUnreadableData << " out of " << nTestCycle
             << " lines of unreadable data at line " << nLines+1 << endl;
        sErrorMessage = "Too much unreadable data in an ASCII file";
        return 1;
      }
      nTestCycle = 0;
    }

    // DIAGNOSTIC: Report parsed contents of this line
    // cout << "line[ " << i << "]:";
    // for( int j=0; j<nvars; j++) {
    //   cout << " " << points( j, nLines);
    // }
    // cout << endl;
    
    // If data were good, increment the number of lines
    if( !isBadData) {
      nLines++;
      if( (nLines+1)%10000 == 0) cerr << "  Read " << nLines+1 << " lines." << endl;
    }
  }

  // DIAGNOSTIC: Examine column_info
  // cout << "data_file_manager::read_ascii_data: Column Information" << endl;
  // for( int j = 0; j < nvars; j++) {
  //   if( column_info[j].hasASCII>0) {
  //     cout << "column[" << j << "]:";
  //     for( 
  //       map<string,int>::iterator iter = (column_info[j].ascii_values_).begin();
  //       iter != (column_info[j].ascii_values_).end(); iter++) {
  //       cout << " (" << (iter->first).c_str() << "," << (iter->second) << ")";
  //     }
  //     cout << endl;
  //   }
  // }  
  
  // Check to see if the user specified that the line of column labels was 
  // commented and all columns were ASCII.  If this happened, it's possible 
  // that user made a mistake, and the column labels were actually in the
  // first uncommented line, so ask the user if this was intentional.  If
  // it wasn't, generate default data and quit so user can try again.
  if( doCommentedLabels_ != 0 && n_ascii_columns() >= nvars) {
    string sWarning = "";
    sWarning.append( "WARNING: All columns appear to be ASCII, as if\n");
    sWarning.append( "the line of column labels was left uncommented.\n");
    sWarning.append( "Do you wish to read it as is?");
    if( make_confirmation_window( sWarning.c_str(), 3, 3) <= 0) {
      create_default_data( 4);
      return 0;
    }
  }
  
  // Loop: Examine the vector of Column_Info objects to alphabetize ASCII 
  // values and renumber the data.
  int nReordered = 0;
  for( int j=0; j<nvars; j++) {
    if( column_info[j].update_ascii_values_and_data(j) >=0) nReordered++;
  }

  // DIAGNOSTIC: Examine column_info again
  // cout << "data_file_manager::read_ascii_data: Updated column Information" << endl;
  // for( int j = 0; j < nvars; j++) {
  //   if( column_info[j].hasASCII>0) {
  //     cout << "column[" << j << "]:";
  //     for( 
  //       map<string,int>::iterator iter = (column_info[j].ascii_values_).begin();
  //       iter != (column_info[j].ascii_values_).end(); iter++) {
  //       cout << " (" << (iter->first).c_str() << "," << (iter->second) << ")";
  //     }
  //     cout << endl;
  //   }
  // }

  // DIAGNOSTIC: Exercise access functions
  // for( int j=0; j<nvars; j++) {
  //   cout << "*COLUMN[ " << j << "]:";
  //   if( is_ascii_column(j) == 0) cout << " Numerical";
  //   else {
  //     for( int i=0; i<n_ascii_values(j); i++) {
  //       string sPooka = ascii_value(j,i);
  //       int iPooka = ascii_value_index(j,sPooka);
  //       cout << " (" << sPooka.c_str() << "," << iPooka << ")";
  //     }
  //   }
  //   cout << endl;
  // }


  // STEP 5: Update NVARS and NPOINTS, report results of the read operation 
  // to the console, close input file, and report success
  if( readSelectionInfo_ != 0) nvars = nvars-1;
  npoints = nLines;

  cout << " -Finished reading " << nvars << "x" << npoints
       << " data block with ";
  if( readSelectionInfo_ == 0) cout << "no ";
  else cout << " added column of ";
  cout << "selection information." << endl;
  cout << "  " << nHeaderLines 
       << " header + " << nLines 
       << " good data + " << nSkip 
       << " skipped lines = " << nRead << " total." << endl;
  inFile.close();
  return 0;
}

//***************************************************************************
// Data_File_Manager::read_binary_file_with_headers() -- Open and read a 
// binary file.  The file is asssumed to consist of a single header line of 
// ASCII with column information, terminated by a newline, followed by a block
// of binary data.  The only viable way to read this seems to be with 
// conventional C-style methods: fopen, fgets, fread, feof, and fclose, from 
// <stdio>.  Returns 0 if successful.
int Data_File_Manager::read_binary_file_with_headers() 
{
  // Attempt to open input file and make sure it exists
  FILE * pInFile;
  pInFile = fopen( inFileSpec.c_str(), "rb");
  if( pInFile == NULL) {
    cerr << "read_binary_file_with_headers: ERROR" << endl
         << " -Couldn't open binary file <" << inFileSpec.c_str() 
         << ">" << endl;
  }
  else {
    cout << "read_binary_file_with_headers:" << endl
         << " -Opening binary file <" << inFileSpec.c_str() 
         << ">" << endl;
  }

  // Use fgets to read a newline-terminated string of characters, test to make 
  // sure it wasn't too long, then load it into a string of header information.
  char cBuf[ MAX_HEADER_LENGTH];
  fgets( cBuf, MAX_HEADER_LENGTH, pInFile);
  if( strlen( cBuf) >= (int)MAX_HEADER_LENGTH) {
    cerr << " -ERROR: Header string is too long, "
         << "increase MAX_HEADER_LENGTH and recompile"
         << endl;
    fclose( pInFile);
    make_confirmation_window( "ERROR: Header string is too long", 1);
    return 1;
  }
  std::string line;
  line.assign( cBuf);

  // There are two possibilities: Conventional Format, in which case there 
  // is only one header line and all data is numerical, and ASCII Format, 
  // in which case the first header line lists the number of columns,
  // successive lines contain column labels, types, and lookup tables for
  // ASCII values, and some data is associated with ASCII values
  // if( strstr( line.c_str(), "ASCII_FORMAT_VP")) {
  // }
  // else {
  // }
  
  // Save existing delimiter character, then examine the line.  If it 
  // contains tabs, temporarily set the deliminer character to a tab.
  // Otherwise set it to whitespace.
  char saved_delimiter_char_ = delimiter_char_;
  if( line.find( '\t') < 0 || line.size() <= line.find( '\t')) {
    cout << " -Header is WHITESPACE delimited" << endl;
    delimiter_char_ = ' ';
  }
  else {
    cout << " -Header is TAB delimited" << endl;
    delimiter_char_ = '\t';
  }

  // Invoke the extract_column_labels method to extract column labels,
  // then reset the delimiter character
  int nLabels = 0;
  nLabels = extract_column_labels( line, 0);
  delimiter_char_ = saved_delimiter_char_;
  
  // Report status to the console
  cout << " -About to read " << nvars
       << " variables from a binary file with " << nLabels
       << " fields (columns) per record (row)" << endl;

  // Now we know the number of variables (nvars), so if we know the number of 
  // points (e.g. from the command line) we can size the main points array 
  // once and for all, and not waste memory.
  if( npoints_cmd_line > 0) npoints = npoints_cmd_line;
  else npoints = MAXPOINTS;
  if( include_line_number) {
    if( !readSelectionInfo_) points.resize( nvars+1, npoints);
    else points.resize( nvars, npoints);
  }
  else {
    if( !readSelectionInfo_) points.resize( nvars, npoints);
    else points.resize( nvars-1, npoints);
  }
  
  // Warn if the input buffer is non-contiguous.
  if( !points.isStorageContiguous()) {
    cerr << "read_binary_file_with_headers: WARNING" << endl
         << " -Input buffer appears to be non-contigous."
         << endl;
    // return 1;
  }

  // Assert possible types or ordering  
  // assert( ordering == COLUMN_MAJOR || ordering == ROW_MAJOR);

  // Read file in Column Major order...
  if( isColumnMajor == 1) {
    cout << " -Attempting to read binary file in column-major order" << endl;
         
    // Define input buffers and make sure they're contiguous
    blitz::Array<float,1> vars( nvars);
    blitz::Range NVARS( 0, nvars-1);
    if( !vars.isStorageContiguous()) {
      cerr << " -ERROR: Tried to read into a noncontiguous buffer."
           << endl;
      fclose( pInFile);
      make_confirmation_window( 
        "ERROR: Tried to read into a noncontiguous buffer", 1);
      return -1;
    }

    // Loop: Read successive rows from file
    int i;
    for( i=0; i<npoints; i++) {
    
      // Read the next NVAR values using conventional C-style fread.
      unsigned int ret = 
        fread( (void *)(vars.data()), sizeof(float), nvars, pInFile);
      
      // Check for normal termination
      if( ret == 0 || feof( pInFile)) {
        cerr << " -Finished reading file at row[ " << i
             << "] with ret, inFile.eof() = ( " << ret
             << ", " << feof( pInFile) << ")" << endl;
        break;
      }
      
      // If wrong number of values was returned, report error.
      if( ret != (unsigned int) nvars) {
        cerr << " -ERROR reading row[ " << i+1 << "], "
             << "returned values " << ret 
             << " NE number of variables " << nvars << endl;
        fclose( pInFile);
        make_confirmation_window( "Error reading row of binary data", 1);
        return 1;
      }

      // Load data array and report progress
      if( !readSelectionInfo_) points( NVARS, i) = vars( NVARS);
      else {
        points( blitz::Range( 0, nvars-2), i) = vars( blitz::Range( 0, nvars-2));
        read_selected( i) = (int) vars( nvars-1);
      }
      if( i>0 && (i%10000 == 0)) 
        cout << "  Reading row " << i << endl;
    }

    // Report success
    cout << " -Finished reading " << i << " rows." << endl;
    npoints = i;
  }

  // ...or read file in Row Major order
  else {
    cout << " -Attempting to read binary file in row-major order "
         << "with nvars=" << nvars
         << ", npoints=" << npoints << endl;
    if( npoints_cmd_line == 0) {
      cerr << " -ERROR, --npoints must be specified for"
           << " --inputformat=rowmajor"
           << endl;
      fclose( pInFile);
      make_confirmation_window( 
        "ERROR: NPOINTS must be specified for ROWMAJOR binary files", 1);
      return 1;
    }
    else {
      npoints = npoints_cmd_line;
    }

    // Define input buffer and make sure it's contiguous
    blitz::Array<float,1> vars( npoints);
    blitz::Range NPTS( 0, npoints-1);
    if( !vars.isStorageContiguous()) {
      cerr << " -ERROR, Tried to read into noncontiguous buffer."
           << endl;
      fclose( pInFile);
      make_confirmation_window( 
        "ERROR: Tried to read into noncontiguous buffer", 1);
      return -1;
    }

    // Loop: Read successive columns from the file
    int i;
    for( i=0; i<nvars; i++) {

      // Read the next NVAR values using conventional C-style fread.
      unsigned int ret = 
        fread( (void *)(vars.data()), sizeof(float), npoints, pInFile);

      // Check for normal termination
      if( ret == 0 || feof( pInFile)) {
        cerr << " -Finished reading file at row[ " << i
             << "] with ret, inFile.eof() = ( " << ret
             << ", " << feof( pInFile) << ")" << endl;
        break;
      }
      
      // If wrong number of values was returned, report error.
      if( ret != (unsigned int)npoints) {
        cerr << " -ERROR reading column[ " << i+1 << "], "
             << "returned values " << ret 
             << " NE number of variables " << nvars << endl;
        fclose( pInFile);
        make_confirmation_window( "ERROR reading column of binary file", 1);
        return 1;
      }

      // Load data array and report progress
      if( !readSelectionInfo_ && i < nvars-1) points( i, NPTS) = vars( NPTS);
      else {
        points( i, NPTS) = vars( NPTS);
        // read_selected( NPTS) = blitz::cast( vars( NPTS), int());
        // read_selected( NPTS) = cast<int>(vars(NPTS));
        for( int j=0; j<npoints; j++) read_selected( j) = (int) vars( j);
      }
      cout << "  Reading column " << i+1 << endl;
    }
    
    // Report success
    cout << " -Finished reading " << i+i
         << " columns" << endl;
  }
  
  // Update NVARS, close input file and terminate
  if( readSelectionInfo_ != 0) nvars = nvars-1;
  fclose( pInFile);
  return 0;
}

//***************************************************************************
// Data_File_Manager::findOutputFile() -- Query user to find the output file.
// Class Vp_File_Chooser is used in preference to the Vp_File_Chooser method 
// to obtain access to member functions such as directory() and to allow the 
// possibility of a derived class with additional controls in the 
// file_chooser window.  Returns 0 if successful.  
int Data_File_Manager::findOutputFile()
{
  // Generate query text and list file extensions, etc for this file type
  string title;
  string pattern;
  if( writeAllData_ != 0) title = "Write all data to file";
  else title = "Write selected data to file";
  if( isAsciiOutput) pattern = "*.{txt,lis,asc}\tAll Files (*)";
  else pattern = "*.bin\tAll Files (*)";

  // Initialize output filespec.  NOTE: cOutFileSpec is defined as const 
  // char* for use with Vp_File_Chooser, which means it could be destroyed 
  // by the relevant destructors!
  const char *cOutFileSpec = sDirectory_.c_str();

  // Instantiate and show an Vp_File_Chooser widget.  NOTE: The pathname 
  // must be passed as a variable or the window will begin in some root 
  // directory.
  Vp_File_Chooser* file_chooser = 
    new Vp_File_Chooser( 
      cOutFileSpec, pattern.c_str(), Vp_File_Chooser::CREATE, title.c_str());
  file_chooser->directory( sDirectory_.c_str());
  file_chooser->isAscii( isAsciiOutput);

  // Loop: Select succesive filespecs until a non-directory is obtained
  while( 1) {
    // if( cOutFileSpec != NULL) file_chooser->directory( cOutFileSpec);

    // Loop: wait until the selection is done, then extract the value.  NOTE: 
    // This usage of while and Fl::wait() seems strange.
    file_chooser->show();
    while( file_chooser->shown()) Fl::wait();
    cOutFileSpec = file_chooser->value();   

    // If no file was specified then quit
    if( cOutFileSpec == NULL) break;

    // If this is a new file, it can't be opened for read
    int isNewFile = 0;
    FILE* pFile = fopen( cOutFileSpec, "r");
    if( pFile == NULL) isNewFile= 1;
    else fclose( pFile);

    // Attempt to open an output stream to make sure the file can be opened 
    // for write.  If it can't, assume that cOutFileSpec was a directory and 
    // make it the working directory.  Otherwise close the output stream.
    // NOTE: This will create an empty file.
    #ifdef __WIN32__
      ofstream os;
      os.open( cOutFileSpec, ios::out|ios::trunc);
      if( os.fail()) {
        cerr << " -DIAGNOSTIC: This should trigger on error opening "
             << cOutFileSpec << "for write" << endl;
        file_chooser->directory( cOutFileSpec);
        directory( (string) cOutFileSpec);
        os.close();
        continue;
      }
      os.close();
      if( isNewFile != 0) break;
    #endif // __WIN32__

    // OLD CODE: If this is a new file, it can't be opened for read, and 
    // we're done
    // FILE* pFile = fopen( cOutFileSpec, "r");
    // if( pFile == NULL) break;
    
    // OLD CODE:We're done examining the file, so close it
    // fclose( pFile);

    // If we got this far, the file must exist and be available to be
    // overwritten, so open a confirmation window and wait for the button 
    // handler to do something.
    string sConfirmText = "File '";
    sConfirmText.append( cOutFileSpec);
    sConfirmText.append( "' already exists.\nOverwrite exisiting file?\n");
    int iConfirmResult = make_confirmation_window( sConfirmText.c_str());

    // If this was a 'CANCEL' request, return without doing anything.  If 
    // this was a 'YES' request, move on.  Otherwise, make sure we're in
    // the right directory and try again.
    if( iConfirmResult < 0) return -1;
    if( iConfirmResult > 0) break;
  } 

  // Obtain file name using the FLTK member function.  This code doesn't work, 
  // but is retained as a comment for descriptive purposes.
  // char *cOutFileSpec = 
  //   Vp_File_Chooser( "write ASCII output to file", NULL, NULL, 0);

  // Get file type and content information
  if( file_chooser->isAscii() != 0) isAsciiOutput = 1;
  else isAsciiOutput = 0;
  delimiter_char_ = file_chooser->delimiter_char();
  if( file_chooser->writeSelectionInfo() != 0) writeSelectionInfo_ = 1;
  else writeSelectionInfo_ = 0;
  if( file_chooser->doCommentedLabels() != 0) doCommentedLabels_ = 1;
  else doCommentedLabels_ = 0;

  // Load outFileSpec
  int iResult = 0;
  if( cOutFileSpec == NULL) {
    outFileSpec = "";
    cout << "Data_File_Manager::findOutputFile: "
         << "closed with no output file specified" << endl;
    iResult = -1;
  }
  else{
    outFileSpec.assign( (string) cOutFileSpec);
    if( isAsciiOutput == 1) 
      cout << "Data_File_Manager::findOutputFile: Writing ASCII data to <";
    else 
      cout << "Data_File_Manager::findOutputFile: Writing binary data to <";
    cout << outFileSpec.c_str() << ">" << endl;
    iResult = 0;
  }

  // Make sure thepathname has been updated!
  directory( (string) file_chooser->directory());

  // Deallocate the Vp_File_Chooser object
  delete file_chooser;  // WARNING! This destroys cOutFileSpec!

  // Report result
  return iResult;
}

//***************************************************************************
// Data_File_Manager::save_data_file( outFileSpec) -- Write ASCII or binary 
// data file to disk.  Returns 0 if successful.
int Data_File_Manager::save_data_file( string outFileSpec)
{
  output_filespec( outFileSpec);
  return save_data_file();
}

//***************************************************************************
// Data_File_Manager::save_data_file() -- Write ASCII or binary data file to 
// disk.  Returns 0 if successful.
int Data_File_Manager::save_data_file()
{
  int result = 0;
  if( isAsciiOutput != 1) result = write_binary_file_with_headers();
  else result = write_ascii_file_with_headers();
  if( result == 0) {
    isSavedFile_ = 1;
    dataFileSpec = outFileSpec;
    isAsciiData = isAsciiOutput;
  }
  return result;
}

//***************************************************************************
// Data_File_Manager::write_ascii_file_with_headers() -- Open and write an 
// ASCII data file.  File will consist of an ASCII header with column names 
// terminated by a newline, followed by successive lines of ASCII data.
// Returns 0 if successful.
int Data_File_Manager::write_ascii_file_with_headers()
{
  // Make sure a file name was specified, create and write the file
  if( outFileSpec.length() <= 0){
    cout << "Data_File_Manager::write_ascii_file_with_headers "
         << "reports that no file was specified" << endl;
    return -1;
  }
  else {
    blitz::Array<float,1> vars( nvars);
    blitz::Range NVARS( 0, nvars-1);
    
    // Open output stream and report any problems
    ofstream os;
    os.open( outFileSpec.c_str(), ios::out|ios::trunc);
    if( os.fail()) {
      cerr << " -ERROR opening " << outFileSpec.c_str() 
           << " for ASCII write" << endl;
      return -1;
    }
    
    // Write output file name (and additional information?) to the header
    os << "! File Name: " << outFileSpec.c_str() << endl;
    
    // Do not write out "line-number" column (header or data)
    // it gets created automatically when a file is read in
    int nvars_out = include_line_number?nvars-1:nvars;

    // Loop: Write comment character to include column labels to the header
    char first_char = ' ';
    if( doCommentedLabels_) first_char = '!';
    for( int i=0; i < nvars_out; i++ ) {
      if( i == 0) os << first_char << setw( 12) << column_info[ i].label;
      else os << delimiter_char_ << " " << setw( 13) << column_info[ i].label;
      // else os << " " << setw( 13) << column_info[ i].label;
    }
    if( writeSelectionInfo_ != 0) os << delimiter_char_ << " " << SELECTION_LABEL;
    os << endl;
    
    // Loop: Write successive ASCII records to the data block using the
    // "default" floatfield format.  This causes integers to be written as
    // integers, floating point as floating point, and numbers with large or 
    // small magnitude as scientific floats. 
    os.precision( 14); 
    os.unsetf( ios::scientific); // force floatfield to default
    int rows_written = 0;
    for( int irow = 0; irow < npoints; irow++) {
      if( writeAllData_ != 0 || selected( irow) > 0) {
        for( int jcol = 0; jcol < nvars_out; jcol++) {
          // if( jcol > 0) os << " ";
          if( jcol > 0) os << delimiter_char_ << " ";
          
          // Process numerical and ASCII values differently
          if( column_info[jcol].hasASCII == 0) os << points( jcol, irow);
          else {
            os << column_info[jcol].ascii_value( (int) points( jcol, irow));
          }
        }
        if( writeSelectionInfo_ != 0) os << delimiter_char_ << " " << selected( irow);
        os << endl;
        rows_written++;
      }
    }

    // Report results
    cout << "wrote " << rows_written << " rows of " << nvars 
         << " variables to ascii file " << outFileSpec.c_str() << endl;
  }
  return 0;
}

//***************************************************************************
// Data_File_Manager::write_binary_file_with_headers() -- Open and write a 
// binary data file.  File will consist of an ASCII header with column names 
// terminated by a newline, followed by a long block of binary data.  Returns
// 0 if successful.
int Data_File_Manager::write_binary_file_with_headers()
{
  // Make sure a file name was specified, create and write the file
  if( outFileSpec.length() <= 0){
    cout << "Data_File_Manager::write_binary_file_with_headers "
         << "reports that no file was specified" << endl;
    return -1;
  }
  else {

    // Do not write out "line-number" column (header or data)
    // it gets created automatically when a file is read in
    int nvars_out = include_line_number?nvars-1:nvars;

    blitz::Array<float,1> vars( nvars_out);
    blitz::Range NVARS( 0, nvars_out-1);
    
    // Open output stream and report any problems
    ofstream os;
    os.open( 
      outFileSpec.c_str(), 
      ios::out|ios::trunc|ios::binary);
      // fstream::out | fstream::trunc | fstream::binary);

    if( os.fail()) {
      cerr << " -ERROR opening " << outFileSpec.c_str() 
           << "for binary write" << endl;
      return -1;
    }
    
    // Loop: Write tab-delimited column labels to the header.  Each label is
    // followed by a space to make sure the file can be read by older versions 
    // of viewpoints.
    // for( int i=0; i < nvars_out; i++ ) os << column_info[ i].label << " ";
    // if( writeSelectionInfo_ != 0) os << "selection";
    for( int i=0; i < nvars_out; i++ ) {
      os << column_info[ i].label << " ";
      if( i<nvars-1) os << '\t';
    }
    if( writeSelectionInfo_ != 0) os << '\t' << " " << SELECTION_LABEL;
    os << endl;
    
    // Loop: Write data and report any problems
    int nBlockSize = nvars*sizeof(float);
    int rows_written = 0;
    for( int i=0; i<npoints; i++) {
      if( writeAllData_ != 0 || selected( i) > 0) {
        vars = points( NVARS, i);
        os.write( (const char*) vars.data(), nBlockSize);
        if( writeSelectionInfo_ != 0) {
          float fselection = (float) (selected(i));
          os.write((const char*)&fselection, sizeof(float));
		}
        if( os.fail()) {
          cerr << "Error writing to" << outFileSpec.c_str() << endl;
          string sWarning = "";
          sWarning.append( "WARNING: Error writing to file\n");
          sWarning.append( outFileSpec);
          make_confirmation_window( sWarning.c_str(), 1);
          return 1;
        }
        rows_written++;
      }
    }
    
    // Report results
    cout << "wrote " << rows_written << " rows of " << nBlockSize 
         << " bytes to binary file " << outFileSpec.c_str() << endl;
  }
  return 0;
}

//***************************************************************************
// Data_File_Manager::edit_column_info( *o) -- Static function to wrap the
// callback function, edit_column_info, that builds and manages the 
// 'Tools|Edit Column Labels' window.
void Data_File_Manager::edit_column_info( Fl_Widget *o)
{
  // DIAGNOSTIC
  cout << endl << "DIAGNOSTIC: Called Data_File_Manager::edit_column_info" << endl;

  // Old attempts to trace back the right number of generations
  // edit_column_info_i( o);
  // ( (Data_File_Manager*)
  //  (o->parent()->parent()->user_data()))->edit_column_info_i( o);
  // ( (Data_File_Manager*)
  //  (o->parent()->parent()))->edit_column_info_i( o);

  // Be sure to trace back the right number of generations
  ((Data_File_Manager*)(o->parent()))->edit_column_info_i( o);
}

//***************************************************************************
// Data_File_Manager::edit_column_info_i( *o) -- Callback function that
// builds and manages the 'Tools|Edit Column Labels' window window.
void Data_File_Manager::edit_column_info_i( Fl_Widget *o)
{
  if( edit_labels_window != NULL) edit_labels_window->hide();

  // Create the 'Tools|Edit Column Labels' window
  Fl::scheme( "plastic");  // optional
  edit_labels_window = new Fl_Window( 250, 305, "Edit Column Labels");
  edit_labels_window->begin();
  edit_labels_window->selection_color( FL_BLUE);
  edit_labels_window->labelsize( 10);
  
  // Write warning in box at top of window
  Fl_Box* warningBox = new Fl_Box( 5, 5, 240, 20);
  warningBox->label( "Warning: this will reset axes \nselections, and scaling");
  warningBox->align( FL_ALIGN_INSIDE|FL_ALIGN_LEFT);

  // Set an invisible box to control resize behavior
  Fl_Box *box = new Fl_Box( 5, 35, 240, 220);
  box->box( FL_NO_BOX);
  // box->box( FL_ROUNDED_BOX);
  edit_labels_window->resizable( box);

  // Define Fl_Check_Browser widget
  edit_labels_widget = new Fl_Check_Browser( 5, 35, 240, 220, "");
  edit_labels_widget->labelsize( 14);
  edit_labels_widget->textsize( 12);
  
  // Load column labels into browser
  refresh_edit_column_info();

  // Invoke callback function to delete labels
  Fl_Button* delete_button = new Fl_Button( 10, 270, 100, 25, "&Delete labels");
  delete_button->callback( (Fl_Callback*) delete_labels, edit_labels_widget);

  // Invoke callback function to quit and close window
  Fl_Button* quit = new Fl_Button( 160, 270, 70, 25, "&Quit");
  quit->callback( (Fl_Callback*) close_edit_labels_window, edit_labels_window);

  // Done creating the 'Tools|Edit Column_Labels' window.  
  edit_labels_window->end();
  // edit_labels_window->set_modal();   // This window shouldn't be modal
  edit_labels_window->show();
}

//***************************************************************************
// Data_File_Manager::refresh_edit_column_info() -- Make sure edit window
// exists, then refresh list of column labels
void Data_File_Manager::refresh_edit_column_info()
{
  if( edit_labels_window == NULL) return;
  edit_labels_widget->clear();
  for( int i=0; i<nvars; i++)
    edit_labels_widget->add( (column_info[ i].label).c_str());
}

//***************************************************************************
// Data_File_Manager::delete_labels( *o, *user_data) -- Callback function to 
// delete columns and their associated labels.  NOTE: Since data and labels
// are stored in separate structures, it is important to be careful that
// their contents remain consistent!
void Data_File_Manager::delete_labels( Fl_Widget *o, void* user_data)
{
  // DIAGNOSTIC
  cout << "Data_File_Manager::delete_labels: checked "
       << edit_labels_widget->nchecked() << "/"
       << edit_labels_widget->nitems() << " items" << endl;
  for( int i=0; i<nvars; i++) {
    cout << "Label[ " << i << "]: (" << column_info[i].label << ") ";
    if( edit_labels_widget->checked(i+1)) cout << "CHECKED";
    cout << endl;
  }

  // If no boxes were checked then quit
  int nChecked = edit_labels_widget->nchecked();
  int nRemain = nvars - nChecked;
  if( nChecked <= 0) return;
  if( nRemain <=1) {
    make_confirmation_window(
      "WARNING: Attempted to delete too many columns", 1);
    return;
  }

  // Check the dimensions of the data array and vector of Column_Info
  // objects to make sure we began with one more column label ('-nothing')
  // than the number of variables.
  int nColumnInfos = column_info.size();
  if( nColumnInfos != nvars+1) {
     cerr << "WARNING: Data_File_Manager::delete_labels was called with " 
          << nColumnInfos
          << " columns and a final label of (" 
          << (column_info[nColumnInfos].label).c_str()
          << ") but only " << nvars << " attributes" << endl;
  }
  
  // Move and resize data and column labels.  Do this inside the same loop 
  // to reduce the chance of doing it wrong.  NOTE: What should be done with
  // the array of ranked points used to perform scaling and normalization?
  blitz::Range NPOINTS( 0, npoints-1);
  int ivar = 0;
  for( int i=0; i<nvars; i++) {
    if( edit_labels_widget->checked(i+1) <= 0) {
      points( ivar, NPOINTS) = points( i, NPOINTS);
      Column_Info column_info_buf;  // Why is this necessary?
      column_info_buf = column_info[i];
      column_info[ivar] = column_info_buf;
      // ranked_points( ivar, NPOINTS) = ranked_points( i, NPOINTS);
      ivar++;
    }
  }
  nvars = ivar;
  points.resizeAndPreserve( nvars, npoints);
  column_info.resize( nvars);
  Column_Info column_info_buf;
  column_info_buf.label = string( "-nothing-");
  column_info.push_back( column_info_buf);
  // ranked_points.resize( nvars, npoints);

  // Check the dimensions of the data array and vector of Column_Info
  // objects to make sure we finished with one more column label 
  // ('-nothing') than the number of variables.
  nColumnInfos = column_info.size();
  if( nColumnInfos != nvars+1) {
     cerr << "WARNING: Data_File_Manager::delete_labels finished with " 
          << nColumnInfos
          << " columns and a final label of (" 
          << (column_info[nColumnInfos].label).c_str()
          << ") but only " << nvars << " attributes" << endl;
  }
  
  // Clear and update menu
  edit_labels_widget->clear();
  for( int i=0; i<nvars; i++)
    edit_labels_widget->add( (column_info[ i].label).c_str());

  // Set flag so the idle callback, cb_manage_plot_window_array, in MAIN 
  // will know to do a Restore Panels operation!
  needs_restore_panels_ = 1;

  // ANOTHER DIAGNOSTIC
  // cout << "Data_File_Manager::delete_labels: finished with "
  //      << edit_labels_widget->nitems() << " items" << endl;
  // for( int i=0; i<nvars; i++) {
  //   cout << "Label[ " << i << "]: (" << column_info[i].label << ") ";
  //   cout << endl;
  // }

  // DIAGNOSTIC
  cout << "Data_File_Manager::delete_labels: finished with "
       << "needs_restore_panels (" << needs_restore_panels_ << ")" << endl;
}

//***************************************************************************
// Data_File_Manager::close_edit_labels_window( *o, *user_data) -- Callback 
// function to close the Edit Column Labels window.  It is assumed that a 
// pointer to the window will be passed as USER_DATA.  WARNING: No error 
// checking is done on USER_DATA!
void Data_File_Manager::close_edit_labels_window( Fl_Widget *o, void* user_data)
{
  ((Fl_Window*) user_data)->hide();
}

//***************************************************************************
// Data_File_Manager::remove_trivial_columns -- Examine an array of data and 
// remove columns for which all values are identical.  Part of the read 
// process.
void Data_File_Manager::remove_trivial_columns()
{
  blitz::Range NPTS( 0, npoints-1);
  int nvars_save = nvars;
  int current=0;

  // Define buffers to record removed columns
  int iRemoved = 0;
  vector <int> removed_columns;

  // Loop: Examine the data array column by colums and remove any columns for 
  // which all values are identical.
  while( current < nvars-1) {
    if( blitz::all( points(current,NPTS) == points(current,0))) {
      cout << "skipping trivial column " 
           << column_info[ current].label << endl;
      for( int j=current; j<nvars-1; j++) {
        points( j, NPTS) = points( j+1, NPTS);
        column_info[ j] = column_info[ j+1];
      }
      removed_columns.push_back( iRemoved);
      nvars--;
      assert( nvars>0);
    }
    else {
      current++;
    }
    iRemoved++;
  }

  // Finish resizing array and column labels and report results
  if( nvars != nvars_save) {
      
    // Report what columns were removed
    cout << "Removed " << nvars_save - nvars << " columns:";
    for( unsigned int i=0; i<removed_columns.size(); i++) {
      int nLineLength = 8;
      nLineLength += 1+(column_info[ i].label).length();
      if( nLineLength > 80) {
        cout << endl << "   ";
        nLineLength = 2 + (column_info[ i].label).length();
      }
      cout << " " << column_info[ i].label;
    }
    cout << endl;
    
    // Resize array and report results
    // XXX need to trim column_info to size nvars+1 
    points.resizeAndPreserve( nvars, npoints);
    column_info[ nvars].label = string( "-nothing-");
    cout << "new data array has " << nvars
         << " columns." << endl;
  }
}

//***************************************************************************
// Data_File_Manager::resize_global_arrays -- Resize various all the global 
// arrays used store raw, sorted, and selected data.
void Data_File_Manager::resize_global_arrays()
{
  blitz::Range NPTS( 0, npoints-1);
  
  // Commented out because this is done elsewhere, if necessary:
  // points.resizeAndPreserve(nvars,npoints);  
  
  // If line numbers are to be included as another field (column) of data 
  // array, create them as the last column.
  if( include_line_number) {
    for (int i=0; i<npoints; i++) {
      points(nvars-1,i) = (float)(i+1);
    }
  }
  
  // Resize and reinitialize list of ranked points to reflect the fact that 
  // no ranking has been done.
  ranked_points.resize( nvars, npoints);
  ranked.resize( nvars);
  ranked = 0;
  
  // Resize and reinitialize selection related arrays and flags.
  inside_footprint.resize( npoints);
  newly_selected.resize( npoints);
  selected.resize( npoints);
  previously_selected.resize( npoints);
  saved_selection.resize(npoints);
  Plot_Window::indices_selected.resize(NBRUSHES,npoints);
  reset_selection_arrays();
}

//***************************************************************************
// Data_File_Manager::create_default_data( nvars_in) -- Load data arrays with 
// default data consisting of dummy data.
void Data_File_Manager::create_default_data( int nvars_in)
{
  // Protect against screwy values of nvars_in
  if( nvars_in < 2) return;
  nvars = nvars_in;
  if( nvars > MAXVARS) nvars = MAXVARS;

  // Loop: Initialize and load the column labels, including the final label 
  // that says 'nothing'.
  column_info.erase( column_info.begin(), column_info.end());
  Column_Info column_info_buf;
  for( int i=0; i<nvars; i++) {
    ostringstream buf;
    buf << "default_"
        << setw( 3) << setfill( '0')
        << i
        << setfill( ' ') << " ";
    column_info_buf.label = string( buf.str());
    column_info.push_back( column_info_buf);
  }
  column_info_buf.label = string( "-nothing-");
  column_info.push_back( column_info_buf);

  // Report column labels
  cout << " -column_labels:";
  int nLineLength = 17;
  for( unsigned int i=0; i < column_info.size(); i++ ) {
    nLineLength += 1+(column_info[ i].label).length();
    if( nLineLength > 80) {
      cout << endl << "   ";
      nLineLength = 4 + (column_info[ i].label).length();
    }
    cout << " " << column_info[ i].label;
  }
  cout << endl;
  cout << " -Generated default header with " << nvars
       << " fields" << endl;

  // Resize data array to avoid the madening frustration of segmentation 
  // errors!  Important!
  npoints = 3;
  points.resize( nvars, npoints);

  // Loop: load each variable with 0 and 1.  These two loops are kept separate
  // for clarity and to facilitate changes
  for( int i=0; i<nvars; i++) {
    points( i, 0) = 0.0;
    points( i, 1) = 0.5;
    points( i, 2) = 1.0;
  }

  // Resize global arrays
  resize_global_arrays();

  // Report results
  cout << "Generated default data with " << npoints 
       << " points and " << nvars << " variables" << endl;
}


//***************************************************************************
// Data_File_Manager::ascii_value( jcol, ival) -- Get ASCII value ival for
// column jcol.
string Data_File_Manager::ascii_value( int jcol, int ival)
{
  if( column_info[jcol].hasASCII == 0) return string( "NUMERIC_VALUE_VP");
  if( 0>jcol || jcol >= nvars) return string( "BAD_COLUMN_INDEX_VP");
  return column_info[jcol].ascii_value(ival);
}

//***************************************************************************
// Data_File_Manager::ascii_value_index( jcol, sToken) -- Index of sToken in
// column jcol.  String is passed by value for efficiency.
int Data_File_Manager::ascii_value_index( int jcol, string &sToken)
{
  if( column_info[jcol].hasASCII == 0) return -1;
  if( 0>jcol || jcol >= nvars) return -1;
  map<string,int>::iterator iter = (column_info[jcol].ascii_values_).find(sToken);
  if( iter != (column_info[jcol].ascii_values_).end()) return iter->second;
  return -1;
}

//***************************************************************************
// Data_File_Manager::directory() -- Get pathname.
string Data_File_Manager::directory()
{
  return sDirectory_; 
}
     
//***************************************************************************
// Data_File_Manager::directory( sDirectory_in) -- Make sure that any old 
// pathname is deallocated, then set the pathname.
void Data_File_Manager::directory( string sDirectory_in)
{
  sDirectory_.erase( sDirectory_.begin(), sDirectory_.end());
  // sDirectory_.append( sDirectory_);
  sDirectory_ = sDirectory_in;
}

//***************************************************************************
// Data_File_Manager::input_filespec() -- Get input filespec.
string Data_File_Manager::input_filespec()
{
  return inFileSpec;
}

//***************************************************************************
// Data_File_Manager::input_filespec( outFileSpecIn) -- Set input_filespec.
void Data_File_Manager::input_filespec( string inFileSpecIn)
{
  inFileSpec = inFileSpecIn;
}

//***************************************************************************
// Data_File_Manager::n_ascii_columns() -- Get number of columns that 
// contain ASCII values
int Data_File_Manager::n_ascii_columns()
{
  int result = 0;
  for( int i=0; i<column_info.size(); i++) {
    if( column_info[i].hasASCII >0) result++;
  }
  return result;
}

//***************************************************************************
// Data_File_Manager::output_filespec() -- Get output filespec.
string Data_File_Manager::output_filespec()
{
  return outFileSpec;
}

//***************************************************************************
// Data_File_Manager::output_filespec( outFileSpecIn) -- Set output_filespec.
void Data_File_Manager::output_filespec( string outFileSpecIn)
{
  outFileSpec = outFileSpecIn;
}
