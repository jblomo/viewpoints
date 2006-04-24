// viewpoints - interactive linked scatterplots and more.
// copyright 2005 Creon Levit, all rights reserved.
//*****************************************************************
// File name: vp.cpp
//
// Class definitions: none
//
// Classes referenced:
//   Various BLITZ templates
//   Any classes in global_definitions_vp.h
//   plot_window -- Plot window
//   control_panel_window -- Control panel window
//
// Required packages
//    FLTK 1.1.6 -- Fast Light Toolkit graphics package
//    FLEWS 0.3 -- Extensions to FLTK 
//    OGLEXP 1.2.2 -- Access to OpenGL extension under Windows
//    GSL 1.6 -- Gnu Scientific Library package for Windows
//    Blitz++ 0.9 -- Various math routines
//
// Compiler directives:
//   May require -D__WIN32__ to compile on windows
//	 See Makefile for linux and OSX compile & link info.
//
// Purpose: viewpoints - interactive linked scatterplots and more.
//
// General design philosophy:
//   1) This code is represents a battle between Creon Levit's 
//      passion for speed and efficiency and Paul Gazis's obsession
//      with organization and clarity, unified by a shared desire to
//      produce a powerful and easy to use tool for exploratory data
//      data analysis.
//
// Functions:
//   usage() -- Print help information
//   make_about_window( *o) -- DRaw the 'About' window
//   load_data_file( *inFileSpec) -- Load and initialize data
//   read_ascii_file_with_headers( *inFileSpec) -- Read ASCII
//   read_binary_file_with_headers( *inFileSpec) -- Read binary
//   write_binary_file_with_headers() -- Write binary file
//   remove_trivial_columns() -- Remove identical data
//   resize_global_arrays() -- Resize global arrays
//   create_main_control_panel( main_x, main_y, main_w, main_h,
//     cWindowLabel) -- Create the main control panel window.
//   create_plot_window_array( argc, argv, main_w) -- Plot windows
//   make_main_menu_bar() -- Create main menu bar (unused)
//   make_global_widgets() -- Controls for main control panel
//   choose_color_deselected( *o) -- Color of nonselected points
//   change_all_axes( *o) -- Change all axes
//   clearAlphaPlanes() -- Clear alpha planes
//   npoints_changed( *o) -- Update number of points changed
//   write_data( *o) -- Write data widget
//   reset_all_plots( void) -- Reset all plots
//   reload_plot_window_array( *o) -- Reload plot windows
//   read_data( *o, *user_data) -- Read data widget
//   redraw_if_changing( *dummy) -- Redraw changing plots
//
// Modification history
// 25-MAR-2006:
// -Reorganized headers for clarity and ease of maintentance
// -Restructured code to make class plot_window and class
//  control_panel_window self-contained.
// -Moved global variables to local files and within classes to
//  limit their scope
//
// Author: Creon Levit   2005-2006
// Modified: P. R. Gazis  14-APR-2006
//*****************************************************************

// Include the necessary include libraries
#include "include_libraries_vp.h"

// Include globals
#include "global_definitions_vp.h"

// Include associated headers and source code
#include "plot_window.h"
#include "control_panel_window.h"
// MCL XXX the following is bogus.  We need separate compilation units.
// and that means we need to deal with globals elegantly.
#include "plot_window.cpp"
#include "control_panel_window.cpp"

// Define and initialize number of screens
int number_of_screens = 0;

// Approximate values of window manager borders & desktop borders 
// (stay out of these).  These are used by the main method when 
// the main control panel window is defined.  And when the plot
// windows are tiled to fit the screen.  Too bad they are only
// "hints" according most window managers (and we all know how
// well managers take hints).
#ifdef __APPLE__
 int top_frame=35, bottom_frame=0, left_frame=0, right_frame=5;
 int top_safe = 1, bottom_safe=5, left_safe=5, right_safe=1;
#else // __APPLE__
 int top_frame=25, bottom_frame=5, left_frame=4, right_frame=5;
 int top_safe = 1, bottom_safe=10, left_safe=10, right_safe=1;
#endif // __APPLE__

// Define and set maximums for header block
const int MAX_HEADER_LENGTH = nvars_max*100;  // Length of header line
const int MAX_HEADER_LINES = 2000;  // Number of header lines

// Define and set default values for file reads
int format=ASCII;   // default input file format
int ordering=COLUMN_MAJOR;   // default input data ordering
int nSkipHeaderLines = 1;  // Number of header lines to skip
const int skip = 0;  // Number of columns to skip

// Define and set default border style for the plot windows
int borderless=0;  // By default, use window manager borders

// Initialize and set the default number of points specified
// by the command line argument.
int npoints_cmd_line = 0;   // NOTE: 0 means read to EOF.

// Define variables to hold main control panel window, tabs 
// widget, and virtual control panel positions.  Consolidated
// here for reasons of clarity.
// const int main_w = 350, main_h = 700;
const int main_w = 350, main_h = 725;
const int tabs_widget_x = 3, tabs_widget_y = 30;
const int tabs_widget_h = 500;
const int cp_widget_x = 3, cp_widget_y = 50;
const int cp_widget_h = 480;
const int global_widgets_x = 10, global_widgets_y = 520;

// Define pointers to hold main control panel, main menu bar, and
// any pop-up windows
Fl_Window *main_control_panel;
Fl_Menu_Bar *main_menu_bar;
Fl_Window *about_window;

// Function definitions for the main method
void usage();
int load_data_file( char* inFileSpec);
int read_ascii_file_with_headers( char* inFileSpec);
int read_binary_file_with_headers( char* inFileSpec);
void write_binary_file_with_headers();
void remove_trivial_columns();
void resize_global_arrays();
void create_main_control_panel( 
  int main_x, int main_y, int main_w, int main_h,
  char* cWindowLabel);
void create_plot_window_array( int argc, char **argv, int main_w);
void make_main_menu_bar();
void make_about_window( Fl_Widget *o);
void make_global_widgets();
void choose_color_deselected( Fl_Widget *o);
void change_all_axes( Fl_Widget *o);
void clearAlphaPlanes();
void npoints_changed( Fl_Widget *o);
void write_data( Fl_Widget *o);
void reset_all_plots( void);
void reload_plot_window_array( Fl_Widget *o);
void read_data( Fl_Widget* o, void* user_data);
void redraw_if_changing( void *dummy);

//*****************************************************************
// usage() -- Print help information and exit
void usage()
{
  cerr << "Usage: vp {optional arguments} datafile" << endl;
  cerr << "  [--format={ascii,binary}] (-f)" << endl;
  cerr << "  [--npoints=<int>] (-n)" << endl;
  cerr << "  [--skip_header_lines=<int>] (-s)" << endl;
  cerr << "  [--rows=<int>] (-r)" << endl;
  cerr << "  [--cols=<int>] (-c)" << endl;
  cerr << "  [--monitors=<int>] (-m)" << endl;
  cerr << "  [--input_file={input filespec}] (-i)" << endl;
  cerr << "  [--borderless] (-b)" << endl;
  cerr << "  [--help] (-h)" << endl;
  exit( -1);
}

//*****************************************************************
// make_about_window( *o) -- Create the 'Help|About' window.
void make_about_window( Fl_Widget *o)
{
  if( about_window != NULL) about_window->hide();
   
  // Create Help|About window
  Fl::scheme( "plastic");  // optional
  about_window = new Fl_Window( 300, 200, "About vp");
  about_window->begin();
  about_window->selection_color( FL_BLUE);
  about_window->labelsize( 10);
  
  // Compose text
  string sAbout = "viewpoints 1.0.1\n";
  sAbout += "(c) 2006 C. Levit and P. R. Gazis\n\n";
  sAbout += "contact information:\n";
  sAbout += "Creon Levit creon@nas.nasa.gov\n";
  sAbout += "Paul R Gazis pgazis@mail.arc.nasa.gov\n\n";

  // Write text
  Fl_Multiline_Output* output_widget = 
    new Fl_Multiline_Output( 5, 5, 290, 190); 
  output_widget->value( sAbout.c_str());

  // XXX: A close button might be nice someday
  // Fl_Button* close = new Fl_Button(100, 150, 70, 30, "&Close");

  // Done creating the 'Help|About' window
  about_window->resizable( about_window);
  about_window->end();
  about_window->show();
}

//*****************************************************************
// load_data_file( inFileSpec) -- Read an ASCII or binary data 
// file, resize arrays to allocate meomory, and set identity
// array.  Returns 0 if successful.
// MCL XXX - refactor this with read_data()
int load_data_file( char* inFileSpec) 
{
  // Read data file and report results
  cout << "Reading input data from <" << inFileSpec << ">" << endl;
  int iReadStatus = 0;
  if( format == BINARY) 
    iReadStatus = read_binary_file_with_headers( inFileSpec);
  else if( format == ASCII)
    iReadStatus = read_ascii_file_with_headers( inFileSpec);

  if( iReadStatus != 0) {
    cout << "Problems reading file <" << inFileSpec << ">" << endl;
    return -1;
  }
  else
    cout << "Finished reading file <" << inFileSpec << ">" << endl;

  // Remove trivial columns
  remove_trivial_columns ();

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

  // Load the identity array
  cout << "Making identity array, a(i)=i" << endl;
  for( int i=0; i<npoints; i++) identity( i)=i;
  return 0;
}

//*****************************************************************
// read_ascii_file_with_headers( inFileSpec) -- Open an ASCII file 
// for input, read and discard the headers, read the data block, 
// and close the file.  Returns 0 if successful.  Could become
// part of the proposed class data_file.
int read_ascii_file_with_headers( char* inFileSpec) 
{
  // Attempt to open input file and make sure it exists
  ifstream inFile;
  inFile.open( inFileSpec, ios::in);
  if( inFile.bad() || !inFile.is_open()) {
    cout << "read_ascii_file_with_headers:" << endl
         << " -ERROR, couldn't open <" << inFileSpec
         << ">" << endl;
    return 1;
  }
  else {
    cout << "read_ascii_file_with_headers:" << endl
         << " -Opening <" << inFileSpec << ">" << endl;
  }

  // Loop: Read successive lines to find the last line of the 
  // header block and the beginning of the data block. NOTE: Since 
  // tellg() and seekg() don't seem to work properly with getline 
  // with all compilers, this must be accomplished by keeping 
  // track of lines explicitly.
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

  // Examine the column labels for errors and report results
  if( nvars > nvars_max) {
    cerr << " -ERROR, too many columns, "
         << "increase nvars_max and recompile"
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
  cout << " -Examined header of <" << inFileSpec << ">," << endl
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

    // Loop: Insert the string into a stream and read it
    std::stringstream ss(line); 
    unsigned isBadData = 0;
	for( int j=0; j<nvars; j++) {
      double x;
      ss >> x;
      points( j, i) = (float) x;

      // FIX THIS MISSING DATA STUFF!! IT IS BROKEN.
      if( ss.eof() && j<nvars-1) {
        cerr << " -ERROR, not enough data on line " << i+2
             << ", aborting!" << endl;
        inFile.close();
        return 1;
      }
      
      // Check for unreadable data and flag line to be skipped
      if( !ss.good() && j<nvars-1) {
        cerr << " -WARNING, unreadable data "
             << "(probably non-numeric) at line " << i+1 
             << " column " << j+1 << "," << endl
             << "  skipping entire line." << endl;
        cerr << "  <" << line.c_str() << ">" << endl;
        isBadData = 1;
        break;
      }
      DEBUG (cout << "points(" << j << "," << i << ") = " << points(j,i) << endl);
    }

    // Loop: Check for bad data flags and flag line to be skipped
    for( int j=0; j<nvars; j++) {
      if( points(j,i) == -9999) {
        cerr << " -WARNING, bad data (-9999) at line " << i 
             << ", column " << j << " - skipping entire line\n";
        isBadData = 1;
        break;
      }
    }

    // If data were good, increment number of lines
    if( !isBadData) {
      i++;
      if( (i+1)%10000 == 0)
        cerr << "  Read " << i+1 << " lines." << endl;
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
// read_binary_file_with_headers( inFileSpec) -- Open and read a 
// binary file.  The file is asssumed to consist6s of an ASCII
// header with column information, terminated by a newline,
// followed by a block of binary data.  The only viable way to 
// read this seems to be with conventional C-style methods, fopen, 
// fgets, fread, feof, and fclose, from <stdio>.  Returns 0 if 
// successful.  Could become part of the proposed class data_file.
int read_binary_file_with_headers( char* inFileSpec) 
{
  // Attempt to open input file and make sure it exists
  FILE * pInFile;
  pInFile = fopen( inFileSpec, "rb");
  if( pInFile == NULL) {
    cout << "read_binary_file_with_headers: ERROR" << endl
         << " -Couldn't open binary file <" << inFileSpec 
         << ">" << endl;
    return 1;
  }
  else {
    cout << "read_binary_file_with_headers:" << endl
         << " -Opening binary file <" << inFileSpec 
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

  // Examine and report content of header
  nvars = column_labels.size();
  if( nvars > nvars_max) {
    cerr << " -ERROR: Too many columns, "
         << "increase nvars_max and recompile"
         << endl;
    // inFile.close();
    fclose( pInFile);
    return 1;
  }
  column_labels.push_back(string("-nothing-"));
  cout << "column_labels = ";
  for( unsigned int i=0; i < column_labels.size(); i++ ) {
    cout << column_labels[i] << " ";
  }  
  cout << endl;
  cout << " -About to read a binary file with " << nvars
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
    blitz::Array<float,1> vars( nvars);
    blitz::Range NVARS( 0, nvars-1);
    if( !vars.isStorageContiguous()) {
      cerr << " -ERROR: Tried to read into a noncontiguous buffer."
           << endl;
      // inFile.close();
      fclose( pInFile);
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
      if( ret != (unsigned int)nvars) {
        cerr << " -ERROR reading row[ " << i+1 << "], "
             << "returned values " << ret 
             << " NE number of variables " << nvars << endl;
        fclose( pInFile);
        return 1;
      }

      // Load data array and report progress
      points( NVARS,i) = vars;
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
         << "row-major order with nvars=" << nvars
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
// write_binary_file_with_headers() -- Open and write a binary 
// data file.  File will consist of an ASCII header with column 
// names terminated by a newline, followed by a long block of 
// binary data.  Could become part of the proposed class
// data_file.
void write_binary_file_with_headers()
{
  // Obtain file name from FLTK member function
  char *output_file_name = 
    fl_file_chooser( 
      "write binary output to file", NULL, NULL, 0);

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
      return;
    }
    
    // Loop: Write column labels
    for( int i=0; i < nvars; i++ ) {
      os << column_labels[ i] << " ";
    }  
    os << endl;
    
    // Loop: Write data and report problems
    int nBlockSize = nvars*sizeof(float);
    for( int i=0; i<npoints; i++) {
      vars = points( NVARS, i);
      os.write( 
        (const char*) vars.data(), nBlockSize);
      if( os.fail()) {
        cerr << "Error writing to" << output_file_name << endl;
        return;
      }
    }
    
    // Report results
    cout << "Finished writing " << npoints
         << " rows with block_size " << nBlockSize << endl;
  }
}

//*****************************************************************
// remove_trivial_columns -- Examine an array of data and remove
// columns for which all values are identical.  Part of the read
// process.  Could become part of the proposed class data_file.
void remove_trivial_columns()
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
// resize_global_arrays -- Resize various all the global arrays 
// used store raw, sorted, and selected data.  Could become part 
// of the proposed class data_file.
void resize_global_arrays()
{
  // points.resizeAndPreserve(nvars,npoints);	

  ranked_points.resize( nvars, npoints);

  ranked.resize( nvars);
  ranked = 0;  // initially, no ranking has been done.

  tmp_points.resize(npoints); // for sort

  texture_coords.resize( npoints);
  identity.resize( npoints);
  newly_selected.resize( npoints);
  selected.resize( npoints);
  previously_selected.resize( npoints);

  selected = 0;
}

//*****************************************************************
// create_main_control_panel( main_x, main_y, main_w, main_h,
// cWindowLabel) -- Create the main control panel window.
void create_main_control_panel( 
  int main_x, int main_y, int main_w, int main_h,
  char* cWindowLabel)
{
  // Create main control panel window
  Fl::scheme( "plastic");  // optional
  main_control_panel = 
    new Fl_Window(
      main_x, main_y, main_w, main_h, cWindowLabel);
  main_control_panel->resizable( main_control_panel);

  // Make main menu bar
  make_main_menu_bar();

  // Add the rest of the global widgets to control panel
  make_global_widgets ();

  // Inside the main control panel, there is a tab widget, cpt, 
  // that contains the sub-panels (groups), one per plot.
  // cpt = new Fl_Tabs( 3, 10, main_w-6, 500);
  cpt = new Fl_Tabs( 
    tabs_widget_x, tabs_widget_y, main_w-6, tabs_widget_h);
  cpt->selection_color( FL_BLUE);
  cpt->labelsize( 10);

  // Done creating main control panel (except for the tabbed 
  // sub-panels created by create_plot_window_array)
  main_control_panel->end();


}

// Create a special panel (really a group under a tab) with label "+"
// this group's widgets effect all the others
// (unless a plot's tab is "locked" - TBI).
// MCL XXX should this be a method of control_panel_window?
//  should it be a singleton?
void create_broadcast_group ()
{
  Fl_Group::current(cpt);	
  control_panel_window *cp = cps[nplots];
  cp = new control_panel_window( cp_widget_x, cp_widget_y, main_w - 6, cp_widget_h);
  cp->label("+");
  cp->labelsize( 10);
  cp->resizable( cp);
  cp->make_widgets( cp);
  cp->end();
  // this group's index is highest (and it has no associated plot window)
  cp->index = nplots;
  // initially, this group has no axes (XXX or anything else, for that matter)
  cp->varindex1->value(nvars);  // initially == "-nothing-"
  cp->varindex2->value(nvars);  // initially == "-nothing-"
  cp->varindex3->value(nvars);  // initially == "-nothing-"
  // this group's callbacks all broadcast any "event" to the other (unlocked) tabs groups.
  for (int i=0; i<cp->children(); i++)
  {
	  Fl_Widget *wp = cp->child(i);
	  wp->callback((Fl_Callback *)(control_panel_window::broadcast_change), cp);
  }
}

//*****************************************************************
// create_plot_window_array( argc, argv, main_w) -- Create array 
// of plot windows with their associated tabs.  As of 10-APR-2006,
// this includes a preliminary version of a test to suppress 
// plots if no data are available.
void create_plot_window_array( int argc, char **argv, int main_w)
{
  // Create and add the virtual sub-panels, each group under a 
  // tab, one per plot.
  for( int i=0; i<nplots; i++) {

    // Create a label for this tab
    ostringstream oss;
    oss << "" << i+1;
    string labstr = oss.str();

    // Set pointer to the current group to the tab widget defined 
    // by create_control_panel and add a new virtual control panel
    // under this tab widget
    Fl_Group::current(cpt);	
    cps[i] = new control_panel_window( cp_widget_x, cp_widget_y, main_w - 6, cp_widget_h);
    cps[i]->copy_label( labstr.c_str());
    cps[i]->labelsize( 10);
    cps[i]->resizable( cps[i]);
    cps[i]->make_widgets( cps[i]);

    // End the group here so that we can create new plot windows 
    // at the top level, then set the pointer to the current group
    // to the top level.
    cps[i]->end();
    Fl_Group::current(0); 

    // Create plotting window i
    int row = i/ncols;
    int col = i%ncols;

    // Account for borderless option
    if( borderless)
      top_frame = bottom_frame = left_frame = right_frame = 1;

    // Determine plot window size and position
    int pw_w =
      ( ( number_of_screens*Fl::w() - 
          (main_w+left_frame+right_frame+right_safe+left_safe+20)) / ncols) -
      (left_frame + right_frame);
    int pw_h = 
      ( (Fl::h() - (top_safe+bottom_safe))/ nrows) - 
      (top_frame + bottom_frame);

    int pw_x = 
      left_safe + left_frame + 
      col * (pw_w + left_frame + right_frame);
    int pw_y = 
      top_safe + top_frame + 
      row * (pw_h + top_frame + bottom_frame);

    pws[i] = new plot_window( pw_w, pw_h);
    pws[i]->copy_label( labstr.c_str());
    pws[i]->position(pw_x, pw_y);
    pws[i]->row = row; 
    pws[i]->column = col;
    pws[i]->end();

    // Link plot window and its associated virtual control panel
    pws[i]->index = cps[i]->index = i;
    cps[i]->pw = pws[i];
    pws[i]->cp = cps[i];

    // Determine which variables to plot, initially
    int ivar, jvar;
    if( i==0) {
      ivar = 0; jvar = 1;

      // Initially the first plot's tab is shown, and its axes 
      // are locked.
      cps[i]->lock_axes_button->value(1);
      cps[i]->show();	// ???
    } else {
      plot_window::upper_triangle_incr( ivar, jvar, nvars);
    }
    cps[i]->varindex1->value(ivar);  
    cps[i]->varindex2->value(jvar);  

    // Could put first of two tests for npoints <=1 here
    if( npoints > 1) {
      pws[i]->extract_data_points();
      pws[i]->reset_view();
    }
    pws[i]->size_range( 10, 10);
    pws[i]->resizable( pws[i]);

    if( borderless) pws[i]->border(0);

    // Could put second of two tests for npoints <=1 here
    if( npoints > 1) pws[i]->show( argc, argv);
    pws[i]->resizable( pws[i]);

  }

  create_broadcast_group ();
}

//*****************************************************************
// make_main_menu_bar() -- Make main menu bar
void make_main_menu_bar()
{
  // Instantiate Fl_Menu_Bar object
  main_menu_bar =
    new Fl_Menu_Bar( 0, 0, main_w, 25);

  // Add menu items
  main_menu_bar->add( 
    "File/Open ASCII file", 0, 
    (Fl_Callback *) read_data, (void*) ASCII);
  main_menu_bar->add( 
    "File/Open binary file", 0, 
    (Fl_Callback *) read_data, (void*) BINARY);
  main_menu_bar->add( 
    "File/Write binary file", 0, 
    (Fl_Callback *) write_data, 0, FL_MENU_DIVIDER);
  main_menu_bar->add( 
    "File/Quit", 0, (Fl_Callback *) exit);
  main_menu_bar->add( 
    "View/Reload Plots", 0, 
    (Fl_Callback *) reload_plot_window_array);
  // main_menu_bar->add( "Help", 0, 0, 0, FL_MENU_INACTIVE);
  main_menu_bar->add( 
    "Help/About", 0, (Fl_Callback *) make_about_window);
}

//*****************************************************************
// make_global_widgets() -- Make controls for main control panel
void make_global_widgets()
{
  // Draw 'npoints' horizontal slider at top of subpanel
  // int xpos=10, ypos=500;
  int xpos = global_widgets_x, ypos = global_widgets_y;
  npoints_slider = 
    new Fl_Hor_Value_Slider_Input(
      xpos+30, ypos+=25, 300-30, 20, "npts");
  npoints_slider->align( FL_ALIGN_LEFT);
  npoints_slider->callback( npoints_changed);
  npoints_slider->value( npoints);
  npoints_slider->step( 1);
  npoints_slider->bounds( 1, npoints);

  // Define a pointer and initialize positions for buttons
  Fl_Button *b;
  int xpos1 = xpos, ypos1 = ypos;

  // Button(1,1): Show nonselected points (on by default)
  show_deselected_button = b = 
    new Fl_Button( xpos, ypos+=25, 20, 20, "show nonselected");
  b->align( FL_ALIGN_RIGHT); 
  b->selection_color( FL_BLUE); 
  b->type( FL_TOGGLE_BUTTON);
  b->value( 1);
  b->callback( (Fl_Callback*) 
    plot_window::toggle_display_deselected);

  // Button(2,1): Add to the selection
  add_to_selection_button = b = 
    new Fl_Button( xpos, ypos+=25, 20, 20, "add to selection");
  b->align( FL_ALIGN_RIGHT); 
  b->selection_color( FL_BLUE); 
  b->type( FL_TOGGLE_BUTTON);
  b->value( 0);	

  // Button(3,1): Invert colors of selected and nonselected data
  invert_selection_button = b = 
    new Fl_Button( xpos, ypos+=25, 20, 20, "invert selection");
  b->align( FL_ALIGN_RIGHT); 
  b->selection_color( FL_BLUE); 
  b->callback( (Fl_Callback*) plot_window::invert_selection);

  // Button(4,1): Clear selection
  clear_selection_button = b = 
    new Fl_Button(xpos, ypos+=25, 20, 20, "clear selection");
  b->align( FL_ALIGN_RIGHT); 
  b->selection_color( FL_BLUE); 
  b->callback( plot_window::clear_selection);

  // Button(5,1): Delete selected data
  delete_selection_button = b = 
    new Fl_Button( xpos, ypos+=25, 20, 20, "kill selected");
  b->align( FL_ALIGN_RIGHT); 
  b->selection_color( FL_BLUE); 
  b->callback( plot_window::delete_selection);

  // Button(6,1): Write binary data file
  write_data_button = b = 
    new Fl_Button( xpos, ypos+=25, 20, 20, "write data");
  b->align( FL_ALIGN_RIGHT); 
  b->selection_color( FL_BLUE); 
  b->callback( write_data);

  // Advance to column 2
  xpos = xpos1 + 150; ypos = ypos1;

  // Button(1,2): Chose color of non-selcted points
  choose_color_deselected_button = b = 
    new Fl_Button( xpos, ypos+=25, 20, 20, "unselected color");
  b->align( FL_ALIGN_RIGHT); 
  b->selection_color( FL_BLUE); 
  b->callback( (Fl_Callback*)choose_color_deselected);

  // Button(2,2): Don't paint?  What does this do?
  dont_paint_button = b = 
    new Fl_Button( xpos, ypos+=25, 20, 20, "don't paint");
  b->align( FL_ALIGN_RIGHT); 
  b->selection_color( FL_BLUE); 
  b->type( FL_TOGGLE_BUTTON);

  // Button(3,2): Randomly change all axes
  change_all_axes_button = b = 
    new Fl_Button( xpos, ypos+=25, 20, 20, "change axes");
  b->align( FL_ALIGN_RIGHT); 
  b->selection_color( FL_BLUE); 
  b->callback( (Fl_Callback*)change_all_axes);

  // Button(4,2): Link all axes
  link_all_axes_button = b = 
    new Fl_Button( xpos, ypos+=25, 20, 20, "link axes");
  b->align( FL_ALIGN_RIGHT); 
  b->selection_color( FL_BLUE); 
  b->type( FL_TOGGLE_BUTTON); 
  b->value( 0);

  // Button(5,1): Reload plot window array
  read_data_button = b = 
    new Fl_Button( xpos, ypos+=25, 20, 20, "reload plots");
  b->align( FL_ALIGN_RIGHT); 
  b->selection_color( FL_BLUE); 
  b->callback( reload_plot_window_array);

  // Button(6,1): Read ASCII data file
  read_data_button = b = 
    new Fl_Button( xpos, ypos+=25, 20, 20, "read data");
  b->align( FL_ALIGN_RIGHT); 
  b->selection_color( FL_BLUE); 
  b->callback( read_data);
}

//*****************************************************************
// choose_color_deselected( *o) -- Choose color of deselected 
// points.  Could this become a static member function of class
// plot_window?
void choose_color_deselected( Fl_Widget *o)
{
  (void) fl_color_chooser(
    "deselected", 
    plot_window::r_deselected, 
    plot_window::g_deselected,
    plot_window::b_deselected);
  pws[ 0]->update_textures (); // XXX 
}

//*****************************************************************
// change_all_axes( *o) -- Invoke the change_axes method of each
// plot_window to change all unlocked axes.
void change_all_axes( Fl_Widget *o) {
  for( int i=0; i<nplots; i++) {
    if( !cps[i]->lock_axes_button->value())
      pws[i]->change_axes();
  }
}

//*****************************************************************
// clearAlphaPlanes() -- Those filthy alpha planes!  It seems that
// no matter how hard you try, you just can't keep them clean!
void clearAlphaPlanes()
{
  glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
  glClearColor( 0.0, 0.0, 0.0, 0.0);
  glClear( GL_COLOR_BUFFER_BIT);
  glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

//*****************************************************************
// npoints_changed( 0) -- Examine slider widget to determine new
// number of points, then invoke a static method of class 
// plot_window to redraw all plots
void npoints_changed( Fl_Widget *o) 
{
  npoints = int( ( (Fl_Slider *)o)->value());
  plot_window::redraw_all_plots( 0);
}

//*****************************************************************
// write_data( o) -- Write data widget.  Invoked by main control
// panel.  Invokes write method to write a binary data file.
void write_data( Fl_Widget *o)
{
  write_binary_file_with_headers();
}

//*****************************************************************
// reset_all_plots() -- Reset all plots.  Invoked by main control 
// panel.
void reset_all_plots()
{
  for( int i=0; i<nplots; i++) {
    pws[i]->reset_view();
  }
}

//*****************************************************************
// reload_plot_window_array( o) -- Check to make sure data are
// available, then delete old tabs, create new tabs, and load data
// into the associated plot windows.
// MCL - XXX refactor this with create_plot_window_array()
void reload_plot_window_array( Fl_Widget *o)
{
  // Check to make sure data are available
  if( npoints <= 1) {
    cout << "ERROR: "
         << "insufficient data to reload plot window array"
         << endl;
    return;
  }

  // Clear children of tab widget to delete old tabs
  cpt->clear();

  // Loop: Create and add new tabbed sub-panels (each a group 
  // under a tab) and load data into the associated plot windows.
  for( int i=0; i<nplots; i++) {
    if( borderless)
      top_frame = bottom_frame = left_frame = right_frame = 1;

#if 0
// MCL XXX these are unused here.  Why?
    int row = i/ncols;
    int col = i%ncols;

    // Determine plot window size and position
    int pw_w =
      ( ( number_of_screens*Fl::w() - 
          (main_w+left_frame+right_frame+right_safe+left_safe+20)) / ncols) -
      (left_frame + right_frame);
    int pw_h = 
      ( (Fl::h() - (top_safe+bottom_safe))/ nrows) - 
      (top_frame + bottom_frame);

    int pw_x = 
      left_safe + left_frame + 
      col * (pw_w + left_frame + right_frame);
    int pw_y = 
      top_safe + top_frame + 
      row * (pw_h + top_frame + bottom_frame);
#endif // 0 

    // Create a label for this tab
    ostringstream oss;
    oss << "" << i+1;
    string labstr = oss.str();

    // Set pointer to the current group to the tab widget defined 
    // by create_control_panel and add a new virtual control panel
    // under this tab widget
    Fl_Group::current(cpt);	
    // cps[i] = new control_panel_window( 3, 30, main_w - 6, 480);
    cps[i] = new control_panel_window( 
      cp_widget_x, cp_widget_y, main_w - 6, cp_widget_h);
    cps[i]->copy_label( labstr.c_str());
    cps[i]->labelsize( 10);
    cps[i]->resizable( cps[i]);
    cps[i]->make_widgets( cps[i]);

    // End the group here so that we can create new plot windows 
    // at the top level, then set the pointer to the current group
    // to the top level.
    cps[i]->end();
    Fl_Group::current(0); 

    // Link plot window and its associated virtual control panel
    pws[i]->index = cps[i]->index = i;
    cps[i]->pw = pws[i];
    pws[i]->cp = cps[i];

    // Determine which variables to plot, initially
    int ivar, jvar;
    if( i==0) {
      ivar = 0; jvar = 1;

      // Initially the first plot's tab is shown, and its axes 
      // are locked.
      cps[i]->lock_axes_button->value(1);
      cps[i]->hide();	
    } 
    else {
      plot_window::upper_triangle_incr( ivar, jvar, nvars);
    }
    cps[i]->varindex1->value(ivar);  
    cps[i]->varindex2->value(jvar);  
    cps[i]->varindex3->value(nvars);  

    // Resize histogram arrays to avoid segmentation errors (!!!),
    // then extract and plot data points.  NOTE: Old code has been 
    // commented out and this is now done via initialze()
    // pws[i]->vertices.resize(npoints,3);
    // pws[i]->x_rank.resize(npoints,1);
    // pws[i]->y_rank.resize(npoints,1);
    // pws[i]->z_rank.resize(npoints,1);
    pws[i]->initialize();
    pws[i]->extract_data_points();

    // A possible alternative to pws[i]->extract_data_points();
    // cps[i]->extract_and_redraw();

    // May be unecessary: should have been handled during 
    // initial setup?
    if( borderless) pws[i]->border(0);

    // Invoke show() to bring back any windows that were closed.
    pws[i]->show();
    pws[i]->resizable( pws[i]);
  }

  // A possible alternative to refreshing windows on an individual
  // basis that doesn't work because the redraw_all_plots() method 
  // merely sets a flag rather than actually redrawing the panels
  // plot_window::redraw_all_plots(0);
}

//*****************************************************************
// read_data( o, user_data) -- Widget to open and read data from 
// an ASCII file.  
void read_data( Fl_Widget* o, void* user_data)
{
  // Evaluate user_data to get file format
  if( (int) user_data == BINARY) format = BINARY;
  else format = ASCII;

  // Invoke the FLTK member function, fl_file_chooser, to get the
  // file name, then invoke read_ascii_file_with_headers or
  // read_binary_file_with_headers to read an ASCII or BINARY file
  int iReadStatus = 0;
  char *inFileSpec = "";
  if( format == ASCII) {
    inFileSpec = 
      fl_file_chooser( 
        "read ASCII input from file", NULL, NULL, 0);

    cout << "Reading ASCII data from <" << inFileSpec 
         << ">" << endl;
    iReadStatus = read_ascii_file_with_headers( inFileSpec);
  }
  else {
    inFileSpec = 
      fl_file_chooser( 
        "read binary input from file", NULL, NULL, 0);

    cout << "Reading binary data from <" << inFileSpec 
         << ">" << endl;
    iReadStatus = read_binary_file_with_headers( inFileSpec);
  }

  // Report results of the read operation
  if( iReadStatus != 0) {
      cout << "Problems reading file <" << inFileSpec << ">" << endl;
      return;
  }
  else
    cout << "Finished reading file <" << inFileSpec << ">" << endl;

  // Remove trivial columns
  remove_trivial_columns ();

  // If only one or fewer records are available then quit before 
  // something terrible happens!
  if( npoints <= 1) {
    cout << "Insufficient data, " << npoints
         << " samples." << endl;
    return;
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

  // Load the identity array
  cout << "Making identity array, a(i)=i" << endl;
  for( int i=0; i<npoints; i++) identity( i)=i;

  // Resize slider
  npoints_slider->bounds(1,npoints);
  npoints_slider->value(npoints);

  // Clear children of tab widget and reload plot window array
  // cpt->clear();  // Now done by reload_plot_window_array( o)
  reload_plot_window_array( o);
}

//*****************************************************************
// redraw_if_changing( dummy) -- Callback function for use by FLTK
// Fl::add_idle.  When an idle callback occurs, redraw any panel
// that is spinning or otherwise needs to be redrawn.
void redraw_if_changing( void *dummy)
{
  // DEBUG( cout << "in redraw_if_changing" << endl) ;
  for( int i=0; i<nplots; i++) {
    // DEBUG ( cout << "  i=" << i << ", needs_redraw=" << pws[i]->needs_redraw << endl );
    if( cps[i]->spin->value() || pws[i]->needs_redraw) {
      pws[i]->redraw();
      pws[i]->needs_redraw = 0;
    }
  }
  float fps = 100.0;
  struct timeval tp;
  static long useconds=0;
  static long seconds=0;

  // has at least 1/fps seconds elapsed? (sort of)
  busy:

  (void) gettimeofday(&tp, (struct timezone *)0);
  if( (tp.tv_sec > seconds) || 
      (((float)(tp.tv_usec - useconds)/1000000.0) > 1/fps)) {
    seconds = tp.tv_sec;
    useconds = tp.tv_usec;
    return;
  }
  else {
       
    // DANGER: Don't monkey with syntax in the call to usleep!
    usleep (1000000/(5*(int)fps));
    goto busy;
  }
}

//*****************************************************************
// Main routine
//
// Purpose: Driver to run everything.  STEP 1: Read and parse
//  command line.  STEP 2: Read data file.  STEP 3: Create main
//  control panel.  This is an involved process that could be
//  broken down into several successive functions.  STEP 4: Enter
//  main process loop until done.
//
// Functions:
//   main() -- main routine
//
// Author:   Creon Levit   unknown
// Modified: P. R. Gazis   13-MAR-2006
//*****************************************************************
//*****************************************************************
// Main -- Driver routine
int main( int argc, char **argv)
{
  cout << "vp: Creon Levit's viewpoints" << endl;

  // STEP 1: Parse the command line
  // cout << "argc<" << argc << ">" << endl;
  // for( int i=0; i<argc; i++) {
  //   cout << "argv[ " << i << "]: <" << argv[ i] << ">" << endl;
  // }

  // DIAGNOSTIC
  // if( argc <= 1) {
  //   argc = 4;
  //   strcpy( *argv, "vp -f b d3d_10000.bin");
  // }

  // Define structure of command-line options
  static struct option long_options[] = {
    { "format", required_argument, 0, 'f'},
    { "npoints", required_argument, 0, 'n'},
    { "skip_header_lines", required_argument, 0, 's'},
    { "ordering", required_argument, 0, 'o'},
    { "rows", required_argument, 0, 'r'},
    { "cols", required_argument, 0, 'c'},
    { "monitors", required_argument, 0, 'm'},
    { "input_file", required_argument, 0, 'i'},
    { "borderless", no_argument, 0, 'b'},
    { "help", no_argument, 0, 'h'},
    { 0, 0, 0, 0}
  };

  // Loop: Invoke GETOPT_LONG to parse successive command-line 
  // arguments (Windows version of GETOPT_LONG is implemented in 
  // LIBGW32).  NOTES: 1) The possible options MUST be listed in 
  // the call to GETOPT_LONG, 2) This process does NOT effect arc 
  // and argv in any way.
  int c;
  char inFileSpec[ 80];
  strcpy( inFileSpec, "");
  while( 
    ( c = getopt_long( 
        argc, argv, 
        "f:n:s:o:r:c:m:i:b:h", long_options, NULL)) != -1) {
  
    // Examine command-line options and extract any optional
    // arguments
    switch( c) {

      // format: Extract format of input file
      case 'f':
        if( !strncmp( optarg, "binary", 1))
          format = BINARY;
        else if( !strncmp( optarg, "ascii", 1))
          format = ASCII;
        else {
          usage();
          exit( -1);
        }
        break;

      // npoints: Extract maximum number of points (samples, rows 
      // of data) to read from the data file
      case 'n':
        npoints_cmd_line = atoi( optarg);
        if( npoints_cmd_line < 1) usage();
        break;
      
      // npoints: Extract maximum number of points (samples, rows 
      // of data) to read from the data file
      case 's':
        nSkipHeaderLines = atoi( optarg);
        if( nSkipHeaderLines < 0) usage();
        break;
      
      // ordering: Extract the ordering of ("columnmajor or 
      // rowmajor") of a binary input file
      case 'o':
        if( !strncmp( optarg, "columnmajor", 1))
          ordering=COLUMN_MAJOR;
        else if ( !strncmp( optarg, "rowmajor", 1))
          ordering=ROW_MAJOR;
        else {
          usage();
          exit( -1);
        }
        break;

      // rows: Extract the number of rows of plot windows
      case 'r':
        nrows = atoi( optarg);
        if( nrows < 1) usage();
        break;

      // cols: Extract the number of columns of plot windows
      case 'c':
        ncols = atoi( optarg);
        if( ncols < 1) usage();
        break;

      // monitors: Extract the number of monitors
      case 'm':
        number_of_screens = atoi( optarg);
        if( number_of_screens < 1) usage();
        break;

      // inputfile: Extract data filespec
      case 'i':
        strcpy( inFileSpec, optarg);
        break;

      // borders: Turn off window manager borders on plot windows
      case 'b':
        borderless = 1;
        break;

      // help: Help
      case 'h':
      case ':':
      case '?':
        usage();
        exit( -1);
        break;
      
      // Default
      default:
        usage();
        exit( -1);
        break;
    }
  }

  // If no arguments were specified, then quit
  if( argc <= 1) {
    usage();
    exit( 0);
  }

  // If no data file was specified, assume the last argument is
  // the filespec.
  if( strlen( inFileSpec) <= 0) strcpy( inFileSpec, argv[ argc-1]);

  // Increment pointers to the optional arguments to get the
  // last argument.
  argc -= optind;
  argv += optind;

  // Set random seed
  srand( (unsigned int) time(0));

  // Restrict format and restruct and set number of plots
  assert( format==BINARY || format==ASCII);
  assert( nrows*ncols <= maxplots);
  nplots = nrows*ncols;

  // STEP 2: Read the data file and quit if problems arose
  if( load_data_file( inFileSpec) < 0) return -1;

  // Fewer points -> bigger starting pointsize
  pointsize = max( 1.0, 6.0 - (int) log10f( (float) npoints));

  // STEP 3: Create main control panel
  // Determine the number of screens.  NOTE screen_count requires 
  // OpenGL 1.7, which was not available under most Windows OS as 
  // of 10-APR-2006.
  #ifndef __WIN32__
    if( number_of_screens <= 0)
      number_of_screens = Fl::screen_count();
  #else 
    if( number_of_screens <= 0)
      number_of_screens = 1;
  #endif   // __WIN32__

  // Set the main control panel size and position.
  // const int main_w = 350, main_h = 700;
  const int main_x = 
    number_of_screens*Fl::w() - 
    (main_w + left_frame + right_frame + right_safe);
  const int main_y = top_frame+top_safe;

  // Create the main control panel window
  create_main_control_panel( 
    main_x, main_y, main_w, main_h,
    "viewpoints -> creon@nas.nasa.gov");

  // Step 4: Create an array of plot windows with associated tabs
  // in the main control panel window
  create_plot_window_array( argc, argv, main_w);

  // now we can show the main control panel and all its subpanels
  main_control_panel->show();

  // Step 5: Set pointer to the function to call when the window 
  // is idle and enter the main event loop
  Fl::add_idle( redraw_if_changing);
  // Fl::add_check(redraw_if_changing);

  // Enter the main event loop
  int result = Fl::run();
  return result;
}
