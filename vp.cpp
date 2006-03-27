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
//   May require D__WIN32__ for the C++ compiler
//
// Purpose: viewpoints - interactive linked scatterplots and more.
//
// General design philosophy:
//   1) This code is represents a battle between Creon Levit's 
//      passion for speed and efficiency and Paul Gazis's obsession
//      with organization and clarity.  Whether the results are
//      brilliant or catastrophic remains to be determined...
//
// Functions:
//   choose_color_deselected( *o) -- Color of nonselected points
//   change_all_axes( *o) -- Change all axes
//   clearAlphaPlanes() -- Clear alpha planes
//   npoints_changed( *o) -- Update number of points changed
//   write_data( *o) -- Write binary file
//   reset_all_plots( void) -- Reset all plots
//   redraw_if_changing( *dummy) -- Redraw changing plots
//   make_global_widgets() -- Controls for main control panel
//   resize_global_arrays() -- Resize global arrays
//   remove_trivial_columns() -- Remove identical data
//   read_ascii_file_with_headers( *inFileSpec) -- Read ASCII
//   read_binary_file_with_headers( *inFileSpec) -- Read binary
//   usage() -- Print help information
//
// Modification history
// 25-MAR-2006:
// -Reorganized headers for clarity and ease of maintentance
// -Restructured code to make class plot_window and class
//  control_panel_window self-contained.
// -Moved global variables to local files and within classes to
//  limit their scope
//
// Author: Creon Levit   unknown
// Modified: P. R. Gazis  27-MAR-2006
//*****************************************************************

// Include the necessary include libraries
#include "include_libraries_vp.h"

// Include globals
#include "global_definitions_vp.h"

// Include associated headers and source code
#include "plot_window.h"
#include "control_panel_window.h"
#include "plot_window.cpp"
#include "control_panel_window.cpp"

// Approximate values of window manager borders & desktop borders 
// (stay out of these).  These are used by the main method when 
// the main control panel window is defined.
#ifdef __APPLE__
 int top_frame=35, bottom_frame=0, left_frame=0, right_frame=5;
 int top_safe = 1, bottom_safe=5, left_safe=5, right_safe=1;
#else // __APPLE__
 int top_frame=25, bottom_frame=5, left_frame=4, right_frame=5;
 int top_safe = 1, bottom_safe=10, left_safe=10, right_safe=1;
#endif // __APPLE__

// Define and set maximums for header block
const int MAX_HEADER_LENGTH = 2000;  // Length of header line
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

// Define pointer to hold main control panel
Fl_Window *main_control_panel;

// Function definitions for the main method
void choose_color_deselected( Fl_Widget *o);
void change_all_axes( Fl_Widget *o);
void clearAlphaPlanes();
void npoints_changed( Fl_Widget *o);
void write_data( Fl_Widget *o);
void reset_all_plots( void);
void redraw_if_changing( void *dummy);
void make_global_widgets();
void resize_global_arrays();
void remove_trivial_columns();
int read_ascii_file_with_headers( char* inFileSpec);
int read_binary_file_with_headers( char* inFileSpec);
void usage();

//*****************************************************************
// choose_color_deselected( *o) -- Choose color of deselected 
// points.  Could this become a static member function of class
// plot_window?
void choose_color_deselected( Fl_Widget *o)
{
  (void) fl_color_chooser(
    "deselected", r_deselected, g_deselected, b_deselected);
  pws[ 0]->update_textures (); // XXX 
}

//*****************************************************************
// change_all_axes( *o) -- Invoke the change_axes method of each
// plot_window to change all unlocked axes.
void change_all_axes( Fl_Widget *o) {
  int nchange = 0;
  for( int i=0; i<nplots; i++) {
    if( !cps[i]->lock_axes_button->value()) nchange++;
  }
  for( int i=0; i<nplots; i++) {
    if( !cps[i]->lock_axes_button->value())
      pws[i]->change_axes( nchange);
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
// write_data( o) -- Open and write a binary data file.  File will
// consist of an ASCII header with column names terminated by a
// newline, followed by a long blonck of binary data.  Invoked by
// main control panel.  Could this become a member function of the 
// proposed data class?
void write_data( Fl_Widget *o)
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
      os << column_labels[i] << " ";
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
// reset_all_plots() -- Reset all plots.  Invoked by main control 
// panel.
void reset_all_plots()
{
  for( int i=0; i<nplots; i++) {
    pws[i]->reset_view();
  }
}

//*****************************************************************
// redraw_if_changing( dummy) -- Callback function for use by FLTK
// Fl::add_idle.  When an idle callback occurs, redraw any panel
// that has been spun or otherwise needs to be redrawn.
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
// make_global_widgets() -- Controls for main control panel
void make_global_widgets()
{
  // Draw 'npoints' horizontal slider at top of subpanel
  int xpos=10, ypos=500;
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

  // Button(2,1): Invert colors of selected and nonselected data
  invert_selection_button = b = 
    new Fl_Button( xpos, ypos+=25, 20, 20, "invert selection");
  b->align( FL_ALIGN_RIGHT); 
  b->selection_color( FL_BLUE); 
  b->callback( (Fl_Callback*) plot_window::invert_selection);

  // Button(3,1): Clear selection
  clear_selection_button = b = 
    new Fl_Button(xpos, ypos+=25, 20, 20, "clear selection");
  b->align( FL_ALIGN_RIGHT); 
  b->selection_color( FL_BLUE); 
  b->callback( plot_window::clear_selection);

  // Button(4,1): Delete selected data
  delete_selection_button = b = 
    new Fl_Button( xpos, ypos+=25, 20, 20, "kill selected");
  b->align( FL_ALIGN_RIGHT); 
  b->selection_color( FL_BLUE); 
  b->callback( plot_window::delete_selection);

  // Button(5,1): Write binary data file
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
}

//*****************************************************************
// resize_global_arrays -- Resize various global arrays used to
// read data
void resize_global_arrays()
{
  // points.resizeAndPreserve(nvars,npoints);	

  ranked_points.resize(nvars,npoints);

  ranked.resize(nvars);
  ranked = 0;  // initially, no ranking has been done.

  tmp_points(npoints); // for sort

  texture_coords.resize(npoints);
  identity.resize(npoints);
  newly_selected.resize(npoints);
  selected.resize(npoints);
  previously_selected.resize(npoints);

  selected=0;
}

//*****************************************************************
// remove_trivial_columns -- Examine an array of data and remove
// columns for which all values are identical.  Part of the read
// process.  Could this become part of the propsoed data class?
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
           << column_labels[current] << endl;
      for( int j=current; j<nvars-1; j++) {
        points( j, NPTS) = points( j+1, NPTS);
        column_labels[j] = column_labels[j+1];
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
// read_ascii_file_with_headers( inFileSpec) -- Open an ASCII file 
// for input, read and discard the headers, read the data block, 
// and close the file.  In this version, the first line of the
// file is always assumed to be the header.  Returns 0 if 
// successful.
int read_ascii_file_with_headers( char* inFileSpec) 
{
  // Attempt to open input file and make sure it exists
  ifstream inFile;
  inFile.open( inFileSpec, ios::in);
  if( inFile.bad()) {
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

  // If no header lines were found or the LASTHEADERLINE buffer is
  // empty, examine the first line of the data block to determine 
  // the number of columns and generate a set of column labels.
  if( nHeaderLines == 0 || lastHeaderLine.length() == 0) {
    std::stringstream ss( line);
    std::string buf;
    nvars = 0;
    while( ss >> buf) {
      nvars++;
      char cbuf[ 80];
	  (void) sprintf(cbuf, "%d", nvars);
      buf = "Column_";
      buf.append( cbuf);
      column_labels.push_back( buf);
    }
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
  if( npoints_cmd_line != 0) npoints = npoints_cmd_line;
  points.resize(nvars,npoints);

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
// fgets, fread, feof, and fclose, from <stdio>.
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
  if( strlen( cBuf) >= MAX_HEADER_LENGTH) {
    cout << " -ERROR: Header string is too long, "
         << "increase MAX_HEADER_LENGTH and recompile"
         << endl;
    fclose( pInFile);
    return 1;
  }
  std::string line;
  line.assign( cBuf);

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
  // NOTE: Otherwise, npoints = MAXPOINTS.
  if( npoints_cmd_line != 0) npoints = npoints_cmd_line;
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
      if( ret != nvars) {
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
      if( ret != nvars) {
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
// usage() -- Print help information and exit
void usage()
{
  cerr << "Usage: vp {optional arguments} datafile" << endl;
  cerr << "  [--format={ascii,binary}] (-f)" << endl;
  cerr << "  [--npoints=<int>] (-n)" << endl;
  cerr << "  [--skip_header_lines=<int>] (-s)" << endl;
  cerr << "  [--rows=<int>] (-r)" << endl;
  cerr << "  [--cols=<int>] (-c)" << endl;
  cerr << "  [--input_file={input filespec}] (-i)" << endl;
  cerr << "  [--borderless] (-b)" << endl;
  cerr << "  [--help] (-h)" << endl;
  exit( -1);
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
  // cout << "argc<" << argc << ">" << endl;
  // for( int i=0; i<argc; i++) {
  //   cout << "argv[ " << i << "]: <" << argv[ i] << ">" << endl;
  // }

  // Define structure to hold command-line options
  static struct option long_options[] = {
    { "format", required_argument, 0, 'f'},
    { "npoints", required_argument, 0, 'n'},
    { "skip_header_lines", required_argument, 0, 's'},
    { "ordering", required_argument, 0, 'o'},
    { "rows", required_argument, 0, 'r'},
    { "cols", required_argument, 0, 'c'},
    { "input_file", required_argument, 0, 'i'},
    { "borderless", no_argument, 0, 'b'},
    { "help", no_argument, 0, 'h'},
    { 0, 0, 0, 0}
  };

  // STEP 1: Parse command line
  
  // Loop: Invoke GETOPT_LONG to advance through and parse 
  // successive command-line arguments (Windows version is
  // implemented in LIBGW32).  NOTES: 1) The possible options MUST 
  // be listed in the call to GETOPT_LONG, 2) This process does NOT 
  // effect arc and argv in any way.
  int c;
  char inFileSpec[ 80];
  strcpy( inFileSpec, "");
  while( 
    ( c = getopt_long( 
        argc, argv, 
        "f:n:s:o:r:c:i:b:h", long_options, NULL)) != -1) {
  
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

      // rows: Extract the number of rows in scatterplot matrix
      case 'r':
        nrows = atoi( optarg);
        if( nrows < 1) usage();
        break;

      // cols: Extract the number of columns in scatterplot matrix
      case 'c':
        ncols = atoi( optarg);
        if (ncols < 1) usage();
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

  // STEP 2: Read the data file

  // Read data file and remove trivial columns
  cout << "Reading input data from <" << inFileSpec << ">" << endl;
  int iReadStatus = 0;
  if( format == BINARY) 
    iReadStatus = read_binary_file_with_headers( inFileSpec);
  else if( format == ASCII)
    iReadStatus = read_ascii_file_with_headers( inFileSpec);
  cout << "Finished reading file <" << inFileSpec << ">" << endl;
  remove_trivial_columns ();

  // If only one or fewer records are available then quit before 
  // something terrible happens!
  if( npoints <= 1) {
    cout << "Insufficient data, " << npoints
         << " samples." << endl;
    return 0;
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

  // Fewer points -> bigger starting pointsize
  pointsize = max( 1.0, 6.0 - (int) log10f( (float) npoints));

  // Make identity
  cout << "making identity array, a(i)=i" << endl;
  for( int i=0; i<npoints; i++) identity( i)=i;

  // STEP 3: Create main control panel

  // Set main control panel size and position.  NOTE screen_count
  // requires OpenGL 1.7, which was not available under Windows as
  // of 14-MAR-2006.
  #ifndef __WIN32__
    int number_of_screens = Fl::screen_count();
  #else
    int number_of_screens = 1;
  #endif   // __WIN32__
  const int main_w = 350, main_h = 700;
  // const int main_x = 
  //   Fl::screen_count()*Fl::w() - 
  //   (main_w + left_frame + right_frame + right_safe), 
  //   main_y = top_frame+top_safe;
  const int main_x = 
    number_of_screens*Fl::w() - 
    (main_w + left_frame + right_frame + right_safe);
  const int main_y = top_frame+top_safe;

  // main control panel window
  Fl::scheme( "plastic");  // optional
  main_control_panel = 
    new Fl_Window(
      main_x, main_y, main_w, main_h, 
      "viewpoints -> creon@nas.nasa.gov");
  main_control_panel->resizable( main_control_panel);

  // Add the rest of the global widgets to control panel
  make_global_widgets ();

  // Inside the main control panel, there is a tab widget that 
  // contains the sub-panels (groups), one per plot.
  cpt = new Fl_Tabs( 3, 10, main_w-6, 500);    
  cpt->selection_color( FL_BLUE);
  cpt->labelsize( 10);

  // Done creating main control panel (except for tabbed 
  // sup-panels, below)
  main_control_panel->end();

  // Create and add the virtul sub-panels (each a group under a 
  // tab), one per plot.
  for( int i=0; i<nplots; i++) {
    int row = i/ncols;
    int col = i%ncols;

    // Account for borderless option
    if( borderless)
      top_frame = bottom_frame = left_frame = right_frame = 1;

    // Plot window size
    // int pw_w = 
    //   ((Fl::screen_count()*Fl::w() - 
    //   (main_w+left_frame+right_frame+right_safe+left_safe+20)) / ncols) - 
    //   (left_frame + right_frame);
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

    // Create a label
    ostringstream oss;
    oss << "" << i+1;
    string labstr = oss.str();

    // Add a new virtual control panel (a group) under the tab
    // widget
    Fl_Group::current(cpt);	
    cps[i] = new control_panel_window( 3, 30, main_w-6, 480);
    cps[i]->copy_label( labstr.c_str());
    cps[i]->labelsize( 10);
    cps[i]->resizable( cps[i]);
    cps[i]->make_widgets( cps[i]);

    // End the group since we want to create new plot windows at 
    // the top level.
    cps[i]->end();
    Fl_Group::current(0); 

    // Create plotting window i
    pws[i] = new plot_window( pw_w, pw_h);
    pws[i]->copy_label( labstr.c_str());
    pws[i]->position(pw_x, pw_y);
    pws[i]->row = row; pws[i]->column = col;
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
      cps[i]->hide();	
    } 
    else {
      plot_window::upper_triangle_incr( ivar, jvar, nvars);
    }
    cps[i]->varindex1->value(ivar);  
    cps[i]->varindex2->value(jvar);  

    pws[i]->extract_data_points();
    pws[i]->reset_view();
    pws[i]->size_range(10, 10);
    pws[i]->resizable(pws[i]);

    if( borderless) pws[i]->border(0);
    pws[i]->show(argc,argv);
    pws[i]->resizable(pws[i]);
  }

  // now we can show the main control panel and all its subpanels
  main_control_panel->show();

  Fl::add_idle( redraw_if_changing);
  // Fl::add_check(redraw_if_changing);

  // Step 4: Enter main event loop
  
  // enter main event loop
  int result = Fl::run();
  return result;
}
