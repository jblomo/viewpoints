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
//   1) Initializers and #define EXTERN are used so that this code
//      can be used with separate compilation units.
//   2) Global variables can be evil.  Move as many of these as
//      possible into specific classes or to the main routine
//   3) Consider putting these variables into a NAMESPACE?
//
// Author: Creon Levit    unknown
// Modified: P. R. Gazis  18-JUL-2006
//*****************************************************************

#ifndef VP_GLOBAL_DEFINITIONS_VP_H
#define VP_GLOBAL_DEFINITIONS_VP_H 1

#ifndef EXTERN
  #define EXTERN extern
#endif
#ifndef INIT
  #define INIT(x)
#endif

// Define debug flag and statement (move to global_definitions.h)
EXTERN int debugging INIT(0);
#define DEBUG(x) do {if (debugging) x;} while (0)

// Set parameters to hold error messages and flags.  These MUST be global
// because they will be set and used throughout the system
EXTERN string sErrorMessage INIT("");  // Defaul error messag is empty

// Define input file formats
#define ASCII 0
#define BINARY 1

// Define input data orderings
#define ROW_MAJOR 0
#define COLUMN_MAJOR 1

// Set parameters to define the default layout of the plot windows.  
// NOTE: Creon notes that MAXPLOTS must be a power of 2 for the
// texture-based coloring scheme to work properly.
EXTERN int nrows INIT(2);  // Default number of rows of plots
EXTERN int ncols INIT(2);   // Default number of columns of plots
EXTERN int nplots INIT(nrows*ncols);  // Default number of plot windows
#define MAXPLOTS 256 // maximum number of plot windows, must be a power of 2.

// Set the maximum number of columns and rows
#define MAXVARS 256  // Maximum number of columns
#define MAXPOINTS 2000000;  // Maximum number of rows (unless overidded by "--npoints=<int>"

// Initialize the actual number of rows (points or values) in 
// the data file and the actual number of columns (fields) in 
// each record.
EXTERN int npoints INIT(MAXPOINTS);   // number of rows in data file
EXTERN int nvars INIT(MAXVARS);		// number of columns in data file

// Define blitz::Arrays to hold raw and ranked (sorted) data 
// arrays.  Used extensively in many classes, so for reasons of 
// simplicity and clarity, these are left global
EXTERN blitz::Array<float,2> points;  // main data array
EXTERN blitz::Array<int,2> ranked_points;   // data, ranked, as needed.
EXTERN blitz::Array<int,1> ranked;	  // flag: 1->column is ranked, 0->not
EXTERN blitz::Array<int,1> identity;   // holds a(i)=i.
EXTERN blitz::Array<unsigned int,1> indices_selected; // indices of points for rendering, packed acording to selection state;
EXTERN std::vector<int> counts_selected; // counts of points in each selected set. Counts_selected[0] = count of nonselected points

// Define blitz::Arrays to flag selected points.  As with the raw
// data, these are left global for simplicity and clarity.
// newly_selected -- true iff point is in newly selected set
// selected -- true if point is selected in any window
// previously_selected -- true iff selected before mouse went down
// nselected -- number of points currently selected
// saved_selection -- saves the old selection when "inverting", so we can go back.
EXTERN blitz::Array<int,1> newly_selected, selected, previously_selected, saved_selection;
EXTERN int nselected;	
EXTERN bool selection_is_inverted;

// Texture co-ordinates?
// EXTERN blitz::Array<GLfloat,1> texture_coords;
EXTERN blitz::Array<GLfloat,2> colors;

// Temporary array (reference) for use with qsort
EXTERN blitz::Array<float,1> tmp_points;

// Define vector of strings to hold variable names.  Used 
// extensively by main routine and class control_panel_window
EXTERN std::vector<std::string> column_labels; 

// Global toggle between scale histogram & scale view :-(
EXTERN int scale_histogram INIT(0);

// Define variable to hold pointsize.  Used in main routine and
// classes control_panel_window and plot_window.  (Move to class
// plot_window?)
EXTERN float pointsize INIT(1.0);

// Flag to indicate that selection has been changed.  Creon notes
// that this should be 'fixed', but this is harder than it looks.
// For this reason, this variable is left global.
EXTERN int selection_changed INIT(1);

// Define main control panel's top level (global) widgets.  Many 
// of these must also be accessible to class plot_window and
// possibly control_panel_window so these are left global.  cpt -- 
// Tab widget to hold virtual control panels for individual plots.
// npoints_slider -- maximum number of points to display in all 
// plots.  Various buttons -- as suggested by their names.
EXTERN Fl_Tabs *cpt;  
EXTERN Fl_Hor_Value_Slider_Input *npoints_slider;
EXTERN Fl_Button *add_to_selection_button,
          *clear_selection_button, 
          *delete_selection_button;
EXTERN Fl_Button *show_deselected_button, 
          *invert_selection_button;
EXTERN Fl_Button *write_data_button;
EXTERN Fl_Button *choose_color_selected_button, 
          *choose_color_deselected_button; 
EXTERN Fl_Button *change_all_axes_button, *link_all_axes_button;
EXTERN Fl_Button *reload_plot_window_array_button;
EXTERN Fl_Button *read_data_button;

// Declare classes control_panel_window and plot_window here so 
// they can be referenced
class control_panel_window;
class plot_window;

// Define pointer arrays of plot windows and control panel 
// windows.  This can't be done until after the relevant class 
// definitions.  NOTE: In the long run, it might be safer to store 
// these in instances of the <vector> container class.
EXTERN plot_window *pws[ MAXPLOTS];

// There is one extra control_panel_window, with index=MAXPLOTS
// It has no associated plot window - it affects all (unlocked) plots.
EXTERN control_panel_window *cps[ MAXPLOTS+1]; 

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
