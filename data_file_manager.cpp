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
// Modified: P. R. Gazis  23-NOV-2007
//***************************************************************************

// Include the necessary include libraries
#include "include_libraries_vp.h"

// Include globals
#include "global_definitions_vp.h"

// Include associated headers and source code
#include "data_file_manager.h"
#include "plot_window.h"

// These should not be necessary
// #include "Vp_File_Chooser.H"   // PRG's new file chooser
// #include "Vp_File_Chooser.cpp"   // PRG's new file chooser

// Set static data members for class Data_File_Manager::

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
  doAppend( 0), doMerge( 0), useSelectedData( 0), isColumnMajor( 0)
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
  doAppend = 0;
  doMerge = 0;
  useSelectedData = 0;
  writeSelectionInfo_ = 0;

  isColumnMajor = 1;
  nSkipHeaderLines = 0;  // Number of header lines to skip
  // nSkipHeaderLines = 1;  // Number of header lines to skip
  // sDirectory_ = ".";  // Default pathname
  inFileSpec = "";  // Default input filespec

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
// Class Vp_File_Chooser is used in preference to the Vp_File_Chooser method 
// to obtain access to member functions such as directory() and to allow the 
// possibility of a derived class with additional controls in the 
// file_chooser window.  Returns 0 if successful.  
int Data_File_Manager::findInputFile()
{
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
  // const char *cInFileSpec = directory().c_str();
  const char *cInFileSpec = sDirectory_.c_str();
  
  // Instantiate and show an Vp_File_Chooser widget.  NOTE: The pathname must
  // be passed as a variable or the window will begin in some root directory.
  Vp_File_Chooser* file_chooser =
    new Vp_File_Chooser( cInFileSpec, pattern.c_str(), Vp_File_Chooser::SINGLE, title.c_str());
  file_chooser->isAscii( isAsciiInput);

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

  // Query the Vp_File_Chooser object to get file type
  if( file_chooser->isAscii() != 0) isAsciiInput = 1;
  else isAsciiInput = 0;
  delimiter_char_ = file_chooser->delimiter_char();
  
  // Load the inFileSpec string
  inFileSpec.assign( (string) cInFileSpec);
  if( isAsciiInput == 1) 
    cout << "Data_File_Manager::findInputFile: Reading ASCII data from <";
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
// Data_File_Manager::load_data_file( inFileSpec) -- Read an ASCII or binary 
// data file, resize arrays to allocate meomory.  Returns 0 if successful.
// int Data_File_Manager::load_data_file( string inFileSpecIn) 
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
  std::vector<std::string> old_column_labels; 
  if( doAppend > 0 || doMerge > 0) {
    old_nvars = points.rows();
    old_npoints = points.columns();
    old_points.resize( points.shape());
    old_points = points;
    old_column_labels = column_labels;
  }
  
  // Read data file.If there was a problem, create default data to prevent 
  // a crash, then quit before something terrible happens!
  cout << "Reading input data from <" << inFileSpec.c_str() << ">" << endl;
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

  // Remove trivial columns
  // XXX this should be a command line (and menu) option, default OFF, since
  // it can take both time and memory
  remove_trivial_columns();

  // If only one or fewer records are available then generate default data 
  // to prevent a crash, then quit before something terrible happens!
  if( nvars <= 1 || npoints <= 1) {
    cout << " -WARNING: Insufficient data, " << nvars << "x" << npoints
         << " samples.  Check delimiter setting." << endl;
    create_default_data( 4);
    return -1;
  }
  else {
    cout << "Loaded " << npoints
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
      column_labels = old_column_labels;
    }
    else if( doAppend > 0) {
      int all_npoints = npoints + old_npoints;
      points.resizeAndPreserve( nvars, npoints);
      old_points.resizeAndPreserve( nvars, all_npoints);

      old_points( blitz::Range( 0, nvars-1), blitz::Range( old_npoints, all_npoints-1)) = points;
      points.resize( old_points.shape());

      points = old_points;
      npoints = all_npoints;
      column_labels = old_column_labels;
    }
    else {
      int all_nvars = nvars + old_nvars;
      points.reverseSelf( blitz::firstDim);
      old_points.reverseSelf( blitz::firstDim);
      points.resizeAndPreserve( all_nvars, npoints);

      points( blitz::Range( nvars, all_nvars-1), blitz::Range( 0, npoints-1)) = old_points;
      points.reverseSelf( blitz::firstDim);
      nvars = all_nvars;

      // Add to list of column labels
      old_column_labels.pop_back();
      for(unsigned int i=0; i<column_labels.size(); i++) {
        old_column_labels.push_back( column_labels[ i]);
      }
      column_labels = old_column_labels;
    }

    // Free memory in case this isn't handled by the compiler.
    old_points.free();
    old_column_labels.erase( old_column_labels.begin(), old_column_labels.end());
  }

  // If we read a different number of points then we anticipated, we resize 
  // and preserve the main data array.  Note this can take lot of time and memory
  // temporarily.  XXX it would be better to handle the growth/shrinkage of this
  // array while reading.
  if( npoints != npoints_cmd_line)
    points.resizeAndPreserve( nvars, npoints);

  // Now that we know the number of variables and points we've read, we can
  // allocate and/or reallocateResize the other global arrays.
  resize_global_arrays();

  return 0;
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
    cout << "read_ascii_file_with_headers:" << endl
         << " -ERROR, couldn't open <" << inFileSpec.c_str()
         << ">" << endl;
    return 1;
  }
  else {
    cout << "read_ascii_file_with_headers:" << endl
         << " -Opening <" << inFileSpec.c_str() << ">" << endl;
  }


  // STEP 2: Read and discard the header block, saving the last line of the
  // header in the LASTHEADERLINE buffer

  // Loop: Read successive lines to find and store the last line of the 
  // header block. NOTE: Since tellg() and seekg() don't seem to work 
  // properly with getline() with all compilers, this must be accomplished 
  // by reading and counting each line explicitly.
  std::string line = "";
  std::string lastHeaderLine = "";
  int nRead = 0, nHeaderLines = 0;
  for( int i = 0; i < MAX_HEADER_LINES; i++) {
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
    if( i < nSkipHeaderLines || 
        line.length() == 0 || line.find_first_of( "!#%") == 0) {
      lastHeaderLine = line;
      nHeaderLines++;
      continue;
    }
    break;
  }
  cout << " -Header block contains " << nHeaderLines 
       << " header lines." << endl;


  // STEP 3: Generate column labels from LASTHEADERLINE if it is full.
  // Otherwise generate default label names.

  // Initialize the column labels
  nvars = 0;
  column_labels.erase( column_labels.begin(), column_labels.end());

  // If no header lines were found or the LASTHEADERLINE buffer is empty, 
  // examine the first line of the data block to determine the number of 
  // columns and generate a set of default column labels.
  if( nHeaderLines == 0 || lastHeaderLine.length() == 0) {
    
    // If the delimiter character is not a tab, replace all tabs in the
    // LINE string with spaces
    if( delimiter_char_ != '\t') replace( line.begin(), line.end(), '\t', ' ');

    // Loop: Insert the LINE string into a stream, define a buffer, read 
    // and count successive tokens, generate default column labels, and 
    // report results.  NOTE: whitespace-delimited and character-delimited
    // files must be handled differently
    std::stringstream ss( line);
    std::string buf;
    if( delimiter_char_ == ' ') {
      while( ss >> buf) {
        nvars++;
        char cbuf[ 80];
        (void) sprintf( cbuf, "%d", nvars);
        buf = "Column_";
        buf.append( cbuf);
        column_labels.push_back( buf);
      }
    }
    else { 
      while( getline( ss, buf, delimiter_char_)) {
        nvars++;
        char cbuf[ 80];
        (void) sprintf( cbuf, "%d", nvars);
        buf = "Column_";
        buf.append( cbuf);
        column_labels.push_back( buf);
      }
    }
    nvars = column_labels.size();
    cout << " -Generated " << nvars 
         << " default column labels." << endl;
  }

  // ...otherwise, header lines were found, so examine the LASTHEADERLINE 
  // buffer to extract column labels.
  else {

    // Discard the leading comment character, if any, of the LASTHEADERLINE
    // string.  The rest of the line is assumed to contain column labels
    if( lastHeaderLine.find_first_of( "!#%") == 0) 
      lastHeaderLine.erase( 0, 1);

    // Loop: Insert the LASTHEADERLINE string into a stream, define a buffer, 
    // read successive labels into the buffer, load them into the array of 
    // column labels, and report results.  NOTE: whitespace-delimited and 
    // character-delimited labels must be handled differently.  Also, it is
    // necessary to trim whitespace and verify character-delimited labels.
    std::stringstream ss( lastHeaderLine);
    std::string buf;
    if( delimiter_char_ == ' ')
      while( ss >> buf) column_labels.push_back(buf);
    else {
      while( getline( ss, buf, delimiter_char_)) {
        string::size_type notwhite = buf.find_first_not_of( " ");
        buf.erase( 0, notwhite);
        notwhite = buf.find_last_not_of( " ");
        buf.erase( notwhite+1);
        if( buf.size() <= 0) buf = "Dummy";
        column_labels.push_back( buf);
      }
    }

    nvars = column_labels.size();
    cout << " -Extracted " << nvars << " column labels." << endl;
  }

  // If there were more than NVARS_CMD_LINE variables, truncate the vector 
  // of column labels, reset NVARS, and warn the user.
  if( nvars_cmd_line > 0 && nvars > nvars_cmd_line) {
    column_labels.erase( column_labels.begin()+nvars_cmd_line, column_labels.end());
    nvars = column_labels.size();
    cout << " -WARNING: Too many variables, truncated list to " << nvars 
         << " column labels." << endl;
  }

  // Examine the number column labels.  If it is too low or high, report 
  // error, close input file, and quit.  Otherwise report success.
  if( nvars <= 1) {
    cerr << " -ERROR, insufficient number of columns, "
         << "check for correct delimiter character"
         << endl;
    inFile.close();
    return 1;
  }
  if( nvars > MAXVARS) {
    cerr << " -ERROR, too many columns, "
         << "increase MAXVARS and recompile"
         << endl;
    inFile.close();
    return 1;
  }
  cout << " -Examined header of <" << inFileSpec.c_str() << ">," << endl
       << "  There should be " << nvars 
       << " fields (columns) per record (row)" << endl;

  // If requested, add a column to contain line numbers
  if( include_line_number) {
    column_labels.push_back( string( "-line number-"));
  }
  
  // Add a final column label that says 'nothing'.
  column_labels.push_back( string( "-nothing-"));
  cout << " -column_labels:";
  int nLineLength = 17;
  for( unsigned int i=0; i < column_labels.size(); i++ ) {
    nLineLength += 1+column_labels[ i].length();
    if( nLineLength > 80) {
      cout << endl << "   ";
      nLineLength = 4 + column_labels[ i].length();
    }
    cout << " " << column_labels[ i];
  }
  cout << endl;


  // STEP 4: Allocate memory and read the data block

  // Now we know the number of variables, NVARS, so if we know the number of 
  // points (e.g. from the command line, we can size the main points array 
  // once and for all, and not waste memory.
  if( npoints_cmd_line > 0) npoints = npoints_cmd_line;
  else npoints = MAXPOINTS;
  if( include_line_number) points.resize( nvars+1, npoints);
  else points.resize( nvars, npoints);
  
  // Loop: Read successive lines from the file
  int nSkip = 0, i = 0;
  unsigned uFirstLine = 1;
  int nTestCycle = 0, nUnreadableData = 0;
  while( !inFile.eof() && i<npoints) {
  
    // Get the next line, check for EOF, and increment accounting information
    if( !uFirstLine) {
      (void) getline( inFile, line, '\n');
      if( inFile.eof()) break;  // Break here to make accounting work right
      nRead++;
    }
    DEBUG (cout << "line is: " << line << endl);
    
    // Skip blank lines and comment lines
    if( line.length() == 0 || line.find_first_of( "!#%") == 0) {
      nSkip++;
      uFirstLine = 0;
      continue;
    }
    uFirstLine = 0;
    nTestCycle++;
    
    // If the delimiter character is not a tab, replace tabs with spaces
    if( delimiter_char_ != '\t')
      replace( line.begin(), line.end(), '\t', ' ');

    // Loop: Insert the string into a stream and read it
    std::stringstream ss( line); 
    unsigned isBadData = 0;
    double x;
    for( int j=0; j<nvars; j++) {
    
      // Get the next word.  NOTE: whitespace-delimited and character-
      // delimited files must be handled differently.
      // PROBLEM: This isn't handling missing values correctly
      if( delimiter_char_ == ' ') ss >> x;
      else {
        std::string buf;
        getline( ss, buf, delimiter_char_);
        
        // Check for missing data
        string::size_type notwhite = buf.find_first_not_of( " ");
        buf.erase( 0, notwhite);
        notwhite = buf.find_last_not_of( " ");
        buf.erase( notwhite+1);
        if( buf.size() <= 0) x = bad_value_proxy_;
        else {
          stringstream bufstream;
          bufstream << buf;
          bufstream >> x;
        }
      }

      // Skip lines that don't appear to contain enough data
      if( ss.eof() && j<nvars-1) {
        cerr << " -WARNING, not enough data on line " << nRead
             << ", skipping this line!" << endl;
        isBadData = 1;
        break;
      }
      
      // Inspect the 'ss' stringstream to check for values of NULL that imply
      // certain types of bad data and/or missing values, replace these with a
      // default value, and clear error flags.  NOTE: for whitespace delimited 
      // files, simply skip lines with missing values.
      if( !ss) {
        points(j,i) = bad_value_proxy_;
        ss.clear();
      }
      else points(j,i) = (float) x;
      
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
      DEBUG (cout << "points(" << j << "," << i << ") = " << points(j,i) << endl);
    }

    // Loop: Check for bad data flags and flag this line to be skipped
    for( int j=0; j<nvars; j++) {
      if( points(j,i) < -90e99) {
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
             << " lines of unreadable data at line " << i+1 << endl;
        sErrorMessage = "Too much unreadable data in an ASCII file";
        return 1;
      }
      nTestCycle = 0;
    }

    // DIAGNOSTIC: Report parsed contents of this line
    // cout << "line[ " << i << "]:";
    // for( int j=0; j<nvars; j++) {
    //   cout << " " << points( j, i);
    // }
    // cout << endl;
    
    // If data were good, increment the number of lines
    if( !isBadData) {
      i++;
      if( (i+1)%10000 == 0) cerr << "  Read " << i+1 << " lines." << endl;
    }
  }
  

  // STEP 5: Update NPOINTS, report results of file read operation to the 
  // console, close input file, and report success
  npoints = i;
  cout << " -Finished reading data block with " << npoints
       << " records." << endl;
  cout << "  " << nHeaderLines 
       << " header + " << i 
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
    cout << "read_binary_file_with_headers: ERROR" << endl
         << " -Couldn't open binary file <" << inFileSpec.c_str() 
         << ">" << endl;
    return 1;
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
    cout << " -ERROR: Header string is too long, "
         << "increase MAX_HEADER_LENGTH and recompile"
         << endl;
    fclose( pInFile);
    return 1;
  }
  std::string line;
  line.assign( cBuf);

  // Initialize the column labels
  nvars = 0;
  column_labels.erase( column_labels.begin(), column_labels.end());

  // Loop: Examine the line to see if it contains any tabs.  If it doesn't,
  // unpack labels as if they were whitespace-delimited.  Otherwise, unpack 
  // labels as if they were tab-delimited and erase any leading and traling
  // whitespace.
  std::stringstream ss( line);
  std::string buf;
  int isTabDelimited = 0;
  // cout << "POOKA (" << line.c_str() << ") POO" << endl;
  // cout << "-first tab position is " << line.find( '\t') << "/" << line.size() << endl;
  if( line.find( '\t') < 0 || line.size() <= line.find( '\t'))
    while( ss >> buf) column_labels.push_back(buf);
  else {
    isTabDelimited = 1;
    while( getline( ss, buf, '\t')) {
      string::size_type notwhite = buf.find_first_not_of( " ");
      buf.erase( 0, notwhite);
      notwhite = buf.find_last_not_of( " ");
      buf.erase( notwhite+1);
      column_labels.push_back(buf);
    }
  }
  nvars = column_labels.size();
  int nvars_in = nvars;

  // If there were more than NVARS_CMD_LINE variables, truncate the vector of 
  // column labels, reset NVARS, and warn the user.
  if( nvars_cmd_line > 0 && nvars > nvars_cmd_line) {
    column_labels.erase( column_labels.begin()+nvars_cmd_line, column_labels.end());
    nvars = column_labels.size();
    cout << " -WARNING: Too many variables, truncated list to " << nvars 
         << " column labels." << endl;
  }

  // Examine and report content of header
  if( nvars > MAXVARS) {
    cerr << " -ERROR: Too many columns, "
         << "increase MAXVARS and recompile"
         << endl;
    fclose( pInFile);
    return 1;
  }

  // If requested, include line number
  if (include_line_number) {
    column_labels.push_back( string( "-line number-"));
  }

  // Add a final column label that says 'nothing'.
  column_labels.push_back(string("-nothing-"));

  // Report label information
  int nLabels = column_labels.size();
  cout << "Read " << nLabels << "/" << nvars;
  if( isTabDelimited <= 0) cout << " whitespace-delimited ";
  else cout << " tab-delimited ";
  cout << "column_labels:" << endl;
  for( unsigned int i=0; i < column_labels.size(); i++ ) {
    cout << column_labels[i];
    if( i < column_labels.size()-1) cout << ", ";
  }  
  cout << endl;
  cout << " -About to read " << nvars
       << " variables from a binary file with " << nvars_in
       << " fields (columns) per record (row)" << endl;

  // Now we know the number of variables (nvars), so if we know the number of 
  // points (e.g. from the command line) we can size the main points array 
  // once and for all, and not waste memory.
  if( npoints_cmd_line > 0) npoints = npoints_cmd_line;
  else npoints = MAXPOINTS;
  if( include_line_number) {
    points.resize( nvars+1, npoints);
  } else {
    points.resize( nvars, npoints);
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
    cout << " -Attempting to read binary file in"
         << " column-major order" << endl;
    blitz::Array<float,1> vars( nvars_in);
    blitz::Range NVARS( 0, nvars-1);
    if( !vars.isStorageContiguous()) {
      cerr << " -ERROR: Tried to read into a noncontiguous buffer."
           << endl;
      fclose( pInFile);
      return -1;
    }

    // Loop: Read successive rows from file
    int i;
    for( i=0; i<npoints; i++) {
    
      // Read the next NVAR values using conventional C-style fread.
      unsigned int ret = 
        fread( (void *)(vars.data()), sizeof(float), nvars_in, pInFile);
      
      // Check for normal termination
      if( ret == 0 || feof( pInFile)) {
        cerr << " -Finished reading file at row[ " << i
             << "] with ret, inFile.eof() = ( " << ret
             << ", " << feof( pInFile) << ")" << endl;
        break;
      }
      
      // If wrong number of values was returned, report error.
      if( ret != (unsigned int)nvars_in) {
        cerr << " -ERROR reading row[ " << i+1 << "], "
             << "returned values " << ret 
             << " NE number of variables " << nvars_in << endl;
        fclose( pInFile);
        return 1;
      }

      // Load data array and report progress
      points( NVARS,i) = vars( NVARS);
      if( i>0 && (i%10000 == 0)) 
        cout << "  Reading row " << i << endl;
    }

    // Report success
    cout << " -Finished reading " << i << " rows." << endl;
    npoints = i;
  }

  // ...or read file in Row Major order
  else {
    cout << " -Attempting to read binary file in"
         << "row-major order with nvars=" << nvars_in
         << ", npoints=" << npoints << endl;
    if( npoints_cmd_line == 0) {
      cerr << " -ERROR, --npoints must be specified for"
           << " --inputformat=rowmajor"
           << endl;
      fclose( pInFile);
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
        return 1;
      }

      // Load data array and report progress
      points( i, NPTS) = vars( NPTS);
      cout << "  Reading column " << i+1 << endl;
    }
    
    // Report success
    cout << " -Finished reading " << i+i
         << " columns" << endl;
  }
  
  // Close input file and terminate
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
  if( useSelectedData != 0) title = "Write all data to file";
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
  if( file_chooser->writeSelectionInfo() != 0) writeSelectionInfo_ = 1;
  else writeSelectionInfo_ = 0;

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
  if( isAsciiOutput != 1) return write_binary_file_with_headers();
  else return write_ascii_file_with_headers();
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

    // Loop: Write column labels to the header
    for( int i=0; i < nvars_out; i++ ) {
      if( i == 0) os << "!" << setw( 12) << column_labels[ i];
      else os << delimiter_char_ << " " << setw( 13) << column_labels[ i];
      // else os << " " << setw( 13) << column_labels[ i];
    }
    if( writeSelectionInfo_ != 0) os << " selection";
    os << endl;
    
    // Loop: Write successive ASCII records to the data block using the
    // "default" floatfield format.  This causes integers to be written
    // as integers, floating point as floating point, and numbers with
    // large or small magnitude as scientific. 
    // floats.
    os.precision(8); 
    os.unsetf (ios::scientific); // force floatfield to default
    int rows_written = 0;
    for( int irow = 0; irow < npoints; irow++) {
      if( useSelectedData == 0 || selected( irow) > 0) {
        for( int jcol = 0; jcol < nvars_out; jcol++) {
          // if( jcol > 0) os << " ";
          if( jcol > 0) os << delimiter_char_ << " ";
          os << points( jcol, irow);
        }
        if( writeSelectionInfo_ != 0) os << " " << selected( irow);
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
    // for( int i=0; i < nvars_out; i++ ) os << column_labels[ i] << " ";
    // if( writeSelectionInfo_ != 0) os << "selection";
    for( int i=0; i < nvars_out; i++ ) {
      os << column_labels[ i] << " ";;
      if( i<nvars-1) os << '\t';
    }
    if( writeSelectionInfo_ != 0) os << '\t' << "selection";
    os << endl;
    
    // Loop: Write data and report any problems
    int nBlockSize = nvars*sizeof(float);
    int rows_written = 0;
    for( int i=0; i<npoints; i++) {
      if( useSelectedData == 0 || selected( i) > 0) {
        vars = points( NVARS, i);
        os.write( (const char*) vars.data(), nBlockSize);
				if( writeSelectionInfo_ != 0) {
          float fselection = (float)(selected(i));
	        os.write((const char*)&fselection, sizeof(float));
				}
        if( os.fail()) {
          cerr << "Error writing to" << outFileSpec.c_str() << endl;
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
           << column_labels[ current] << endl;
      for( int j=current; j<nvars-1; j++) {
        points( j, NPTS) = points( j+1, NPTS);
        column_labels[ j] = column_labels[ j+1];
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
      nLineLength += 1+column_labels[ i].length();
      if( nLineLength > 80) {
        cout << endl << "   ";
        nLineLength = 2 + column_labels[ i].length();
      }
      cout << " " << column_labels[ i];
    }
    cout << endl;
    
    // Resize array and report results
    // XXX need to trim column_labels to size nvars+1 
    points.resizeAndPreserve( nvars, npoints);
    column_labels[ nvars] = string( "-nothing-");
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
  
  // done elsewhere, if necessary:
  // points.resizeAndPreserve(nvars,npoints);  
  
  // If line numbers are to be included as another field (column) of data array, create them
  // as the last column.
  if (include_line_number) {
    for (int i=0; i<npoints; i++) {
      points(nvars-1,i) = (float)(i+1);
    }
  }
  
  // Resize and reinitialize list of ranked points to reflect the fact that no
  // ranking has been done.
  ranked_points.resize( nvars, npoints);
  ranked.resize( nvars);
  ranked = 0;
  
  // Resize and reinitialize selection related arrays and flags.
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
  column_labels.erase( column_labels.begin(), column_labels.end());
  for( int i=0; i<nvars; i++) {
    ostringstream buf;
    buf << "default_"
        << setw( 3) << setfill( '0')
        << i
        << setfill( ' ') << " ";
    column_labels.push_back( buf.str());
  }
  column_labels.push_back( string( "-nothing-"));

  // Report colum labels
  cout << " -column_labels:";
  int nLineLength = 17;
  for( unsigned int i=0; i < column_labels.size(); i++ ) {
    nLineLength += 1+column_labels[ i].length();
    if( nLineLength > 80) {
      cout << endl << "   ";
      nLineLength = 4 + column_labels[ i].length();
    }
    cout << " " << column_labels[ i];
  }
  cout << endl;
  cout << " -Generated default header with " << nvars
       << " fields" << endl;

  // Resize data array to avoid the madening frustration of segmentation 
  // errors!  Important!
  npoints = 2;
  points.resize( nvars, npoints);

  // Loop: load each variable with 0 and 1.  These two loops are kept separate
  // for clarity and to facilitate changes
  for( int i=0; i<nvars; i++) {
    points( i, 0) = 0.0;
    points( i, 1) = 1.0;
  }

  // Resize global arrays
  resize_global_arrays ();

  // Report results
  cout << "Generated default data with " << npoints 
       << " points and " << nvars << " variables" << endl;
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
