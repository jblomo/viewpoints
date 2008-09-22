// viewpoints - interactive linked scatterplots and more.
// copyright 2005 Creon Levit and Paul Gazis, all rights reserved.
//***************************************************************************
// File name: global_definitions_vp.h
//
// Class definitions: none
//   myCompare -- Member class used by std::stable_sort
//
// Classes referenced:
//   Various BLITZ templates
//
// Purpose: Global variables used by Creon Levit's viewpoints.  These must 
//   be defined before they are used in any class or function definitions.
//
// General design philosophy:
//   1) Initializers and #define EXTERN are used so that this code can be 
//      used with separate compilation units.
//   2) Global variables can be evil.  Move as many of these as possible 
//      into specific classes or to the main routine
//   3) Consider putting these variables into a NAMESPACE?
//
// Author: Creon Levit    2005-2006
// Modified: P. R. Gazis  16-SEP-2008
//***************************************************************************

#ifndef VP_GLOBAL_DEFINITIONS_VP_H
#define VP_GLOBAL_DEFINITIONS_VP_H 1

// The following trick allows a single include file (this one) to consistently 
// define and/or declare global variables, as required, across compilation units.  
// To use it, insert a line: #define DEFINE_GLOBALS at the beginning of (exactly) 
// one .c or .c++ file, and include this file in all .c and .c++ files.
#ifdef DEFINE_GLOBALS
  // define a variable, and initialize its value if requested to do so.
  #define GLOBAL
  #define INIT(x) =x
#else // DEFINE_GLOBALS
  // otherwise, declare the same variable, qualified with "extern", and do not initialize it.
  #define GLOBAL extern
  #define INIT(x)
#endif // DEFINE_GLOBALS

// Use the Standard Template Library
using namespace std;

// Define debug flag and statement (move to global_definitions.h)
GLOBAL int debugging INIT(0);

#define DEBUG(x) do {if (debugging) x;} while (0)

GLOBAL string about_string INIT ("");

// Set parameters to hold error messages and flags.  These MUST be global
// because they will be set and used throughout the system
GLOBAL string sErrorMessage INIT("");  // Default error message is empty

// Set parameters to define the default layout of the plot windows.
GLOBAL int nrows INIT(2);  // Default number of rows of plots
GLOBAL int ncols INIT(2);   // Default number of columns of plots
GLOBAL int nplots INIT(nrows*ncols);  // Default number of plot windows
#define MAXPLOTS 256 // maximum number of plot windows

// number of symbols avaliable for plotting (i.e. number of textures available 
// for points sprites) and also the number of symbols in the symbols_menu.
#define NSYMBOLS 54

// Set the maximum number of columns and rows
#define MAXVARS 400  // Maximum number of columns
#define MAXPOINTS 3000000  // Maximum number of rows (unless overidded by "--npoints=<int>")

// Initialize the actual number of rows (points or values) in the data file 
// and the actual number of columns (fields) in each record.
GLOBAL int npoints INIT(MAXPOINTS);   // number of rows in data file
GLOBAL int nvars INIT(MAXVARS);    // number of columns in data file

// use openGL vertex buffer objects (VBOs).  
GLOBAL bool use_VBOs INIT(true);

// Define various operating mode flags
GLOBAL bool expert_mode INIT(false);
GLOBAL bool read_from_stdin INIT(false);
GLOBAL bool trivial_columns_mode INIT(true);
GLOBAL bool preserve_old_data_mode INIT(true);
GLOBAL bool be_verbose INIT(false);
GLOBAL bool update_on_mouse_up INIT(true);

// Define blitz::Arrays to hold raw and ranked (sorted) data arrays.  Used 
// extensively in many classes, so for reasons of simplicity and clarity, 
// these are left global
// GLOBAL blitz::Array<float,2> points;  // main data array
// GLOBAL blitz::Array<int,2> ranked_points;   // data, ranked, as needed.
// GLOBAL blitz::Array<int,1> ranked;    // flag: 1->column is ranked, 0->not

// Define blitz::Arrays to flag selected points.  As with the raw data, these 
// are left global for simplicity and clarity.
// inside_footprint -- true for points that lie inside the footprint of the brush.
// newly_selected -- true iff point is in newly selected set.
// selected -- index of the brush that most recently selected the point, or 0.
// previously_selected -- index of the brush that previously selected the point
// nselected -- number of points currently selected
// saved_selection -- saves the old selection when "inverting", so we can go back.
GLOBAL blitz::Array<int,1> inside_footprint;
GLOBAL blitz::Array<int,1> newly_selected;
GLOBAL blitz::Array<int,1> selected;
GLOBAL blitz::Array<int,1> previously_selected;
GLOBAL blitz::Array<int,1> saved_selection;
GLOBAL int nselected;  
GLOBAL bool selection_is_inverted INIT(false);

// Temporary array (reference) for use with qsort
GLOBAL blitz::Array<float,1> tmp_points;

// Need to include class definitions for Column_Info so we can define a vector
// of Column_Info objects to hold column labels and associated info.  This is
// used extensively by the data_file_manager, comntrol_panel_window, and
// plot_window classes. 
// NOTE: As of 10-JUL-2008, this code has been moved to class Data_File_Manager
// Class Column_Info;
// #include "column_info.h"
// GLOBAL std::vector<Column_Info> column_info; 

// Global toggle between scale histogram & scale view :-(
GLOBAL int scale_histogram INIT(0);

// Define variable to hold pointsize.  Used in main routine and classes 
// Control_Panel_Window and Plot_Window.  (Move to class plot_window?)
GLOBAL float default_pointsize INIT(1.0);

// Define main control panel's top level (global) widgets.  Many of these must 
// also be accessible to class plot_window and possibly Control_Panel_Window 
// so these are left global.
// cpt -- Tab widget to hold virtual control panels for individual plots.
// npoints_slider -- maximum number of points to display in all 
// plots.  Various buttons -- as suggested by their names.
GLOBAL Fl_Tabs *cpt;  
GLOBAL Fl_Tabs *brushes_tab;
GLOBAL Fl_Button *add_to_selection_button, *clear_selection_button, *delete_selection_button;
GLOBAL Fl_Button *show_deselected_button, *mask_out_deselected, *invert_selection_button;
GLOBAL Fl_Button *write_data_button;
GLOBAL Fl_Button *choose_color_selected_button, *choose_color_deselected_button; 
GLOBAL Fl_Repeat_Button *change_all_axes_button;
GLOBAL Fl_Button *link_all_axes_button;
GLOBAL Fl_Button *defer_redraws_button;
GLOBAL Fl_Button *reload_plot_window_array_button;
GLOBAL Fl_Button *read_data_button;

// Define class Data_File_Manager (must use pointer with incomplete def'n)
class Data_File_Manager;
GLOBAL Data_File_Manager *pdfm;

// Declare classes Control_Panel_Window and Plot_Window here so they can be 
// referenced
class Control_Panel_Window;
class Plot_Window;
class Brush; 

// Define pointer arrays of plot windows and control panel windows.  This 
// can't be done until after the relevant class definitions.  NOTE: In the 
// long run, it might be safer to store these in instances of the 
// <vector> container class.
GLOBAL Plot_Window *pws[ MAXPLOTS];

// There is one extra Control_Panel_Window, with index=MAXPLOTS.  It has no 
// associated plot window - it affects all (unlocked) plots.
GLOBAL Control_Panel_Window *cps[ MAXPLOTS+1]; 

#define NBRUSHES 7
GLOBAL Brush *brushes[NBRUSHES];  // MCL XXX this should be a static c++ vector handled in brush.cpp

// Make absolutely certain variables for point sprites are defined
#ifndef GL_POINT_SPRITE_ARB
  #define GL_POINT_SPRITE_ARB 0x8861
#endif
#ifndef GL_COORD_REPLACE_ARB
  #define GL_COORD_REPLACE_ARB 0x8862
#endif
#ifndef GL_POINT_SPRITE
  #define GL_POINT_SPRITE 0x8861
#endif
#ifndef GL_COORD_REPLACE
  #define GL_COORD_REPLACE 0x8862
#endif

//# Make absolutely certain SVN_VERSION is defined
#ifdef __WIN32__
  #define SVN_VERSION "Windows revision 249"
#endif
#ifndef SVN_VERSION
  #define SVN_VERSION "unknown_version"
#endif

// Define global to hold serialization version.  These must be made global
// so they will be available to serialization methods for different classes
// WARNING: The 'current_' and 'last_' values must record the last revision 
// in which the serialization methods were changed and the last revision 
// for which configuration files were supported!
GLOBAL int current_serialization_version INIT(225);
GLOBAL int last_supported_serialization_version INIT(-100);
GLOBAL int serialization_file_version INIT(-1);  // Will be reset at runtime

// Define pointer to hold confirmation window
GLOBAL Fl_Window *confirmation_window;
GLOBAL Fl_Window *find_window;

// Persistant state for gsl random number generator (Mersenne Twister)
GLOBAL gsl_rng *vp_gsl_rng;

// Global function definitions
GLOBAL int make_confirmation_window( const char* text, int nButtons = 3, int nLines = 2);
GLOBAL int make_find_window( const char* text, char *res);
GLOBAL void reset_selection_arrays();

//***************************************************************************
// Class: MyCompare
//
// Class definitions:
//   MyCompare -- Member class to maintain a comparison operator
//
// Classes referenced: none
//
// Purpose: Member class to maintain a comparison operator for use by 
//   std::stable_sort
//
// Functions:
//   comparison operator
//
// Author: Creon Levit     unknown
// Modified: P. R. Gazis   25-MAR-2006
//***************************************************************************
class MyCompare
{
public:
  bool operator()( const int i, const int j) {  
    return tmp_points(i) < tmp_points(j);
    // return (tpoints[i]<tpoints[j]);
  }
};

float pow2(float x);

#endif   // VP_GLOBAL_DEFINITIONS_VP_H


