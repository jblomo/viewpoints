// viewpoints - interactive linked scatterplots and more.
// copyright 2005 Creon Levit and Paul Gazis, all rights reserved.
//***************************************************************************
// File name: vp.cpp
//
// Class definitions: none
//
// Classes referenced:
//   Various BLITZ templates
//   Any classes in global_definitions_vp.h
//   Plot_Window -- Plot window
//   Control_Panel_Window -- Control panel window
//   Data_File_Manager -- Manage data files
//
// Required packages
//    FLTK 1.1.6 -- Fast Light Toolkit graphics packageF
//    FLEWS 0.3 -- Extensions to FLTK 
//    OGLEXP 1.2.2 -- Access to OpenGL extension under Windows
//    GSL 1.6 -- Gnu Scientific Library package for Windows
//    Blitz++ 0.9 -- Various math routines
//
// Compiler directives:
//   May require -D__WIN32__ to compile on windows
//   See Makefile for linux and OSX compile & link info.
//
// Purpose: viewpoints - interactive linked scatterplots and more.
//
// General design philosophy:
//   1) This code represents a battle between Creon Levit's passion for speed
//      and efficiency and Paul Gazis's obsession with organization and 
//      clarity, unified by a shared desire to produce a powerful and easy to 
//      use tool for exploratory data analysis.  Creon's code reflects a 
//      strong 'C' heritage.  Paul's code is written in C++ using the 'if 
//      only it were JAVA' programming style.
//   2) Functions in the main routine are sufficiently interdependant that it
//      would be difficult to distribute them among separate objects.  For 
//      this reason they are left in main().  It would be quite possible to
//      combine them into a single vp class, but they have been left in 
//      main() for reasons of simplicity.
//   3) This code takes advantage of the FLTK graphics toolkit, the BOOST
//      array package, BOOST serialization, and several other C++ packages.
//      For the most part, this is straightforward, but use of BOOST 
//      serialization is complicated by its lack of documentation and its 
//      assumptions regarding the availability and use of constructors.
//   4) In addition to BOOST serialization, parts of this code also store
//      settings via the FLTK FL_Preferences class.
//
// Functions:
//   usage() -- Print help information
//   make_help_about_window( *o) -- Draw the 'About' window
//   create_main_control_panel( main_x, main_y, main_w, main_h, cWindowLabel) 
//     -- Create the main control panel window.
//   cb_main_control_panel( *o, *u); -- Callback for main control panel
//   create_broadcast_group() -- Create special panel under tabs
//   manage_plot_window_array( *o, *u) -- Manage plot window array
//   cb_manage_plot_window_array( *o) -- Idle callback
//   make_help_view_window( *o) -- Make Help View window
//   textsize_help_view_widget( *o, *u) -- Change help text size
//   close_help_window( *o, *u -- Help View window callback
//   make_main_menu_bar() -- Create main menu bar
//   make_file_name_window( *o) -- Make View|File Name window
//   make_options_window( *o) -- Make Tools|Options window
//   cb_options_window( *o, *u) -- Process Tools|Options window
//   step_help_view_widget( *o, *u) -- Step through the Help|Help window.
//   make_global_widgets() -- Controls for main control panel
//   change_all_axes( *o) -- Change all axes
//   npoints_changed( *o) -- Update number of points changed
//   resize_selection_index_arrays( nplots_old, nplots) -- Resize arrays 
//   write_data( *o, *u) -- Write data widget
//   reset_all_plots() -- Reset all plots
//   read_data( *o, *u) -- Read data widget
//   load_state( *o) -- Load saved state
//   save_state( *o) -- Save current state
//   redraw_if_changing( *dummy) -- Redraw changing plots
//   reset_selection_arrays() -- Reset selection arrays
//
// Author: Creon Levit    2005-2006
// Modified: P. R. Gazis  16-SEP-2008
//***************************************************************************

// Include the necessary include libraries
#include "include_libraries_vp.h"
#include <FL/Fl_Tooltip.H>
#include <FL/Fl_Scroll.H>

// define and initialize globals
#define DEFINE_GLOBALS
#include "global_definitions_vp.h"

// Include associated headers and source code
#include "data_file_manager.h"
#include "plot_window.h"
#include "control_panel_window.h"
#include "brush.h"
#include "unescape.h"

// Define and initialize number of screens
static int number_of_screens = 0;

// Approximate values of window manager borders & desktop borders (stay out of
// these). The "*_frame" constants keep windows from crowding the coresponding 
// screen edge.  The "*_safe" constants keep windows from overlapping each 
// other.  These are used when the main control panel window is defined.  And 
// when the plot windows are tiled to fit the screen. Too bad they are only 
// "hints" according most window managers (and we all know how well managers 
// take hints).
#ifdef __APPLE__
 static int top_frame=35, bottom_frame=0, left_frame=0, right_frame=5;
 static int top_safe = 1, bottom_safe=5, left_safe=5, right_safe=1;
#else // __APPLE__
 static int top_frame=25, bottom_frame=5, left_frame=4, right_frame=5;
 static int top_safe = 1, bottom_safe=10, left_safe=10, right_safe=1;
#endif // __APPLE__

// These are needed to pass to manage_plot_window_array
static int global_argc;
static char **global_argv;

// Define and set default border style for the plot windows
static int borderless=0;  // By default, use window manager borders

// Needed to track position in help window
static int help_topline;

// Define variables to hold main control panel window, tabs widget, and 
// virtual control panel positions.  Consolidated here for reasons of clarity.

// Increase this when the main panel needs to get wider:
static const int main_w = 365;       

// Increase this when the main panel needs to get taller, including situations
// when cp_widget_h increases:
// static const int main_h = 850;
static const int main_h = 890;

// Increase this when the plot controls need more height to fit in their subpanel
// static const int cp_widget_h = 505; 
static const int cp_widget_h = 430; 
static const int brushes_h = 250;

// The rest of these should not have to change
static const int tabs_widget_h = cp_widget_h+20;
// static const int global_widgets_y = tabs_widget_h + brushes_h + 20;
static const int global_widgets_y = tabs_widget_h + brushes_h + 30;
static const int tabs_widget_x = 3, tabs_widget_y = 30;
static const int cp_widget_x = 3, cp_widget_y = tabs_widget_y+20;
// static const int brushes_x = 3, brushes_y = tabs_widget_y+tabs_widget_h;
static const int brushes_x = 3, brushes_y = tabs_widget_y+tabs_widget_h+10;
static const int global_widgets_x = 10;

// Instantiate a preferences object to manage persistent preferences
static Fl_Preferences
  prefs_( Fl_Preferences::USER, "viewpoints.arc.nasa.gov", "viewpoints");

// Define class to hold data file manager
Data_File_Manager dfm;

// Define pointers to hold main control panel, main menu bar, and any pop-up 
// windows.  NOTE: help_view_widget must be defined here so it will be 
// available to callback functions
Fl_Window *main_control_panel;
Fl_Scroll *main_scroll;
Fl_Menu_Bar *main_menu_bar;
Fl_Window *options_window;
Fl_Window *about_window;
Fl_Window *help_view_window;
Fl_Help_View *help_view_widget;

// Define pointers to widgets for the Tools|Options window
Fl_Check_Button* expertButton;
Fl_Check_Button* trivialColumnsButton;
Fl_Check_Button* preserveOldDataButton;
Fl_Input* bad_value_proxy_input;

// Function definitions for the main method
void usage();
void make_help_about_window( Fl_Widget *o);
void create_main_control_panel(
  int main_x, int main_y, int main_w, int main_h, char* cWindowLabel);
void cb_main_control_panel( Fl_Widget *o, void* user_data);
void create_brushes( int w_x, int w_y, int w_w, int w_h);
void brushes_tab_cb();
void create_broadcast_group();
void manage_plot_window_array( Fl_Widget *o, void* user_data);
void cb_manage_plot_window_array( void* o);
void make_main_menu_bar();
void make_file_name_window( Fl_Widget *o);
void make_options_window( Fl_Widget *o);
void cb_options_window( Fl_Widget *o, void* user_data);
void make_help_view_window( Fl_Widget *o);
void textsize_help_view_widget( Fl_Widget *o, void* user_data);
void close_help_window( Fl_Widget *o, void* user_data);
void step_help_view_widget( Fl_Widget *o, void* user_data);
void make_global_widgets();
void change_all_axes( Fl_Widget *o);
void npoints_changed( Fl_Widget *o);
void write_data( Fl_Widget *o, void* user_data);
void reset_all_plots();
void read_data( Fl_Widget* o, void* user_data);
int load_initial_state( string configFileSpec);
int load_state( Fl_Widget* o);
int save_state( Fl_Widget* o);
void redraw_if_changing( void *dummy);
void reset_selection_arrays();

//***************************************************************************
// usage() -- Print help information to the console and exit.  NOTE: This is
// formatted for 80 columns without wrapoaround because this is still a
// common console setting.
void usage()
{
  cerr << endl;
  cerr << "Usage: vp {optional arguments} {optional filename}" << endl;
  cerr << endl;
  cerr << "Optional arguments:" << endl;
  cerr << "  -b, --borderless            "
       << "Don't show decorations on plot windows.  NOTE:" << endl
       << "                              "
       << "this option may freeze plot windows in place." << endl;
  cerr << "  -B, --no_vbo                "
       << "Don't use openGL vertex buffer objects.  Try" << endl
       << "                              "
       << "this option if problems arise loading VBOs" << endl;
  cerr << "  -c, --cols=NCOLS            "
       << "Startup with this many columns of plot windows," << endl
       << "                              "
       << "default=2" << endl;
  cerr << "  -C, --config_file=FILENAME  "
       << "Read saved configuration from FILENAME." << endl;
  cerr << "  -d, --delimiter=CHAR        "
       << "Interpret CHAR as a field separator, default is" << endl
       << "                              "
       << "whitespace." << endl;
  cerr << "  -f, --format={ascii,binary} "
       << "Input file format, default=ascii.  NOTE: for ASCII" << endl
       << "                              "
       << "files, the data block is assumed to begin with an" << endl
       << "                              "
       << "uncommented line that contains column labels" << endl;
  cerr << "  -i, --input_file=FILENAME   "
       << "Read input data from FILENAME." << endl;
  cerr << "  -I, --stdin                 "
       << "Read input data from stdin." << endl;
  cerr << "  -L, --commented_labels      "
       << "Expect the line that contains column labels to" << endl
       << "                              "
       << "begin with a comment character." << endl;
  cerr << "  -m, --monitors=NSCREENS     "
       << "Try and force output to display across NSCREENS" << endl
       << "                              "
       << "screens if available." << endl;
  cerr << "  -M, --missing_values=NUMBER "
       << "Set the value of unreadable, nonnumeric, empty," << endl
       << "                              "
       << "or missing data to NUMBER, default=0.0." << endl;
  cerr << "  -n, --npoints=NPOINTS       "
       << "Read at most NPOINTS from input file, default is" << endl
       << "                              "
       << "min(until_EOF, 2000000)." << endl;
  cerr << "  -o, --ordering={rowmajor,columnmajor} " << endl
       << "                              "
       << "Ordering for binary data, default=columnmajor." << endl;
  cerr << "  -P, --preserve_data=(T,F)   "
       << "Preserve old data if a file read fails, default=TRUE." << endl;
  cerr << "  -r, --rows=NROWS            "
       << "Startup with this many rows of plot windows," << endl
       << "                              "
       << "default=2." << endl;
  cerr << "  -s, --skip_lines=NLINES     "
       << "Skip NLINES at start of input file, default=0." << endl;
  cerr << "  -t, --trivial_columns=(T,F) "
       << "Remove columns with a single value, default=TRUE." << endl;
  cerr << "  -v, --nvars=NVARS           "
       << "Input has NVARS values per point (only for row" << endl
       << "                              "
       << "major binary data)." << endl;
  cerr << "  -h, --help                  "
       << "Display this message and exit." << endl;
  cerr << "  -x, --expert                "
       << "enable expert mode (bypass confirmations, etc.)" << endl;
  cerr << "  -O, --verbose               "
       << "enable verbOse output with more diagnostics." << endl;
  cerr << "  -V, --version               "
       << "Output version information and exit." << endl;

  exit( -1);
}

//***************************************************************************
// make_help_about_window( *o) -- Create the 'Help|About' window.
void make_help_about_window( Fl_Widget *o)
{
  if( about_window != NULL) about_window->hide();
   
  // Create Help|About window
  Fl::scheme( "plastic");  // optional
  about_window = new Fl_Window( 360, 220, "About vp");
  about_window->begin();
  about_window->selection_color( FL_BLUE);
  about_window->labelsize( 10);
  
  // Write text to box label and align it inside box
  Fl_Box* output_box = new Fl_Box( 5, 5, 350, 180);
  output_box->box( FL_SHADOW_BOX);
  output_box->color( 7);
  output_box->selection_color( 52);
  output_box->labelfont( FL_HELVETICA);
  output_box->labelsize( 15);
  output_box->align( FL_ALIGN_TOP|FL_ALIGN_CENTER|FL_ALIGN_INSIDE);
  output_box->copy_label( about_string.c_str());

  // Invoke a multi-purpose callback function to close window
  Fl_Button* close = new Fl_Button( 240, 190, 60, 25, "&Close");
  close->callback( (Fl_Callback*) close_help_window, about_window);

  // Done creating the 'Help|About' window
  // about_window->resizable( about_window);
  about_window->resizable( NULL);
  about_window->end();
  about_window->show();
}

//***************************************************************************
// create_main_control_panel( main_x, main_y, main_w, main_h, cWindowLabel) 
// -- Create the main control panel window.
void create_main_control_panel( 
  int main_x, int main_y, int main_w, int main_h, char* cWindowLabel)
{
  // Create main control panel window
  Fl_Group::current(0);
  Fl::scheme( "plastic");  // optional
  Fl_Tooltip::delay(1.0);
  Fl_Tooltip::hoverdelay(1.0);
  Fl_Tooltip::size(12);
  
  main_h = min(main_h, Fl::h() - (top_frame + bottom_frame));
  main_control_panel = new Fl_Window( main_x, main_y, main_w, main_h, cWindowLabel);
  main_control_panel->resizable( main_control_panel);

  // Add callback function to intercept 'Close' operations
  main_control_panel->callback( (Fl_Callback*) cb_main_control_panel, main_control_panel);

  // Make main menu bar and add the global widgets to control panel
  make_main_menu_bar();

  // All controls (except the main menu bar) in the main panel are inside an Fl_Scroll, 
  // because there are too many controls to all fit vertically on some small screens.
  // Eventally, we will make the sub-panels independently expandable and reorganize 
  // the gui to alleviate this problem.
  main_scroll = new Fl_Scroll(0, main_menu_bar->h(), main_w, main_h - main_menu_bar->h());
  main_scroll->box(FL_NO_BOX);
  
  // MCL XXX
  // if I move this call to create_brushes() to the end of this routine, to just before
  // the call to main_control_panel->end(), I get a core dump.  That's' too bad......
  create_brushes( brushes_x, brushes_y, main_w-6, brushes_h);

  // the widgets at the bottom of the main panel.  Seems they need to created here. :-?
  make_global_widgets ();

  // Inside the main control panel, there is a tab widget, cpt, 
  // that contains the sub-panels (groups), one per plot.
  cpt = new Fl_Tabs( tabs_widget_x, tabs_widget_y, main_w-6, tabs_widget_h);
  cpt->selection_color( FL_BLUE);
  cpt->labelsize( 10);

  // Done creating main control panel (except for the tabbed 
  // sub-panels created by manage_plot_window_array)
  main_scroll->end();
  main_control_panel->end();
  Fl_Group::current(0);  
}

//***************************************************************************
// cb_main_control_panel( *o, *user_data) -- Callback (and potentially 
// close) the main control panel window.  It is assumed that a pointer to
// the window will be passed as USER_DATA.  WARNING: No error checking is 
// done on USER_DATA!
void cb_main_control_panel( Fl_Widget *o, void* user_data)
{
  if( expert_mode || make_confirmation_window( "Quit?  Are you sure?") > 0) {
    ((Fl_Window*) user_data)->hide();
    exit( 0);
  }
}

//***************************************************************************
// create_brushes( w_x, w_y, w_w, w_h) -- Create bushes.  Move this to class 
// Brush?
void create_brushes( int w_x, int w_y, int w_w, int w_h)
{
  // Fl_Group::current(0);  // not a subwindow
  // brushes_window = new Fl_Window( w_x, w_y, w_w, w_h, "brushes");
  // brushes_window->resizable(brushes_window);

  // Inside the main control panel, there is a Tabs widget
  // that contains all the indidual brush's sub-panels.
  brushes_tab = new Fl_Tabs( w_x, w_y, w_w, w_h);
  brushes_tab->selection_color( FL_BLUE);
  brushes_tab->labelsize( 10);
  brushes_tab->callback( (Fl_Callback*) brushes_tab_cb);
  // brushes_tab->clear_visible_focus(); // disbaled for future soloing...

  Fl_Group::current(brushes_tab);
  for (int i=0; i<NBRUSHES; i++) {
    // create a brush (Fl_Group) corresponding to the tab
    brushes[i] = new Brush(w_x, w_y+20, w_w-6, w_h-(20+6));
  }
  brushes_tab->end();

  // we have to start with some brush active, and since
  // brushes[0] is for "unselected" points, we start with brushes[1]
  brushes_tab->value(brushes[1]);
  brushes_tab_cb();

  // brushes_window->end();
  // brushes_window->show();
}

//***************************************************************************
// brushes_tab_cb() -- Callback to keep tab's colored labels drawn in the 
// right color when they're selected.  Move this to class Brush?
void brushes_tab_cb() {
  brushes_tab->labelcolor(brushes_tab->value()->labelcolor());
  brushes_tab->redraw_label();
}

//***************************************************************************
// create_broadcast_group () -- Create a special panel (really a group under 
// a tab) with label "+" this group's widgets effect all the others (unless 
// a plot's tab is "locked" - TBI).  MCL XXX should this be a method of 
// Control_Panel_Window or should it be a singleton?
void create_broadcast_group ()
{
  Fl_Group::current(cpt);  
  Control_Panel_Window *cp = cps[nplots];
  cp = new Control_Panel_Window( cp_widget_x, cp_widget_y, main_w - 6, cp_widget_h);
  cp->label("all");
  cp->labelsize( 10);
  cp->resizable( cp);
  cp->make_widgets( cp);
  cp->end();

  // this group's index is highest (and it has no associated plot window)
  cp->index = nplots;

  // this group's callbacks all broadcast any "event" to the other 
  // (unlocked) tabs groups.  with a few exceptions... (for now)
  for (int i=0; i<cp->children(); i++) {
    Fl_Widget *wp = cp->child(i);
    wp->callback( (Fl_Callback *)(Control_Panel_Window::broadcast_change), cp);
  }

  // MCL XXX these widgets cause crashes or misbehaviors in the broadcast
  // panel, so disable them for now.
  cp->sum_vs_difference->deactivate();
  cp->no_transform->deactivate();
  cp->cond_prop->deactivate();
  cp->fluctuation->deactivate();

  // Initially, this group has no axes (XXX or anything else, for that matter)
  cp->varindex1->value(nvars);  // initially == "-nothing-"
  cp->varindex2->value(nvars);  // initially == "-nothing-"
  cp->varindex3->value(nvars);  // initially == "-nothing-"
}

//***************************************************************************
// manage_plot_window_array( *o, *u) -- General-purpose method to create, 
// manage, and reload the plot window array.  It saves any existing axis 
// information, deletes old tabs, creates new tabs, restores existing axis 
// information, and loads new data into new plot windows.  
//   WARNING: This method is the 800-lb gorilla, on which much of the GUI
// depends!  Like all gorillas, it is delicate, sensitive, and must be 
// treated with a profound mixture of caution and respect.  In particular, 
// slight changes in the FLTK calls can lead to elusive segmentation faults!  
// Test any changes carefully!
//   WARNING: There is little protection against missing data, and gorillas
// can misbehave if they can't find their bananas!
//   NOTE: Little attempt has been made to optimize this method for speed.  
//
//   There are four possible behaviors, which all must be recognized, 
// identified, and treated differently:
// 1) INITIALIZE -- NULL argument.  Set nplots_old = 0.  
// 2) NEW_DATA -- Called from a menu or as part of a 'load saved state' 
//    operation.  In this mode, new data is read into the VBOs, so all
//    plot windows must be hidden and redrawn!
// 3) REFRESH_WINDOWS -- Called from a menu.  In this mode, some windows 
//    may be preserved without the need for redrawing.
// 4) RELOAD -- Called from a button or menu.  NOTE: These no longer happen, 
//    and this operation is no longer supported!
// Redrawing of plot windows is controlled by the value of NPLOTS_OLD.  This
// is initialized to NPLOTS.  For an INITIALIZE or NEW_DATA operation, it is 
// then reset to zero, so that all existing plots will be hidden so they can 
// be redrawn with new data as noted above.  
//   In some cases, it is desirable to restore window parameters such as 
// axis labels or normalization schemes.  Whether this happens is controlled
// by the 'do_restore_settings' flag.
void manage_plot_window_array( Fl_Widget *o, void* user_data)
{
  // Define an enumeration to hold a list of operation types
  enum operationType { INITIALIZE = 0, NEW_DATA, REFRESH_WINDOWS, RELOAD};
  
  // Define and initialize the operationType switch, old number of plots,
  // widget title, and pointers to the pMenu_ and pButton objects.
  operationType thisOperation = INITIALIZE;
  char widgetTitle[ 80];
  char userData[ 80];
  strcpy( widgetTitle, "");
  strcpy( userData, "");
  Fl_Menu_* pMenu_;
  Fl_Button* pButton;
  
  // Define and set flags and state variables to control the number of plots 
  // to be preserved and whether or not to restore their control panel
  // settings and positions
  int nplots_old = nplots;
  int do_restore_settings = 0;
  int do_restore_positions = 0;
  
  // Determine how the method was invoked, and set flags and parameters
  // accordingly.  If method was called with a NULL arguments, assume this 
  // is an initialization operation and set the old number of plots to zero, 
  // otherwise identify the argument type via a dynamic cast, extract the 
  // widget title, and set the old and new numbers of plots accordingly.
  // CASE 1: If the widget was NULL, this is an initialzation operation
  if( o == NULL) {
    thisOperation = INITIALIZE;
    nplots_old = 0;
  }

  // CASE 2: If this was an Fl_Menu_ widget, default to a resize operation,
  // then figure out what operation was requested and revise the switches
  // and array descriptions accordingly
  else if( (pMenu_ = dynamic_cast <Fl_Menu_*> (o))) {
    thisOperation = REFRESH_WINDOWS;
    nplots_old = nplots;
  
    // Get widget title and user data, and make absolutely sure that these
    // aren't NULL
    strcpy( widgetTitle, "");
    strcpy( userData, "");
    if( ((Fl_Menu_*) o)->text() != NULL) 
      strcpy( widgetTitle, ((Fl_Menu_*) o)->text());
    if( user_data != NULL) strcpy( userData, (char*) user_data);
  
    // Examine widget title and user data to determine behavior on a case by
    // case basis for reasons of clarity
    if( strncmp( userData, "NEW_DATA", 8) == 0) {
      thisOperation = NEW_DATA;
    }
    else if( strncmp( userData, "REFRESH_WINDOWS", 15) == 0) {
      thisOperation = REFRESH_WINDOWS;
      do_restore_settings = 1;
      do_restore_positions = 1;
    }
    else if( strncmp( widgetTitle, "Open", 4) == 0 ||
        strncmp( userData, "Open", 4) == 0 ||
        strncmp( widgetTitle, "Load", 4) == 0) {
      thisOperation = NEW_DATA;
      do_restore_settings = 1;
      do_restore_positions = 1;
    }
    else if( strncmp( widgetTitle, "Append", 6) == 0 ||
        strncmp( widgetTitle, "Merge", 5) == 0 ||
        strncmp( widgetTitle, "Clear all", 9) == 0) {
      thisOperation = NEW_DATA;
      do_restore_settings = 1;
      do_restore_positions = 1;
    }
    else if( strncmp( widgetTitle, "Add Row ", 8) == 0) {
      thisOperation = REFRESH_WINDOWS;
      nrows++;
    }
    else if( strncmp( widgetTitle, "Add Colu", 8) == 0) {
      thisOperation = REFRESH_WINDOWS;
      ncols++;
    }
    else if( strncmp( widgetTitle, "Remove R", 8) == 0 && nrows>1) {
      thisOperation = REFRESH_WINDOWS;
      nrows--;
    }
    else if( strncmp( widgetTitle, "Remove C", 8) == 0 && ncols>1) {
      thisOperation = REFRESH_WINDOWS;
      ncols--;
    }
    else if( strncmp( widgetTitle, "Reload F", 8) == 0) {
      thisOperation = REFRESH_WINDOWS;
      do_restore_settings = 1;
      do_restore_positions = 1;
      // Reset brushes because load resets brush sizes
      for( int j=0; j<NBRUSHES; j++) brushes[j]->reset();
    }
    else if( strncmp( widgetTitle, "Restore", 7) == 0) {
      thisOperation = REFRESH_WINDOWS;
      do_restore_settings = 1;
      do_restore_positions = 1;
    }
  
    // When reading new data, invoke Fl_Gl_Window.hide() (instead of the 
    // destructor!) to destroy all plot windows along with their context, 
    // including VBOs.  Then set nplots_old to zero because we'll need to
    // redraw all of these plots.  NOTE: If this is not done, and one
    // attempts to preserve some panels without redrawing them, VBO usage
    // could fail.
    if( thisOperation == NEW_DATA) {
      for( int i=0; i<nplots; i++) pws[i]->hide();
      nplots_old = 0;
    }
  }

  // CASE 3: If this was a button widget, assume it was a reload operation,
  // since no other buttons can invoke this method.  NOTE: This operation
  // doesn't work and is no longer supported!
  else if( (pButton = dynamic_cast <Fl_Button*> (o))) {
    thisOperation = RELOAD;
    nplots_old = nplots;
    strcpy( widgetTitle, ((Fl_Menu_*) o)->label());
    cerr << "manage_plot_window_array: WARNING, "
         << "RELOAD operation not supported!" << endl;
  }

  // DEFAULT: Default to a REFRESH_WINDOWS operation, in which all plots
  // are preserved with new settings, and warn the user.
  else {
    thisOperation = REFRESH_WINDOWS;
    nplots_old = nplots;
    strcpy( widgetTitle, "default");
    cerr << "manage_plot_window_array: WARNING, "
         << "defaulted to a REFRESH_WINDOW operation." << endl;
  }

  // DIAGNOSTIC
  // std::vector<std::string> diag_stuff;
  // diag_stuff.push_back( "INITIALIZE");
  // diag_stuff.push_back( "REFRESH_WINDOWS");
  // diag_stuff.push_back( "NEW_DATA");
  // diag_stuff.push_back( "RELOAD");
  // cout << "DIAGNOSTIC, manage_plot_window_array: widgetTitle(" << widgetTitle  
  //      << ") userData(" << userData << ")" << endl;
  // cout << "DIAGNOSTIC, manage_plot_window_array: thisOperation " 
  //      << thisOperation << " (" << diag_stuff[ thisOperation].c_str() 
  //      << ")" << endl;
  
  // Determine how many plot windows are available to be saved.  If this is an
  // INITIALIZE operation, set this number to zero.
  int nplots_save = nplots;
  if( thisOperation == INITIALIZE) nplots_save = 0;

  // Save an array of Plot_Window objecst with the positions of the existing 
  // plot windows.  NOTE: This must be an array rather than a pointer array 
  // to invoke the default constructor.
  Plot_Window pws_save[ MAXPLOTS+1];
  cout << "manage_plot_window_array: saving information for " << nplots_save 
       << " windows" << endl;
  for( int i=0; i<nplots_save; i++) {
    pws[i]->make_state();
    pws_save[i].copy_state( pws[i]);
  }
  
  // Save an array of Control_Panel_Window objects with axis, normalization, 
  // and transform style information of the existing control panels
  Control_Panel_Window cps_save[ MAXPLOTS+1];
  for( int i=0; i<nplots_save; i++) {
    cps[i]->make_state();
    cps_save[i].copy_state( cps[i]);
  }
  
  // Save an array of Brush objects with information of the existing brushes
  Brush brushes_save[ NBRUSHES];
  for( int i=0; i<NBRUSHES; i++) {
    brushes[i]->make_state();
    brushes_save[i].copy_state( brushes[i]);
  }

  // Recalculate number of plots
  nplots = nrows * ncols;

  // Clear children of the tab widget to delete old tabs
  cpt->clear();

  // Create and add the virtual sub-panels, each group under a tab, one
  // group per plot.
  for( int i=0; i<nplots; i++) {
    int row = i/ncols;
    int col = i%ncols;

    // Account for the 'borderless' option
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

    // Create a label for this tab
    ostringstream oss;
    oss << "" << i+1;
    string labstr = oss.str();

    // Set the pointer to the current group to the tab widget defined by
    // create_control_panel and add a new virtual control panel under this
    // tab widget
    Fl_Group::current( cpt);  
    cps[i] = new Control_Panel_Window(cp_widget_x, cp_widget_y, main_w - 6, cp_widget_h);
    cps[i]->index = i;
    cps[i]->copy_label( labstr.c_str());
    cps[i]->labelsize( 10);
    cps[i]->resizable( cps[i]);
    cps[i]->make_widgets( cps[i]);

    // End the group here so that we can create new plot windows at the top
    // level, then set the pointer to the current group to the top level.
    cps[i]->end();
    Fl_Group::current( 0); 

    // If this was an INITIALIZE, REFRESH_WINDOWS, or NEW_DATA operation,
    // then create or restore the relevant windows.  NOTE: If this code was 
    // executed during a reload operation (which is no longer supported!), 
    // it would cause a segmentation fault due to problems with the way the
    // shown() and hide() calls work.
    if( thisOperation == INITIALIZE || 
        thisOperation == REFRESH_WINDOWS || 
        thisOperation == NEW_DATA) {
      if( i >= nplots_old) {
        DEBUG(cout << "Creating new plot window " << i << endl);
        pws[i] = new Plot_Window( pw_w, pw_h, i);
        cps[i]->pw = pws[i];
        pws[i]->cp = cps[i];
      }
      else {
        pws[i]->index = i;
        cps[i]->pw = pws[i];
        pws[i]->cp = cps[i];
        pws[i]->size( pw_w, pw_h);
      }
      pws[i]->copy_label( labstr.c_str());
      pws[i]->position(pw_x, pw_y);
      pws[i]->row = row; 
      pws[i]->column = col;
      pws[i]->end();
    }

    // Always link the plot window and its associated virtual control panel
    assert ((pws[i]->index == i) && (cps[i]->index == i));
    cps[i]->pw = pws[i];
    pws[i]->cp = cps[i];

    // Always invoke Plot_Window::upper_triangle_incr to determine which 
    // variables to plot in new panels.
    int ivar, jvar;
    if( i==0) {
      ivar = 0;
      jvar = 1;
      
      // If this is an initialize operation, then the plot window array is being 
      // created, the tabs should come up free of context.  It also might be
      // desirable that the first plot's tab be shown with its axes locked.
      if( thisOperation == INITIALIZE) {
        cps[i]->hide();  
      }
    }
    else Plot_Window::upper_triangle_incr( ivar, jvar, nvars);

    // If there has been an explicit request to restore the saved control 
    // panel settings or the number of plots has changed, restore those 
    // settings, then generate any new settings that may be required.
    if( do_restore_settings != 0 ||
        nplots != nplots_old && i<nplots_old) {

      // Make sure axis indices are in range
      cps_save[i].restrict_axis_indices( nvars-1, nvars-1, nvars);

      cps[i]->copy_state( &cps_save[i]);
      cps[i]->load_state();
    } 
    else {
      cps[i]->varindex1->value(ivar);  
      cps[i]->varindex2->value(jvar);  
      cps[i]->varindex3->value(nvars);  
    }

    // KLUDGE: If there has been an explicit request to restore the saved
    // control panel settings or the number of plots has changed, restore the 
    // brush settings, but only do this once, during the first iteration.
    // XXX PRG: This seems unreliable.  Is there a better place to put this?
    if( i==0 && (do_restore_settings != 0 || nplots != nplots_old)) {
      for( int j=0; j<NBRUSHES; j++) {
        brushes[i]->copy_state( &brushes_save[i]);
        brushes[i]->load_state();
      }
    }

    // If this is an INITIALIZE, REFRESH_WINDOWS, or NEW_DATA operation, 
    // test for missing data, extract data, reset panels, and make them 
    // resizable.  Otherwise it must be a RELOAD operation (which is no
    // longer supported!), and we must invoke the relevant Plot_Window 
    // member functions to initialize and draw panels.
    if( thisOperation == INITIALIZE || 
        thisOperation == REFRESH_WINDOWS || 
        thisOperation == NEW_DATA) {
      if( npoints > 1) {
        pws[i]->extract_data_points();
        pws[i]->reset_view();
      }
      pws[i]->size_range( 10, 10);
      pws[i]->resizable( pws[i]);
    }
    else {   // RELOAD no longer supported!
      pws[i]->initialize();
      pws[i]->extract_data_points();
    }

    // KLUDGE: If this is a "append", "merge", "reload file" or "restore 
    // panels" operation, restore old plot window positions.  This should 
    // be controlled by a flag rather than examining WIDGETTITLE.
    // if( strncmp( userData, "REFRESH_WINDOWS", 15) == 0 ||
    //     strncmp( widgetTitle, "Append", 6) == 0 ||
    //     strncmp( widgetTitle, "Merge", 5) == 0 ||
    //     strncmp( widgetTitle, "Reload", 6) == 0 ||
    //     strncmp( widgetTitle, "Restore", 7) == 0) {
    if( do_restore_positions != 0) {
      for( int i=0; i<nplots_save; i++) {
        pws[ i]->copy_state( &pws_save[i]);
        pws[ i]->load_state();
        
        // DIAGNOSTIC
        // cout << "  window[ " << i << "/" << nplots_old
        //      << "]: ( " << pws[ i]->x()
        //      << ", " << pws[ i]->y()
        //      << ", " << pws[ i]->w()
        //      << ", " << pws[ i]->h() << ")" << endl;
      }
    }
    
    // Account for the 'borderless' option
    if( borderless) pws[i]->border(0);

    // Make sure the window has been shown and check again to make absolutely 
    // sure it is resizable.  NOTE: pws[i]->show() with no arguments is not 
    // sufficient when windows are created.
    if( !pws[i]->shown()) {
      DEBUG(cout << "showing plot window " << i << endl);
        pws[i]->show( global_argc, global_argv);
    }
    pws[i]->resizable( pws[i]);

    // Turn on the 'show' capability of Plot_Window::reset_view();
    pws[i]->do_reset_view_with_show = 1;
  }

  // Set the color arrays to make sure points get drawn.
  pws[0]->color_array_from_selection();
  
  // Invoke Fl_Gl_Window::hide() (rather than the destructor, which may
  // produce strange behavior) to rid of any superfluous plot windows
  // along with their contexts.
  if( nplots < nplots_old)
    for( int i=nplots; i<nplots_old; i++) pws[i]->hide();
  
  // Create a master control panel to encompass all the tabs
  create_broadcast_group ();
}

//***************************************************************************
// cb_manage_plot_window_array( o) -- Idle callback to see if the plot 
// windows need to be refreshed.
void cb_manage_plot_window_array( void* o)
{
  // Examine and set flags
  if( dfm.needs_restore_panels() <= 0) return;
  dfm.needs_restore_panels( 0);

  // DIAGNOSTIC
  cout << "MAIN::cb_manage_plot_window_array: DIAGNOSTIC "
       << "manage_plot_window_array invoked via callback" << endl;

  // NOTE: We need some way to revise the window scales.  Is there also
  // some way to avoid chosing random axes from the reduced set?
  // for( int i=0; i<nplots; i++) pws[i]->extract_data_points();

  // KLUDGE: make manage_plot_window_array think it's called from a menu.  
  // manage_plot_window_array( main_menu_bar, (void*) "REFRESH_WINDOWS");
  manage_plot_window_array( main_menu_bar, (void*) "NEW_DATA");
}

//***************************************************************************
// make_main_menu_bar() -- Make main menu bar.  NOTE: because the FLTK 
// documentation recommends against manipulating the Fl_Menu_Item array 
// directly, this is done via the add() method of Fl_Menu_.
void make_main_menu_bar()
{
  // Instantiate the Fl_Menu_Bar object
  main_menu_bar =
    new Fl_Menu_Bar( 0, 0, main_w, 25);

  // Add File menu items.  NOTE: In some cases, the values of the title 
  // and user_data fields of the main_menu_bar object may be used to control 
  // the behavior of the manage_plot_window_array method.
  main_menu_bar->add( 
    "File/Open data file       ", 0, 
    (Fl_Callback *) read_data, (void*) "open data file");
  main_menu_bar->add( 
    "File/Append more data     ", 0, 
    (Fl_Callback *) read_data, (void*) "append more data");
  main_menu_bar->add( 
    "File/Merge another file   ", 0, 
    (Fl_Callback *) read_data, (void*) "merge another file", FL_MENU_DIVIDER);
  main_menu_bar->add( 
    "File/Save all data        ", 0, 
    (Fl_Callback *) write_data, (void*) "save data");
  main_menu_bar->add( 
    "File/Save selected data   ", 0, 
    (Fl_Callback *) write_data, (void*) "save selected data", FL_MENU_DIVIDER);
  main_menu_bar->add( 
    "File/Load configuration   ", 0, 
    (Fl_Callback *) load_state);
  main_menu_bar->add( 
    "File/Save configuration   ", 0, 
    (Fl_Callback *) save_state, 0, FL_MENU_DIVIDER);
  main_menu_bar->add( 
    "File/Clear all data       ", 0, 
    (Fl_Callback *) read_data, (void*) "clear all data");
  main_menu_bar->add( 
    "File/Quit   ", 0, (Fl_Callback *) exit);

  // Add View menu items
  main_menu_bar->add( 
    "View/File Name    ", 0, 
    (Fl_Callback *) make_file_name_window, 0, FL_MENU_DIVIDER);
  main_menu_bar->add( 
    "View/Add Row   ", 0, 
    (Fl_Callback *) manage_plot_window_array);
  main_menu_bar->add( 
    "View/Add Column   ", 0, 
    (Fl_Callback *) manage_plot_window_array);
  main_menu_bar->add( 
    "View/Remove Row   ", 0, 
    (Fl_Callback *) manage_plot_window_array);
  main_menu_bar->add( 
    "View/Remove Column   ", 0, 
    (Fl_Callback *) manage_plot_window_array, 0, FL_MENU_DIVIDER);
  main_menu_bar->add( 
    "View/Reload File     ", 0, 
    (Fl_Callback *) read_data, (void*) "reload file");
  main_menu_bar->add( 
    "View/Restore Panels  ", 0, 
    (Fl_Callback *) manage_plot_window_array);
  main_menu_bar->add( 
    "View/Default Panels ", 0, 
    (Fl_Callback *) manage_plot_window_array);

  // Add Tools menu items
  main_menu_bar->add( 
    "Tools/Edit Column Labels ", 0, 
    (Fl_Callback *) dfm.edit_column_info);
  main_menu_bar->add( 
    "Tools/Options            ", 0, 
    // (Fl_Callback *) make_options_window, 0, FL_MENU_INACTIVE);
    (Fl_Callback *) make_options_window);

  // Add Help menu items
  main_menu_bar->add( 
    "Help/Viewpoints Help   ", 0, (Fl_Callback *) make_help_view_window);
  main_menu_bar->add( 
    "Help/About   ", 0, (Fl_Callback *) make_help_about_window);
  
  // Set colors, fonts, etc
  main_menu_bar->color( FL_BACKGROUND_COLOR);
  main_menu_bar->textfont( FL_HELVETICA);
  main_menu_bar->textsize( 14);
  main_menu_bar->down_box( FL_FLAT_BOX);
  main_menu_bar->selection_color( FL_SELECTION_COLOR);
  
  // This example is included to illustrate how awkward it can  be to access 
  // elements of the Fl_Menu_Item array directly.
  // for( int i=0; i<main_menu_bar->size(); i++) {
  //   Fl_Menu_Item *pMenuItem = 
  //     (Fl_Menu_Item*) &(main_menu_bar->menu()[i]);
  //   pMenuItem->labelsize(32);
  // }
}

//***************************************************************************
// make_file_name_window( *o) -- Create the 'View|File Name' window.
void make_file_name_window( Fl_Widget *o)
{
  string sConfirmText = dfm.input_filespec();
  if( sConfirmText.size()<=0) sConfirmText = "No input file is specified.";
  int iConfirmResult = make_confirmation_window( sConfirmText.c_str(), 1);
}

//***************************************************************************
// make_options_window( *o) -- Create the 'Tools|Options' window.
void make_options_window( Fl_Widget *o)
{
  if( options_window != NULL) options_window->hide();
   
  // Create Tools|Options window
  Fl::scheme( "plastic");  // optional
  options_window = new Fl_Window( 300, 200, "Options");
  options_window->begin();
  options_window->selection_color( FL_BLUE);
  options_window->labelsize( 10);
  
  // Write text to box label and align it inside box
  // Fl_Box* options_box = new Fl_Box( 5, 5, 290, 360);
  // box->box( FL_NO_BOX);
  // options_window->resizable( box);
  options_window->resizable( NULL);

  // Expert mode checkbox
  {
    Fl_Check_Button* o = expertButton = 
      new Fl_Check_Button( 10, 10, 150, 20, " Expert Mode");
    o->down_box( FL_DOWN_BOX);
    o->value( expert_mode == true);
    o->tooltip( "Suppress confirmation windows");
  }
  
  // Remove trivial columns
  {
    Fl_Check_Button* o = trivialColumnsButton = 
      new Fl_Check_Button( 10, 35, 250, 20, " Remove Trivial Columns");
    o->down_box( FL_DOWN_BOX);
    o->value( trivial_columns_mode == true);
    o->tooltip( "Remove any columns that only contain one value");
  }
  
  // Preserve Old Data
  {
    Fl_Check_Button* o = preserveOldDataButton = 
      new Fl_Check_Button( 10, 60, 250, 20, " Preserve Old Data");
    o->down_box( FL_DOWN_BOX);
    o->value( preserve_old_data_mode == true);
    o->tooltip( "Preserve old data for restoration if a read operation fails");
  }
  
  // Bad value proxy field
  {
    Fl_Input* o = bad_value_proxy_input =
      new Fl_Input( 10, 85, 70, 20, " Bad Value Proxy");
    o->align( FL_ALIGN_RIGHT);
    stringstream ss_float;
    string s_float;
    ss_float << dfm.bad_value_proxy();
    ss_float >> s_float;
    o->value( s_float.c_str());
    o->tooltip( "Value used for missing or bad data");
  }

  // Invoke a multi-purpose callback function to process window
  Fl_Button* ok_button = new Fl_Button( 150, 170, 40, 25, "&OK");
  ok_button->callback( (Fl_Callback*) cb_options_window, ok_button);
  Fl_Button* cancel = new Fl_Button( 200, 170, 60, 25, "&Cancel");
  cancel->callback( (Fl_Callback*) cb_options_window, cancel);

  // Done creating the 'Help|About' window
  // options_window->resizable( options_window);
  options_window->resizable( NULL);
  options_window->end();
  options_window->show();
}

//***************************************************************************
// cb_options_window( *o, *user_data) -- Callback function to process the
// Options window.  It is assumed that the necessary pointers will be 
// passed as USER_DATA.  WARNING: No error checking is done on USER_DATA!
void cb_options_window( Fl_Widget *o, void* user_data)
{
  if( user_data == NULL) {
    options_window->hide();
    return;
  }

  if( strcmp( ((Fl_Widget*) user_data)->label(), "&OK") == 0) {
    int i_expert_mode = expertButton->value();
    prefs_.set( "expert_mode", i_expert_mode);
    expert_mode = ( i_expert_mode != 0);

    int i_trivial_columns_mode = trivialColumnsButton->value();
    prefs_.set( "trivial_columns_mode", i_trivial_columns_mode);
    trivial_columns_mode = ( i_trivial_columns_mode != 0);

    int i_preserve_old_data_mode = preserveOldDataButton->value();
    prefs_.set( "preserve_old_data_mode", i_preserve_old_data_mode);
    preserve_old_data_mode = ( i_preserve_old_data_mode != 0);

    float bad_value_proxy = strtof( bad_value_proxy_input->value(), NULL);
    dfm.bad_value_proxy( bad_value_proxy);
  }
  options_window->hide();
}

//***************************************************************************
// make_help_view_window( *o) -- Create the 'Help|Help' window.
void make_help_view_window( Fl_Widget *o)
{
  if( help_view_window != NULL) help_view_window->hide();
  
  // Create Help|Help window
  Fl::scheme( "plastic");  // optional
  help_view_window = new Fl_Window( 600, 500, "Viewpoints Help");
  help_view_window->begin();
  help_view_window->selection_color( FL_BLUE);
  help_view_window->labelsize( 10);

  // Define Fl_Help_View widget
  help_view_widget = new Fl_Help_View( 5, 5, 590, 450, "");
  (void) help_view_widget->load( "vp_help_manual.htm");
  help_view_widget->labelsize( 14);
  help_view_widget->textsize( 14);
  help_topline = help_view_widget->topline();

  // Invoke callback functions to change text size
  Fl_Box* fontsize_box = new Fl_Box( 15, 465, 60, 25, "Font Size:"); 
  fontsize_box->align( FL_ALIGN_CENTER);
  Fl_Button* shrink_font = new Fl_Button( 80, 465, 25, 25, "-");
  shrink_font->callback( (Fl_Callback*) textsize_help_view_widget, (void*) -2);
  Fl_Button* grow_font = new Fl_Button( 110, 465, 25, 25, "+");
  grow_font->callback( (Fl_Callback*) textsize_help_view_widget, (void*) 2);
  
  // Invoke callback function to move through help_view widget
  int help_height = help_view_widget->h();
  help_height = 3*help_height/4;
  Fl_Button* back = new Fl_Button( 325, 465, 70, 25, "&Back");
  back->callback( (Fl_Callback*) step_help_view_widget, (void*) -help_height);
  Fl_Button* fwd = new Fl_Button( 400, 465, 70, 25, "&Fwd");
  fwd->callback( (Fl_Callback*) step_help_view_widget, (void*) help_height);

  // Invoke callback function to close window
  Fl_Button* close = new Fl_Button( 500, 465, 70, 25, "&Close");
  close->callback( (Fl_Callback*) close_help_window, help_view_window);

  // Done creating the 'Help|Help' window
  help_view_window->resizable( help_view_window);
  help_view_window->end();
  help_view_window->show();
}

//***************************************************************************
// textsize_help_view_window( *o, *user_data) -- Step through the 'Help|Help' 
// window.
void textsize_help_view_widget( Fl_Widget *o, void* user_data)
{
  int help_textsize = help_view_widget->textsize() + (int) user_data;
  if( help_textsize < 8) help_textsize=8;
  help_view_widget->textsize( help_textsize);
}

//***************************************************************************
// close_help_window( *o, *user_data) -- Callback function to close a Help 
// window.  It is assumed that a pointer to the window will be passed as 
// USER_DATA.  WARNING: No error checking is done on USER_DATA!
void close_help_window( Fl_Widget *o, void* user_data)
{
  ((Fl_Window*) user_data)->hide();
}

//***************************************************************************
// step_help_view_window( *o, *user_data) -- Step through the 'Help|Help' 
// window.
void step_help_view_widget( Fl_Widget *o, void* user_data)
{
  help_topline += (int) user_data;
  if( help_topline < 0) help_topline=0;
  help_view_widget->topline( help_topline);
}

//***************************************************************************
// make_global_widgets() -- Make controls for main control panel
void make_global_widgets()
{
  int xpos = global_widgets_x, ypos = global_widgets_y;
  // Define a pointer and initialize positions for buttons
  Fl_Button *b;
  int xpos1 = xpos, ypos1 = ypos;

  // Button(1,1): Show nonselected points (on by default)
  show_deselected_button = b = new Fl_Button( xpos, ypos+=25, 20, 20, "show nonselected");
  b->align( FL_ALIGN_RIGHT); 
  b->selection_color( FL_BLUE); 
  b->type( FL_TOGGLE_BUTTON);
  b->value( 1);
  b->callback( (Fl_Callback*) Plot_Window::toggle_display_deselected);
  b->tooltip("toggle visibility of nonselected points in all plots");

  // Button(2,1): mask out deselected
  mask_out_deselected = b = new Fl_Button( xpos, ypos+=25, 20, 20, "mask nonselected");
  b->type(FL_TOGGLE_BUTTON);
  b->align( FL_ALIGN_RIGHT); 
  b->selection_color( FL_BLUE); 
  b->tooltip("don't select nonselected points");

  // Button(3,1): Invert selected and nonselected data
  invert_selection_button = b = new Fl_Button( xpos, ypos+=25, 20, 20, "invert selection");
  b->align( FL_ALIGN_RIGHT); 
  b->selection_color( FL_BLUE); 
  b->callback( (Fl_Callback*) Plot_Window::invert_selection);
  b->tooltip("invert selection status of all points");

  // Button(4,1): Clear all selections
  clear_selection_button = b = new Fl_Button(xpos, ypos+=25, 20, 20, "clear selections");
  b->align( FL_ALIGN_RIGHT); 
  b->selection_color( FL_BLUE); 
  b->callback( Plot_Window::clear_selections);
  b->tooltip("clear all selections");

  // Button(5,1): Delete selected data
  delete_selection_button = b = new Fl_Button( xpos, ypos+=25, 20, 20, "kill selected");
  b->align( FL_ALIGN_RIGHT); 
  b->selection_color( FL_BLUE); 
  b->callback( Plot_Window::delete_selection);
  b->tooltip("remove selected points completely (does not change data on disk)");

  // Advance to column 2
  xpos = xpos1 + 150; ypos = ypos1;

  // Button(3,2): permute all unlocked axes
  change_all_axes_button = new Fl_Repeat_Button( xpos, ypos+=25, 20, 20, "change axes");
  change_all_axes_button->align( FL_ALIGN_RIGHT); 
  change_all_axes_button->selection_color( FL_BLUE); 
  change_all_axes_button->callback( (Fl_Callback*)change_all_axes);
  change_all_axes_button->tooltip("automatically choose new axes for all plots");

  // Button(4,2): Link all axes
  link_all_axes_button = b = new Fl_Button( xpos, ypos+=25, 20, 20, "link axes");
  b->align( FL_ALIGN_RIGHT); 
  b->selection_color( FL_BLUE); 
  b->type( FL_TOGGLE_BUTTON); 
  b->value( 0);
  b->tooltip("toggle linked viewing transormations");

  // Button(5,2): defer redraws (for huge data)
  defer_redraws_button = b = new Fl_Button( xpos, ypos+=25, 20, 20, "defer redraws");
  b->align( FL_ALIGN_RIGHT); 
  b->selection_color( FL_BLUE); 
  b->type( FL_TOGGLE_BUTTON); 
  b->value( 0);
  b->tooltip("update selections on mouse-up only (for large data)");
}

//***************************************************************************
// change_all_axes( *o) -- Invoke the change_axes method of each Plot_Window 
// to change all unlocked axes.
void change_all_axes( Fl_Widget *o)
{
  // Loop: Examine successive plots and change the axes of those for which 
  // the x or y axis is unlocked.
  for( int i=0; i<nplots; i++) {
    if( !( cps[i]->lock_axis1_button->value() && 
           cps[i]->lock_axis2_button->value()))
      pws[i]->change_axes( 0);
  }
  Plot_Window::redraw_all_plots(0);
}

//***************************************************************************
// npoints_changed( 0) -- Examine slider widget to determine the new number 
// of points, then invoke a static method of class Plot_Window to redraw all 
// plots.
void npoints_changed( Fl_Widget *o) 
{
  npoints = int( ( (Fl_Slider *)o)->value());
  Plot_Window::redraw_all_plots( 0);
}

//***************************************************************************
// write_data( o) -- Write data widget.  Invoked by main control panel.  
// Invokes the write method of class data_file_manager to write a data file.
void write_data( Fl_Widget *o, void* user_data)
{
  // Evaluate user_data to get ASCII or binary file format
  // if( strstr( (char *) user_data, "binary") != NULL) dfm.ascii_output( 0);
  // else dfm.ascii_output( 1);

  // Evaluate user_data to determine if only selected data are to be used
  if( strstr( (char *) user_data, "selected") != NULL) dfm.write_all_data( 0);
  else dfm.write_all_data( 1);

  // Query user to find name of output file.  If no file was specified, 
  // return immediately and hope the calling routine can handle this.
  int iQueryStatus = 0;
  iQueryStatus = dfm.findOutputFile();
  if( iQueryStatus != 0) {
    cout << "No output file was selected" << endl;
    return;
  }
  
  // Invoke the data file manager to save the file or fail gracefully
  dfm.save_data_file();
}

//***************************************************************************
// reset_all_plots() -- Reset all plots.  Invoked by main control panel.
void reset_all_plots()
{
  for( int i=0; i<nplots; i++) pws[i]->reset_view();
}

//***************************************************************************
// read_data( o, user_data) -- Widget that invokes the read method of class
// data_file_manager to open and read data from a file.
void read_data( Fl_Widget* o, void* user_data)
{
  // Begin by assuming that the input file is ASCII
  // dfm.ascii_input( 1);

  // Evaluate USER_DATA to get the operation type
  if( strstr( (char *) user_data, "append") != NULL) dfm.do_append( 1);
  else dfm.do_append( 0);
  if( strstr( (char *) user_data, "merge") != NULL) dfm.do_merge( 1);
  else dfm.do_merge( 0);

  // If this was a "Clear all data" operation, query the user and set the
  // doCleaarAllData flag.  If this was a "Reload File" operation and we 
  // know the name of the data file, do nothing.  Otherwise invoke the 
  // findInputFile() method of the data_file_manager object to query the 
  // user for the input filename.  If no file is specified, return 
  // immediately and hope the calling routine can handle this situation.
  unsigned doClearAllData = 0;
  if( strstr( (char *) user_data, "clear") != NULL) {
    if( !expert_mode &&
        make_confirmation_window( "Clear all data?  Are you sure?") <= 0)
      return;
    doClearAllData = 1;
  }
  else if( ( strstr( (char *) user_data, "reload") != NULL) &&
      (dfm.input_filespec()).length() > 0) {
  }
  else{
    int iQueryStatus = 0;
    iQueryStatus = dfm.findInputFile();
    if( iQueryStatus != 0) {
      cout << "No input file was selected" << endl;
      return;
    }
  }

  // If the doClearAllData flag is set, invoke the create_default_data()
  // method of the Data_File_Manager object to generate default data, 
  // otherwise invoke the load_data_file() method to read the data file.  
  // Error reporting is handled by the relevant methods.
  if( doClearAllData) dfm.create_default_data( 10);
  else dfm.load_data_file();

  // If only one or fewer records are available, quit before something 
  // terrible happens!
  if( npoints <= 1) {
    cout << "Insufficient data, " << npoints
         << " samples.  Loading default data." << endl;
    dfm.create_default_data( 10);
  }
  else {
    cout << "Loaded " << npoints
         << " samples with " << nvars << " fields" << endl;
  }
  
  // Resize the slider
  // npoints_slider->bounds(1,npoints);
  // npoints_slider->value(npoints);

  // MCL XXX why is the following code repeated three times in this file?
  // Fewer points -> bigger starting default_pointsize
  default_pointsize = max( 1.0, 6.0 - log10f( (float) npoints));
  Brush::set_sizes(default_pointsize);

  // DIAGNOSTIC
  // cout << "Finished dfm.load_data_file and about to refresh plots" << endl;
  // char ans;
  // cin >> ans;
  
  // Clear children of the tab widgets and reload the plot window array.
  manage_plot_window_array( o, NULL);
}

//***************************************************************************
// load_initial_state( configFileSpec) -- Use BOOST serialization to load 
// the initial saved state from an XML archive.  NOTE: For reasons related
// to the configuration filespec and manage_plot_window_array, this must be 
// handled differently from the ordinary load_state operation.  NOTE: Since 
// BOOST reads configuration files as sequential files rather than XML, the 
// order and number of the load and save operations is extremely important, 
// and this is noted explictly in the comments below.
int load_initial_state( string configFileSpec)
{
  // Create dummy menu for use with manage_plot_window_array.  Instantiate 
  // an actual object to avoid potential dangers of pointers.
  Fl_Menu_Bar dummy_menu( 0, 0, 1, 1);

  // Create a file stream for input and make sure it exists.  This will
  // be closed when destructors are called.  NOTE: If this ASSERT triggers,
  // something has gone badly wrong in the WHILE loop above.
  std::ifstream inputFileStream( configFileSpec.c_str(), std::ios::binary);
  assert( inputFileStream.good());

  // Install most of the load procedure in a try-catch process to protect
  // against corrupt serialization files
  try {
    // Create and open an archive for input.  As before, this will be
    // closed when destructors are called
    boost::archive::xml_iarchive inputArchive( inputFileStream);

    // STEP 1/8) Begin by checking the serialization file version to see if 
    // it's supported
    inputArchive >> BOOST_SERIALIZATION_NVP( serialization_file_version);
    if( serialization_file_version < last_supported_serialization_version) throw 0;
    
    // STEP 2/8) Get data file from archive and read it
    inputArchive >> BOOST_SERIALIZATION_NVP( dfm);
    if( dfm.input_filespec().length() <= 0) dfm.create_default_data( 10);
    else {
      if( dfm.load_data_file() != 0) dfm.create_default_data( 10);
      else 
        cout << "Loaded " << npoints
             << " samples with " << nvars << " fields" << endl;
    }

    // Fewer points -> bigger starting default_pointsize
    default_pointsize = max( 1.0, 6.0 - log10f( (float) npoints));
    Brush::set_sizes(default_pointsize);
  
    // STEPS 3/8 and 4/8) Read configuration information from archive.  
    // NOTE: Must be done before first call to MANAGE_PLOT_WINDOW_ARRAY!
    inputArchive >> BOOST_SERIALIZATION_NVP( nrows);
    inputArchive >> BOOST_SERIALIZATION_NVP( ncols);

    // Set user_data for this widget to indicate that this is a NEW_DATA 
    // operation, then invoke MANAGE_PLOT_WINDOW_ARRAY to clear children of 
    // the tab widget and reload the plot window array.
    // manage_plot_window_array( o, (void*) "NEW_DATA");
    manage_plot_window_array( &dummy_menu, (void*) "NEW_DATA");

    // STEP 5/8) Read NPLOTS from the archive to make sure it is consistent 
    // with the stored control panel and plot window information.  NOTE: If 
    // something went wrong with MANAGE_PLOT_WINDOW_ARRAY, this could cause 
    // problems.
    inputArchive >> BOOST_SERIALIZATION_NVP( nplots);

    // STEPS 6/8 and 7/8) Define temporary arrays, load these with 
    // serialization information, then transfer this information to the 
    // actual arrays.  NOTE: This awkward procedure is imposed by the 
    // limitations of the constructors for these classes.
    Control_Panel_Window *cps_input[ MAXPLOTS+1]; 
    inputArchive >> BOOST_SERIALIZATION_NVP( cps_input);
    for( int i=0; i<nplots; i++) {
      cps[i]->copy_state( cps_input[ i]);
      cps[i]->load_state();
    }
    Plot_Window *pws_input[ MAXPLOTS+1]; 
    inputArchive >> BOOST_SERIALIZATION_NVP( pws_input);
    for( int i=0; i<nplots; i++) {
      pws[i]->copy_state( pws_input[ i]);
      pws[i]->load_state();
    }

    // STEP 8/8) Do the same thing for brushes
    Brush *brushes_input[ NBRUSHES];
    inputArchive >> BOOST_SERIALIZATION_NVP( brushes_input);
    for( int i=0; i<NBRUSHES; i++) {
      brushes[i]->copy_state( brushes_input[ i]);
      brushes[i]->load_state();
    }
        
    // Set user_data to indicate that this is a RESIZE operation, then i
    // invoke manage_plot_window( o) to apply configuration.
    // manage_plot_window_array( o, (void*) "REFRESH_WINDOWS");
    manage_plot_window_array( &dummy_menu, (void*) "REFRESH_WINDOWS");
  }
  catch( int i_thrown) {
    string sWarning = "";
    sWarning.append( "WARNING: Configuration file is an older or\n");
    sWarning.append( "unsupported version that cannot be loaded");
    make_confirmation_window( sWarning.c_str(), 1);
    cerr << "Main::load_initial_state: WARNING, "
         << "Unsupported version of configuration file" << endl;
    cerr << "       load_initial_state reports exception (" << i_thrown
         << ")" << endl;

    // manage_plot_window_array( main_menu_bar, (void*) "NEW_DATA");
    manage_plot_window_array( &dummy_menu, (void*) "NEW_DATA");
    return 0;
  }
  catch( exception &e) {
    string sWarning = "";
    sWarning.append( "WARNING: load_initial_state reports an exception.\n");
    sWarning.append( "Configuration file appears to be damaged or\n");
    sWarning.append( "obsolete.  Results may be unpredictable");
    make_confirmation_window( sWarning.c_str(), 1, 3);
    cerr << "Main::load_initial_state: WARNING, "
         << "problem loading serialization file" << endl;
    cerr << "      load_initial_state reports exception (" << e.what()
         << ")" << endl;

    // manage_plot_window_array( main_menu_bar, (void*) "NEW_DATA");
    manage_plot_window_array( &dummy_menu, (void*) "NEW_DATA");
    return 0;
  }
  
  // If this is an older serialization file, query the user to determine if
  // they wish to resave it as a newer version
  if( serialization_file_version < current_serialization_version) {
    string sWarning = "";
    sWarning.append( "WARNING: Configuration file appears to be an\n");
    sWarning.append( "older version that is only partially supported.\n");
    sWarning.append( "Do you wish to resave a new version?");

    // Note the use of &dummy_menu to make save_state( *o) work
    // make_confirmation_window( sWarning.c_str(), 1, 3);
    if( make_confirmation_window( sWarning.c_str(), 2, 3) == 1) {
      save_state( &dummy_menu);
    }
  }

  // Report success
  return 1;
}

//***************************************************************************
// load_state( o) -- Use BOOST serialization to load a saved state from an
// XML archive.  NOTE: Since BOOST reads these as sequential files rather
// than XML, the order and number of the load and save operations is 
// extremely important, and this noted explicetly in the comments below.
int load_state( Fl_Widget* o)
{
  // Extract directory from the Data_File_Manager class to initialize the 
  // filespec for the XML archive.  NOTE: cInFileSpec is defined as const 
  // char* for use with Vp_File_Chooser, which means it could be destroyed 
  // by the relevant destructors!
  const char *cInFileSpec = dfm.directory().c_str();
  // const char *cInFileSpec = strcat( dfm.directory().c_str(), "vp.xml");

  // Instantiate and show an Vp_File_Chooser widget and switch it to the
  // Configuration File Mode.  NOTE: The pathname must be passed as a 
  // variable or the window will begin in some root directory.
  char* title = "Load saved configuration from file";
  char* pattern = "*.xml\tAll Files (*)";
  Vp_File_Chooser* file_chooser =
    new Vp_File_Chooser( cInFileSpec, pattern, Vp_File_Chooser::SINGLE, title);
  file_chooser->directory( cInFileSpec);
  file_chooser->isConfigFileMode( 1);

  // Loop: wait until the file selection is done.  NOTE: This version 
  // doesn't work and is retained only for archival purposes
  // file_chooser->show();
  // while( file_chooser->shown()) Fl::wait();
  // cInFileSpec = file_chooser->value();   

  // Loop: Set the directory, then select fileSpecs until a non-directory is 
  // obtained.  NOTE: This should all be handled by the file_chooser object, 
  // but it's necessary to add some protection in case the user selects a
  // pathname that does not correspnd to a file.
  while( 1) {
    if( cInFileSpec != NULL) file_chooser->directory( cInFileSpec);

    // Loop: wait until the file selection is done
    file_chooser->show();
    while( file_chooser->shown()) Fl::wait();
    cInFileSpec = file_chooser->value();   

    // If no file was specified then quit
    if( cInFileSpec == NULL) {
      cerr << "Main::load_state: "
           << "No input file was specified" << endl;
      break;
    }

    // In FLTK 1.1.7 under Windows, the fl_filename_isdir method doesn't work, 
    // so try to open this file to see if it is a directory.  If it is, set 
    // the pathname and continue.  Otherwise merely update the pathname stored
    // in the Data_File_Manager object.
    FILE* pFile = fopen( cInFileSpec, "r");
    if( pFile == NULL) {
      file_chooser->directory( cInFileSpec);
      dfm.directory( (string) cInFileSpec);
      continue;
    }
    else {
      dfm.directory( (string) file_chooser->directory());
    }

    fclose( pFile);
    break;         
  } 
  int isConfigOnly = file_chooser->isConfigOnly();

  // If no file was specified then report, deallocate the Vp_File_Chooser 
  // object, and quit.
  if( cInFileSpec == NULL) {
    cerr << "Main::load_state: "
         << "No input file was specified" << endl;
    delete file_chooser;  // WARNING! Destroys cInFileSpec!
    return -1;
  }

  // Print empty line to console for reasons of aesthetics
  cout << endl;

  // Create a file stream for input and make sure it exists.  This will
  // be closed when destructors are called.  NOTE: If this ASSERT triggers,
  // something has gone badly wrong in the WHILE loop above.
  std::ifstream inputFileStream( cInFileSpec, std::ios::binary);
  assert( inputFileStream.good());

  // Install most of the load procedure in a try-catch process to protect
  // against corrupt serialization files
  try {
    // Create and open an archive for input.  As before, this will be
    // closed when destructors are called
    boost::archive::xml_iarchive inputArchive( inputFileStream);

    // STEP 1/8) Begin by checking the serialization file version to see if 
    // it's supported
    inputArchive >> BOOST_SERIALIZATION_NVP( serialization_file_version);
    if( serialization_file_version < last_supported_serialization_version) throw 0;
    
    // STEP 2/8) Copy existing state, if any, of the data file manager, then
    // get file i/o information from archive and read input file
    Data_File_Manager dfm_save;
    dfm_save.copy_state( &dfm);
    int nvars_save = nvars;
    int npoints_save = npoints;
    inputArchive >> BOOST_SERIALIZATION_NVP( dfm);
    if( isConfigOnly) {
      dfm.copy_state( &dfm_save);
      nvars = nvars_save;
      npoints = npoints_save;
      nvars = (dfm.column_info).size()-1;
      npoints = ((dfm.column_info[0]).points).rows();
    }
    else if( dfm.input_filespec().length() <= 0) dfm.create_default_data( 10);
    else {
      if( dfm.load_data_file() != 0) dfm.create_default_data( 10);
      else 
        cout << "Loaded " << npoints
             << " samples with " << nvars << " fields" << endl;
    }

    // Fewer points -> bigger starting default_pointsize
    default_pointsize = max( 1.0, 6.0 - log10f( (float) npoints));
    Brush::set_sizes(default_pointsize);
  
    // STEPS 3/8 and 4/8) Read configuration information from archive.  
    // NOTE: Must be done before first call to MANAGE_PLOT_WINDOW_ARRAY!
    inputArchive >> BOOST_SERIALIZATION_NVP( nrows);
    inputArchive >> BOOST_SERIALIZATION_NVP( ncols);

    // Set user_data for this widget to indicate that this is a NEW_DATA 
    // operation, then invoke MANAGE_PLOT_WINDOW_ARRAY to clear children of 
    // the tab widget and reload the plot window array.
    manage_plot_window_array( o, (void*) "NEW_DATA");

    // STEP 5/8) Read NPLOTS from the archive to make sure it is consistent 
    // with the stored control panel and plot window information.  NOTE: If 
    // something went wrong with MANAGE_PLOT_WINDOW_ARRAY, this could cause 
    // problems.
    inputArchive >> BOOST_SERIALIZATION_NVP( nplots);

    // STEPS 6/8 and 7/8) Define temporary arrays, load these with 
    // serialization information, then transfer this information to the 
    // actual arrays.  NOTE: This awkward procedure is imposed by the 
    // limitations of the constructors for these classes.
    Control_Panel_Window *cps_input[ MAXPLOTS+1]; 
    inputArchive >> BOOST_SERIALIZATION_NVP( cps_input);
    for( int i=0; i<nplots; i++) {
      cps[i]->copy_state( cps_input[ i]);
      cps[i]->load_state();
    }
    Plot_Window *pws_input[ MAXPLOTS+1]; 
    inputArchive >> BOOST_SERIALIZATION_NVP( pws_input);
    for( int i=0; i<nplots; i++) {
      pws[i]->copy_state( pws_input[ i]);
      pws[i]->load_state();
    }

    // STEP 8/8) Do the same thing for brushes
    Brush *brushes_input[ NBRUSHES];
    inputArchive >> BOOST_SERIALIZATION_NVP( brushes_input);
    for( int i=0; i<NBRUSHES; i++) {
      brushes[i]->copy_state( brushes_input[ i]);
      brushes[i]->load_state();
    }
        
    // Set user_data to indicate that this is a RESIZE operation, then i
    // invoke manage_plot_window( o) to apply configuration.
    manage_plot_window_array( o, (void*) "REFRESH_WINDOWS");
  }
  catch( int i_thrown) {
    string sWarning = "";
    sWarning.append( "WARNING: Configuration file is an older or\n");
    sWarning.append( "unsupported version that cannot be loaded.");
    make_confirmation_window( sWarning.c_str(), 1);
    cerr << "Main::load_state: WARNING, "
         << "Unsupported version of configuration file" << endl;
    cerr << "       load_state reports exception (" << i_thrown
         << ")" << endl;

    manage_plot_window_array( main_menu_bar, (void*) "NEW_DATA");
    return 0;
  }
  catch( exception &e) {
    string sWarning = "";
    sWarning.append( "WARNING: load_state reports an exception.\n'");
    sWarning.append( "Configuration file appears to be damaged\n");
    sWarning.append( "or obsolete.  Results may be unpredictable.");
    make_confirmation_window( sWarning.c_str(), 1, 3);
    cerr << "Main::load_state: WARNING, "
         << "problem loading serialization file" << endl;
    cerr << "      load_state reports exception (" << e.what()
         << ")" << endl;

    manage_plot_window_array( main_menu_bar, (void*) "NEW_DATA");
    return 0;
  }
  
  // Refresh display
  // manage_plot_window_array( o,  (void*) "Resize");

  // If this is an older serialization file, query the user to determine if
  // they wish to resave it as a newer version
  if( serialization_file_version < current_serialization_version) {
    string sWarning = "";
    sWarning.append( "WARNING: Configuration file appears to be an\n");
    sWarning.append( "older version that is only partially supported.\n");
    sWarning.append( "Do you wish to resave a new version?");

    if( make_confirmation_window( sWarning.c_str(), 2, 3) == 1) {
      save_state( o);
    }
  }

  // Report success
  return 1;
}

//***************************************************************************
// save_state( o) -- Use BOOST serialization to save the current state to an 
// XML archive.  NOTE: Since BOOST reads these as sequential files rather
// than XML, the order and number of the load and save operations is 
// extremely important, and tis noted explicetly in the comments below.
int save_state( Fl_Widget* o)
{
  // Extract directory from the Data_File_Manager class to initialize the 
  // filespec for the XML archive.  NOTE: cOutFileSpec is defined as const 
  // char* for use with Vp_File_Chooser, which means it could be destroyed 
  // by the relevant destructors!
  const char *cOutFileSpec = dfm.directory().c_str();
  // const char *cOutFileSpec = strcat( dfm.directory().c_str(), "vp.xml");

  // Instantiate and show an Vp_File_Chooser widget.  NOTE: The pathname 
  // must be passed as a variable or the window will begin in some root 
  // directory.
  char* title = "Save current configuration to file";
  char* pattern = "*.xml\tAll Files (*)";
  Vp_File_Chooser* file_chooser = 
    new Vp_File_Chooser( 
      cOutFileSpec, pattern, Vp_File_Chooser::CREATE, title);
  file_chooser->directory( cOutFileSpec);
  file_chooser->isConfigFileMode( 1);

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
    // NOTE: If the file exists, it will be opened for append, then closed.
    // Otherwise this will create and close an empty file.
    #ifdef __WIN32__
      ofstream os;
      // os.open( cOutFileSpec, ios::out|ios::trunc);
      os.open( cOutFileSpec, ios::out|ios::app);
      if( os.fail()) {
        cerr << " -DIAGNOSTIC: This should trigger on error opening "
             << cOutFileSpec << "for write" << endl;
        file_chooser->directory( cOutFileSpec);
        dfm.directory( (string) cOutFileSpec);
        os.close();
        continue;
      }
      os.close();
      if( isNewFile != 0) break;
    #endif // __WIN32__

    // Presumably this test should be OS-independant
    if( isNewFile != 0) break;

    // If we got this far, the file must exist and be available to be
    // overwritten, so open a confirmation window and wait for the button 
    // handler to do something.
    string sConfirmText = "Configuration file already exists:\n'";
    sConfirmText.append( cOutFileSpec);
    sConfirmText.append( "'\nOverwrite exisiting file?\n");
    int iConfirmResult = make_confirmation_window( sConfirmText.c_str(), 3, 3);

    // If this was a 'CANCEL' request, return without doing anything.  If 
    // this was a 'YES' request, move on.  Otherwise, make sure we're in
    // the right directory and try again.
    if( iConfirmResult < 0) return -1;
    if( iConfirmResult > 0) break;
  } 

  // If no file was specified then report, deallocate the Vp_File_Chooser 
  // object, and quit.
  if( cOutFileSpec == NULL) {
    cerr << "Main::load_state: "
         << "No output file was specified" << endl;
    delete file_chooser;  // WARNING! Destroys cInFileSpec!
    return -1;
  }

  // Create an output file stream for output and make sure it exists
  std::ofstream outputFileStream( cOutFileSpec);
  assert( outputFileStream.good());

  // Create archive, which will be closed when destructors are called.
  boost::archive::xml_oarchive outputArchive( outputFileStream);

  // STEP 1/8) Begin by saving current serialization file version number
  outputArchive << boost::serialization::make_nvp( 
    "serialization_file_version", current_serialization_version);

  // STEP 2/8) Write class instance to archive
  outputArchive << BOOST_SERIALIZATION_NVP( dfm);

  // STEPS 3/8, 4/8, and 5/8) Write plot window array information to 
  // archive
  outputArchive << BOOST_SERIALIZATION_NVP( nrows);
  outputArchive << BOOST_SERIALIZATION_NVP( ncols);
  outputArchive << BOOST_SERIALIZATION_NVP( nplots);

  // STEPS 6/8 and 7/8) Serialize directly from the relevant arrays
  outputArchive << BOOST_SERIALIZATION_NVP( cps);
  outputArchive << BOOST_SERIALIZATION_NVP( pws);

  // STEP 8/8) Do the same thing for brushes
  outputArchive << BOOST_SERIALIZATION_NVP( brushes);
  
  // Report success
  return 1;
}

//***************************************************************************
// redraw_if_changing( dummy) -- Callback function for use by FLTK 
// Fl::add_idle.  When an idle callback occurs, redraw any plot that is 
// spinning or otherwise needs to be redrawn.
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
  Fl::repeat_timeout(0.01, redraw_if_changing);
  return;
}

//***************************************************************************
// reset_selection_arrays() -- Reset selection arrays to 'unselected'.
void reset_selection_arrays()
{
  inside_footprint = 0;
  newly_selected = 0;
  selected = 0;
  previously_selected = 0;
  saved_selection = 0;
  nselected = 0;
  selection_is_inverted = false;
  Plot_Window::indices_selected = 0;
  for( int i=0; i<npoints; i++) {
    Plot_Window::indices_selected(0,i) = i;
  }
}

//***************************************************************************
// Main routine
//
// Purpose: Driver to run everything.  STEP 1: Read and parse the command 
//  line.  STEP 2: Read the input data file or create a default data set.  
//  STEP 3: Create the main control panel.  STEP 4: Create the plot window 
//  array.  STEP 5: Enter the main execution loop.
//
// Functions:
//   main() -- main routine
//
// Author:   Creon Levit   unknown
// Modified: P. R. Gazis   16-SEP-2008
//***************************************************************************
//***************************************************************************
// Main -- Driver routine
int main( int argc, char **argv)
{
  ostringstream fltk_version_ss;
  fltk_version_ss << Fl::version();
  
  // XXX: In a more perfect world, this should be included with the global 
  // definitions

  about_string = "\n\
    viewpoints 2.2.0 \n\
    " + string(SVN_VERSION) + "\n\
    \n\
    using fltk version (major + 0.01*minor): " + fltk_version_ss.str() + "\n\
    \n\
    (c) 2006 M. Creon Levit and Paul R. Gazis   \n\
    \n\
    creon.levit@@nasa.gov \n\
    Paul.R.Gazis@@nasa.gov \n\
    \n";

  // STEP 1: Parse the command line
  //cout << "argc<" << argc << ">" << endl;
  //for( int i=0; i<argc; i++) {
  //  cout << "argv[ " << i << "]: <" << argv[ i] << ">" << endl;
  //}

  // Define structure of command-line options
  static struct option long_options[] = {
    { "format", required_argument, 0, 'f'},
    { "npoints", required_argument, 0, 'n'},
    { "nvars", required_argument, 0, 'v'},
    { "skip_lines", required_argument, 0, 's'},
    { "trivial_columns", required_argument, 0, 't'},
    { "ordering", required_argument, 0, 'o'},
    { "preserve_data", required_argument, 0, 't'},
    { "rows", required_argument, 0, 'r'},
    { "cols", required_argument, 0, 'c'},
    { "monitors", required_argument, 0, 'm'},
    { "input_file", required_argument, 0, 'i'},
    { "config_file", required_argument, 0, 'C'},
    { "missing_values", required_argument, 0, 'M'},
    { "delimiter", required_argument, 0, 'd'},
    { "borderless", no_argument, 0, 'b'},
    { "no_vbo", no_argument, 0, 'B'},
    { "help", no_argument, 0, 'h'},
    { "commented_labels", no_argument, 0, 'L'},
    { "expert", no_argument, 0, 'x'},
    { "verbose", no_argument, 0, 'O'},
    { "version", no_argument, 0, 'V'},
    { "stdin", no_argument, 0, 'I'},
		// Apple OS X "provides" this next argument when any program invoked by clicking on its icon
    { "psn_", required_argument, 0, 'p'}, 
    { 0, 0, 0, 0}
  };

  // Extract persistent variables
  int i_expert_mode;
  prefs_.get( "expert_mode", i_expert_mode, 0);
  expert_mode = ( i_expert_mode != 0);
  int i_trivial_columns_mode;
  prefs_.get( "trivial_columns_mode", i_trivial_columns_mode, 0);
  trivial_columns_mode = ( i_trivial_columns_mode != 0);
  int i_preserve_old_data_mode;
  prefs_.get( "preserve_old_data_mode", i_preserve_old_data_mode, 0);
  preserve_old_data_mode = ( i_preserve_old_data_mode != 0);

  // Initialize the data file manager, just in case, even though this should
  // already have been done by the constructor, then set global pointer for 
  // by everyone.
  dfm.initialize();
  pdfm = &dfm;

  // Loop: Invoke GETOPT_LONG to parse successive command-line arguments 
  // (Windows version of GETOPT_LONG is implemented in LIBGW32).  NOTES: 1) 
  // The possible options MUST be listed in the call to GETOPT_LONG, 2) This 
  // process does NOT effect arc and argv in any way.
  int c;
  string inFileSpec = "";
  string configFileSpec = "";
  char delimiter_char_ = ' ';
  while( 
    ( c = getopt_long_only( 
        argc, argv, 
        "f:n:v:s:t:o:P:r:c:m:i:C:M:d:bBhLxOVIp", long_options, NULL)) != -1) {
  
    // Examine command-line options and extract any optional arguments
    switch( c) {

      // format: Extract format of input file
      case 'f':
        if( !strncmp( optarg, "ascii", 1)) dfm.inputFileType( 0);
        else if( !strncmp( optarg, "binary", 1)) dfm.inputFileType( 1);
        else {
          usage();
          exit( -1);
        }
        break;

      // npoints: Extract maximum number of points (samples, rows of data) to 
      // read from the data file
      case 'n':
        dfm.npoints_cmd_line = atoi( optarg);
        if( dfm.npoints_cmd_line < 1)  {
          usage();
          exit( -1);
        }
        break;
      
      // nvars: Extract maximum number of variables (attributes) to read from 
      // each line of data file
      case 'v':
        dfm.nvars_cmd_line = atoi( optarg);
        if( dfm.nvars_cmd_line < 1)  {
          usage();
          exit( -1);
        }
        break;
      
      // nSkipHeaderLines: Extract number of header lines to skip at the
      // beginning of the data file
      case 's':
        dfm.n_skip_lines( atoi( optarg));
        if( dfm.n_skip_lines() < 0)  {
          usage();
          exit( -1);
        }
        break;
      
      // trivial_column_mode: Set flag to remove trivial columns
      case 't':
        if( !strncmp( optarg, "true", 1)) trivial_columns_mode = true;
        else if( !strncmp( optarg, "TRUE", 1)) trivial_columns_mode = true;
        else if( !strncmp( optarg, "false", 1)) trivial_columns_mode = false;
        else if( !strncmp( optarg, "FALSE", 1)) trivial_columns_mode = false;
        else {
          usage();
          exit( -1);
        }
        break;
      
      // ordering: Extract the ordering of ("columnmajor or rowmajor") of a 
      // binary input file
      case 'o':
        if( !strncmp( optarg, "columnmajor", 1))
          dfm.column_major( 1);
        else if ( !strncmp( optarg, "rowmajor", 1))
          dfm.column_major( 0);
        else {
          usage();
          exit( -1);
        }
        break;

      // preserve_old_data_mode: Set flag to preserve old data during reads
      case 'P':
        if( !strncmp( optarg, "true", 1)) preserve_old_data_mode = true;
        else if( !strncmp( optarg, "TRUE", 1)) preserve_old_data_mode = true;
        else if( !strncmp( optarg, "false", 1)) preserve_old_data_mode = false;
        else if( !strncmp( optarg, "FALSE", 1)) preserve_old_data_mode = false;
        else {
          usage();
          exit( -1);
        }
        break;
      
      // rows: Extract the number of rows of plot windows
      case 'r':
        nrows = atoi( optarg);
        if( nrows < 1)  {
          usage();
          exit( -1);
        }
        break;

      // cols: Extract the number of columns of plot windows
      case 'c':
        ncols = atoi( optarg);
        if( ncols < 1)  {
          usage();
          exit( -1);
        }
        break;

      // monitors: Extract the number of monitors
      case 'm':
        number_of_screens = atoi( optarg);
        if( number_of_screens < 1)  {
          usage();
          exit( -1);
        }
        break;

      // Missing or unreadable values get set to this number
      case 'M':
        // bad_value_proxy = strtof (optarg, NULL);
        // if( !bad_value_proxy) {
        dfm.bad_value_proxy( strtof( optarg, NULL));
        if( !dfm.bad_value_proxy()) {
          usage();
          exit( -1);
        }
        break;

      // set field delimiter character
      case 'd':
        if( optarg!=NULL) {
          std::string buf(unescape(optarg));
          delimiter_char_ = buf[0];
          cout << "Main: delimiter character is: (" << delimiter_char_
               << ")" << endl;
        } else {
          usage();
          exit( -1);
        }
        break;

      // input_file: Extract data filespec
      case 'i':
        inFileSpec.append( optarg);
        break;

      // config_file: Extract data filespec
      case 'C':
        configFileSpec.append( optarg);
        break;

      // borders: Turn off window manager borders on plot windows
      case 'b':
        borderless = 1;
        break;

      // don't use openGL vertex buffer objects (VBOs)
      case 'B':
        use_VBOs = false;
        break;

      // enable verbose output
      case 'L':
        dfm.do_commented_labels( 1);
        break;

      // turn on expert mode
      case 'x':
        expert_mode = true;
        break;

      // enable verbose output
      case 'O':
        be_verbose = true;
        break;

     // show version information (managed by svn), and exit
      case 'V':
        cout << about_string;
        exit (0);
        break;

     // read from stdin instead of from a file.
      case 'I':
        read_from_stdin = true;
        break;

     // Apple OSX gratuitously adds a command line argument -psn_xxx
     // where xxx is the "process serial number".  We skip over it.
     // (Is there a cleaner way to do this?)
      case 'p':
        cout << "ignoring -psn_xxx argument added by OSX" << endl;
        // skip over the rest of the -psn_* argument that OSX insists on adding
        argc -= optind;
        argv += optind;
        break;

      // help, or unknown option, or missing argument
      case 'h':
      case ':':
      case '?':
      default:
        usage();
        exit( -1);
        break;
    }
  }

  // Set persistent variables
  if( expert_mode) i_expert_mode = 1;
  else i_expert_mode = 0;
  prefs_.set( "expert_mode", i_expert_mode);
  if( trivial_columns_mode) i_trivial_columns_mode = 1;
  else i_trivial_columns_mode = 0;
  prefs_.set( "trivial_columns_mode", i_trivial_columns_mode);
  if( preserve_old_data_mode) i_preserve_old_data_mode = 1;
  else i_preserve_old_data_mode = 0;
  prefs_.set( "preserve_old_data_mode", i_preserve_old_data_mode);

  // If no data file was specified, but there was at least one argument 
  // in the command line, assume the last argument is the filespec.
  if( inFileSpec.length() <= 0 && configFileSpec.length() <= 0 && argc > 1)
    inFileSpec.append( argv[ argc-1]);

  // Increment pointers to the optional arguments to get the last argument.
  argc -= optind;
  argv += optind;

  // Set random seed (deprecated unix rand(3))
  srand( (unsigned int) time(0));

  // Initialize gsl random number generator (Mersenne Twister)
  // gsl_rng_env_setup();   // Not needed
  vp_gsl_rng = gsl_rng_alloc( gsl_rng_mt19937);

  // Restrict format and restrict and initialize the number of plots.  NOTE: 
  // nplots will later be reset by manage_plot_window_array( NULL) 
  assert( nrows*ncols <= MAXPLOTS);
  nplots = nrows*ncols;

  // STEP 2: Read the data file or create a 10-D default data set if the 
  // read attempt fails
  if( inFileSpec.length() <= 0) dfm.create_default_data( 10);
  else {
    dfm.input_filespec( inFileSpec);
    dfm.delimiter_char( delimiter_char_);
    cout << "Main: Set delimiter character to: (" << dfm.delimiter_char()
         << ")" << endl;
    if( dfm.load_data_file() != 0) dfm.create_default_data( 10);
  }
  
  // Fewer points -> bigger starting default_pointsize
  default_pointsize = max( 1.0, 6.0 - log10f( (float) npoints));
  Brush::set_sizes(default_pointsize);

  // STEP 3: Create main control panel.
  // Determine the number of screens.  NOTE screen_count requires OpenGL 1.7, 
  // which was not available under most Windows OS as of 10-APR-2006.
  #ifndef __WIN32__
    if( number_of_screens <= 0)
      number_of_screens = Fl::screen_count();
  #else 
    if( number_of_screens <= 0)
      number_of_screens = 1;
  #endif   // __WIN32__

  // Set the main control panel size and position.
  const int main_x = number_of_screens*Fl::w() - (main_w + left_frame + right_frame + right_safe);
  const int main_y = top_frame+top_safe;

  // Create the main control panel window
  create_main_control_panel( 
    main_x, main_y, main_w, main_h,
    "viewpoints -> Fast Interactive Visualization");

  // Step 4: Call manage_plot_window_array with a NULL argument to
  // initialize the plot window array.  KLUDGE ALERT: argc and argv are
  // 'globalized' to make them available to manage_plot_window_array.
  global_argc = argc;
  global_argv = argv;
  manage_plot_window_array( NULL, NULL);

  // Invoke Plot_Window::initialize_selection to clear the random selection 
  // that can occur when vp is initialized on some Linux systems.  
  if( dfm.read_selection_info() != 1)
    Plot_Window::initialize_selection();

  // Now we can show the main control panel and all its subpanels
  main_control_panel->show();

  // Step 5: Register functions to call on a reglar basis, when no other
  // events (mouse, etc.) are waiting to be processed.
  // Do not use Fl::add_idle().  It causes causes a busy-wait loop.
  // use Fl:add_timeout() instead.
  Fl::add_timeout( 0.01, redraw_if_changing);

  // For some reason, add_timout doesn't seem to work.  But add_check 
  // seems to avoid the problem with the busy-wait loop
  // Fl::add_timeout(0.25, cb_manage_plot_window_array);
  Fl::add_check( cb_manage_plot_window_array);

  // Load initial configuration if one was specified
  if( configFileSpec.length() > 0) load_initial_state( configFileSpec);

  // Enter the main event loop
  int result = Fl::run();

  // Free the gsl random number generator
  gsl_rng_free( vp_gsl_rng);
  return result;
}
