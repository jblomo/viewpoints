// viewpoints - interactive linked scatterplots and more.
// copyright 2005 Creon Levit, all rights reserved.
//*****************************************************************
// File name: data_file_manager.cpp
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
// Purpose: Source code for <data_file_manager.h>
//
// Author: Creon Levit    unknown (really? I knew him well)
// Modified: P. R. Gazis  26-OCT-2006
//*****************************************************************

// Include the necessary include libraries
#include "include_libraries_vp.h"

// Include globals
#include "global_definitions_vp.h"

// Include associated headers and source code
#include "data_file_manager.h"

// Set static data members for class data_file_manager::
//

// Define and set maximums length of header lines and number of
// lines in the header block
const int data_file_manager::MAX_HEADER_LENGTH = MAXVARS*100;
const int data_file_manager::MAX_HEADER_LINES = 2000;

//*****************************************************************
// data_file_manager::data_file_manager() -- Default constructor,
// calls initializer.
data_file_manager::data_file_manager()
{
  sPathname = ".";  // Default pathname
  initialize();
}

//*****************************************************************
// data_file_manager::initialize() -- Reset control parameters.
void data_file_manager::initialize()
{
  // Set default values for file reads.
  // format=ASCII;   // default input file format
  ordering=COLUMN_MAJOR;   // default input data ordering
  nSkipHeaderLines = 1;  // Number of header lines to skip
  // sPathname = ".";  // Default pathname
  inFileSpec = "";  // Default input filespec
  uWriteAll = 1;   // Write all data by default

  // Initialize the number of points and variables specified by 
  // the command line arguments.  NOTE: 0 means read to EOF or
  // end of line.
  npoints_cmd_line = 0;
  nvars_cmd_line = 0;
  
  // Initialize number of points and variables
  npoints = MAXPOINTS;
  nvars = MAXVARS;
}

//*****************************************************************
// data_file_manager::load_data_file( inFileSpec) -- Read an ASCII 
// or binary data file, resize arrays to allocate meomory, and set 
// identity array.  Returns 0 if successful.
// MCL XXX - refactor this with read_data()
int data_file_manager::load_data_file( string inFileSpecIn) 
{
  // PRG XXX: Would it be possible or desirable to examine the 
  // file directly here to determine or verify its format?

  // Load input filespec
  inFileSpec = inFileSpecIn;
         
  // Read data file and report results
  cout << "Reading input data from <" << inFileSpec.c_str() << ">" << endl;
  int iReadStatus = 0;
  if( format == BINARY) 
    iReadStatus = read_binary_file_with_headers();
  else if( format == ASCII)
    iReadStatus = read_ascii_file_with_headers();

  if( iReadStatus != 0) {
    cout << "Problems reading file <" << inFileSpec.c_str() << ">" << endl;
    return -1;
  }
  else
    cout << "Finished reading file <" << inFileSpec.c_str() << ">" << endl;

  // Remove trivial columns
  remove_trivial_columns();

  // If only one or fewer records are available then quit before 
  // something terrible happens!
  if( npoints <= 1) {
    cout << "Insufficient data, " << npoints
         << " samples." << endl;
    return -1;
  }
  else {
    cout << "Loaded " << npoints
         << " samples with " << nvars << " fields" << endl;
  }
  
  // If we read a different number of points then we anticipated, 
  // we resize and preserve.  Note this can take lot of time and 
  // memory, temporarily.
  if( npoints != npoints_cmd_line)
    points.resizeAndPreserve( nvars, npoints);

  // Resize global arrays
  resize_global_arrays ();

  // Initialize selection array to zero
  // NOTE: Since everyone starts of as "non-selected", it is no 
  // longer correct to initialize the selection index array.
  // for( int i=0; i<npoints; i++) indices_selected(0,i)=i;
  selected = 0;

  // Load the identity array
  cout << "Making identity array, a(i)=i" << endl;
  for( int i=0; i<npoints; i++) identity( i)=i;
  return 0;
}

//*****************************************************************
// data_file_manager::read_ascii_file_with_headers() -- Open an 
// ASCII file for input, read and discard the headers, read the 
// data block, and close the file.  Returns 0 if successful.
int data_file_manager::read_ascii_file_with_headers() 
{
  // Attempt to open input file and make sure it exists
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

  // Loop: Read successive lines to find the last line of the header 
  // block and the beginning of the data block. NOTE: Since tellg() 
  // and seekg() don't seem to work properly with getline with all 
  // compilers, this must be accomplished by keeping track of lines 
  // explicitly.
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
    
    // If this line is supposed to be skipped or if it begins with 
    // a comment character, skip it and update the LASTHEADERLINE 
    // buffer
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

  // Initialize the column labels
  nvars = 0;
  column_labels.erase( column_labels.begin(), column_labels.end());

  // If no header lines were found or the LASTHEADERLINE buffer is
  // empty, examine the first line of the data block to determine 
  // the number of columns and generate a set of column labels.
  if( nHeaderLines == 0 || lastHeaderLine.length() == 0) {
    std::stringstream ss( line);
    std::string buf;
    while( ss >> buf) {
      nvars++;
      char cbuf[ 80];
      (void) sprintf(cbuf, "%d", nvars);
      buf = "Column_";
      buf.append( cbuf);
      column_labels.push_back( buf);
    }
    nvars = column_labels.size();
    cout << " -Generated " << nvars 
         << " default column labels." << endl;
  }

  // ...otherwise, examine the LASTHEADERLINE buffer to extract 
  // column labels 
  else {

    // Discard leading comment character, if any.  The rest of the 
    // line is assumed to contain column labels separated by 
    // whitespace
    if( lastHeaderLine.find_first_of( "!#%") == 0) 
      lastHeaderLine.erase( 0, 1);
      
    // Loop: Insert the input string into a stream, define a 
    // buffer, read successive labels into the buffer and load 
    // them into the array of column labels
    std::stringstream ss( lastHeaderLine);
    std::string buf;
    while( ss >> buf) column_labels.push_back(buf);
    nvars = column_labels.size();
    cout << " -Extracted " << nvars 
         << " column labels." << endl;
  }

  // If there were more than nvars_cmd_line variables, truncate
  // the vector of column labels and reset nvars.
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
  cout << " -Examined header of <" << inFileSpec.c_str() << ">," << endl
       << "  There should be " << nvars 
       << " fields (columns) per record (row)" << endl;

  // Now we know the number of variables (nvars), so if we know the 
  // number of points (e.g. from the command line, we can size the 
  // main points array once and for all, and not waste memory.
  if( npoints_cmd_line > 0) npoints = npoints_cmd_line;
  else npoints = MAXPOINTS;
  points.resize( nvars, npoints);

  // Loop: Read file
  int nSkip = 0, i = 0;
  unsigned uFirst = 1;
  int nTestCycle = 0, nUnreadableData = 0;
  while( !inFile.eof() && i<npoints) {
  
    // Get next line and ignore empty lines and coment lines
    if( !uFirst) {
      (void) getline( inFile, line, '\n');
      if( inFile.eof()) break;  // To make accounting work right
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

    // Loop: Insert the string into a stream and read it
    std::stringstream ss(line); 
    unsigned isBadData = 0;
    for( int j=0; j<nvars; j++) {
      double x;
      ss >> x;
      points( j, i) = (float) x;

      // Skip lines that do not appear to contain enough data
      if( ss.eof() && j<nvars-1) {
        cerr << " -WARNING, not enough data on line " << nRead
             << ", skipping this line!" << endl;
        isBadData = 1;
        break;
      }
      
      // Check for unreadable data and flag line to be skipped
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

    // Loop: Check for bad data flags and flag line to be skipped
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
       << " data + " << nSkip 
       << " skipped lines = " << nRead << " total." << endl;

  // Close input file, report results of file read operation
  // to the console, and return success
  inFile.close();
  return 0;
}

//*****************************************************************
// data_file_manager::read_binary_file_with_headers() -- Open and 
// read a binary file.  The file is asssumed to consist of a single
// header line of ASCII with column information, terminated by a 
// newline, followed by a block of binary data.  The only viable 
// way to read this seems to be with conventional C-style methods: 
// fopen, fgets, fread, feof, and fclose, from <stdio>.  Returns 0 
// if successful.
int data_file_manager::read_binary_file_with_headers() 
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

  // Use fgets to read a newline-terminated string of characters, 
  // test to make sure it wasn't too long, then load it into a
  // string of header information.
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

  // Loop: unpack the string of header information to obtain
  // column labels.
  std::stringstream ss( line);
  std::string buf;
  while( ss >> buf) column_labels.push_back(buf);
  nvars = column_labels.size();
  int nvars_in = nvars;

  // If there were more than nvars_cmd_line variables, truncate
  // the vector of column labels and reset nvars.
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
  column_labels.push_back(string("-nothing-"));
  cout << "column_labels = ";
  for( unsigned int i=0; i < column_labels.size(); i++ ) {
    cout << column_labels[i] << " ";
  }  
  cout << endl;
  cout << " -About to read " << nvars
       << " variables from a binary file with " << nvars_in
       << " fields (columns) per record (row)" << endl;

  // Now we know the number of variables (nvars), so if we know the 
  // number of points (e.g. from the command line) we can size the 
  // main points array once and for all, and not waste memory.
  if( npoints_cmd_line > 0) npoints = npoints_cmd_line;
  else npoints = MAXPOINTS;
  points.resize( nvars, npoints);
  
  // Warn if the input buffer is non-contiguous.
  if( !points.isStorageContiguous()) {
    cerr << "read_binary_file_with_headers: WARNING" << endl
         << " -Input buffer appears to be non-contigous."
         << endl;
    // return 1;
  }

  // Assert possible types or ordering  
  assert( ordering == COLUMN_MAJOR || ordering == ROW_MAJOR);

  // Read file in Column Major order
  if( ordering == COLUMN_MAJOR) {
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
  if( ordering == ROW_MAJOR) {
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

    blitz::Array<float,1> vars( npoints);
    blitz::Range NPTS( 0, npoints-1);
    if( !vars.isStorageContiguous()) {
      cerr << " -ERROR, Tried to read into noncontiguous buffer."
           << endl;
      fclose( pInFile);
      return -1;
    }

    // Loop: Read successive columns from file
    int i;
    for( i=0; i<nvars; i++) {

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
      if( ret != (unsigned int)nvars) {
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

//*****************************************************************
// data_file_manager::write_ascii_file_with_headers() -- Open and 
// write an ASCII data file.  File will consist of an ASCII header 
// with column names terminated by a newline, followed by successive
// lines of ASCII data.
void data_file_manager::write_ascii_file_with_headers()
{
  // Initialize read status and filespec.  NOTE: inFileSpec is defined as 
  // const char* for use with Fl_File_Chooser, which means it could be 
  // destroyed by the relevant destructors!
  // string sPathname = ".";
  const char *output_file_name = sPathname.c_str();
  const char* pattern = "*.{txt,lis,asc}\tAll Files (*)";
  const char* title = "write ASCII output to file";

  // Instantiate and show an Fl_File_Chooser widget.  NOTE: The pathname 
  // must be passed as a variable or the window will begin in some root 
  // directory.
  Fl_File_Chooser* file_chooser = 
    new Fl_File_Chooser( 
      output_file_name, pattern, Fl_File_Chooser::CREATE, title);

  // Loop: Select fileSpecs until a non-directory is obtained
  while( 1) {
    if( output_file_name != NULL) file_chooser->directory( output_file_name);

    // Loop: wait until the file selection is done
    file_chooser->show();
    while( file_chooser->shown()) Fl::wait();
    output_file_name = file_chooser->value();   

    // If no file was specified then quit
    if( output_file_name == NULL) {
      cout << "No output file was specified" << endl;
      break;
    }

    // For some reason, the fl_filename_isdir method doesn't seem to work, so
    // try to open this file to see if it is a directory.
    FILE* pFile = fopen( output_file_name, "w");
    if( pFile == NULL) {
      file_chooser->directory( output_file_name);
      sPathname.erase( sPathname.begin(), sPathname.end());
      sPathname.append( output_file_name);
      continue;
    }
    fclose( pFile);
    break;         
  } 

  // Obtain file name using the FLTK member function.  This doesn't work.
  // char *output_file_name = 
  //   fl_file_chooser( 
  //     "write ASCII output to file", NULL, NULL, 0);

  // If a file name was specified, create and write the file
  if( output_file_name) {
    blitz::Array<float,1> vars( nvars);
    blitz::Range NVARS( 0, nvars-1);
    
    // Open output stream and report any problems
    ofstream os;
    os.open( output_file_name, ios::out|ios::trunc);

    if( os.fail()) {
      cerr << "Error opening" << output_file_name 
           << "for writing" << endl;
      delete file_chooser;
      return;
    }
    
    // Write output file name (and additional information?) to the header
    os << "! File Name: " << output_file_name << endl;
    
    // Loop: Write column labels to the header
    for( int i=0; i < nvars; i++ ) {
      if( i == 0) os << "!" << setw( 12) << column_labels[ i];
      else os << " " << setw( 13) << column_labels[ i];
    }
    os << endl;
    
    // Loop: Write successive ASCII records to the data block in 8-digit 
    // scientific notation.
    os << setiosflags( ios::scientific) << setw( 8);
    for( int irow = 0; irow < npoints; irow++) {
      if( uWriteAll > 0 || selected( irow) > 0) {
        for( int jcol = 0; jcol < nvars; jcol++) {
          if( jcol > 0) os << " ";
          os << points( jcol, irow);
        }
        os << endl;
      }
    }

    // Report results
    cout << "Finished writing " << npoints
         << " rows with " << nvars << " variables" << endl;
  }

  // Deallocate the Fl_File_Chooser object
  delete file_chooser;
}

//*****************************************************************
// data_file_manager::write_binary_file_with_headers() -- Open and 
// write a binary data file.  File will consist of an ASCII header 
// with column names terminated by a newline, followed by a long 
// block of binary data. 
void data_file_manager::write_binary_file_with_headers()
{
  // Initialize read status and filespec.  NOTE: inFileSpec is
  // defined as const char* for use with Fl_File_Chooser, which 
  // means it could be destroyed by the relevant destructors!
  // string sPathname = ".";
  const char *output_file_name = sPathname.c_str();
  const char* pattern = "*.bin\tAll Files (*)";
  const char* title = "write binary output to file";

  // Instantiate and show an Fl_File_Chooser widget.  NOTE: The
  // pathname must be passed as a variable or the window will
  // begin in some root directory.
  Fl_File_Chooser* file_chooser = 
    new Fl_File_Chooser( output_file_name, pattern, Fl_File_Chooser::CREATE, title);

  // Loop: Select fileSpecs until a non-directory is obtained
  while( 1) {
    if( output_file_name != NULL) file_chooser->directory( output_file_name);

    // Loop: wait until the file selection is done
    file_chooser->show();
    while( file_chooser->shown()) Fl::wait();
    output_file_name = file_chooser->value();   

    // If no file was specified then quit
    if( output_file_name == NULL) {
      cout << "No output file was specified" << endl;
      break;
    }

    // For some reason, the fl_filename_isdir method doesn't seem
    // to work, so try to open this file to see if it is a directory.
    FILE* pFile = fopen( output_file_name, "w");
    if( pFile == NULL) {
      file_chooser->directory( output_file_name);
      sPathname.erase( sPathname.begin(), sPathname.end());
      sPathname.append( output_file_name);
      continue;
    }
    fclose( pFile);
    break;         
  } 

  // Obtain file name using the FLTK member function.  This doesn't work.
  // char *output_file_name = 
  //   fl_file_chooser( 
  //     "write binary output to file", NULL, NULL, 0);

  // If a file name was specified, create and write file
  if( output_file_name) {
    blitz::Array<float,1> vars( nvars);
    blitz::Range NVARS( 0, nvars-1);
    
    // Open output stream and report any problems
    ofstream os;
    os.open( 
      output_file_name, 
      ios::out|ios::trunc|ios::binary);
      // fstream::out | fstream::trunc | fstream::binary);

    if( os.fail()) {
      cerr << "Error opening" << output_file_name 
           << "for writing" << endl;
      delete file_chooser;
      return;
    }
    
    // Loop: Write column labels to the header
    for( int i=0; i < nvars; i++ ) os << column_labels[ i] << " ";
    os << endl;
    
    // Loop: Write data and report any problems
    int nBlockSize = nvars*sizeof(float);
    for( int i=0; i<npoints; i++) {
      if( uWriteAll > 0 || selected( i) > 0) {
        vars = points( NVARS, i);
        os.write( (const char*) vars.data(), nBlockSize);
        if( os.fail()) {
          cerr << "Error writing to" << output_file_name << endl;
          delete file_chooser;
          return;
        }
      }
    }
    
    // Report results
    cout << "Finished writing " << npoints
         << " rows with block_size " << nBlockSize << endl;
  }

  // Deallocate the Fl_File_Chooser object
  delete file_chooser;
}

//*****************************************************************
// data_file_manager::remove_trivial_columns -- Examine an array 
// of data and remove columns for which all values are identical.  
// Part of the read process.
void data_file_manager::remove_trivial_columns()
{
  blitz::Range NPTS( 0, npoints-1);
  int nvars_save = nvars;
  int current=0;

  // Define buffers to record removed columns
  int iRemoved = 0;
  vector <int> removed_columns;

  // Loop: Examine the data array column by colums and remove any
  // columns for which all values are identical.
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

//*****************************************************************
// data_file_manager::resize_global_arrays -- Resize various all 
// the global arrays used store raw, sorted, and selected data.
void data_file_manager::resize_global_arrays()
{
  // points.resizeAndPreserve(nvars,npoints);  

  // Resize and reinitialize list of ranked points to reflect the
  // fact that no ranking has been done.
  ranked_points.resize( nvars, npoints);
  ranked.resize( nvars);
  ranked = 0;
  
  // Resize temporary array used for sort
  tmp_points.resize(npoints);

  // Resize array used for color map?
  identity.resize( npoints);

  // Resize selection arrays
  newly_selected.resize( npoints);
  selected.resize( npoints);
  previously_selected.resize( npoints);
  saved_selection.resize(npoints);
  indices_selected.resize(nplots+1,npoints);
  number_selected.resize(nplots+1);

  // Initialize selection arrays
  number_selected = 0; 
  indices_selected = 0;
  newly_selected = 0;
  selected = 0;
  previously_selected = 0;
  saved_selection = 0;
  nselected = 0;
  selection_is_inverted = false;

  // Intially, all points are in the nonselected set
  number_selected( 0) = npoints;
}


//*****************************************************************
// data_file_manager::create_default_data( nvars_in) -- Load data
// arrays with default data consisting of dummy data.
void data_file_manager::create_default_data( int nvars_in)
{
  // Protect against screwy values of nvars_in
  if( nvars_in < 2) return;
  nvars = nvars_in;
  if( nvars > MAXVARS) nvars = MAXVARS;

  // Loop: Initialize and load the column labels, including the 
  // final label that says 'nothing'.
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

  // Resize data array to avoid the madening frustration of 
  // segmentation errors!  Important!
  npoints = 2;
  points.resize( nvars, npoints);

  // Loop: load each variable with 0 and 1.  These two loops are
  // kept separate for clarity and to facilitate changes
  for( int i=0; i<nvars; i++) {
    points( i, 0) = 0.0;
    points( i, 1) = 1.0;
  }

  // Resize global arrays
  resize_global_arrays ();

  // Load the identity array
  cout << "Making identity array, a(i)=i" << endl;
  for( int i=0; i<npoints; i++) identity( i)=i;

  // Report results
  cout << "Generated default data with " << npoints 
       << " points and " << nvars << " variables" << endl;
}

//*****************************************************************
// data_file_manager::directory() --  Get pathname.
string data_file_manager::directory()
{
  return sPathname; 
}
     
//*****************************************************************
// data_file_manager::directory( sPathname) --  Set pathname.
void data_file_manager::directory( string sPathnameIn)
{
  sPathname = sPathnameIn;
}
