// viewpoints - interactive linked scatterplots and more.
// copyright 2005 Creon Levit, all rights reserved.
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
// Modified: P. R. Gazis  16-MAR-2007
//***************************************************************************

// Include the necessary include libraries
#include "include_libraries_vp.h"

// Include globals
#include "global_definitions_vp.h"

// Include associated headers and source code
#include "data_file_manager.h"
#include "plot_window.h"

// These should not be necessary
// #include "New_File_Chooser.H"   // PRG's new file chooser
// #include "New_File_Chooser.cpp"   // PRG's new file chooser

// Set static data members for class Data_File_Manager::

// Define and set maximums length of header lines and number of lines in the 
// header block
const int Data_File_Manager::MAX_HEADER_LENGTH = MAXVARS*100;
const int Data_File_Manager::MAX_HEADER_LINES = 2000;

const bool include_line_number = false; // MCL XXX This should be an option

//***************************************************************************
// Data_File_Manager::Data_File_Manager() -- Default constructor, calls the
// initializer.
Data_File_Manager::Data_File_Manager() : isAsciiInput( 1), 
  isAsciiOutput( 0), useSelectedData( 0), isColumnMajor( 0)
{
  sPathname = ".";  // Default pathname
  initialize();
}

//***************************************************************************
// Data_File_Manager::initialize() -- Reset control parameters.
void Data_File_Manager::initialize()
{
  // Set default values for file reads.
  isAsciiInput = 1;
  isAsciiOutput = 0;
  useSelectedData = 0;

  isColumnMajor = 1;
  nSkipHeaderLines = 1;  // Number of header lines to skip
  // sPathname = ".";  // Default pathname
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
// Class New_File_Chooser is used in preference to the New_File_Chooser method 
// to obtain access to member functions such as directory() and to allow the 
// possibility of a derived class with additional controls in the 
// file_chooser window.  Returns 0 if successful.  
int Data_File_Manager::findInputFile()
{
  // Generate text, file extensions, etc, for this file type
  char* title = NULL;
  char* pattern = NULL;
  if( isAsciiInput) {
    title = "Read ASCII input from file";
    pattern = "*.{txt,lis,asc}\tAll Files (*)";
  }
  else {
    title =  "Read binary input from file";
    pattern = "*.bin\tAll Files (*)";
  }

  // Initialize read status and filespec.  NOTE: cInFileSpec is defined as
  // const char* for use with New_File_Chooser, which means it could be 
  // destroyed by the relevant destructors!
  const char *cInFileSpec = directory().c_str();

  // Instantiate and show an New_File_Chooser widget.  NOTE: The pathname must
  // be passed as a variable or the window will begin in some root directory.
  New_File_Chooser* file_chooser =
    new New_File_Chooser( cInFileSpec, pattern, New_File_Chooser::SINGLE, title);

  // Loop: Select fileSpecs until a non-directory is obtained
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

    // For some reason, the fl_filename_isdir method doesn't seem to work, so 
    // try to open this file to see if it is a directory.
    FILE* pFile = fopen( cInFileSpec, "r");
    if( pFile == NULL) {
      file_chooser->directory( cInFileSpec);
      directory( (string) cInFileSpec);
      continue;
    }
    fclose( pFile);
    break;         
  } 

  // If no file was specified then quit and deallocate the 
  // New_File_Chooser object
  if( cInFileSpec == NULL) {
    cerr << "Data_File_Manager::findInputFile: "
         << "No input file was specified" << endl;
    delete file_chooser;  // WARNING! Destroys cInFileSpec!
    return -1;
  }

  // Load inFileSpec
  inFileSpec.assign( (string) cInFileSpec);
  if( isAsciiInput == 1) 
    cout << "Data_File_Manager::findInputFile: Reading ASCII data from <";
  else 
    cout << "Data_File_Manager::findInputFile: Reading binary data from <";
  cout << inFileSpec.c_str() << ">" << endl;

  // Deallocate file_chooser
  delete file_chooser;  // WARNING! This destroys cInFileSpec!

  // Perform partial initialization and return success
  nSkipHeaderLines = 1;
  npoints_cmd_line = 0;
  nvars_cmd_line = 0;
  npoints = MAXPOINTS;
  nvars = MAXVARS;

  return 0;
}

//***************************************************************************
// Data_File_Manager::load_data_file( inFileSpec) -- Copy input filespec, then
// invoke load_data_file to load  this file.
int Data_File_Manager::load_data_file( string inFileSpec) 
{
  input_filespec( inFileSpec);
  return load_data_file();
}

//***************************************************************************
// Data_File_Manager::load_data_file( inFileSpec) -- Read an ASCII or binary 
// data file, resize arrays to allocate meomory.
// Returns 0 if successful.
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
  
  // Read data file and report results
  cout << "Reading input data from <" << inFileSpec.c_str() << ">" << endl;
  int iReadStatus = 0;
  if( isAsciiInput == 0)
    iReadStatus = read_binary_file_with_headers();
  else
    iReadStatus = read_ascii_file_with_headers();

  if( iReadStatus != 0) {
    cout << "Data_File_Manager::load_data_file: "
         << "Problems reading file <" << inFileSpec.c_str() << ">" << endl;
    return -1;
  }
  else
    cout << "Data_File_Manager::load_data_file: Finished reading file <" 
         << inFileSpec.c_str() << ">" << endl;

  // Remove trivial columns
  remove_trivial_columns();

  // If only one or fewer records are available then quit before something 
  // terrible happens!
  if( npoints <= 1) {
    cout << "Insufficient data, " << npoints
         << " samples." << endl;
    return -1;
  }
  else {
    cout << "Loaded " << npoints
         << " samples with " << nvars << " fields" << endl;
  }
  
  // MCL XXX Now that we're done reading, we can update nvars to count possible
  // additional program-generated variables (presently only the line number).
  if (include_line_number) {
    nvars = nvars+1;
  }
  
  // If we read a different number of points then we anticipated, we resize 
  // and preserve the main data array.  Note this can take lot of time and memory
  // temporarily.  XXX it would be better to handle the growth/shrinkage of this
  // array while reading.
  if( npoints != npoints_cmd_line)
    points.resizeAndPreserve( nvars, npoints);

  // Now that we know the number of variables and the number of points
  // that we've read, we can allocate/reallocateResize the other global arrays.
  resize_global_arrays ();

  // Set the 'selected' flag to initialize the selections to zero.  NOTE: 
  // Since everyone starts of as "non-selected", it is no longer correct to 
  // initialize the selection index array explicitly.
  // for( int i=0; i<npoints; i++) indices_selected(0,i)=i;
  selected = 0;

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

  // STEP 2: Read and discard the header block

  // Loop: Read successive lines to find the last line of the header block and
  // the beginning of the data block. NOTE: Since tellg() and seekg() don't 
  // seem to work properly with getline with all compilers, this must be 
  // accomplished by keeping track of lines explicitly.
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

  // STEP 3: Generate column labels

  // Initialize the column labels
  nvars = 0;
  column_labels.erase( column_labels.begin(), column_labels.end());

  // DIAGNOSTIC
  // delimiter_char = ',';
  
  // If no header lines were found or the LASTHEADERLINE buffer is empty, 
  // examine the first line of the data block to determine the number of 
  // columns and generate a set of column labels.
  if( nHeaderLines == 0 || lastHeaderLine.length() == 0) {
    
    // Invoke the REPLACE method from <algorithm> to replace tabs and/or a
    // user-specified delimiter character so that operator>> will work.
    replace( line.begin(), line.end(), '\t', ' ');
    if( delimiter_char != ' ') {
      replace( line.begin(), line.end(), delimiter_char, ' ');
    }

    // Loop: Insert the input string into a stream, define a buffer, read 
    // and count successive tokens, generate default column labels, and 
    // report results.
    std::stringstream ss( line);
    std::string buf;
    while( ss >> buf) {
      nvars++;
      char cbuf[ 80];
      (void) sprintf( cbuf, "%d", nvars);
      buf = "Column_";
      buf.append( cbuf);
      column_labels.push_back( buf);
    }
    nvars = column_labels.size();
    cout << " -Generated " << nvars 
         << " default column labels." << endl;
  }

  // ...otherwise, header lines were found, so examine the LASTHEADERLINE 
  // buffer to extract column labels.
  else {

    // Discard the leading comment character, if any.  The rest of the 
    // line is assumed to contain column labels separated by whitespace
    if( lastHeaderLine.find_first_of( "!#%") == 0) 
      lastHeaderLine.erase( 0, 1);
    
    // Invoke the REPLACE method from <algorithm> to replace tabs and/or a
    // user-specified delimiter character so that operator>> will work.
    replace( lastHeaderLine.begin(), lastHeaderLine.end(), '\t', ' ');
    if( delimiter_char != ' ') {
      replace (lastHeaderLine.begin(), lastHeaderLine.end(), delimiter_char, ' ');
    }

    // Loop: Insert the input string into a stream, define a buffer, read 
    // successive labels into the buffer, load them into the array of column
    // labels, and report results
    std::stringstream ss( lastHeaderLine);
    std::string buf;
    while( ss >> buf) column_labels.push_back(buf);
    nvars = column_labels.size();
    cout << " -Extracted " << nvars 
         << " column labels." << endl;
  }

  // If there were more than nvars_cmd_line variables, truncate the vector of 
  // column labels and reset nvars.
  if( nvars_cmd_line > 0 && nvars > nvars_cmd_line) {
    column_labels.erase( column_labels.begin()+nvars_cmd_line, column_labels.end());
    nvars = column_labels.size();
    cout << " -Truncated list to " << nvars 
         << " column labels." << endl;
  }

  // Examine the column labels for errors and report results
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
  if (include_line_number) {
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

  // STEP 4: Allocate memory and read data block

  // Now we know the number of variables, NVARS, so if we know the number of 
  // points (e.g. from the command line, we can size the main points array 
  // once and for all, and not waste memory.
  if( npoints_cmd_line > 0) npoints = npoints_cmd_line;
  else npoints = MAXPOINTS;
  if (include_line_number) points.resize( nvars+1, npoints);
  else points.resize( nvars, npoints);
  
  // Loop: Read successive lines from the file
  int nSkip = 0, i = 0;
  unsigned uFirst = 1;
  int nTestCycle = 0, nUnreadableData = 0;
  while( !inFile.eof() && i<npoints) {
  
    // Get the next line, check for EOF, and increment accounting information
    if( !uFirst) {
      (void) getline( inFile, line, '\n');
      if( inFile.eof()) break;  // Break here to make accounting work right
      nRead++;
    }
    DEBUG (cout << "line is: " << line << endl);
    
    // Skip blank lines and comment lines
    if( line.length() == 0 || line.find_first_of( "!#%") == 0) {
      nSkip++;
      uFirst = 0;
      continue;
    }
    uFirst = 0;
    nTestCycle++;
    
    // Replace tabs with ' ' so that operator>> will work
    replace( line.begin(), line.end(), '\t', ' ');

    // Loop: Insert the string into a stream and read it
    std::stringstream ss( line); 
    unsigned isBadData = 0;
    double x;
    for( int j=0; j<nvars; j++) {
      
      // Invoke opperator>> to read and parse floating point data.  In general,
      // this will recognize certain types of non-numeric data.
      // float xTrial;
      // if( !( ss >> xTrial)) {
      //   x = bad_value_proxy;
      //   ss.clear();
      // }
      // else x = xTrial;
      ss >> x;

      // Skip lines that do not appear to contain enough data
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
        points(j,i) = bad_value_proxy;
        ss.clear();
      }
      else {
        points(j,i) = (float) x;
      }
      
      // Advance past the next field delimiter character, or to the end of the 
      // line, whichever comes first.
      ss.ignore( line.length(), delimiter_char);

      // Check for unreadable data and flag this line to be skipped.  NOTE:
      // This should never happen, because error flags were cleared above.
      if( !ss.good() && j<nvars-1) {
        cerr << " -WARNING, unreadable data "
             << "(binary or ASCII?) at line " << nRead
             << " column " << j+1 << "," << endl
             << "  skipping entire line." << endl;
        // cerr << "  <" << line.c_str() << ">" << endl;
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
    
    // If data were good, increment number of lines
    if( !isBadData) {
      i++;
      if( (i+1)%10000 == 0) cerr << "  Read " << i+1 << " lines." << endl;
    }
  }
  
  // Update NPOINTS and report results
  npoints = i;
  cout << " -Finished reading data block with " << npoints
       << " records." << endl;
  cout << "  " << nHeaderLines 
       << " header + " << i 
       << " good data + " << nSkip 
       << " skipped lines = " << nRead << " total." << endl;

  // STEP 5: Close input file, report results of file read operation to the 
  // console, and return success
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

  // Loop: unpack the string of header information to obtain column labels.
  std::stringstream ss( line);
  std::string buf;
  while( ss >> buf) column_labels.push_back(buf);
  nvars = column_labels.size();
  int nvars_in = nvars;

  // If there were more than nvars_cmd_line variables, truncate the vector of 
  // column labels and reset nvars.
  if( nvars_cmd_line > 0 && nvars > nvars_cmd_line) {
    column_labels.erase( column_labels.begin()+nvars_cmd_line, column_labels.end());
    nvars = column_labels.size();
    cout << " -Truncated list to " << nvars 
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

  if (include_line_number) {
    column_labels.push_back( string( "-line number-"));
  }

  // Add a final column label that says 'nothing'.
  column_labels.push_back(string("-nothing-"));

  cout << "column_labels = ";
  for( unsigned int i=0; i < column_labels.size(); i++ ) {
    cout << column_labels[i] << " ";
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
  if (include_line_number) {
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

  // Read file in Column Major order
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

  // Read file in Row Major order
  if( isColumnMajor != 1) {
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
// Class New_File_Chooser is used in preference to the New_File_Chooser method 
// to obtain access to member functions such as directory() and to allow the 
// possibility of a derived class with additional controls in the 
// file_chooser window.  Returns 0 if successful.  
int Data_File_Manager::findOutputFile()
{
  // Generate query text and list file extensions, etc for this file type
  char* title = NULL;
  char* pattern = NULL;
  if( isAsciiOutput) {
    if( useSelectedData != 0) title = "Write ASCII output to file";
    else title = "Write selected ASCII output to file";
    pattern = "*.{txt,lis,asc}\tAll Files (*)";
  }
  else {
    if( useSelectedData != 0) title = "Write binary output to file";
    else title = "Write selected binary output to file";
    pattern = "*.bin\tAll Files (*)";
  }

  // Initialize output filespec.  NOTE: cOutFileSpec is defined as const 
  // char* for use with New_File_Chooser, which means it could be destroyed 
  // by the relevant destructors!
  const char *cOutFileSpec = sPathname.c_str();

  // Instantiate and show an New_File_Chooser widget.  NOTE: The pathname 
  // must be passed as a variable or the window will begin in some root 
  // directory.
  New_File_Chooser* file_chooser = 
    new New_File_Chooser( 
      cOutFileSpec, pattern, New_File_Chooser::CREATE, title);

  // Loop: Select succesive filespecs until a non-directory is obtained
  while( 1) {
    if( cOutFileSpec != NULL) file_chooser->directory( cOutFileSpec);

    // Loop: wait until the selection is done, then extract the value.  NOTE: 
    // This usage of while and Fl::wait() seems strange.
    file_chooser->show();
    while( file_chooser->shown()) Fl::wait();
    cOutFileSpec = file_chooser->value();   

    // If no file was specified then quit
    if( cOutFileSpec == NULL) break;

    // If this is a new file, it can't be opened for read, and we're done
    FILE* pFile = fopen( cOutFileSpec, "r");
    if( pFile == NULL) break;
    
    // For some reason, the fl_filename_isdir method doesn't seem to work, so
    // make sure this file is closed, the try to open this file to see if it 
    // is a directory.  If it is, update pathname and contnue.
    if( pFile != NULL) fclose( pFile);
    pFile = fopen( cOutFileSpec, "w");
    if( pFile == NULL) {
      file_chooser->directory( cOutFileSpec);
      sPathname.erase( sPathname.begin(), sPathname.end());
      sPathname.append( cOutFileSpec);
      continue;
    }
    
    // If we got this far, the file must exist and be available to be
    // overwritten, so open a confirmation window and wait for the button 
    // handler to do something.
    confirmResult = CANCEL_FILE;
    make_confirm_window( cOutFileSpec);

    // If this was a 'CANCEL' request, make sure file is closed, then return.
    if( confirmResult == CANCEL_FILE) {
      fclose( pFile);
      return -1;
    }
    
    // If this was a 'NO' request, make sure the pathname is correct, then 
    // continue.
    if( confirmResult == NO_FILE) {
      file_chooser->directory( sPathname.c_str());
      fclose( pFile);
      continue;
    }
    
    // We've verified that this file exists and the user intends to overwrite
    // it, so close it and move on
    confirmResult = YES_FILE;
    fclose( pFile);
    break;
  } 

  // Obtain file name using the FLTK member function.  This doesn't work, but
  // is retained for descriptive purposes.
  // char *cOutFileSpec = 
  //   New_File_Chooser( "write ASCII output to file", NULL, NULL, 0);

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

  // Deallocate the New_File_Chooser object
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
      cerr << "Error opening" << outFileSpec.c_str() 
           << "for writing" << endl;
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
      else os << " " << setw( 13) << column_labels[ i];
    }
    os << endl;
    
    // Loop: Write successive ASCII records to the data block in 8-digit 
    // scientific notation.
    os << setiosflags( ios::scientific) << setw( 8);
    int rows_written = 0;
    for( int irow = 0; irow < npoints; irow++) {
      if( useSelectedData == 0 || selected( irow) > 0) {
        for( int jcol = 0; jcol < nvars_out; jcol++) {
          if( jcol > 0) os << " ";
          os << points( jcol, irow);
        }
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
      cerr << "Error opening" << outFileSpec.c_str() 
           << "for writing" << endl;
      return -1;
    }
    
    // Loop: Write column labels to the header
    for( int i=0; i < nvars_out; i++ ) os << column_labels[ i] << " ";
    os << endl;
    
    // Loop: Write data and report any problems
    int nBlockSize = nvars*sizeof(float);
    int rows_written = 0;
    for( int i=0; i<npoints; i++) {
      if( useSelectedData == 0 || selected( i) > 0) {
        vars = points( NVARS, i);
        os.write( (const char*) vars.data(), nBlockSize);
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
    cout << "Removed " << nvars_save - nvars << "columns:";
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
  
  // Resize selection arrays
  newly_selected.resize( npoints);
  selected.resize( npoints);
  previously_selected.resize( npoints);
  saved_selection.resize(npoints);
  Plot_Window::indices_selected.resize(nplots+1,npoints);
  Plot_Window::number_selected.resize(nplots+1);

  // Initialize selection arrays
  Plot_Window::number_selected = 0; 
  Plot_Window::indices_selected = 0;
  newly_selected = 0;
  selected = 0;
  previously_selected = 0;
  saved_selection = 0;
  nselected = 0;
  selection_is_inverted = false;

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
  return sPathname; 
}
     
//***************************************************************************
// Data_File_Manager::directory( sPathname) -- Set pathname.
void Data_File_Manager::directory( string sPathnameIn)
{
  sPathname = sPathnameIn;
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

//***************************************************************************
// Data_File_Manager::make_confirm_window( output_file_name) -- Confirmation 
// window
void Data_File_Manager::make_confirm_window( const char* output_file_name)
{
  // Intialize flag and destroy any existing window
  // MCL XXX rule #2: "Compile cleanly at high warning levels." 
  confirmResult = CANCEL_FILE;
  if( confirm_window != NULL) confirm_window->hide();
  
  // Create confirmation window
  Fl::scheme( "plastic");  // optional
  confirm_window = new Fl_Window( 400, 100, "Confirm File Overwrite");
  confirm_window->begin();
  confirm_window->selection_color( FL_BLUE);
  confirm_window->labelsize( 10);
  
  // Compose text. NOTE use of @@ in conjunction with label()
  string sConfirm = "File '";
  sConfirm.append( output_file_name);
  sConfirm.append( "' already exists.\nOverwrite exisiting file?\n");

  // Write text to box label and align it inside box
  Fl_Box* output_box = new Fl_Box( 5, 5, 390, 60, sConfirm.c_str());
  // output_box->box( FL_SHADOW_BOX);
  output_box->box( FL_NO_BOX);
  output_box->color( 7);
  output_box->selection_color( 52);
  output_box->labelfont( FL_HELVETICA);
  output_box->labelsize( 15);
  output_box->align( FL_ALIGN_TOP|FL_ALIGN_CENTER|FL_ALIGN_INSIDE);

  // Define buttons and invoke callback functions to handle them
  Fl_Button* yes_button = new Fl_Button( 90, 70, 60, 25, "&Yes");
  Fl_Button* no_button = new Fl_Button( 170, 70, 60, 25, "&No");
  Fl_Button* cancel_button = new Fl_Button( 250, 70, 60, 25, "&Cancel");

  // Done creating the confirmation window
  confirm_window->resizable( confirm_window);
  confirm_window->end();
  confirm_window->show();
  
  // Loop: While the window is open, wait and check the read queue until the 
  // right widget is activated
  while( confirm_window->shown()) {
    Fl::wait();
    for( ; ;) {   // Is this loop needed?
      Fl_Widget* o = Fl::readqueue();
      if( !o) break;

      // Has the window been closed or a button been pushed?
      if( o == yes_button) {
        confirmResult = YES_FILE;
        confirm_window->hide();
        return;
      }
      else if( o == no_button) {
        confirmResult = NO_FILE;
        confirm_window->hide();
        return;
      }
      else if( o == cancel_button) {
        confirmResult = CANCEL_FILE;
        confirm_window->hide();
        return;
      }
      else if( o == confirm_window) {
        confirmResult = CANCEL_FILE;

        // Don't need to hide window because user has already deleted it
        // confirm_window->hide();
        return;
      }
    }
  }
}
