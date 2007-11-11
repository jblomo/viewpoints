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
//    FLTK 1.1.6 -- Fast Light Toolkit graphics package
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
//
// Functions:
//   usage() -- Print help information
//   make_help_about_window( *o) -- Draw the 'About' window
//   create_main_control_panel( main_x, main_y, main_w, main_h, cWindowLabel) 
//     -- Create the main control panel window.
//   cb_main_control_panel( *o, *u); -- Callback for main control panel
//   create_broadcast_group() -- Create special panel under tabs
//   manage_plot_window_array( *o, *u) -- Manage plot window array
//   make_help_view_window( *o) -- Make Help View window
//   close_help_window( *o, *u -- Help View window callback
//   make_main_menu_bar() -- Create main menu bar (unused)
//   step_help_view_widget( *o, *u) -- Step through the 'Help|Help' window.
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
//
// Author: Creon Levit    2005-2006
// Modified: P. R. Gazis  07-NOV-2007
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
static const int main_h = 850;      

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

// Define class to hold data file manager
Data_File_Manager dfm;

// Define pointers to hold main control panel, main menu bar, and any pop-up 
// windows.  NOTE: help_view_widget must be defined here so it will be 
// available to callback functions
Fl_Window *main_control_panel;
Fl_Scroll *main_scroll;
Fl_Menu_Bar *main_menu_bar;
Fl_Window *about_window;
Fl_Window *help_view_window;
Fl_Help_View *help_view_widget;

// Function definitions for the main method
void usage();
void make_help_about_window( Fl_Widget *o);
void create_main_control_panel(int main_x, int main_y, int main_w, int main_h, char* cWindowLabel);
void cb_main_control_panel( Fl_Widget *o, void* user_data);
void create_broadcast_group();
void manage_plot_window_array( Fl_Widget *o, void* user_data);
void make_main_menu_bar();
void make_help_view_window( Fl_Widget *o);
void close_help_window( Fl_Widget *o, void* user_data);
void step_help_view_widget( Fl_Widget *o, void* user_data);
void make_global_widgets();
void change_all_axes( Fl_Widget *o);
void npoints_changed( Fl_Widget *o);
void resize_selection_index_arrays( int nplots_old, int nplots);
void write_data( Fl_Widget *o, void* user_data);
void reset_all_plots();
void read_data( Fl_Widget* o, void* user_data);
int load_state( Fl_Widget* o);
int save_state( Fl_Widget* o);
void redraw_if_changing( void *dummy);

//***************************************************************************
// usage() -- Print help information to the console and exit.  NOTE: Problems 
// may arise when window is too narrow.
void usage()
{
  cerr << endl;
  cerr << "Usage: vp {optional arguments} {optional filename}" << endl;
  cerr << endl;
  cerr << "Optional arguments:" << endl;
  cerr << "  -b, --borderless            "
       << "don't show decorations on plot windows" << endl;
  cerr << "  -B, --no_vbo                "
       << "don't use openGL vertex buffer objects" << endl;
  cerr << "  -c, --cols=NCOLS            "
       << "startup showing this many columns of plot windows, default=2" << endl;
  cerr << "  -d, --delimiter=CHAR        "
       << "interpret CHAR as field separator, default is whitespace" << endl;
  cerr << "  -f, --format={ascii,binary} "
       << "input file format, default=ascii" << endl;
  cerr << "  -i, --input_file=FILENAME   "
       << "read input data from FILENAME" << endl;
  cerr << "  -m, --monitors=NSCREENS     "
       << "try and force output to display across NSCREENS screens if available" << endl;
  cerr << "  -M, --missing_values=NUMBER "
       << "set the value of any unreadable, nonnumeric, empty, or missing values to NUMBER, default=0.0" << endl;
  cerr << "  -n, --npoints=NPOINTS       "
       << "read at most NPOINTS from input file, default is min(until_EOF, 2000000)" << endl;
  cerr << "  -o, --ordering={rowmajor,columnmajor} "
       << "ordering for binary data, default=columnmajor" << endl;
  cerr << "  -r, --rows=NROWS            "
       << "startup showing this many rows of plot windows, default=2" << endl;
  cerr << "  -s, --skip_header_lines=NLINES "
       << "skip over NLINES lines at start of input file, default=0" << endl;
  cerr << "  -v, --nvars=NVARS           "
       << "input has NVARS values per point (only for row major binary data)" << endl;
  cerr << "  -h, --help                  "
       << "display this message and then exit" << endl;
  cerr << "  -x, --expert                "
       << "enable expert mode (bypass confirmations, read from stdin, etc.)" << endl;
  cerr << "  -V, --version               "
       << "output version information and then exit" << endl;

  exit( -1);
}

//***************************************************************************
// make_help_about_window( *o) -- Create the 'Help|About' window.
void make_help_about_window( Fl_Widget *o)
{
  if( about_window != NULL) about_window->hide();
   
  // Create Help|About window
  Fl::scheme( "plastic");  // optional
  about_window = new Fl_Window( 300, 200, "About vp");
  about_window->begin();
  about_window->selection_color( FL_BLUE);
  about_window->labelsize( 10);
  
  // Write text to box label and align it inside box
  Fl_Box* output_box = new Fl_Box( 5, 5, 290, 160);
  output_box->box(FL_SHADOW_BOX);
  output_box->color(7);
  output_box->selection_color(52);
  output_box->labelfont(FL_HELVETICA);
  output_box->labelsize(15);
  output_box->align(FL_ALIGN_TOP|FL_ALIGN_CENTER|FL_ALIGN_INSIDE);
  output_box->copy_label(about_string.c_str());

  // Invoke universal callback function to close window
  Fl_Button* close = new Fl_Button( 200, 170, 60, 25, "&Close");
  close->callback( (Fl_Callback*) close_help_window, about_window);

  // Done creating the 'Help|About' window
  about_window->resizable( about_window);
  about_window->end();
  about_window->show();
}

//***************************************************************************
// brushes_tab_cb() -- Callback to keep tab's colored labels drawn in the 
// right color when they're selected.  Move this to class Brush?
void brushes_tab_cb() {
  brushes_tab->labelcolor(brushes_tab->value()->labelcolor());
  brushes_tab->redraw_label();
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
  brushes_tab->callback((Fl_Callback*)brushes_tab_cb);
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

//  brushes_window->end();
//  brushes_window->show();
}

//***************************************************************************
// create_main_control_panel( main_x, main_y, main_w, main_h, cWindowLabel) 
// -- Create the main control panel window.
void create_main_control_panel( int main_x, int main_y, int main_w, int main_h, char* cWindowLabel)
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
  main_control_panel->callback((Fl_Callback*) cb_main_control_panel, main_control_panel);

  // Make main menu bar and add the global widgets to control panel
  make_main_menu_bar();

  // All controls (except the main menu bar) in the main panel are inside an Fl_Scroll, because
  // there are too many controls to all fit vertically on some small screens.
  // Eventally, we will make the sub-panels independently expnadable, ad reoragnize the gui, to
  // alleviate this problem.
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
//   There are four possible behaviors, which all must be recognized, 
// identified, and treated differently:
// 1) Initialization -- NULL argument.  Set nplots_old = 0.  
// 2) New data -- Called from a button, a menu, or as part of a 'load saved
//    state' operation.  Set nplots_old = 0.
// 3) Resize operation -- Called from a menu or as part of a 'load saved
//    state' operation.  Set nplots_old = nplots, then calculate a new value 
//    of nplots.  
// 4) Reload operation -- Called from a button or menu.  Keep nplots = 
//    nplots_old.  
// NOTE: Little attempt has been made to optimize this method for speed.  
// WARNINGS: 1) This method is delicate, and slight changes in the FLTK calls 
// could lead to elusive segmentation faults!  Test any changes carefully!  
// 2) There is little protection against missing data!
void manage_plot_window_array( Fl_Widget *o, void* user_data)
{
  // Define an enumeration to hold a list of operation types
  enum operationType { INITIALIZE = 0, NEW_DATA, RESIZE, RELOAD};
  
  // Define and initialize the operationType switch, old number of plots,
  // widget title, and pointers to the pMenu_ and pButton objects.
  operationType thisOperation = INITIALIZE;
  int nplots_old = nplots;
  char widgetTitle[ 80];
  char userData[ 80];
  strcpy( widgetTitle, "");
  strcpy( userData, "");
  Fl_Menu_* pMenu_;
  Fl_Button* pButton;

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
    thisOperation = RESIZE;
    nplots_old = nplots;

    strcpy( widgetTitle, ((Fl_Menu_*) o)->text());
    if( strncmp( widgetTitle, "Add Row ", 8) == 0) nrows++;
    else if( strncmp( widgetTitle, "Add Colu", 8) == 0) ncols++;
    else if( strncmp( widgetTitle, "Remove R", 8) == 0 && nrows>1) nrows--;
    else if( strncmp( widgetTitle, "Remove C", 8) == 0 && ncols>1) ncols--;

    // Get any user data that may have been set by calling method, and make
    // absolutely sure it isn't NULL.
    // if( o->user_data() != NULL) strcpy( userData, (char*) o->user_data());
    // else strcpy( userData, "");
    strcpy( userData, "");
    if( user_data != NULL) strcpy( userData, (char*) user_data);

    // When reading new data, invoke Fl_Gl_Window.hide() (instead of the 
    // destructor!) to destroy all plot windows along with their context, 
    // including VBOs
    if( strncmp( widgetTitle, "Open", 4) == 0 ||
        strncmp( widgetTitle, "Append", 6) == 0 ||
        strncmp( widgetTitle, "Merge", 6) == 0 ||
        strncmp( userData, "Open", 4) == 0) {
      thisOperation = NEW_DATA;
      nplots_old = 0;
      for( int i=0; i<nplots; i++) pws[i]->hide();
    }
  }

  // CASE 3: If this was a button widget, assume it was a reload operation,
  // since no other buttons can invoke this method
  else if( (pButton = dynamic_cast <Fl_Button*> (o))) {
    thisOperation = RELOAD;
    nplots_old = nplots;
    strcpy( widgetTitle, ((Fl_Menu_*) o)->label());
  }

  // DEFAULT: Default to a reload operation
  else {
    thisOperation = RELOAD;
    nplots_old = nplots;
  }

  // DIAGNOSTIC
  // std::vector<std::string> diag_stuff;
  // diag_stuff.push_back( "INITIALIZE");
  // diag_stuff.push_back( "NEW_DATA");
  // diag_stuff.push_back( "RESIZE");
  // diag_stuff.push_back( "RELOAD");
  // cout << "DIAGNOSTIC, manage_plot_window_array: widgetTitle(" << widgetTitle  
  //      << ") userData(" << userData << ")" << endl;
  // cout << "DIAGNOSTIC, manage_plot_window_array: thisOperation " 
  //      << thisOperation << " (" << diag_stuff[ thisOperation].c_str() 
  //      << ")" << endl;
    
  // Recalculate number of plots
  nplots = nrows * ncols;

  // Always save old variable indices and normalization styles, if any.  
  // QUESTION: are these array declarations safe on all compilers when 
  // nplots_old = 0?
  int ivar_old[ nplots_old];
  int jvar_old[ nplots_old];
  int kvar_old[ nplots_old];
  int x_normalization_style_old[ nplots_old];
  int y_normalization_style_old[ nplots_old];
  int z_normalization_style_old[ nplots_old];
  int x_axis_locked[ nplots_old];
  int y_axis_locked[ nplots_old];
  int z_axis_locked[ nplots_old];
  for( int i=0; i<nplots_old; i++) {
    ivar_old[ i] = cps[i]->varindex1->value();
    jvar_old[ i] = cps[i]->varindex2->value();
    kvar_old[ i] = cps[i]->varindex3->value();
    x_normalization_style_old[ i] = cps[i]->x_normalization_style->value();
    y_normalization_style_old[ i] = cps[i]->y_normalization_style->value();
    z_normalization_style_old[ i] = cps[i]->z_normalization_style->value();
    x_axis_locked[ i] = cps[i]->lock_axis1_button->value();
    y_axis_locked[ i] = cps[i]->lock_axis2_button->value();
    z_axis_locked[ i] = cps[i]->lock_axis3_button->value();
  }
  
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

    // If this was an initialize, resize, or new_data operation, then create 
    // or restore the relevant windows.  NOTE: If this code was executed 
    // during a reload operation, it would cause a segmentation fault due to
    // problems with the way shown() and hide() work.
    if( thisOperation == INITIALIZE || 
        thisOperation == RESIZE || 
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

    // If the number of plots has changed, restore the old variable indices and
    // normalization styles for the old panels.  Otherwise set new variable 
    // indices for the new panels    
    if( nplots != nplots_old && i<nplots_old) {
      cps[i]->varindex1->value( ivar_old[i]);  
      cps[i]->varindex2->value( jvar_old[i]);
      cps[i]->varindex3->value( kvar_old[i]);
      cps[i]->x_normalization_style->value( x_normalization_style_old[i]);  
      cps[i]->y_normalization_style->value( y_normalization_style_old[i]);  
      cps[i]->z_normalization_style->value( z_normalization_style_old[i]);  
      cps[i]->lock_axis1_button->value( x_axis_locked[i]);  
      cps[i]->lock_axis2_button->value( y_axis_locked[i]);  
      cps[i]->lock_axis3_button->value( z_axis_locked[i]);  
    } 
    else {
      cps[i]->varindex1->value(ivar);  
      cps[i]->varindex2->value(jvar);  
      cps[i]->varindex3->value(nvars);  
    }

    // If this is an initialization, resize, or new_data operation, test for 
    // missing data, extract data, reset panels, and make them resizable.  
    // Otherwise it must be a reload operation and we must invoke the 
    // relevant Plot_Window member functions to initialize and draw panels.
    if( thisOperation == INITIALIZE || 
        thisOperation == RESIZE ||
        thisOperation == NEW_DATA) {
      if( npoints > 1) {
        pws[i]->extract_data_points();
        pws[i]->reset_view();
      }
      pws[i]->size_range( 10, 10);
      pws[i]->resizable( pws[i]);
    }
    else {
      pws[i]->initialize();
      pws[i]->extract_data_points();
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
    "File/Open file            ", 0, 
    (Fl_Callback *) read_data, (void*) "open ASCII");
  main_menu_bar->add( 
    "File/Append more data     ", 0, 
    (Fl_Callback *) read_data, (void*) "append ASCII");
  main_menu_bar->add( 
    "File/Merge another file   ", 0, 
    (Fl_Callback *) read_data, (void*) "merge ASCII", FL_MENU_DIVIDER);
  main_menu_bar->add( 
    "File/Save selected data   ", 0, 
    (Fl_Callback *) write_data, (void*) "selected ASCII");
  main_menu_bar->add( 
    "File/Save all data        ", 0, 
    (Fl_Callback *) write_data, (void*) "save ASCII", FL_MENU_DIVIDER);
  main_menu_bar->add( 
    "File/Load configuration   ", 0, 
    (Fl_Callback *) load_state);
  main_menu_bar->add( 
    "File/Save configuration   ", 0, 
    (Fl_Callback *) save_state, 0, FL_MENU_DIVIDER);
  main_menu_bar->add( 
    "File/Quit   ", 0, (Fl_Callback *) exit);

  // Add View menu items
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
    "View/Restore Panels  ", 0, 
    (Fl_Callback *) manage_plot_window_array);

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
// make_help_view_window( *o) -- Create the 'Help|Help' window.
void make_help_view_window( Fl_Widget *o)
{
  if( help_view_window != NULL) help_view_window->hide();
  
  // Create Help|Help window
  Fl::scheme( "plastic");  // optional
  help_view_window = new Fl_Window( 600, 400, "Viewpoints Help");
  help_view_window->begin();
  help_view_window->selection_color( FL_BLUE);
  help_view_window->labelsize( 10);

  // Define Fl_Help_View widget
  help_view_widget = new Fl_Help_View( 5, 5, 590, 350, "");
  (void) help_view_widget->load( "vp_help_manual.htm");
  help_view_widget->labelsize( 14);
  help_topline = help_view_widget->topline();
  
  // Invoke callback function to move through help_view widget
  Fl_Button* back = new Fl_Button( 325, 365, 70, 30, "&Back");
  back->callback( (Fl_Callback*) step_help_view_widget, (void*) -60);
  Fl_Button* fwd = new Fl_Button( 400, 365, 70, 30, "&Fwd");
  fwd->callback( (Fl_Callback*) step_help_view_widget, (void*) 60);

  // Invoke callback function to close window
  Fl_Button* close = new Fl_Button( 500, 365, 70, 30, "&Close");
  close->callback( (Fl_Callback*) close_help_window, help_view_window);

  // Done creating the 'Help|Help' window
  help_view_window->resizable( help_view_window);
  help_view_window->end();
  help_view_window->show();
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
  if( strstr( (char *) user_data, "binary") != NULL) dfm.ascii_output( 0);
  else dfm.ascii_output( 1);

  // Evaluate user_data to determine if only selected data are to be used
  if( strstr( (char *) user_data, "selected") != NULL) dfm.selected_data( 1);
  else dfm.selected_data( 0);

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
  dfm.ascii_input( 1);

  // Evaluate USER_DATA to get the operation type
  if( strstr( (char *) user_data, "append") != NULL) dfm.do_append( 1);
  else dfm.do_append( 0);
  if( strstr( (char *) user_data, "merge") != NULL) dfm.do_merge( 1);
  else dfm.do_merge( 0);

  // Invoke the findInputFile() methind of the data_file_manager object to 
  // query the user for the input filename.  If no file is specified, return 
  // immediately and hope the calling routine can handle this situation.
  int iQueryStatus = 0;
  iQueryStatus = dfm.findInputFile();
  if( iQueryStatus != 0) {
    cout << "No input file was selected" << endl;
    return;
  }

  // Invoke the load_data_file() method of the data_file_manager object to 
  // read the data file.  Error reporting is handled by the method itself.
  dfm.load_data_file();

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
// load_state( o) -- Use BOOST serialization to load a saved state from an
// XML archive.
int load_state( Fl_Widget* o)
{
  // Initialize filespec for the XML archive.  NOTE: cOutFileSpec is defined 
  // as const char* for use with Vp_File_Chooser, which means it could be 
  // destroyed by the relevant destructors!
  const char *cInFileSpec = dfm.directory().c_str();
  // const char *cInFileSpec = strcat( dfm.directory().c_str(), "vp.xml");

  // Instantiate and show an Vp_File_Chooser widget.  NOTE: The pathname 
  // must be passed as a variable or the window will begin in some root 
  // directory.
  char* title = "Load saved configuration from file";
  char* pattern = "*.xml\tAll Files (*)";
  Vp_File_Chooser* file_chooser =
    new Vp_File_Chooser( cInFileSpec, pattern, Vp_File_Chooser::SINGLE, title);

  // Loop: wait until the file selection is done
  file_chooser->show();
  while( file_chooser->shown()) Fl::wait();
  cInFileSpec = file_chooser->value();   

  // If no file was specified then report, deallocate the Vp_File_Chooser 
  // object, and quit.
  if( cInFileSpec == NULL) {
    cerr << "Main::load_state: "
         << "No input file was specified" << endl;
    delete file_chooser;  // WARNING! Destroys cInFileSpec!
    return -1;
  }

  // Create a file stream for input and make sure it exists.  This will
  // be closed when destructors are called
  std::ifstream inputFileStream( cInFileSpec, std::ios::binary);
  assert( inputFileStream.good());

  // Create and open an archive for input.  As before, this will be
  // closed when destructors are called
  boost::archive::xml_iarchive inputArchive( inputFileStream);

  // Get data file from archive and read it
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
  
  // Set user_data for this widget to indicate that this is a READ operation, 
  // then invoke manage_plot_window( o) to clear children of the tab widget 
  // and reload plot window array.
  // o->user_data( (void*) "Read");
  manage_plot_window_array( o, (void*) "Read");

  // Read configuration information from archive
  inputArchive >> BOOST_SERIALIZATION_NVP( nrows);
  inputArchive >> BOOST_SERIALIZATION_NVP( ncols);

  // Set user_data for this widget to indicate that this is a RESIZE 
  // operation, then invoke manage_plot_window( o) to apply configuration.
  // o->user_data( (void*) "Resize");
  manage_plot_window_array( o,  (void*) "Resize");

  // Refresh display
  manage_plot_window_array( o,  (void*) "Resize");

  // Report success
  return 1;
}

//***************************************************************************
// save_state( o) -- Use BOOST serialization to save the current state to an 
// XML archive.
int save_state( Fl_Widget* o)
{
  // Initialize filespec for the XML archive.  NOTE: cOutFileSpec is defined 
  // as const char* for use with Vp_File_Chooser, which means it could be 
  // destroyed by the relevant destructors!
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

  // Loop: Select succesive output filespecs until a non-directory is 
  // obtained and found acceptible to the user
  while( 1) {

    // Loop: Wait until the file selection is done
    file_chooser->show();
    while( file_chooser->shown()) Fl::wait();
    cOutFileSpec = file_chooser->value();   

    // If no file was specified then quit
    if( cOutFileSpec == NULL) break;

    // If this is a new file, it can't be opened for read, and we're done
    FILE* pFile = fopen( cOutFileSpec, "r");
    if( pFile == NULL) break;
    
    // If we got this far, the file must exist and be available to be
    // overwritten, so close it, then open a confirmation window and wait 
    // for the button handler to do something.
    fclose( pFile);
    int confirmationResult = 
      make_confirmation_window( "Overwrite existing file?");

    // If this was a 'CANCEL' request, return without doing anything.  If 
    // this was a 'YES' request, move on.  Otherwise, make sure we're in
    // the right directory and try again.
    if( confirmationResult < 0) return -1;
    if( confirmationResult > 0) break;
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

  // Write class instance to archive
  outputArchive << BOOST_SERIALIZATION_NVP( dfm);
  outputArchive << BOOST_SERIALIZATION_NVP( nrows);
  outputArchive << BOOST_SERIALIZATION_NVP( ncols);

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

void reset_selection_arrays () {
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
// Modified: P. R. Gazis   04-OCT-2006
//***************************************************************************
//***************************************************************************
// Main -- Driver routine
int main( int argc, char **argv)
{
  // XXX: In a perfect world, this should be included with the global 
  // definitions
  about_string = "\n\
    viewpoints 2.0 \n\
    " + string(SVN_VERSION) + "\n\
    \n\
    (c) 2006 M. Creon Levit and Paul R. Gazis   \n\
        creon.levit@@nasa.gov \n\
        pgazis@@mail.arc.nasa.gov \n\
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
    { "skip_header_lines", required_argument, 0, 's'},
    { "ordering", required_argument, 0, 'o'},
    { "rows", required_argument, 0, 'r'},
    { "cols", required_argument, 0, 'c'},
    { "monitors", required_argument, 0, 'm'},
    { "input_file", required_argument, 0, 'i'},
    { "missing_values", required_argument, 0, 'M'},
    { "delimiter", required_argument, 0, 'd'},
    { "borderless", no_argument, 0, 'b'},
    { "no_vbo", no_argument, 0, 'B'},
    { "help", no_argument, 0, 'h'},
    { "expert", no_argument, 0, 'x'},
    { "version", no_argument, 0, 'V'},
		// Apple OS X "provides" this next argument when any program invoked by clicking on its icon
    { "psn_", required_argument, 0, 'p'}, 
    { 0, 0, 0, 0}
  };

  // Initialize the data file manager, just in case
  dfm.initialize();

  // Loop: Invoke GETOPT_LONG to parse successive command-line arguments 
  // (Windows version of GETOPT_LONG is implemented in LIBGW32).  NOTES: 1) 
  // The possible options MUST be listed in the call to GETOPT_LONG, 2) This 
  // process does NOT effect arc and argv in any way.
  int c;
  string inFileSpec = "";
  while( 
    ( c = getopt_long_only( 
        argc, argv, 
        "f:n:v:s:o:r:c:m:i:M:d:bBhxVp", long_options, NULL)) != -1) {
  
    // Examine command-line options and extract any optional arguments
    switch( c) {

      // format: Extract format of input file
      case 'f':
        if( !strncmp( optarg, "binary", 1)) dfm.ascii_input( 0);
        else if( !strncmp( optarg, "ascii", 1)) dfm.ascii_input( 1);
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
        dfm.n_skip_header_lines( atoi( optarg));
        if( dfm.n_skip_header_lines() < 0)  {
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
          // delimiter_char = buf[0];
          // cout << "delimiter character is: " << delimiter_char << endl;
          dfm.delimiter_char( buf[0]);
          cout << "delimiter character is: " << dfm.delimiter_char() << endl;
        } else {
          usage();
          exit( -1);
        }
        break;

      // inputfile: Extract data filespec
      case 'i':
        inFileSpec.append( optarg);
        break;

      // borders: Turn off window manager borders on plot windows
      case 'b':
        borderless = 1;
        break;

      // don't use openGL vertex buffer objects (VBOs)
      case 'B':
        use_VBOs = false;
        break;

      // turn on expert mode
      case 'x':
        expert_mode = true;
        break;

     // show version information (managed by svn), and exit
      case 'V':
        cout << about_string;
        exit (0);
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

  // If no data file was specified, but there was at least one argument 
  // in the command line, assume the last argument is the filespec.
  if( inFileSpec.length() <= 0 && argc > 1) inFileSpec.append( argv[ argc-1]);

  // Increment pointers to the optional arguments to get the last argument.
  argc -= optind;
  argv += optind;

  // Set random seed
  srand( (unsigned int) time(0));

  // Restrict format and restrict and initialize the number of plots.  NOTE: 
  // nplots will later be reset by manage_plot_window_array( NULL) 
  assert( nrows*ncols <= MAXPLOTS);
  nplots = nrows*ncols;

  // STEP 2: Read the data file create a 10-d default data set if the read 
  // attempt fails
  if( inFileSpec.length() <= 0) dfm.create_default_data( 10);
  else {
    dfm.input_filespec( inFileSpec);
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
  create_main_control_panel(main_x, main_y, main_w, main_h, "viewpoints -> creon.levit@nasa.gov");

  // Step 4: Call manage_plot_window_array with a NULL argument to
  // initialize the plot window array.  KLUDGE ALERT: argc and argv are
  // 'globalized' to make them available to manage_plot_window_array.
  global_argc = argc;
  global_argv = argv;
  manage_plot_window_array( NULL, NULL);

  // Invoke Plot_Window::initialize_selection to clear the random selection 
  // that can occur when vp is initialized on some Linux systems.  
  Plot_Window::initialize_selection();

  // Now we can show the main control panel and all its subpanels
  main_control_panel->show();

  // Step 5: Set pointer to the function to call when the window is idle and 
  // enter the main event loop
  // Fl::add_idle( redraw_if_changing);
  Fl::add_timeout(0.01, redraw_if_changing);

  // Enter the main event loop
  int result = Fl::run();
  return result;
}
