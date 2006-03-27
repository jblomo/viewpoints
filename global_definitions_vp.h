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
// Modified: P. R. Gazis  27-MAR-2006
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

// Set the maximum number of columns and rows
const int nvars_max = 256;  // Maximum number of columns
const int MAXPOINTS = 2000000;  // Maximum number of rows

// Set parameters to define the default layout of the plot 
// windows.  NOTE: Creon notes that maxplots must be a power 
// of 2 for textures and that this will 'cause trouble'.
int nrows=2, ncols=2;   // Default number of rows and columns
int nplots = nrows*ncols;  // Default number of plot windows
const int maxplots=64;  // Maxmimum number of plots

// Initialize the actual number of rows (points or values) in 
// the data file and the actual number of columns (fields) in 
// each record.
int npoints = MAXPOINTS;   // number of rows in data file
int nvars = nvars_max;		// number of columns in data file

// Global toggle between scale histogram & scale view :-(
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

// Define vector of strings to hold variable names
std::vector<std::string> column_labels; 

// Set the initial fraction of the window to be used for data to 
// allow room for axes, labels, etc.  (Move to plot_window class?)
const float initial_pscale = 0.8; 

// Define variable to hold pointsize.  (Move to plot_window class?)
float pointsize = 1.0;

// Specify how the RGB and alpha source and destination blending 
// factors are computed.  (Move to plot_window class?)
int sfactor = GL_CONSTANT_COLOR;
int dfactor = GL_DST_ALPHA;

// Flag to indicate that selection has been changed.  Creon notes
// that this should be fixed, but that this is than it looks.
int selection_changed = 1;

// Define main control panel's top level (global) widgets.  Many 
// of these must also be accessible to class control_panel_window.
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

// Associate integer values with normalization styles.  Used by
// plot_window::normalize and definition of control_panel_window
// NOTE: If possible, all the normalization information should
// become static member variables of class control_panel_window
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
const int n_normalization_styles = 11;

// Declare classes control_panel_window and plot_window here so 
// they can be referenced
class control_panel_window;
class plot_window;

// Define pointer arrays of plot windows and control panel 
// windows.  This can't be done until after the relevant class 
// definitions.  NOTE: In the long run, it might be safer to store 
// these in instances of the <vector> container class.
plot_window *pws[ maxplots];
control_panel_window *cps[ maxplots];

// Define parameters used for textures?	
GLfloat texture_images[2][4*(maxplots)];
GLfloat pointscolor[4] = {1,1,1,1};
GLfloat texenvcolor[4] = {1,1,1,1};
GLuint texnames[2];
int textures_initialized = 0;

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
