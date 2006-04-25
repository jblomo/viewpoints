//*****************************************************************
// File name: global_definitions_vp.h
//
// Class definitions: none
//   myCompare -- Member class used by std::stable_sort
//
// Classes referenced:
//   Various BLITZ templates
//
// Purpose: Global variables used by Creon Levit's viewpoints.
//   These must be defined before they are used in any class or
//   function definitions.
//
// General design philosophy:
//   1) Global variables can be evil.  Move as many of these as
//      possible into specific classes or to the main routine
//   2) Consider putting these variables into a NAMESPACE
//
// Author: Creon Levit    unknown
// Modified: P. R. Gazis  25-APR-2006
//*****************************************************************

#ifndef VP_GLOBAL_DEFINITIONS_VP_H
#define VP_GLOBAL_DEFINITIONS_VP_H 1

// Define debug flag and statement (move to global_definitions.h)
int debugging = 0;
#define DEBUG(x) do {if (debugging) x;} while (0)

// Define input file formats
#define ASCII 0
#define BINARY 1

// Define input data orderings
#define ROW_MAJOR 0
#define COLUMN_MAJOR 1

// Set parameters to define the default layout of the plot 
// windows.  NOTE: Creon notes that maxplots must be a power 
// of 2 for textures and that this will 'cause trouble'.
int nrows=2, ncols=2;   // Default number of rows and columns
int nplots = nrows*ncols;  // Default number of plot windows
const int maxplots=64;  // Maxmimum number of plots

// Set the maximum number of columns and rows
const int nvars_max = 256;  // Maximum number of columns
const int MAXPOINTS = 2000000;  // Maximum number of rows

// Initialize the actual number of rows (points or values) in 
// the data file and the actual number of columns (fields) in 
// each record.
int npoints = MAXPOINTS;   // number of rows in data file
int nvars = nvars_max;		// number of columns in data file

// Define blitz::Arrays to hold raw and ranked (sorted) data 
// arrays.  Used extensively in many classes, so for reasons of 
// simplicity and clarity, these are left global
blitz::Array<float,2> points;  // main data array
blitz::Array<int,2> ranked_points;   // data, ranked, as needed.
blitz::Array<int,1> ranked;	  // flag: 1->column is ranked, 0->not
blitz::Array<int,1> identity;   // holds a(i)=i.

// Define blitz::Arrays to flag selected points.  As with the raw
// data, these are left global for simplicity and clarity.
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

// Temporary array (reference) for use with qsort
blitz::Array<float,1> tmp_points;

// Define vector of strings to hold variable names.  Used 
// extensively by main routine and class control_panel_window
std::vector<std::string> column_labels; 

// Global toggle between scale histogram & scale view :-(
int scale_histogram = 0;

// Define variable to hold pointsize.  Used in main routine and
// classes control_panel_window and plot_window.  (Move to class
// plot_window?)
float pointsize = 1.0;

// Flag to indicate that selection has been changed.  Creon notes
// that this should be 'fixed', but this is harder than it looks.
// For this reason, this variable is left global.
int selection_changed = 1;

// Define main control panel's top level (global) widgets.  Many 
// of these must also be accessible to class plot_window and
// possibly control_panel_window so these are left global.  cpt -- 
// Tab widget to hold virtual control panels for individual plots.
// npoints_slider -- maximum number of points to display in all 
// plots.  Various buttons -- as suggested by their names.
Fl_Tabs *cpt;  
Fl_Hor_Value_Slider_Input *npoints_slider;
Fl_Button *add_to_selection_button,
          *clear_selection_button, 
          *delete_selection_button;
Fl_Button *show_deselected_button, 
          *invert_selection_button;
Fl_Button *write_data_button;
Fl_Button *choose_color_selected_button, 
          *choose_color_deselected_button; 
Fl_Button *dont_paint_button,
          *change_all_axes_button;
Fl_Button *link_all_axes_button;
Fl_Button *reload_plot_window_array_button;
Fl_Button *read_data_button;

// Declare classes control_panel_window and plot_window here so 
// they can be referenced
class control_panel_window;
class plot_window;

// Define pointer arrays of plot windows and control panel 
// windows.  This can't be done until after the relevant class 
// definitions.  NOTE: In the long run, it might be safer to store 
// these in instances of the <vector> container class.
plot_window *pws[ maxplots];

// There is one extra control_panel_window, with index=maxplots
// It has no associated plot window - it affects all (unlocked) plots.
control_panel_window *cps[ maxplots+1]; 

//*****************************************************************
// Class: myCompare
//
// Class definitions:
//   myCompare -- Member class to maintain a comparison operator
//
// Classes referenced: none
//
// Purpose: Member class to maintain a comparison operator for use
//   by std::stable_sort
//
// Functions:
//   comparison operator
//
// Author: Creon Levit     unknown
// Modified: P. R. Gazis   25-MAR-2006
//*****************************************************************
class myCompare
{
public:
  bool operator()( const int i, const int j) {  
    return tmp_points(i) < tmp_points(j);
    // return (tpoints[i]<tpoints[j]);
  }
};

#endif   // VP_GLOBAL_DEFINITIONS_VP_H
