// viewpoints - interactive linked scatterplots and more.
// copyright 2005 Creon Levit, all rights reserved.
//*****************************************************************
// File name: grid2.cpp
//
// Class definitions:
//   plot_window -- Plot window
//   control_panel_window -- Control panel window
//   my_compare -- unknown
//
// Classes referenced:
//
// Required packages
//    FLTK 1.1.6 -- Fast Light Toolkit graphics package
//    FLEWS 0.3 -- Extensions to FLTK 
//    OGLEXP 1.2.2 -- Access to OpenGL extension under Windows
//    GSL 1.6 -- Gnu Scientific Library package for Windows
//    Blitz++ 0.9 -- Various math routines
//
// Compiler directives: -DWIN32
//
// Purpose: viewpoints - interactive linked scatterplots and more.
//
// General design philosophy:
//   1) Add comments and get this code to run under Windows.
//
// Functions:
//   invert_selection() -- Invert color of selected points.
//   toggle_display_delected( *o) -- Toggle colors
//   clear_selection( *o) -- Clear selection
//   choose_color_deselected( *o) -- Color of non-selected points
//   delete_selection( *o) -- Delete selection
//   upper_triangle_incr( int &i, int &j, const int nvars)
//   change_all_axes( Fl_Widget *o)
//   initialize_textures()
//   clearAlphaPlanes()
//   npoints_changed( Fl_Widget *o) 
//   write_data( Fl_Widget *o)
//   redraw_all_plots (int p)
//   reset_all_plots();
//   redraw_if_changing( void * dummy));
//   make_global_widgets();
//   remove_trivial_columns(); 
//   resize_global_arrays();
//   read_ascii_file_with_headers( char* inFileSpec);
//   read_binary_file_with_headers( char* inFileSpec);
//   usage();
//
// Modification history
// 13-MAR-2006:
// -Added the necessary defines and includes for Windows, 
//  including #define usleep and specific references to GSL.
// -Identified necessary libraries and the order in which they 
//  must be referenced.
// -Moved definition of float pmin in plot_window::normalize
// -Removed use of Fl::screen_count (fltk 1.7)
// - '#if out' the #ifdef __APPLE__ includes of glext.h
// - Moved remove_trivial_columns, resize_global_arrays,
// read_ascii_file_with_headers, read_ascii_file,
// read_binary_file, read_binary_file_with_headers, and usage() 
// to immediately before main() for diagnostic purposes
// - Add code to extract data filespec from the command-line
// explicitly and pass it to the file read routines.  Removed
// calls to exit(iStatus) from the read routines and modified
// them to return an int value to indicate success or failure.
// This is then tested by the main routine.
// 14-MAR-2006
// - Removed read_ascii_file and read_binary_file, which don't
// seem to be used.
// - Modifed usage() to add information and use new-style io.
// - Added references to a WIN32 compiler directive to control
// Windows-specific #if and #ifdef statements.
// 16-MAR-2006
// - Changed read_binary_file_with_headers to make it work under
// Windows.  Open input file explicitly as a FILE*, use fgetf to
// read the header line, and fread with pInFile instead of stdin.
// 20-MAR-2006
// - Replaced 'goto nextline:' in read_ascii_data_with_headers
// with the flag 'isBadData'.
// - Revised read_ascii_data_with_headers to recognize and handle
// the comment characters '!#%'.
// - Revised read routines and remove_trivial_columns to clarify
// reports to console.
// - Revised main routine to quit if no data was read.
//
// Author: Creon Levit   unknown
// Modified: P. R. Gazis  14-MAR-2006
//*****************************************************************
#define __WINDOWS__

// General note on defines: Several defines are required to account 
// for variable and function definitions that seem to be missing.  
// This is a temporary fix that should be replaced as soon as 
// possible
// #ifndef MAXFLOAT 
//   #define MAXFLOAT 3.402823466e+38f 
// #endif
#ifdef __WINDOWS__
  #define MAXFLOAT 3.402823466e+38f 
  #define usleep(v) Sleep(v/1000)
#endif // __WINDOWS__

// General note in includes: The FLTK package should be handled by
// the Dev-C++ programming environment, but several include 
// libraries are needed in addition to the regular Dev-C++ and
// FLTK libraries.  These are listed below
//
// Flews
//   c:\devusr\flews
// OglExt (Needed to use OpenGL 1.2+)
//   c:\devuser\oglext\include
// GSL (Needed for parts of Blitz++)
//   c:\devuser\GnuWin32\include
//   c:\devuser\GnuWin32\include\glibc
// Blitz++
//   c:\devusr\blitz

// Add C includes here.  Note that some of the relevant libraries
// are part of GSL rather than the Dev-C++ package.  Locations of
// some crucial files are described below:
// <unistd.h>    -- c:\Dev-cpp\include
// <fcntl.h>     -- c:\Dev-cpp\include
// <assert.h>    -- c:\Dev-cpp\include
// <sys/types.h> -- c:\Dev-cpp\include\sys
// <sys/uio.h>   -- c:\devuser\GnuWin32\include\glibc\sys
// <sys/stat.h>  -- c:\Dev-cpp\include\sys
// <sys/time.h>  -- c:\Dev-cpp\include
//               -- c:\Dev-cpp\include\sys
//               -- c:\devuser\GnuWin32\include\glibc
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <math.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <getopt.h>

// These includes should all be part of Dev-C++
// <float.h>  -- c:\Dev-cpp\include
// <values.h> -- c:\Dev-cpp\include
//            -- c:\devuser\GnuWin32\include\glibc
#ifdef __APPLE__
  #include <float.h>
#else
  #include <values.h>
#endif // __APPLE__

// Add C++ includes here.  These should all be part of Dev-C++
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
 
// FLTK.  These includes should be handled by Dev-C++
#include <FL/math.h>
#include <FL/gl.h>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Hor_Value_Slider.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Color_Chooser.H>

// flews (FLTK extension) extras.  These should be located in
// c:\devusr\flews as described above
#include <FL/Fl_flews.h>
#include <FL/Fl_Value_Slider_Input.H>
#include "Fl_Hor_Value_Slider_Input.H"  // my modified flews slider

// OpenGL extensions.  NOTE: Some of these will clobber a Windows 
// compile, so they must be excluded by the relevant defines!
#ifdef __APPLE__
  #include <OpenGL/glext.h>
#endif // __APPLE__
#ifdef __LINUX__
  #include <GL/glext.h>
#endif // __LINUX__
#ifdef __WINDOWS__
  // OglExt.  Obtain from c:\devusr\oglext\include as described
  // above and invoke as described in OGLEXT documentation
  #define GL_GLEXT_PROTOTYPES
  #include <glext.h>
  #include <GL/glext.h>
#endif // __WINDOWS__

// GSL (Must be included before Blitz++!)
#ifdef __WINDOWS__
  #include <C:\devusr\GnuWin32\include\config.h>
  #include <winx/sys/timex.h>
#endif // __WINDOWS__
#include <gsl/gsl_sys.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_cdf.h>

// Blitz++ (C++ array operations via template metaprogramming)
#include <blitz/array.h>

// BLAS
#ifdef __APPLE__
  #include <vecLib/vBLAS.h>
#elif linux
  //extern "C" {
  //# include <cblas.h>
  //}
#endif // __APPLE__
      
// debugging and application-specific definitions
#include "vp.h"

// Use the Standard Template Library
using namespace std;

// Define and set default values
int format=ASCII;   // default input file format
int ordering=COLUMN_MAJOR;   // default input data ordering
const int MAX_HEADER_LENGTH = 2000;  // Length of header line
const int MAX_HEADER_LINES = 2000;  // Number of header lines

// Set the maximum number of columns and rows, points, or samples,
// and the number of columns to skip at the beginning of each row.
int nSkipHeaderLines = 1;  // Number of header lines to skip
const int nvars_max = 256;  // Maximum number of columns
const int MAXPOINTS = 2000000;  // Maximum number of rows
const int skip = 0;  // Number of columns to skip

// Set parameters to define the default layout and border style of
// the plot windows.  NOTE: Creon notes that maxplots must be a 
// power of 2 for textures and that this will cause trouble.
int nrows=2, ncols=2;   // Default number of rows and columns
int nplots = nrows*ncols;  // Default number of plot windows
const int maxplots=64;  // Maxmimum number of plots
int borderless=0;  // By default, use window manager borders

// Specify classes for access by one-pass compilers
class plot_window;
class control_panel_window;

// Initialize the actual number of rows (points or values) in 
// the data file and command line and the actual number of 
// columns in each record.
int npoints = MAXPOINTS;   // number of rows in data file
int npoints_cmd_line = 0;   // number of points on cmd line.
int nvars = nvars_max;		// number of columns in data file

// Global toggle between scale histgram & scale view :-(
int scale_histogram = 0;

// Define blitz::Array variables to hold data arrays
blitz::Array<float,2> points;  // main data array
blitz::Array<int,2> ranked_points;   // data, ranked, as needed.
blitz::Array<int,1> ranked;	  // flag: 1->column is ranked, 0->not
blitz::Array<int,1> identity;   // holds a(i)=i.

// Define blitz::Arrays to flag selected points
// newly_selected -- true iff point is in newly selected set
// selected -- true if point is selected in any window
// previously_selected -- true iff selected before mouse went down
// nselected -- number of points currently selected
blitz::Array<GLshort,1> newly_selected;	
blitz::Array<GLshort,1> selected;	
blitz::Array<GLshort,1> previously_selected;	
int nselected = 0;	

// Texture co-ordinates?
blitz::Array<GLshort,1> texture_coords;

// Define parameters used by the selection algorithm
double r_deselected=1.0, g_deselected=0.01, b_deselected=0.01;

// Temporary array (reference) for qsort
blitz::Array<float,1> tmp_points;
float *tpoints;

// Define vector of strings to hold variable names
std::vector<std::string> column_labels; 

// Set the initial fraction of the window to be used for data to 
// allow room for axes, labels, etc.  (Move to plot_window class?)
const float initial_pscale = 0.8; 

// These variables do not seem to be used
// float xmin, xmax, gmax;

// Define variable to hold pointsize.  (Move to plot_window class?)
float pointsize = 1.0;

// This variable does not seem to be used
// int istart = 0;

// Specify how the RGB and alpha source and destination blending 
// factors are computed.  (Move to plot_window class?)
int sfactor = GL_CONSTANT_COLOR;
int dfactor = GL_DST_ALPHA;

// Function definitions
void redraw_all_plots( int);
void reset_all_plots( void);

// Flag to indicate that selection has been changed.  Creon notes
// that this should be fixed, but that this is than it looks.
int selection_changed = 1;

// Define pointer to hold main control panel
Fl_Window *main_control_panel;

// Define main control panel's top level (global) widgets:
// cpt -- tabs to hold individual plot's virtual control panels
// npoints_slider -- max number of points to display in all plots
Fl_Tabs *cpt;  
Fl_Hor_Value_Slider_Input *npoints_slider;
Fl_Button *add_to_selection_button, 
          *clear_selection_button, 
          *delete_selection_button;
Fl_Button *show_deselected_button, 
          *invert_selection_button;
Fl_Button *write_data_button;
Fl_Button *choose_color_selected_button, 
          *choose_color_deselected_button, 
          *dont_paint_button;
Fl_Button *change_all_axes_button;
Fl_Button *link_all_axes_button;

// the plot_window class is subclass of an ftlk openGL window that 
// also handles certain keyboard & mouse events.  It is where data 
// is displayed.  There are usually several open at one time.
//*****************************************************************
// Class: plot_window
//
// Class definitions:
//   plot_window -- Maintain and manage plot window
//
// Classes referenced:
//   control_panel_window
//
// Purpose: Derived class of Fl_Gl_Window to construct, draw,
//   and manage a plot window
//
// Functions:
//   plot_window( w, h) -- Constructor
//
//   draw() -- Draw plot
//   draw_grid() -- Draw grid
//   void draw_axes() -- Draw axes
//   // void draw_labels();
//   // void draw_scale();
//   draw_data_points() -- Draw data points
//   void draw_center_glyph();
//   void update_linked_transforms();
//   handle( event) -- Main event handler
//   handle_selection() -- Handle this selection?
//	 void screen_to_world( xs, ys, x, y);
//   void print_selection_stats();
//   draw_histograms() -- Draw histogram
//   compute_rank( blitz::Array<float,1>, blitz::Array<int,1>);
//   compute_histograms () -- Compute histograms
//   normalize();
//   extract_data_points();
//   transform_2d();
//   reset_selection_box();
//   color_array_from_new_selection();
//   color_array_from_selection() -- Color selected points?
//   update_textures ();
//   choose_color_selected ();
//   reset_view() -- Reset plot
//   redraw_one_plot();
//   change_axes( nchange);
//
// Author: Creon Levit    unknown
// Modified: P. R. Gazis   14-MAR-2006
//*****************************************************************
class plot_window : public Fl_Gl_Window
{
  protected:
            
    // Draw routines
    void draw();
    void draw_grid();
    void draw_axes();
    // void draw_labels();
    // void draw_scale();
    void draw_data_points();
    void draw_center_glyph();
    void update_linked_transforms();

    // Event handlers
    int handle (int event);
    void handle_selection();

	void screen_to_world(
      float xs, float ys, float &x, float &y);
	void print_selection_stats();

    int xprev, yprev, xcur, ycur;
    float xdragged, ydragged;
    float xcenter, ycenter, zcenter;
	float xscale, yscale, zscale;
    float xzoomcenter, yzoomcenter, zzoomcenter;
    float xdown, ydown, xtracked, ytracked;
	int extend_selection;
    static int count;

    // Arrays and routines for histograms
    int nbins;
    blitz::Array<float,2> counts, counts_selected;
    void compute_histogram( int);
    float xhscale, yhscale;
    void draw_histograms ();
	int show_center_glyph;

  public:
    plot_window(int w, int h);

    blitz::Array<float,2> vertices;
    blitz::Array<int,1> x_rank, y_rank, z_rank;
    float amin[3], amax[3]; // min and max for data's bounding box in x, y, and z;
    float wmin[3], wmax[3]; // min and max for window's bounding box in x, y, and z;
    void compute_rank( 
      blitz::Array<float,1> a, 
      blitz::Array<int,1> a_rank, 
      int var_index);
    static const int nbins_default = 128;
    static const int nbins_max = 1024;

    // Routines for histograms
    void compute_histograms ();
    int normalize( 
      blitz::Array<float,1> a, blitz::Array<int,1> a_rank, 
      int style, int axis_index);

    // Define strings to hold axis labels
    std::string xlabel, ylabel, zlabel;

    // Pointer to and index of the control panel tab associated 
    // with this plot window.  Each plot window has the same index
    // as its associated tab.
    control_panel_window *cp;
    int index;

    // Initial ordering of windows on screen.  Upper left window is 
    // (1,1)
	int row, column;

    // More plot routines
    int extract_data_points();
    int transform_2d();

    // Routines and variables to handle selection
    void reset_selection_box();
    void color_array_from_new_selection();
    void color_array_from_selection();
	void update_textures ();
	void choose_color_selected ();
	double r_selected, g_selected, b_selected;

    // Routines to redraw plots
    void reset_view();
    void redraw_one_plot();
    void change_axes( int nchange);
    float angle;
    int needs_redraw;
};

// Initialize number of plot windows
int plot_window::count = 0;

//*****************************************************************
// plot_window::plot_window( w, h) -- Constructor.  Increment
// count of plot wndows, resize arrays, and set mode.
plot_window::plot_window(int w,int h) : Fl_Gl_Window(w,h) 
{
  // Initialize count and set format parameters.
  count++;
  show_center_glyph = 0;
  r_selected=0.01, g_selected=0.01, b_selected=1.0;

  // Resize arrays
  vertices.resize(npoints,3);
  x_rank.resize( npoints);
  y_rank.resize( npoints);
  z_rank.resize( npoints);
  nbins = nbins_default;
  counts.resize( nbins_max,3);
  counts_selected.resize(nbins_max,3);

  // Set mode
  #if TRY_DEPTH
    // Creon notes that this doesn't seem to work on PBG4 OSX
    if( can_do(FL_RGB|FL_DOUBLE|FL_ALPHA|FL_DEPTH)) {
      mode( FL_RGB|FL_DOUBLE|FL_ALPHA|FL_DEPTH);
    }
    else {
      cout << "Warning: depth buffering not enabled" << endl;
      mode( FL_RGB|FL_DOUBLE|FL_ALPHA);
    }
  #else
    mode( FL_RGB8|FL_DOUBLE|FL_ALPHA);
  #endif
}

//*****************************************************************
// Class: control_panel_window
//
// Class definitions:
//   control_panel_window
//
// Classes referenced:
//   plot_window -- Maintain and manage plot window
//
// Purpose: Derived class of Fl_Gl_Window to construct, draw,
//   and manage a plot window
//
// Functions:
//   control_panel_window( w, h) -- Constructor
//
//   maybe_redraw () -- Set redraw flag
//   make_widgets( control_panel_window *cpw);
//   extract_and_redraw ();
//   choose_color_selected( Fl_Widget *w, control_panel_window *cpw)
//   static_extract_and_redraw (Fl_Widget *w, control_panel_window *cpw)
//   static_maybe_redraw(Fl_Widget *w, control_panel_window *cpw)
//   replot (Fl_Widget *w, control_panel_window *cpw)
//   reset_view (Fl_Widget *w, control_panel_window *cpw)
//   redraw_one_plot (Fl_Widget *w, control_panel_window *cpw)
//
// Author: Creon Levit   unknown
// Modified: P. R. Gazis  14-MAR-2006
//*****************************************************************
class control_panel_window : public Fl_Group
{
  protected:
    void maybe_redraw ();

  public:
    control_panel_window( int x, int y, int w, int h);
    void make_widgets( control_panel_window *cpw);
    void extract_and_redraw ();

    static void choose_color_selected(
      Fl_Widget *w, control_panel_window *cpw)
		{ cpw->pw->choose_color_selected() ;}
    static void static_extract_and_redraw( 
      Fl_Widget *w, control_panel_window *cpw)
        { cpw->extract_and_redraw(); }
    static void static_maybe_redraw(
      Fl_Widget *w, control_panel_window *cpw)
        { cpw->maybe_redraw() ;}
    static void replot(
      Fl_Widget *w, control_panel_window *cpw)
        { /* cpw->pw->redraw(); */ cpw->pw->needs_redraw=1;}
    static void reset_view(
      Fl_Widget *w, control_panel_window *cpw)
        { cpw->pw->reset_view() ;}
    static void redraw_one_plot(
      Fl_Widget *w, control_panel_window *cpw)
        { cpw->pw->redraw_one_plot();}

    Fl_Hor_Value_Slider_Input *pointsize_slider;
    Fl_Hor_Value_Slider_Input *Bkg, *Lum, *Alph;
    Fl_Hor_Value_Slider_Input *rot_slider;
    Fl_Hor_Value_Slider_Input *nbins_slider;
    Fl_Choice *varindex1, *varindex2, *varindex3;
	
    Fl_Button *reset_view_button;
    Fl_Button *spin, *dont_clear, *show_points, 
              *show_deselected_points, 
              *show_axes, *show_grid, *show_labels, 
              *show_histogram;
    Fl_Button *show_scale, *lock_axes_button;
    // Fl_Button *x_equals_delta_x, *y_equals_delta_x;
    Fl_Group *transform_style;
    Fl_Button *sum_vs_difference, *polar, *no_transform;
    Fl_Choice *x_normalization_style, 
              *y_normalization_style, 
              *z_normalization_style;

    // Pointer to and index of the plot window associated with 
    // this control panel tab.  Each plot window has the same 
    // color and index as its associated control panel tab.
    plot_window *pw;
    int index;	
};

//*****************************************************************
// control_panel_window::control_panel_window( w, h) -- 
// Default constructor.  Do nothing.
control_panel_window::control_panel_window(
  int x, int y, int w, int h) : Fl_Group( x, y, w, h)
{}

// Define pointer arrays of plot windows and control panel 
// windows.  NOTE: In the long run, it might be safer to store 
// these in instances of the <vector> container class.
plot_window *pws[maxplots];
control_panel_window *cps[maxplots];

// These menu related lists should really be class variables in 
// class control_panel_window
Fl_Menu_Item varindex_menu_items[ nvars_max+2]; 

// Associate integer values with normalization styles
const int NORMALIZATION_NONE 	= 0;
const int NORMALIZATION_MINMAX 	= 1;
const int NORMALIZATION_ZEROMAX = 2;
const int NORMALIZATION_MAXABS	= 3;
const int NORMALIZATION_TRIM_1E2 = 4;
const int NORMALIZATION_TRIM_1E3 = 5;
const int NORMALIZATION_THREESIGMA = 6;
const int NORMALIZATION_LOG10 = 7;
const int NORMALIZATION_SQUASH = 8;
const int NORMALIZATION_RANK = 9;
const int NORMALIZATION_GAUSSIANIZE = 10;

// Define normalization style labels and normalization styles, then
// determine the number of normalization styles and define an array
// of Fl_menu_items to hold them.
const char *normalization_style_labels[] = { 
  "none", "minmax", "zeromax", "maxabs", "trim 10^-2", 
  "trim 10^-3", "threesigma", "log_10", "squash", "rank",
  "gaussianize"};
int normalization_styles[] = {
  NORMALIZATION_NONE, NORMALIZATION_MINMAX,
  NORMALIZATION_ZEROMAX, NORMALIZATION_MAXABS, 
  NORMALIZATION_TRIM_1E2, NORMALIZATION_TRIM_1E3, 
  NORMALIZATION_THREESIGMA, NORMALIZATION_LOG10,
  NORMALIZATION_SQUASH, NORMALIZATION_RANK, 
  NORMALIZATION_GAUSSIANIZE};
const int n_normalization_styles = 
  sizeof( normalization_styles) / 
  sizeof( normalization_styles[0]);
Fl_Menu_Item 
  normalization_style_menu_items[ n_normalization_styles+1];

//*****************************************************************
// invert_selection() -- Invert color of selected points.  Could
// this become a static member function of class plot_window?
void invert_selection ()
{
  selected( blitz::Range(0,npoints-1)) = 
    !selected( blitz::Range(0,npoints-1));
  nselected = npoints-nselected;
  pws[ 0]->color_array_from_selection();
  redraw_all_plots( 0);
}

//*****************************************************************
// toggle_display_selection( *o) -- Toggle colors of selected and 
// non-selected points.  Could this become a static member 
// function of class plot_window?
void toggle_display_delected( Fl_Widget *o)
{
  // Toggle the value of the button manually, but only if we were 
  // called via a keypress in a plot window
  // Shouldn't there be an easier way?
  if( o == NULL)
    show_deselected_button->value( 
      1 - show_deselected_button->value());

  // Creon notes that something wrong here....
  redraw_all_plots (0);
}

//*****************************************************************
// clear_selection( *o) -- Clear selection.  Could this become a 
// static member function of class plot_window?
void clear_selection( Fl_Widget *o)
{
  // Loop: Loop through all the plots
  for( int i=0; i<nplots; i++) {
    pws[i]->reset_selection_box ();
  }
  newly_selected = 0;
  selected = 0;
  previously_selected = 0;
  nselected = 0;
  pws[0]->color_array_from_selection (); // So, I'm lazy.
  redraw_all_plots (0);
}

//*****************************************************************
// choose_color_selected() -- Choose color of selected points.
// Could this become a static member function of plot_window?
void plot_window::choose_color_selected()
{
  (void) fl_color_chooser(
    "selected", r_selected, g_selected, b_selected);
  update_textures();
}

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
// delete_deselected( *o) -- Delete selection.  Could this become 
// a static member function of class plot_window?
void delete_selection( Fl_Widget *o)
{
  blitz::Range NVARS(0,nvars-1);
  int ipoint=0;
  for( int n=0; n<npoints; n++) {
    if( !selected(n)) {
      points(NVARS,ipoint) = points(NVARS,n);
      ipoint++;
    }
  }

  // If some point(s) got deleted, everyone's ranking needs to 
  // be recomputed
  if( ipoint != npoints)  {
    ranked = 0;	

    npoints = ipoint;
    npoints_slider->bounds(1,npoints);
    npoints_slider->value(npoints);

    clear_selection( (Fl_Widget *)NULL);
	
    for( int j=0; j<nplots; j++) {
      cps[j]->extract_and_redraw();
    }
  }
}

//*****************************************************************
// upper_triangle_incr( i, j, nvars) -- Increment row index i and 
// column index j "down and to the right" in an upper triangular 
// matrix, with wrapping;
void upper_triangle_incr( int &i, int &j, const int nvars)
{
  static int offset = 1;
  i++;
  j++;
  if( j > nvars-1) {
    i = 0;
    offset++;
    j = i+offset;
  }

  // Start all over if we need to.
  if( j > nvars-1) {
    i = 0;
    j = 1;
    offset = 1;
  }
  assert( i >= 0);
  assert( j > 0);
  assert( i < nvars-1);
  assert( j < nvars);
}

//*****************************************************************
// plot_window::change_axes( nchange) -- Change axes
void plot_window::change_axes( int nchange)
{
  // this seems a little verbose.....
  int i=cp->varindex1->value();
  int j=cp->varindex2->value();
  for( int k=0; k<nchange; k++)
    upper_triangle_incr( i, j, nvars);
  cp->varindex1->value(i);
  cp->varindex2->value(j);
  cp->extract_and_redraw();
}

//*****************************************************************
// change_all_axes( *o) -- Invoke cthe change_axes method of each
// plot_window to change all unlocked axes.
void change_all_axes( Fl_Widget *o) {
  int nchange = 0;
  for( int i=0; i<nplots; i++) {
    if( !cps[i]->lock_axes_button->value()) nchange++;
  }
  for( int i=0; i<nplots; i++) {
    if (!cps[i]->lock_axes_button->value())
      pws[i]->change_axes (nchange);
  }
}


//*****************************************************************
// plot_window::update_linked_transforms() -- Use current plot's 
// scale and offset to update all the others that show (any of) 
// the same axes (using the same normalization).
void plot_window::update_linked_transforms()
{
  if( !link_all_axes_button->value()) return;

  // get this plot's axis indices and normalization styles
  int axis1=cp->varindex1->value(); 
  int style1 = cp->x_normalization_style->value();
  int axis2=cp->varindex2->value(); 
  int style2 = cp->y_normalization_style->value();

  // Loop: Find other plot windows that have any of the same axis 
  // indices active and the same normalization style, update the 
  // appropriate translation and scale values for them.
  for( int i=0; i<nplots; i++) {
    plot_window *p = pws[i];

    // don't need to update ourself
    if( p == this) continue; 

    // individual plots may override this "feature"
    if( p->cp->lock_axes_button->value()) continue;

    // Finally, figure out what me may want to change and how
	if( p->cp->varindex1->value() == axis1 && 
        p->cp->x_normalization_style->value() == style1) {
      p->xscale = xscale; 
      p->xcenter = xcenter;
      p->needs_redraw = 1;
    }
    else if( p->cp->varindex1->value() == axis2 && 
             p->cp->x_normalization_style->value() == style2) {
      p->xscale = yscale; 
      p->xcenter = ycenter;
      p->needs_redraw = 1;
    }

    if( p->cp->varindex2->value() == axis1 && 
        p->cp->y_normalization_style->value() == style1) {
      p->yscale = xscale; 
      p->ycenter = xcenter;
      p->needs_redraw = 1;
    }
    else if( p->cp->varindex2->value() == axis2 && 
             p->cp->y_normalization_style->value() == style2) {
      p->yscale = yscale; 
      p->ycenter = ycenter;
      p->needs_redraw = 1;
    }

    // This is needed to make sure the scale marks on the axis are 
    // updated
    p->screen_to_world(-1, -1, p->wmin[0], p->wmin[1]);
    p->screen_to_world(+1, +1, p->wmax[0], p->wmax[1]);
  }
}

//*****************************************************************
// plot_window::handle( event) -- Main event handler.
int plot_window::handle( int event)
{
  // Current plot window (getting mouse drags, etc) must get 
  // redrawn before others so that selections get colored 
  // correctly.  Ugh.
  switch(event) {
    case FL_PUSH:
      DEBUG(cout << "FL_PUSH at " << xprev << ", " << yprev << endl);

      // Show the control panel associated with this plot window.
      cpt->value(cps[this->index]);	
      xprev = Fl::event_x();
      yprev = Fl::event_y();

      if( (Fl::event_state() == FL_BUTTON2) || 
          (Fl::event_state() == (FL_BUTTON1 | FL_CTRL))) {

      // XXX wish this worked
      #if 0
        xzoomcenter = (float)xprev;
        xzoomcenter = + (2.0*(xzoomcenter/(float)w()) -1.0) ; // window -> [-1,1]
			
        yzoomcenter = (float)yprev;
        yzoomcenter = - (2.0*(yzoomcenter/(float)h()) -1.0) ; // window -> [-1,1]
      #endif
      }

      // left button down = start new selection
      if( Fl::event_state() & FL_BUTTON1) {
        static int previous_window, current_window = -1;
        previous_window = current_window;
        current_window = index;
        if( current_window != previous_window)
          previously_selected( blitz::Range(0,npoints-1)) = 
            selected( blitz::Range(0,npoints-1));
			
        // not moving or extending old selection
        if(! (Fl::event_key(FL_Shift_L) || Fl::event_key(FL_Shift_R))) {
          extend_selection = 0;

          xdown = (float)xprev;
          xdown = + (2.0*(xdown/(float)w()) -1.0) ; // window -> [-1,1]
          xdown = xdown / xscale;
          xdown = xdown + xcenter;
			
          ydown = (float)yprev;
          ydown = - (2.0*(ydown/(float)h()) -1.0) ; // window -> [-1,1]
          ydown = ydown/yscale;
          ydown = ydown + ycenter;
        }
        else {
          // previously_selected(blitz::Range(0,npoints-1)) = 0;
        }
      }

      // start translating
      if( Fl::event_state(FL_BUTTON3) || 
          ( Fl::event_state(FL_BUTTON1) && Fl::event_state(FL_ALT))) {
        show_center_glyph = 1;
        needs_redraw = 1;
      }
      return 1;

    case FL_DRAG:
      DEBUG (printf ("FL_DRAG, event_state: %x\n", Fl::event_state()));
      xcur = Fl::event_x();
      ycur = Fl::event_y();

      xdragged = xcur - xprev;
      ydragged = -(ycur - yprev);
      xprev = xcur;
      yprev = ycur;

      // translate = drag with right mouse (or alt-left-mouse)
      if( Fl::event_state(FL_BUTTON3) || 
          (Fl::event_state(FL_BUTTON1) && Fl::event_state(FL_ALT))) {
        float xmove = xdragged*(1/xscale)*(2.0/w());
        float ymove = ydragged*(1/yscale)*(2.0/h());
        xcenter -= xmove;
        ycenter -= ymove;
        DEBUG ( cout << "translating (xcenter, ycenter) = (" << xcenter << ", " << ycenter << ")" << endl);
        // redraw ();
        show_center_glyph = 1;
        needs_redraw = 1;
        update_linked_transforms ();
      }

      // scale = drag with middle-mouse (or c-left-mouse)
      else if( Fl::event_state(FL_BUTTON2) || 
               (Fl::event_state(FL_BUTTON1) && Fl::event_state(FL_CTRL))) {
		if( scale_histogram) {
          xhscale *= 1 + xdragged*(2.0/w());
          yhscale *= 1 + ydragged*(2.0/h());
        } 
        else {
          xscale *= 1 + xdragged*(2.0/w());
          yscale *= 1 + ydragged*(2.0/h());
          zscale *= 1 + 0.5 * (xdragged*(2.0/w()) + ydragged*(2.0/h()));  // XXX hack.
          DEBUG ( cout << "scaling (xscale, yscale) = (" << xscale << ", " << yscale << ")" << endl);
        }
        // redraw();
        needs_redraw = 1;
        update_linked_transforms ();
      }

      // continue selection = drag with left mouse
      else if( Fl::event_state(FL_BUTTON1)) {
        // right key down = move selection
        // left shift down = extend selection (bug on OSX - no left key events)
        if( Fl::event_key(FL_Shift_L) || Fl::event_key(FL_Shift_R)) {
          xdown += xdragged*(1/xscale)*(2.0/w());
          ydown += ydragged*(1/yscale)*(2.0/h());
          xtracked += xdragged*(1/xscale)*(2.0/w());
          ytracked += ydragged*(1/yscale)*(2.0/h());
          if (Fl::event_key(FL_Shift_R)) {
            extend_selection = 0;
          }
          else {
            extend_selection = 1;
          }
        }
        else {
          xtracked = + (2.0*(xcur/(float)w()) -1.0) ; // window -> [-1,1]
          xtracked = xtracked / xscale;
          xtracked = xtracked + xcenter;
				
          ytracked = - (2.0*(ycur/(float)h()) -1.0) ; // window -> [-1,1]
          ytracked = ytracked/yscale;
          ytracked = ytracked + ycenter;
        }
        
        int isdrag = !Fl::event_is_click();
        // printf ("FL_DRAG & FL_BUTTON1, event_state: %x  isdrag = %d  xdragged=%f  ydragged=%f\n", Fl::event_state(), isdrag, xdragged, ydragged);
        if( isdrag==1 && ((abs(xdragged)+abs(ydragged))>=1)) {
          selection_changed = 1;
          redraw_all_plots (index);
        }
      }
      screen_to_world (-1, -1, wmin[0], wmin[1]);
      screen_to_world (+1, +1, wmax[0], wmax[1]);
      return 1;

    // Mouse up
    case FL_RELEASE:   
      DEBUG (cout << "FL_RELEASE at " << Fl::event_x() << ", " << Fl::event_y() << endl);
      // selection_changed = 0;
      if( show_center_glyph) {
        show_center_glyph = 0;
        needs_redraw = 1;
      }
      return 1;

    // keypress, key is in Fl::event_key(), ascii in 
    // Fl::event_text().  Return 1 if you understand/use the 
    // keyboard event, 0 otherwise...
    case FL_KEYDOWN:
      DEBUG ( cout << "FL_KEYDOWN, event_key() = " << Fl::event_key() << endl);
      switch( Fl::event_key()) {
		// XXX should figure out how to share shortcuts between plot windows and control panels... later

        case 'q':   // exit
        case '\027':  // quit
          exit( 0);

        case 'x':   // delete slected points from all future processing
        case FL_Delete:
          delete_selection ((Fl_Widget *)NULL);
          return 1;

        case 'i':   // invert or restore (uninvert) selection
          invert_selection ();
          return 1;

        case 'c':   // clear selection
          clear_selection ((Fl_Widget *)NULL);
          return 1;
          
        case 'd': // don't display / display deselected dots
          toggle_display_delected ((Fl_Widget *)NULL);
          return 1;

        case 'r':
          extract_data_points ();
          //redraw();
          return 1;

        case 'h':
          scale_histogram=1;
          return 1;

        default:
          return 0;
      }

    case FL_KEYUP:
      DEBUG ( cout << "FL_KEYUP" << endl);
      switch( Fl::event_key()) {
        case 'h':
          scale_histogram=0;
          return 1;

        default:
          return 0;
      }

    // Shortcut, key is in Fl::event_key(), ascii in 
    // Fl::event_text().  Return 1 if you understand/use the 
    // shortcut event, 0 otherwise...
    case FL_SHORTCUT:
      return 0;

    // Pass other events to the base class...
    default:
      return Fl_Gl_Window::handle( event);}
} 


//*****************************************************************
// plot_window::reset_selection_box() -- Reset selection box.
void plot_window::reset_selection_box()
{
  xdragged = ydragged = 0.0;
  xzoomcenter = yzoomcenter = zzoomcenter = 0.0;
  xdown = ydown = xtracked = ytracked = 0.0;
  xprev = yprev = xcur = ycur = 0;
}

//*****************************************************************
// plot_window::reset_view() -- Reset pan, zoom, and angle.
void plot_window::reset_view()
{
  int axis2 = (int)(cp->varindex3->mvalue()->user_data());

  xscale = 2.0 / (wmax[0]-wmin[0]);
  yscale = 2.0 / (wmax[1]-wmin[1]);
  if (axis2 != nvars)
    zscale = 2.0 / (wmax[2]-wmin[2]);
  else
    zscale = 1.0;
	
  // Initiallly, datapoints only span 0.8 of the window dimensions, 
  // which allows room around the edges for labels, tickmarks, 
  // histograms....
  xscale *= initial_pscale; 
  yscale *= initial_pscale; 
  zscale *= initial_pscale; 

  xcenter = (wmin[0]+wmax[0]) / 2.0;
  ycenter = (wmin[1]+wmax[1]) / 2.0;
  if( axis2 != nvars)
    zcenter = (wmin[2]+wmax[2]) / 2.0;
  else
    zcenter = 0.0;

  xhscale = 1.0;
  yhscale = 1.0;

  angle = 0.0;
  cp->spin->value(0);
  cp->rot_slider->value(0.0);
  cp->dont_clear->value(0);

  reset_selection_box ();
  if( count ==1) {
    // color_array_from_selection (); // HUH????
  }
  needs_redraw = 1;
}

//*****************************************************************
// plot_window::draw() -- Main draw method that calls others.
void plot_window::draw() 
{
  DEBUG (cout << "in draw: " << xcenter << " " << ycenter << " " << xscale << " " << yscale << endl);

  // the valid() property can avoid reinitializing matrix for 
  // each redraw:
  if( !valid()) {
    valid(1);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, -MAXFLOAT, MAXFLOAT);
    glViewport(0, 0, w(), h());
    glDisable(GL_LIGHTING);
    // glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    // this next idiom is necessary, per window, to map GL_SHORT 
    // texture coordinate values to [0..1] for texturing.
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();	
    glScalef( 
      1.0/(float)maxplots, 
      1.0/(float)maxplots, 
      1.0/(float)maxplots); 
    glMatrixMode(GL_MODELVIEW);

    #ifdef FAST_APPLE_VERTEX_EXTENSIONS
      glEnableClientState(GL_VERTEX_ARRAY_RANGE_APPLE);
    #endif // FAST_APPLE_VERTEX_EXTENSIONS

  }
  
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef( 
    xzoomcenter*xscale, 
    yzoomcenter*yscale, 
    zzoomcenter*zscale);
  if( cp->spin->value())
    angle += cp->rot_slider->value()/100.0;
  else
    angle = cp->rot_slider->value();
  glRotatef(angle, 0.0, 1.0, 0.1);
  glScalef (xscale, yscale, zscale);
  glTranslatef (-xcenter, -ycenter, -zcenter);
  glTranslatef (-xzoomcenter, -yzoomcenter, -zzoomcenter);

  if( cp->dont_clear->value() == 0) {
    // glClearColor(0.0,0.0,0.0,0.0);
    glClearColor(
      cp->Bkg->value(), cp->Bkg->value(), cp->Bkg->value(), 0.0);
    glClearDepth(0);
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    draw_grid();
  }

  if( selection_changed) {
    handle_selection ();
  }
  draw_data_points();
  draw_center_glyph();
  draw_histograms ();
  draw_axes();
}

//*****************************************************************
// control_panel_window::maybe_draw() -- Check plot windows to see
// if they need to be redrwan.
void control_panel_window::maybe_redraw() 
{
  // kludge.  Avoid double redraw when setting "don't clear".
  if( dont_clear->value()) return;

  //pw->redraw();
  pw->needs_redraw = 1;
}

//*****************************************************************
// plot_window::draw_grid() -- Draw a grid.
void plot_window::draw_grid()
{
  glBlendFunc(GL_ONE, GL_ZERO);
  // glBlendFunc(sfactor, dfactor);
  // glEnable(GL_LINE_SMOOTH);
  // glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
  // glLineWidth(0.5);
  glLineWidth(1.0);
  if( cp->show_grid->value()) {
    if( cp->Bkg->value() <= 0.2)
      glColor4f(0.2,0.2,0.2,0.0);
    else
      glColor4f( 
        0.8*cp->Bkg->value(), 0.8*cp->Bkg->value(), 
        0.8*cp->Bkg->value(), 0.0);

    // draw the grid here
    glBegin( GL_LINES);
    for( int k=-1; k<=1; k+=2) {
      for( int i=1; i<=10; i++) {

        // XY plane
        glVertex3f (-1.0, 0.1*i*k, 0.0); 
        glVertex3f (+1.0, 0.1*i*k, 0.0);
        glVertex3f (0.1*i*k, -1.0, 0.0); 
        glVertex3f (0.1*i*k, +1.0, 0.0);

        // YZ plane
        glVertex3f (0.0, -1.0, 0.1*i*k); 
        glVertex3f (0.0, +1.0, 0.1*i*k);
        glVertex3f (0.0, 0.1*i*k, -1.0); 
        glVertex3f (0.0, 0.1*i*k, +1.0);

        // XZ plane
        glVertex3f (-1.0, 0.0, 0.1*i*k); 
        glVertex3f (+1.0, 0.0, 0.1*i*k);
        glVertex3f (0.1*i*k, 0.0, -1.0); 
        glVertex3f (0.1*i*k, 0.0, +1.0);
      }
    }
    glEnd();
  }
}

//*****************************************************************
// plot_window::screen_to_world( xscreen, yscreen, xworld, yworld)
// -- Convert from screen to world co-ordinates?
void plot_window::screen_to_world( 
  float xscreen, float yscreen, float &xworld, float &yworld)
{
  // cout << "screen_to_world" << endl;
  // cout << "  before" << xscreen << " " << yscreen
  //      << " " << xworld << " " << yworld << endl;
  xworld = ( xscreen*initial_pscale / xscale) + xcenter;
  yworld = ( yscreen*initial_pscale / yscale) + ycenter;
  //cout << "  after " << xscreen << " " << yscreen
  //     << " " << xworld << " " << yworld << endl;
}

//*****************************************************************
// plot_window::draw_axes() -- If requested, draw and label the
// axes
void plot_window::draw_axes ()
{
  // If requested draw axes
  if( cp->show_axes->value()) {
    glPushMatrix();
    glLoadIdentity();

    // Define the extra (relative) distance that axes extend past 
    // leftmost and rightmost tickmarks and set initial pscale
    float a = 0.1; 
    float c = initial_pscale;
    glScalef( c, c, c);

    gl_font( FL_HELVETICA, 10);
    glBlendFunc( GL_ONE, GL_ZERO);
    if( cp->Bkg->value() <= 0.4)
      glColor4f( 0.7,0.7,0.0,0.0);
    else
      glColor4f( 
        0.4*cp->Bkg->value(), 
        0.4*cp->Bkg->value(), 
        0.0*cp->Bkg->value(), 0.0);

    // Draw the axes
    glBegin( GL_LINES);

    // X data
    glVertex3f( -(1+a), -(1+a), -(1+a));
    glVertex3f( +(1+a), -(1+a), -(1+a));
    
    // Y data
    glVertex3f( -(1+a), -(1+a), -(1+a));
    glVertex3f (-(1+a), +(1+a), -(1+a));
    
    // Z axis
    glVertex3f( -(1+a), -(1+a), -(1+a));
    glVertex3f( -(1+a), -(1+a), +(1+a));

    glEnd();

    // Define a buffer used to lable tic marks and set the 
    // offset factor for tic mark length. b<1 -> inwards, 
    // b>1 -> outwards, b==1 -> no tick.
    char buf[ 1024];
    float b = 1.5;

    // If requested, draw tic marks to show scale	
    if( cp->show_scale->value()) {
      glBegin( GL_LINES);

       // lower X-axis tick
      glVertex3f( -1, -(1+a), -(1+a)); 
      glVertex3f( -1, -(1+b*a), -(1+a));

      // upper X-axis tick
      glVertex3f( +1, -(1+a), -(1+a)); 
      glVertex3f( +1, -(1+b*a), -(1+a));

      // lower Y-axis tick
      glVertex3f( -(1+a), -1, -(1+a)); 
      glVertex3f( -(1+b*a), -1, -(1+a)); 

      // upper Y-axis tick
      glVertex3f( -(1+a), +1, -(1+a)); 
      glVertex3f( -(1+b*a), +1, -(1+a)); 

      // XXX Z-axis ticks clutter 2D plots
      b = 1; 

      // lower Z-axis tick
      glVertex3f( -(1+a), -(1+a), -1); 
      glVertex3f( -(1+b*a), -(1+a), -1);

      // upper Z-axis tick
      glVertex3f (-(1+a), -(1+a), +1); 
      glVertex3f (-(1+b*a), -(1+a), +1); 

      glEnd();

      //  offset for scale values. b<1 -> inwards, 
      // b>1 -> outwards, b==1 -> on axis.
      b = 2;  

      // lower X-axis scale value
      snprintf( buf, sizeof(buf), "%+.3g", wmin[0]); 
      gl_draw( 
        (const char *)buf, 
        -1.0-gl_width((const char *)buf)/(w()), -(1+b*a));

      // upper X-axis scale value
      snprintf(buf, sizeof(buf), "%+.3g", wmax[0]); 
      gl_draw(
        (const char *)buf, 
        +1.0-gl_width((const char *)buf)/(w()), -(1+b*a));

      // This value of b is used for...?
      b = 2.4;

      // lower Y-axis scale value
      snprintf( buf, sizeof(buf), "%+.3g", wmin[1]);
      gl_draw( (const char *)buf, -(1+b*a), -1.0f+a/4);

      // upper Y-axis scale value
      snprintf( buf, sizeof(buf), "%+.3g", wmax[1]);
      gl_draw( (const char *)buf, -(1+b*a), +1.0f+a/4);
    }

    // If requested, draw tic mark labels
    if( cp->show_labels->value()) {

      // offset for axis labels values. b<1 -> inwards, 
      // b>1 -> outwards, b==1 -> on axis.
      b = 2; 

      float wid = gl_width(xlabel.c_str())/(float)(w());
      gl_draw( (const char *)(xlabel.c_str()), -wid, -(1+b*a));	

      b = 1.5;
      gl_draw( (const char *)(ylabel.c_str()), -(1+b*a), 1+b*a);
    }

    glPopMatrix();
  }
}

//*****************************************************************
// plot_window::draw_center_glyph() -- Draw a glyph in the center
// of a selected region?)
void plot_window::draw_center_glyph ()
{
  if( !show_center_glyph) return;

  glDisable( GL_DEPTH_TEST);
  glEnable( GL_COLOR_LOGIC_OP);
  glLogicOp( GL_INVERT);
	
  glPushMatrix ();
  glLoadIdentity();
  glBlendFunc( GL_ONE, GL_ZERO);
  glBegin( GL_LINES);

  glColor4f(0.7,0.7,0.7,0.0);

  glVertex3f( -0.025, 0.0, 0.0); 
  glVertex3f( 0.025, 0.0, 0.0);

  glVertex3f( 0.0, -0.025, 0.0); 
  glVertex3f( 0.0, 0.025, 0.0);

  glEnd();
  glPopMatrix();
  glDisable( GL_COLOR_LOGIC_OP);
}

//*****************************************************************
// plot_window::print_selection_stats() -- Write statistics for
// selection (to screen?)
void plot_window::print_selection_stats ()
{
  glDisable( GL_DEPTH_TEST);
  glEnable( GL_COLOR_LOGIC_OP);
  glLogicOp( GL_INVERT);
  glPushMatrix();
  glLoadIdentity ();
  gl_font( FL_COURIER, 10);
  glBlendFunc( GL_ONE, GL_ZERO);
  glColor4f( 0.7,0.7,0.7,0.0);

  // Define character buffer to allocate storage and print
  // message to screen  
  char buf[ 1024];
  snprintf( 
    buf, sizeof(buf), "%8d/%d (%5.2f%%) selected", 
    nselected, npoints, 100.0*nselected/(float)npoints);
  gl_draw( (const char *)buf, 0.0f, 0.9f);

  glPopMatrix();
  glDisable( GL_COLOR_LOGIC_OP);
}

//*****************************************************************
// plot_window::handle_selection() -- Handler to handle selection
// operations.
void plot_window::handle_selection ()
{
  int draw_selection_box = 1;
  if( draw_selection_box) {
    glBlendFunc( GL_ONE, GL_ZERO);
    glLineWidth( 1.0);
    glColor4f( 0.25,0.25,0.75,0.0);
    glBegin( GL_LINE_LOOP);

    glVertex2f( xdown, ydown);
    glVertex2f( xtracked, ydown);
    glVertex2f( xtracked, ytracked);
    glVertex2f( xdown, ytracked);

    glEnd();
  }
  blitz::Range NPTS( 0, npoints-1);	

  // Identify newly-selected points
  newly_selected(NPTS) = where( 
    ( vertices( NPTS, 0)>fmaxf( xdown, xtracked) || 
      vertices( NPTS, 0)<fminf( xdown, xtracked) ||
      vertices( NPTS, 1)>fmaxf( ydown, ytracked) || 
      vertices( NPTS, 1)<fminf( ydown, ytracked)),
     0, index+1);

  // Add newly-selected points to existing or previous selection
  if( add_to_selection_button->value()) {
    selected( NPTS) = where( 
      newly_selected( NPTS), newly_selected( NPTS), 
      selected( NPTS));
  }
  else {
    selected(NPTS) = where(
      newly_selected(NPTS), newly_selected(NPTS),
      previously_selected(NPTS));
  }		

  // Determine and print selection statistics
  nselected = blitz::count(selected(NPTS)>0);
  print_selection_stats();
  color_array_from_new_selection ();

  // done flagging selection for this plot
  selection_changed = 0;
}

// Define parameters used for textures?	
float mincolor= 0.01, alpha1 = 1.0;

GLfloat texture_images[2][4*(maxplots)];

GLfloat pointscolor[4] = {1,1,1,1};
GLfloat texenvcolor[4] = {1,1,1,1};

GLuint texnames[2];

int textures_initialized = 0;

//*****************************************************************
// initialize_textures() -- Initialize textures.  This method
// requires a global static flag, textures_initialized, but it is
// only used by plot_window::color_array_from_selection.  Could it 
// become a member function of class plot_window?
void initialize_textures()
{
  if( textures_initialized) return;

  glPixelStorei( GL_UNPACK_ALIGNMENT, 1);
  glGenTextures( 2, texnames);

  // Color for de-selected points when they are displayed
  texture_images[0][0] = r_deselected;
  texture_images[0][1] = g_deselected;
  texture_images[0][2] = b_deselected;
  texture_images[0][3] = 1.0; 

  // Color for de-selected points when they are not displayed
  texture_images[1][0] = 0.00; 
  texture_images[1][1] = 0.00; 
  texture_images[1][2] = 0.00; 
  texture_images[1][3] = 0.0;

  // Loop: Set color(s) for selected points.  Remember that 
  // the 0th color is reserved for deselected points.
  for( int i=0; i<nplots; i++) {
    int j=4*(i+1);  

    // Initial colors of selected points
    texture_images[0][j+0] = 
      texture_images[1][j+0] = pws[0]->r_selected;  // need to fix this.
    texture_images[0][j+1] = 
      texture_images[1][j+1] = pws[0]->g_selected;
    texture_images[0][j+2] = 
      texture_images[1][j+2] = pws[0]->b_selected;
    texture_images[0][j+3] = 
      texture_images[1][j+3] = 1.0; 
  }

  // Loop: Set textures?
  for( unsigned int i=0; 
       i<sizeof(texnames)/sizeof(texnames[0]); i++) {
    glBindTexture( GL_TEXTURE_1D, texnames[i]);
    glTexParameteri( 
      GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(
      GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(
      GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexImage1D( 
      GL_TEXTURE_1D, 0, GL_RGBA8, maxplots, 0, GL_RGBA, 
      GL_FLOAT, texture_images[i]);
  }
  
  // Set flag to indicate that textures have been initialized
  textures_initialized = 1;
}

//*****************************************************************
// plot_window::update_textures() -- Update textures.
void plot_window::update_textures ()
{
  // New color for selected points (selection in this window only)
  int j = 4*(index+1);
  texture_images[0][j+0] = texture_images[1][j+0] = r_selected;
  texture_images[0][j+1] = texture_images[1][j+1] = g_selected;
  texture_images[0][j+2] = texture_images[1][j+2] = b_selected;
  texture_images[0][j+3] = texture_images[1][j+3] = 1.0; 

  // color for de-selected points when they are displayed
  texture_images[0][0] = r_deselected;
  texture_images[0][1] = g_deselected;
  texture_images[0][2] = b_deselected;
  texture_images[0][3] = 1.0; 

  // Loop: Set textures for selected points?
  for( unsigned int i=0; 
       i<sizeof(texnames)/sizeof(texnames[0]); i++) {
    glBindTexture( GL_TEXTURE_1D, texnames[i]);
    glTexImage1D(
      GL_TEXTURE_1D, 0, GL_RGBA8, maxplots, 0, GL_RGBA, 
      GL_FLOAT, texture_images[i]);
  }
}

//*****************************************************************
// plot_window::color_array_from_selection() -- Color selected
// points?  NOTE: The fact that this calls initialize_textures
// complicates the class organisation.
void plot_window::color_array_from_selection()
{
  initialize_textures();
  update_textures();

  blitz::Range NPTS( 0, npoints-1);	

  if (dont_paint_button->value()) {
  }

  texture_coords(NPTS) = selected(NPTS);
}

//*****************************************************************
// plot_window::color_array_from_new_selection() -- Invoke
// color_array_from_selection to color new selection.
void plot_window::color_array_from_new_selection()
{
  color_array_from_selection ();
}

//*****************************************************************
// clearAlphaPlanes() -- Those filthy alpha planes!  It seems that
// no matter how hard you try, you just can't keep them clean!
void clearAlphaPlanes()
{
  glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
  glClearColor (0.0, 0.0, 0.0, 0.0);
  glClear (GL_COLOR_BUFFER_BIT);
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

//*****************************************************************
// plot_window::draw_data_points() -- If requested, draw the data
void plot_window::draw_data_points()
{
  // cout << "pw[" << index << "]: draw_data_points() " << endl;
  if ( !cp->show_points->value())return;

  glDisable( GL_DEPTH_TEST);

  glEnable(GL_TEXTURE_1D);
  glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  // for testing
  // glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE); 
  glColor4fv(pointscolor);
  // GL_MODULATE ignores this
  glTexEnvfv( GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, texenvcolor); 
  
  glPointSize( cp->pointsize_slider->value());

  float const_color[4];

  const_color[0] = 
    const_color[1] = const_color[2] = cp->Lum->value(); 
  const_color[3] = cp->Alph->value();
  glBlendColor(
    const_color[0], const_color[1], 
    const_color[2], const_color[3]);

  glBlendFunc( sfactor, dfactor);

  GLshort *texturep = (GLshort *)texture_coords.data();
  glTexCoordPointer (1, GL_SHORT, 0, texturep);

  // Tell the GPU where to find the correct texture coordinate 
  // (colors) for each vertex.
  int tmp_alpha_test = 0;

  // XXX need to resolve local/global controls issue
  if( show_deselected_button->value() && 
      cp->show_deselected_points->value()) {
    glBindTexture( GL_TEXTURE_1D, texnames[0]);
  }
  else {
    glBindTexture( GL_TEXTURE_1D, texnames[1]);
    
    // Cull any deselected points (alpha==0.0), whatever the 
    // blendfunc:
    glEnable( GL_ALPHA_TEST);
    glAlphaFunc( GL_GEQUAL, 0.5);  
    tmp_alpha_test = 1;
  }

  // Tell the GPU where to find the vertices;
  GLfloat *vertexp = (GLfloat *)vertices.data();

  // Are we plotting in two dimensions or three?  axis3 == 
  // "-nothing-" means 2D plotting
  if( cp->varindex3->value() == nvars) 
    glVertexPointer (2, GL_FLOAT, 3*sizeof(GL_FLOAT), vertexp); 
  else
    glVertexPointer (3, GL_FLOAT, 0, vertexp); 

  #ifdef FAST_APPLE_VERTEX_EXTENSIONS
    // for static data
    glVertexArrayParameteriAPPLE(
      GL_VERTEX_ARRAY_STORAGE_HINT_APPLE, 
      GL_STORAGE_CACHED_APPLE);  

     // for dynamic data
    //  glVertexArrayParameteriAPPLE( 
    //    GL_VERTEX_ARRAY_STORAGE_HINT_APPLE, 
    // GL_STORAGE_SHARED_APPLE); 

   glVertexArrayRangeAPPLE(
     3*npoints*sizeof(GLfloat),(GLvoid *)vertexp);
  #endif // FAST_APPLE_VERTEX_EXTENSIONS

  // tell the GPU to draw the vertices.
  glDrawArrays( GL_POINTS, 0, npoints);

  if( tmp_alpha_test == 1 ) glDisable(GL_ALPHA_TEST);
  glDisable( GL_TEXTURE_1D);
}

//*****************************************************************
// plot_window::compute_histogram( axis) -- If requested, compute 
// histograms for axis 'axis'.
void plot_window::compute_histogram( int axis)
{
  if( !(cp->show_histogram->value())) return;

  nbins = (int)(cp->nbins_slider->value());
  // cout << "nbins = " << nbins << endl;
  blitz::Range BINS(0,nbins-1);
  counts(BINS,axis) = 0.0;
  counts_selected(BINS,axis) = 0.0;
  float range = amax[axis]-amin[axis];

  // Loop: Sum over all data points
  for( int i=0; i<npoints; i++) {
    float x = vertices(i,axis);
    int bin=(int)(nbins*((x-amin[axis])/range));
    if( bin < 0) bin = 0;
    if( bin >= nbins) bin=nbins-1;
    counts(bin,axis)++;
    if( selected(i)) counts_selected(bin,axis)++;
  }
  
  // Normalize results.  NOTE: This will die horribly if there
  // is no data
  counts( BINS, axis) = 
    (5.0*nbins/(float)nbins_default)*counts(BINS,axis)/
    ((float)(npoints));
  counts_selected(BINS,axis) = 
    (5.0*nbins/(float)nbins_default)*counts_selected(BINS,axis)/
    ((float)(npoints));
}

//*****************************************************************
// plot_window::compute_histogram() -- Invoke compute_histogram to
// compute histograms for axes 0 and 1.
void plot_window::compute_histograms ()
{
  compute_histogram(0);
  compute_histogram(1);
}

//*****************************************************************
// plot_window::draw_histogram() -- If requested, draw histograms.
void plot_window::draw_histograms()
{
  if( !(cp->show_histogram->value())) return;

  // histograms base is this far from edge of window
  float hoffset = 0.01; 

  glPushMatrix();

  // x-axis histograms
  glLoadIdentity();
  glTranslatef( xzoomcenter*xscale, 0.0, 0);
  glScalef( xscale, yhscale, 1.0);
  glTranslatef( -xcenter, -1.0/yhscale, 0.0);
  glTranslatef( -xzoomcenter, 0.0, 0);
  glTranslatef( 0, hoffset, 0);

  // histograms cover pointclouds
  glTranslatef (0.0, 0.0, 0.1);
  float xwidth = (amax[0]-amin[0]) / (float)(nbins);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Loop: Draw x-axis histogram (all points)
  float x = amin[0];
  glColor4f (0.0, 1.0, 0.0, 0.5);
  glBegin(GL_LINE_STRIP);
  glVertex2f(x,0.0);
  for( int bin=0; bin<nbins; bin++) {
    glVertex2f(x,counts(bin,0));   // left edge
	glVertex2f(x+xwidth,counts(bin,0));	  // Top edge
    //glVertex2f(x+xwidth,0.0);   // Right edge
    x+=xwidth;
  }
  glVertex2f(x,0.0);					
  glEnd();

  // If points were selected, refactor selcted points ofthe
  // x-axis histogram?
  if( nselected > 0) {
    x = amin[0];
    glColor4f (0.25, 1.0, 0.25, 1.0);
    glBegin(GL_LINE_STRIP);
    glVertex2f(x,0.0);					
    for( int bin=0; bin<nbins; bin++) {
      glVertex2f(x,counts_selected(bin,0));   // left edge
      glVertex2f(x+xwidth,counts_selected(bin,0));   // top edge
      // glVertex2f(x+xwidth,0.0);   // right edge 
      x+=xwidth;
    }
    glVertex2f(x,0.0);					
    glEnd();
  }
	
  // y-axis histograms
  glLoadIdentity();
  glTranslatef( 0.0, yzoomcenter*yscale, 0);
  glScalef( xhscale, yscale, 1.0);
  glTranslatef( -1.0/xhscale, -ycenter, 0.0);
  glTranslatef( 0.0, -yzoomcenter, 0);
  glTranslatef( hoffset, 0, 0);
  float ywidth = (amax[1]-amin[1]) / (float)(nbins);

  // Loop: draw y-axis histogram (all points)
  float y = amin[1];
  glColor4f( 0.0, 1.0, 0.0, 0.5);
  glBegin( GL_LINE_STRIP);
  glVertex2f( 0.0, y);					
  for( int bin=0; bin<nbins; bin++) {
    glVertex2f(counts(bin,1),y);   // bottom
    glVertex2f(counts(bin,1), y+ywidth);   // right edge
    // glVertex2f(0.0, y+ywidth);   // top edge 
    y+=ywidth;
  }
  glVertex2f(0.0,y);					
  glEnd();

  // If points were selected, refactor selcted points of the
  // y-axis histogram?
  if( nselected > 0) {
    y = amin[1];
    glColor4f( 0.25, 1.0, 0.25, 1.0);
    glBegin( GL_LINE_STRIP);
    glVertex2f( 0.0, y);
    for( int bin=0; bin<nbins; bin++) {
      glVertex2f(counts_selected(bin,1),y);   // bottom
      glVertex2f(counts_selected(bin,1), y+ywidth);   // right edge
      // glVertex2f(0.0, y+ywidth);   // top edge 
      y+=ywidth;
    }
    glVertex2f(0.0,y);					
    glEnd();
  }

  glPopMatrix();
}

//*****************************************************************
// plot_window::transform_2d() -- If requested...
int plot_window::transform_2d()
{
  if( cp->no_transform->value()) return 1;  // no transform
	
  blitz::Range NPTS(0,npoints-1);

  blitz::Array <float,1> tmp1(npoints), tmp2(npoints);
  tmp1 = vertices(NPTS,0);
  tmp2 = vertices(NPTS,1);

  if( cp->sum_vs_difference->value()) {
    vertices(NPTS,0) = (sqrt(2.0)/2.0) * (tmp1 + tmp2);
    vertices(NPTS,1) = (sqrt(2.0)/2.0) * (tmp1 - tmp2);
  }
  else if( cp->polar->value()) {
    vertices(NPTS,0) = atan2(tmp1, tmp2);
    vertices(NPTS,1) = sqrt(pow2(tmp1)+pow2(tmp2));
  }
  return 1;
}

//*****************************************************************
// plot_window::normalize( a, a_rank, style, axis_index) --  Apply
// normalization of the requested style.
int plot_window::normalize(
  blitz::Array<float,1> a, 
  blitz::Array<int,1> a_rank, 
  int style, int axis_index)
{
  blitz::Range NPTS(0,npoints-1);

  #ifdef CHECK_FOR_NANS_IN_NORMALIZATION
    blitz::Array<int,1> inrange(npoints);
    inrange = where(
      ((a(NPTS) < MAXFLOAT) && (a(NPTS) > -MAXFLOAT)), 1, 0);
    float tmin = min(where(inrange,a(NPTS), MAXFLOAT));
    float tmax = max(where(inrange,a(NPTS),-MAXFLOAT));
  #else // CHECK_FOR_NANS_IN_NORMALIZATION
    float tmin = a(a_rank(0));
    float tmax = a(a_rank(npoints-1));
    blitz::Array<float, 1> tmp(npoints);
  #endif // CHECK_FOR_NANS_IN_NORMALIZATION

  float mu,sigma;
  float pmin;

  switch( style) {
    case NORMALIZATION_NONE:
      wmin[axis_index] = -1;
      wmax[axis_index] = +1;
      return 1;

    case NORMALIZATION_MINMAX:
      wmin[axis_index] = tmin;
      wmax[axis_index] = tmax;
      return 1;

    // all positive data fits in window, zero at "left" of axis.
    case NORMALIZATION_ZEROMAX: 
      wmin[axis_index] = 0.0;
      wmax[axis_index] = tmax;
      return 1;

    // all data fits in window w/zero at center of axis
    case NORMALIZATION_MAXABS:  
      tmax = fmaxf(fabsf(tmin),fabsf(tmax));
      if( tmax != 0.0) {
        wmin[axis_index] = -tmax;
        wmax[axis_index] = tmax;
      }
      return 1;

    // median at center of axis, axis extends to include at 
    // least 99% of data
    case NORMALIZATION_TRIM_1E2:
      {
        float trim = 1e-2;
        wmin[axis_index] = 
          a(a_rank((int)((0.0 + (0.5*trim))*npoints)));
        wmax[axis_index] = 
          a(a_rank((int)((1.0 - (0.5*trim))*npoints)));
		return 1;
      }

    // median at center of axis, axis extends to include at 
    // least 99.9% of data
    case NORMALIZATION_TRIM_1E3:  
      {
        float trim = 1e-3;
        wmin[axis_index] = 
          a(a_rank((int)((0.0 + (0.5*trim))*npoints)));
        wmax[axis_index] = 
          a(a_rank((int)((1.0 - (0.5*trim))*npoints)));
        return 1;
      }

    // mean at center of axis, axis extends to +/- 3*sigma
    case NORMALIZATION_THREESIGMA:  
      mu = mean(a(NPTS));
      sigma = sqrt((1.0/(float)npoints)*sum(pow2(a(NPTS)-mu)));
      DEBUG (cout << "mu, sigma = " << mu << ", " << sigma << endl);
      if( finite(mu) && (sigma!=0.0)) {
        wmin[axis_index] = mu - 3*sigma;
        wmax[axis_index] = mu + 3*sigma;
      }
      return 1;

    // negative numbers get assigned a log of zero.
    case NORMALIZATION_LOG10: 
      if( tmin <= 0.0) {
        cerr << "Warning: "
             << "attempted to take logarithms of nonpositive "
             << " numbers. Those logs were set to zero." 
             << endl;
      }
      // find smallest positive element
      pmin = min( where( a(NPTS)>0, a(NPTS), MAXFLOAT));
      a(NPTS) = where( a(NPTS) > 0, log10(a(NPTS)), 0);
      wmin[axis_index] = log10(pmin);
      wmax[axis_index] = a(a_rank(npoints-1));
      return 1;

    // simple sigmoid, (-inf,0,+inf) -> (-1,0,+1)
    case NORMALIZATION_SQUASH: 
      a(NPTS) = a(NPTS)/(1+abs(a(NPTS)));
      wmin[axis_index] = a(a_rank(0));
      wmax[axis_index] = a(a_rank(npoints-1));
      return 1;

    // replace each item with its rank, normalized from 0 to 1
    case NORMALIZATION_RANK:
      for( int i=0; i<npoints; i++) {
        a( a_rank(i)) = float(i) / ((float)npoints-1);
      }
      wmin[axis_index] = 0;
      wmax[axis_index] = 1;
      return 1;
      
    // Gaussianize the data, with the cnter of the gaussian 
    // at the median.
    case NORMALIZATION_GAUSSIANIZE: 
      for( int i=0; i<npoints; i++) {
        a( a_rank(i)) = 
          (1.0/5.0) *
          (float)gsl_cdf_ugaussian_Pinv((double)(float(i+1) / 
          (float)(npoints+2)));
      }
      wmin[axis_index] = -1.0;
      wmax[axis_index] = +1.0;
      return 1;
    
    // Default: do nothing
    default:
    return 0;
  }
}

//*****************************************************************
// Class: myCompare
//
// Class definitions:
//   myCompare --
//
// Classes referenced: none
//
// Purpose: unknown
//
// Functions: unknown
//
// Author: Creon Levit    unknown
// Modified: P. R. Gazis   14-MAR-2006
//*****************************************************************
class myCompare
{
public:
	bool operator()(const int i, const int j)
		{  
			return tmp_points(i) < tmp_points(j);
			// return (tpoints[i]<tpoints[j]);
		}
};

//*****************************************************************
// plot_window::compute_rank() --
void plot_window::compute_rank(
  blitz::Array<float,1> a, 
  blitz::Array<int,1> a_rank, 
  int var_index)
{
  blitz::Range NPTS(0,npoints-1);
  if( !ranked(var_index)) {
    if( !a.isStorageContiguous()) {
      cerr << "Warning: sorting with non-contiguous data." 
           << endl;
    }
    if( !a_rank.isStorageContiguous()) {
      cerr << "Warning: sorting with non-contiguous rank." 
           << endl;
    }
    a_rank(NPTS) = identity(NPTS);
		
    tmp_points.reference(a);
    int *lo = a_rank.data(), *hi = lo + npoints;
    std::stable_sort(lo, hi, myCompare());

    ranked(var_index) = 1;  // now we are ranked
	ranked_points(var_index,NPTS) = a_rank(NPTS);  // and our rank is cached!
    cout << "  cache STORE at index " << var_index << endl;
  }
  else {
    a_rank=ranked_points(var_index,NPTS);// use previously cached rank!
    cout << "  CACHE HIT   at index " << var_index << endl;
  }
}

//*****************************************************************
// plot_window::extract_data_points() --
int plot_window::extract_data_points ()
{
  // get the labels for the plot's axes
  int axis0 = (int)(cp->varindex1->mvalue()->user_data());
  int axis1 = (int)(cp->varindex2->mvalue()->user_data());
  int axis2 = (int)(cp->varindex3->mvalue()->user_data());

  xlabel = column_labels[axis0];
  ylabel = column_labels[axis1];
  if( axis2 != nvars) zlabel = column_labels[axis2];
  else zlabel = "";
	
  blitz::Range NPTS(0,npoints-1);

  cout << "plot " << row << ", " << column << endl;
  cout << " pre-normalization: " << endl;

  compute_rank(points(axis0,NPTS),x_rank,axis0);
  cout << "  min: " << xlabel << "(" << x_rank(0) 
       << ") = " << points(axis0,x_rank(0));
  cout << "  max: " << xlabel 
       << "(" << x_rank(npoints-1) 
       << ") = " << points(axis0,x_rank(npoints-1)) << endl;
   
  compute_rank(points(axis1,NPTS),y_rank,axis1);
  cout << "  min: " << ylabel << "("  << y_rank(0) 
       << ") = " << points(axis1,y_rank(0));
  cout << "  max: " << ylabel << "(" << y_rank(npoints-1) 
       << ") = " << points(axis1,y_rank(npoints-1)) << endl;

  if( axis2 != nvars) {
    compute_rank( points(axis2,NPTS),z_rank,axis2);
    cout << "  min: " << zlabel << "(" << z_rank(0) 
         << ") = " << points(axis2,z_rank(0));
    cout << "  max: " << zlabel << "(" << z_rank(npoints-1) 
         << ") = " << points(axis2,z_rank(npoints-1)) << endl;
  }

  cout << " post-normalization: " << endl;

  // This copies the data...
  vertices(NPTS,0) = points(axis0,NPTS);  
  // ...but this doesn't. See blitz++ manual.
  blitz::Array<float,1> xpoints = vertices(NPTS,0); 
  
  vertices(NPTS,1) = points(axis1,NPTS);
  blitz::Array<float,1> ypoints = vertices(NPTS,1);

  if( axis2 != nvars)
    vertices(NPTS,2) = points(axis2,NPTS);
  else
    vertices(NPTS,2) = 0;

  blitz::Array<float,1> zpoints = vertices(NPTS,2);

  (void) normalize(
    xpoints, x_rank, cp->x_normalization_style->value(), 0);
  amin[0] = xpoints(x_rank(0));
  amax[0] = xpoints(x_rank(npoints-1));
  cout << "  min: " << xlabel << "(" << x_rank(0) 
       << ") = " << xpoints(x_rank(0));
  cout << "  max: " << xlabel << "(" << x_rank(npoints-1) 
       << ") = " << xpoints(x_rank(npoints-1)) << endl;
    
  (void) normalize(
    ypoints, y_rank, cp->y_normalization_style->value(), 1);
  amin[1] = ypoints(y_rank(0));
  amax[1] = ypoints(y_rank(npoints-1));
  cout << "  min: " << ylabel << "(" << y_rank(0)
       << ") = " << ypoints(y_rank(0));
  cout << "  max: " << ylabel << "(" << y_rank(npoints-1) 
       << ") = " << ypoints(y_rank(npoints-1)) << endl;

  if( axis2 != nvars) {
    (void) normalize(
      zpoints, z_rank, cp->z_normalization_style->value(), 2);
    amin[2] = zpoints(z_rank(0));
    amax[2] = zpoints(z_rank(npoints-1));
    cout << "  min: " << zlabel << "(" << z_rank(0)
         << ") = " << zpoints(z_rank(0));
    cout << "  max: " << zlabel << "(" << z_rank(npoints-1) 
         << ") = " << zpoints(z_rank(npoints-1)) << endl;
  } 
  else {
    amin[2] = -1.0;
    amax[2] = +1.0;
  }

  reset_view();
  (void) transform_2d();

  // XXX need to refactor this.  This is needed to make sure 
  // the scale marks on the axis are updated
  screen_to_world( -1, -1, wmin[0], wmin[1]);
  screen_to_world(+1, +1, wmax[0], wmax[1]);

  compute_histograms();
  return 1;
}

//*****************************************************************
// plot_window::extract_and_redraw() --
void control_panel_window::extract_and_redraw ()
{
  if( pw->extract_data_points()) {

    #ifdef FAST_APPLE_VERTEX_EXTENSIONS
      GLvoid *vertexp = (GLvoid *)pw->vertices.data();
      glFlushVertexArrayRangeAPPLE(
        3*npoints*sizeof(GLfloat), vertexp);
    #endif // FAST_APPLE_VERTEX_EXTENSIONS

    //pw->redraw ();
    pw->needs_redraw = 1;
  }
}

//*****************************************************************
// npoints_changed( 0) -- Where is this used?  Should it become a
// member function of class plot_window?
void npoints_changed( Fl_Widget *o) 
{
  npoints = int( ( (Fl_Slider *)o)->value());
  redraw_all_plots( 0);
}

//*****************************************************************
// write_data( o) -- Open and write a binary data file.  File will
// consist of an ASCII header with column names terminated by a
// newline, followed by a long blonck of binary data.  Could this
// become a member function of class control_pane_window of the
// hypthetical data class?
void write_data( Fl_Widget *o)
{
  char *output_file_name = 
    fl_file_chooser( 
      "write binary output to file", NULL, NULL, 0);

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
    cout << "Finished writing " << npoints
         << " rows with block_size " << nBlockSize << endl;
  }
}

//*****************************************************************
// control_panel_window::make_widgets( cpw) -- Make widgets
void control_panel_window::make_widgets( control_panel_window *cpw)
{
  // since these (virtual) control panels are really groups inside 
  // a tab inside a window, set their child widget's coordinates 
  // relative to their enclosing window's position.  (I think ;-)
  int xpos = this->x()+50;
  int ypos = this->y()+20;

  Fl_Button *b;

  pointsize_slider = 
    new Fl_Hor_Value_Slider_Input(
      xpos, ypos, cpw->w()-60, 20, "size");
  pointsize_slider->align(FL_ALIGN_LEFT);
  pointsize_slider->value(pointsize);
  pointsize_slider->step(0.25);
  pointsize_slider->bounds(0.1,20.0);
  pointsize_slider->callback((Fl_Callback*)replot, this);

  Bkg = 
    new Fl_Hor_Value_Slider_Input(
      xpos, ypos+=25, cpw->w()-60, 20, "Bkg");
  Bkg->align(FL_ALIGN_LEFT);
  Bkg->step(0.0001);
  Bkg->bounds(0.0,1.0);
  Bkg->callback((Fl_Callback*)replot, this);
  Bkg->value(0.0);

  Lum = 
    new Fl_Hor_Value_Slider_Input(
      xpos, ypos+=25, cpw->w()-60, 20, "Lum");
  Lum->align(FL_ALIGN_LEFT);
  Lum->callback((Fl_Callback*)replot, this);
  Lum->step(0.0001);
  Lum->bounds(0,1.0);
  Lum->value(1.0);

  Alph = 
    new Fl_Hor_Value_Slider_Input( 
      xpos, ypos+=25, cpw->w()-60, 20, "Alph");
  Alph->align(FL_ALIGN_LEFT);
  Alph->callback((Fl_Callback*)replot, this);
  Alph->step(0.0001);
  Alph->bounds(0.25,0.5);
  Alph->value(0.5);

  rot_slider = 
    new Fl_Hor_Value_Slider_Input( 
      xpos, ypos+=25, cpw->w()-60, 20, "rot");
  rot_slider->align(FL_ALIGN_LEFT);
  rot_slider->callback((Fl_Callback*)replot, this);
  rot_slider->value(0.0);
  rot_slider->step(0.001);
  rot_slider->bounds(-180.0, 180.0);

  nbins_slider = 
    new Fl_Hor_Value_Slider_Input(
      xpos, ypos+=25, cpw->w()-60, 20, "nbins");
  nbins_slider->align(FL_ALIGN_LEFT);
  nbins_slider->callback((Fl_Callback*)redraw_one_plot, this);
  nbins_slider->value(plot_window::nbins_default);
  nbins_slider->step(1);
  nbins_slider->bounds(2,plot_window::nbins_max);

  // dynamically build the variables menu
  // cout << "starting menu build, nvars = " << nvars << endl;
  for( int i=0; i<=nvars; i++) {
    // cout << "label " << i 
    //      << " = " << column_labels[i].c_str() << endl;
    varindex_menu_items[i].label(
      (const char *)(column_labels[i].c_str()));
    varindex_menu_items[i].user_data((void *)i);
  }
  varindex_menu_items[nvars+1].label(0);

  xpos = 10;

  varindex1 = 
    new Fl_Choice (xpos, ypos+=45, 100, 25, "axis 1");
  varindex1->align(FL_ALIGN_TOP);
  varindex1->textsize(12);
  varindex1->menu(varindex_menu_items);
  varindex1->callback(
    (Fl_Callback*)static_extract_and_redraw, this);

  varindex2 = 
    new Fl_Choice (xpos+100, ypos, 100, 25, "axis 2");
  varindex2->align(FL_ALIGN_TOP);
  varindex2->textsize(12);
  varindex2->menu(varindex_menu_items);
  varindex2->callback(
    (Fl_Callback*)static_extract_and_redraw, this);

  varindex3 = 
    new Fl_Choice (xpos+200, ypos, 100, 25, "axis 3");
  varindex3->align(FL_ALIGN_TOP);
  varindex3->textsize(12);
  varindex3->menu(varindex_menu_items);
  varindex3->value(nvars);  // initially, axis3 == "-nothing-"
  varindex3->callback(
    (Fl_Callback*)static_extract_and_redraw, this);

  // NLoop: Genenerate normalization style menu
  for( int i=0; i<n_normalization_styles; i++) {
    normalization_style_menu_items[i].label(
      normalization_style_labels[i]);
    normalization_style_menu_items[i].user_data(
      (void *)normalization_styles[i]);
  }
  normalization_style_menu_items[n_normalization_styles].label(0);

  x_normalization_style = 
    new Fl_Choice (xpos, ypos+=45, 100, 25, "normalize x");
  x_normalization_style->align(FL_ALIGN_TOP);
  x_normalization_style->textsize(12);
  x_normalization_style->menu(normalization_style_menu_items);
  x_normalization_style->value(NORMALIZATION_TRIM_1E3);
  x_normalization_style->callback(
    (Fl_Callback*)static_extract_and_redraw, this);
 
  y_normalization_style =
    new Fl_Choice (xpos+100, ypos, 100, 25, "normalize y");
  y_normalization_style->align(FL_ALIGN_TOP);
  y_normalization_style->textsize(12);
  y_normalization_style->menu(normalization_style_menu_items);
  y_normalization_style->value(NORMALIZATION_TRIM_1E3); 
  y_normalization_style->callback(
    (Fl_Callback*)static_extract_and_redraw, this);
 
  z_normalization_style = 
    new Fl_Choice (xpos+200, ypos, 100, 25, "normalize z");
  z_normalization_style->align(FL_ALIGN_TOP);
  z_normalization_style->textsize(12);
  z_normalization_style->menu(normalization_style_menu_items);
  z_normalization_style->value(NORMALIZATION_TRIM_1E3); 
  z_normalization_style->callback(
    (Fl_Callback*)static_extract_and_redraw, this);
 
  int xpos2 = xpos;
  int ypos2 = ypos;
  
  reset_view_button = b = 
    new Fl_Button(xpos2, ypos+=25, 20, 20, "reset view ");
  b->align(FL_ALIGN_RIGHT); b->selection_color(FL_BLUE);
  b->callback((Fl_Callback*) reset_view, this);

  spin = b= new Fl_Button(xpos2, ypos+=25, 20, 20, "spin");
  b->align(FL_ALIGN_RIGHT); b->selection_color(FL_BLUE);
  b->type(FL_TOGGLE_BUTTON);

  dont_clear = 
    new Fl_Button(xpos2, ypos+=25, 20, 20, "don't clear");
  dont_clear->align(FL_ALIGN_RIGHT);
  dont_clear->type(FL_TOGGLE_BUTTON);
  dont_clear->selection_color(FL_BLUE);
  dont_clear->callback(
    (Fl_Callback*)static_maybe_redraw, this);

  transform_style = 
    new Fl_Group (xpos2-1, ypos+25-1, 20+2, 4*25+2);

  no_transform = b = 
    new Fl_Button(xpos2, ypos+=25, 20, 20, "identity");
  b->callback((Fl_Callback*)static_extract_and_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_RADIO_BUTTON); 
  b->selection_color(FL_BLUE);

  sum_vs_difference = b = 
    new Fl_Button(xpos2, ypos+=25, 20, 20, "sum vs. diff.");
  b->callback((Fl_Callback*)static_extract_and_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_RADIO_BUTTON); 
  b->selection_color(FL_BLUE);
	
  polar = b = 
    new Fl_Button(xpos2, ypos+=25, 20, 20, "polar");
  b->callback((Fl_Callback*)static_extract_and_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_RADIO_BUTTON); 
  b->selection_color(FL_BLUE);
	
  transform_style->end();
  no_transform->setonly();

  ypos=ypos2;
  xpos=xpos2+100;

  show_points = b = 
    new Fl_Button(xpos, ypos+=25, 20, 20, "points");
  b->callback((Fl_Callback*)static_maybe_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);	
  b->value(1);

  show_deselected_points = b = 
    new Fl_Button(xpos, ypos+=25, 20, 20, " unselected");
  b->callback((Fl_Callback*)static_maybe_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);	
  b->value(1);

  show_axes = b = 
    new Fl_Button(xpos, ypos+=25, 20, 20, "axes");
  b->callback((Fl_Callback*)static_maybe_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);	
  b->value(1);

  show_labels = b = 
    new Fl_Button(xpos, ypos+=25, 20, 20, "labels");
  b->callback((Fl_Callback*)static_maybe_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);	
  b->value(1);

  show_scale = b = 
    new Fl_Button(xpos, ypos+=25, 20, 20, "scales");
  b->callback((Fl_Callback*)static_maybe_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);	
  b->value(1);

  show_grid = b = 
    new Fl_Button(xpos, ypos+=25, 20, 20, "grid");
  b->callback((Fl_Callback*)static_maybe_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);
  b->value(0);

  show_histogram = b = 
    new Fl_Button(xpos, ypos+=25, 20, 20, "histograms");
  b->callback((Fl_Callback*)redraw_one_plot, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);	
  b->value(0);

  ypos=ypos2;
  xpos=xpos2+200;

  b = new Fl_Button(xpos, ypos+=25, 20, 20, "selection color");
  b->align(FL_ALIGN_RIGHT); 
  b->selection_color(FL_BLUE); 
  b->callback((Fl_Callback*)choose_color_selected, this);

  lock_axes_button = b = 
    new Fl_Button(xpos, ypos+=25, 20, 20, "lock axes");
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);
  b->value(0);
}

//*****************************************************************
// control_panel_window::redraw_one_plot() -- Invoke methods to
// redraw one plot.
void plot_window::redraw_one_plot ()
{
  DEBUG( cout << "in redraw_one_plot" << endl ) ;
  compute_histograms();
  redraw();
  Fl::flush();
  needs_redraw = 0;
}

//*****************************************************************
// control_panel_window::redraw_all_plots( p) -- Invoke methods to
// redraw all plots cylically, sarting with plot p
void redraw_all_plots( int p)
{
  DEBUG( cout << "in redraw_all_plots(" << p << ")" << endl ) ;

  // redraw all plots, cyclically, sarting with plot p.  This p 
  // is important, since the draw() routine for a plot handles 
  // the selection region, and the active plot (the one where we 
  // are making the selection) must update the selected set and 
  // the color/texture arrays *before* all the other plots get 
  // redrawn.  Ugh.
  for( int i=0; i<nplots; i++) {
    int j=(p+i)%nplots;
    pws[j]->compute_histograms();
    pws[j]->redraw();
    Fl::flush();
    pws[j]->needs_redraw = 0;
  }
}

//*****************************************************************
// reset_all_plots() -- Reset all plots.  Should this become a
// member function of comntrol_panel_window?
void reset_all_plots()
{
  for( int i=0; i<nplots; i++) {
    pws[i]->reset_view();
  }
}

//*****************************************************************
// redraw_if_changing( dummy) -- Redarw changed plots?  Should 
// this become a member function of comntrol_panel_window?
void redraw_if_changing (void * dummy)
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
// make_global_widgets -- Should this become a member function of 
// comntrol_panel_window?
void make_global_widgets()
{
  Fl_Button *b;

  int xpos=10, ypos=500;
  npoints_slider = 
    new Fl_Hor_Value_Slider_Input(
      xpos+30, ypos+=25, 300-30, 20, "npts");
  npoints_slider->align(FL_ALIGN_LEFT);
  npoints_slider->callback(npoints_changed);
  npoints_slider->value(npoints);
  npoints_slider->step(1);
  npoints_slider->bounds(1,npoints);

  int xpos1 = xpos, ypos1 = ypos;

  show_deselected_button = b = 
    new Fl_Button(xpos, ypos+=25, 20, 20, "show nonselected");
  b->align(FL_ALIGN_RIGHT); 
  b->selection_color(FL_BLUE); 
  b->type(FL_TOGGLE_BUTTON);
  b->value(1);
  b->callback((Fl_Callback*)toggle_display_delected);

  add_to_selection_button = b = 
    new Fl_Button(xpos, ypos+=25, 20, 20, "add to selection");
  b->align(FL_ALIGN_RIGHT); 
  b->selection_color(FL_BLUE); 
  b->type(FL_TOGGLE_BUTTON);
  b->value(0);	

  invert_selection_button = b = 
    new Fl_Button(xpos, ypos+=25, 20, 20, "invert selection");
  b->align(FL_ALIGN_RIGHT); 
  b->selection_color(FL_BLUE); 
  b->callback((Fl_Callback*)invert_selection);

  clear_selection_button = b = 
    new Fl_Button(xpos, ypos+=25, 20, 20, "clear selection");
  b->align(FL_ALIGN_RIGHT); 
  b->selection_color(FL_BLUE); 
  b->callback(clear_selection);

  delete_selection_button = b = 
    new Fl_Button(xpos, ypos+=25, 20, 20, "kill selected");
  b->align(FL_ALIGN_RIGHT); 
  b->selection_color(FL_BLUE); 
  b->callback(delete_selection);

  write_data_button = b = 
    new Fl_Button(xpos, ypos+=25, 20, 20, "write data");
  b->align(FL_ALIGN_RIGHT); 
  b->selection_color(FL_BLUE); 
  b->callback( write_data);

  xpos = xpos1 + 150; ypos = ypos1;

  choose_color_deselected_button = b = 
    new Fl_Button(xpos, ypos+=25, 20, 20, "unselected color");
  b->align(FL_ALIGN_RIGHT); 
  b->selection_color(FL_BLUE); 
  b->callback((Fl_Callback*)choose_color_deselected);

  dont_paint_button = b = 
    new Fl_Button(xpos, ypos+=25, 20, 20, "don't paint");
  b->align(FL_ALIGN_RIGHT); 
  b->selection_color(FL_BLUE); 
  b->type(FL_TOGGLE_BUTTON);

  change_all_axes_button = b = 
    new Fl_Button(xpos, ypos+=25, 20, 20, "change axes");
  b->align(FL_ALIGN_RIGHT); 
  b->selection_color(FL_BLUE); 
  b->callback((Fl_Callback*)change_all_axes);

  link_all_axes_button = b = 
    new Fl_Button(xpos, ypos+=25, 20, 20, "link axes");
  b->align(FL_ALIGN_RIGHT); 
  b->selection_color(FL_BLUE); 
  b->type(FL_TOGGLE_BUTTON); 
  b->value(0);
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
// columns for which all values are identifcal
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
    for( int i=0; i<removed_columns.size(); i++) {
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
      itoa( nvars, cbuf, 10);
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
// Purpose: Driver to run everything
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

  // Set main control panel size and position.  NOTE screen_count
  // requires OpenGL 1.7, which was not available under Windows as
  // of 14-MAR-2006.
  #ifndef __WINDOWS__
    int number_of_screens = Fl::screen_count();
  #else
    int number_of_screens = 1;
  #endif   // WINDOWS
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
      upper_triangle_incr( ivar, jvar, nvars);
    }
    cps[i]->varindex1->value(ivar);  
    cps[i]->varindex2->value(jvar);  

    pws[i]->extract_data_points();
    pws[i]->reset_view();
    pws[i]->size_range(10, 10);
    pws[i]->resizable(pws[i]);

    if (borderless) pws[i]->border(0);
    pws[i]->show(argc,argv);
    pws[i]->resizable(pws[i]);
  }

  // now we can show the main control panel and all its subpanels
  main_control_panel->show();

  Fl::add_idle( redraw_if_changing);
  // Fl::add_check(redraw_if_changing);

  // enter main event loop
  int result = Fl::run();
  return result;
}
