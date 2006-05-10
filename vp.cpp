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
//   See Makefile for linux and OSX compile & link info.
//
// Purpose: viewpoints - interactive linked scatterplots and more.
//
// General design philosophy:
//   1) This code is represents a battle between Creon Levit's 
//      passion for speed and efficiency and Paul Gazis's obsession
//      with organization and clarity, unified by a shared desire to
//      produce a powerful and easy to use tool for exploratory data
//      data analysis.  Creon's code reflects a strong 'C' heritage.  
//      Paul's code is written in C++ using the 'if only it were JAVA' 
//      programming style.
//
// Functions:
//   usage() -- Print help information
//   make_help_about_window( *o) -- DRaw the 'About' window
//   create_main_control_panel( main_x, main_y, main_w, main_h,
//     cWindowLabel) -- Create the main control panel window.
//   create_broadcast_group() -- Create special panel under tabs
//   manage_plot_window_array( *o) -- Manage plot window array
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
// Author: Creon Levit   2005-2006
// Modified: P. R. Gazis  09-MAY-2006
//*****************************************************************

// Include the necessary include libraries
#include "include_libraries_vp.h"

// Include globals, and turn on initializers (using #define EXTERN)
// initialize globals
#define EXTERN
#define INIT(x) = x
#include "global_definitions_vp.h"

// Include associated headers and source code
#include "data_file_manager.h"
#include "plot_window.h"
#include "control_panel_window.h"

// Define and initialize number of screens
static int number_of_screens = 0;

// Approximate values of window manager borders & desktop borders 
// (stay out of these).  These are used by the main method when 
// the main control panel window is defined.  And when the plot
// windows are tiled to fit the screen.  Too bad they are only
// "hints" according most window managers (and we all know how
// well managers take hints).
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

// Define variables to hold main control panel window, tabs 
// widget, and virtual control panel positions.  Consolidated
// here for reasons of clarity.
// const int main_w = 350, main_h = 700;
static const int main_w = 350, main_h = 725;
static const int tabs_widget_x = 3, tabs_widget_y = 30;
static const int tabs_widget_h = 500;
static const int cp_widget_x = 3, cp_widget_y = 50;
static const int cp_widget_h = 480;
static const int global_widgets_x = 10, global_widgets_y = 520;

// Define class to hold data file manager
data_file_manager dfm;

// Define pointers to hold main control panel, main menu bar, and
// any pop-up windows.  NOTE: help_view_widget must be defined here
// so it will be available to callback functions
Fl_Window *main_control_panel;
Fl_Menu_Bar *main_menu_bar;
Fl_Window *about_window;
Fl_Window *help_view_window;
Fl_Help_View *help_view_widget;

// Function definitions for the main method
void usage();
void make_help_about_window( Fl_Widget *o);
void create_main_control_panel( 
  int main_x, int main_y, int main_w, int main_h,
  char* cWindowLabel);
void create_broadcast_group();
void manage_plot_window_array( Fl_Widget *o);
void make_main_menu_bar();
void make_help_view_window( Fl_Widget *o);
void close_help_window( Fl_Widget *o, void* user_data);
void step_help_view_widget( Fl_Widget *o, void* user_data);
void make_global_widgets();
void choose_color_deselected( Fl_Widget *o);
void change_all_axes( Fl_Widget *o);
void clearAlphaPlanes();
void npoints_changed( Fl_Widget *o);
void write_data( Fl_Widget *o);
void reset_all_plots( void);
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
  
  // Compose text. NOTE use of @@ in conjunction with label()
  string sAbout = "viewpoints 1.0.3\n";
  sAbout += "(c) 2006 C. Levit and P. R. Gazis\n\n";
  sAbout += "contact information:\n";
  sAbout += "Creon Levit creon@@nas.nasa.gov\n";
  sAbout += "Paul R Gazis pgazis@@mail.arc.nasa.gov\n\n";

  // Write text (old version with Fl_Multiline_Output)
  // Fl_Multiline_Output* output_widget = 
  //   new Fl_Multiline_Output( 5, 5, 290, 190); 
  // output_widget->value( sAbout.c_str());

  // Write text to box label and align it inside box
  Fl_Box* output_box = new Fl_Box( 5, 5, 290, 160, sAbout.c_str());
  output_box->box(FL_SHADOW_BOX);
  output_box->color(7);
  output_box->selection_color(52);
  output_box->labelfont(FL_HELVETICA);
  output_box->labelsize(15);
  output_box->align(FL_ALIGN_TOP|FL_ALIGN_CENTER|FL_ALIGN_INSIDE);

  // Invoke universal callback function to close window
  Fl_Button* close = new Fl_Button( 200, 170, 60, 25, "&Close");
  close->callback( (Fl_Callback*) close_help_window, about_window);

  // Done creating the 'Help|About' window
  about_window->resizable( about_window);
  about_window->end();
  about_window->show();
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
  // sub-panels created by manage_plot_window_array)
  main_control_panel->end();
}

//*****************************************************************
// create_broadcast_group () -- Create a special panel (really a
// group under a tab) with label "+" this group's widgets effect 
// all the others (unless a plot's tab is "locked" - TBI).
// MCL XXX should this be a method of control_panel_window?
//  should it be a singleton?
void create_broadcast_group ()
{
  Fl_Group::current(cpt);  
  control_panel_window *cp = cps[nplots];
  cp = new control_panel_window( cp_widget_x, cp_widget_y, main_w - 6, cp_widget_h);
  cp->label("all");
  cp->labelsize( 10);
  cp->resizable( cp);
  cp->make_widgets( cp);
  cp->end();

  // this group's index is highest (and it has no associated plot window)
  cp->index = nplots;

  // this group's callbacks all broadcast any "event" to the other (unlocked) 
  // tabs groups.  with a few exceptions... (for now)
  for (int i=0; i<cp->children(); i++)
  {
    Fl_Widget *wp = cp->child(i);
    wp->callback((Fl_Callback *)(control_panel_window::broadcast_change), cp);
  }

  // MCL XXX these widgets cause crashes or misbehaviors in the global panel, 
  // so skip them for now.
  cp->choose_selection_color_button->deactivate();
  cp->sum_vs_difference->deactivate();
  cp->polar->deactivate();
  cp->no_transform->deactivate();

  // initially, this group has no axes (XXX or anything else, for that matter)
  cp->varindex1->value(nvars);  // initially == "-nothing-"
  cp->varindex2->value(nvars);  // initially == "-nothing-"
  cp->varindex3->value(nvars);  // initially == "-nothing-"
}

//*****************************************************************
// manage_plot_window_array( o) -- General-purpose method to create, 
// manage, and reload the plot window array.  It saves any existing 
// axis information, deletes old tabs, creates new tabs, restores 
// existing axis information, and loads new data into new plot 
// windows.  There are four possible behaviors, which all must be
// recognized, identified, and treated differently:
// 1) Initialization -- NULL argument.  Set nplots_old = 0.  
//    Identified by a flag, uInitialize.
// 2) New data -- Called from button or menu.  Set nplots_old = 0.
//    Identified by uIintialize == 0 and nplots_old == 0.
// 3) Resize operation -- Called from menu.  Set nplots_old = 
//    nplots, then calculate a new value of nplots.  Identified by 
//    uInitialize == 0 and nplots_old != nplots.
// 4) Reload operation -- Called from button or menu.  Keep 
//    nplots = nplots_old.  Identified by uInitialize == 0 and 
//    nplots_old == nplots.
// NOTE: Little attempt has been made to optimize this method for 
// speed.  WARNINGS: 1) Tbis method is delicate, and slight changes
// in the FLTK calls could lead to elusive segmentation faults!
// Test any changes carefully!  2) There is little protected against 
// missing data!
void manage_plot_window_array( Fl_Widget *o)
{
  // Determine how the method was invoked and set the uInitialize
  // flag and plot numbers accordingly.  
  unsigned uInitialize = 0;
  int nplots_old = nplots;
  char title[ 80];
  strcpy( title, "");
  Fl_Menu_* pMenu_;
  Fl_Button* pButton;
  if( o == NULL) {
    uInitialize = 1;
    nplots_old = 0;
  }
  else if( (pMenu_ = dynamic_cast <Fl_Menu_*> (o))) {
    strcpy( title, ((Fl_Menu_*) o)->text());
    if( strncmp( title, "Add Row ", 8) == 0) nrows++;
    else if( strncmp( title, "Add Colu", 8) == 0) ncols++;
    else if( strncmp( title, "Remove R", 8) == 0 && nrows>1) nrows--;
    else if( strncmp( title, "Remove C", 8) == 0 && ncols>1) ncols--;

    if( strncmp( title, "Read", 4) == 0) nplots_old = 0;
    else nplots_old = nplots;
  }
  else if( (pButton = dynamic_cast <Fl_Button*> (o))) {
    strcpy( title, ((Fl_Menu_*) o)->label());
    if( strncmp( title, "Read", 4) == 0) nplots_old = 0;
    else nplots_old = nplots;
  }
  else nplots_old = nplots;
  nplots = nrows * ncols;

  // Save old variable indices and normalization styles, if any.  
  // QUESTION: are these array declarations safe on all compilers
  // when nplots_old = 0?
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
  
  // Clear children of tab widget to delete old tabs
  cpt->clear();

  // Create and add the virtual sub-panels, each group under a 
  // tab, one per plot.
  for( int i=0; i<nplots; i++) {
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

    // Create a label for this tab
    ostringstream oss;
    oss << "" << i+1;
    string labstr = oss.str();

    // Set pointer to the current group to the tab widget defined 
    // by create_control_panel and add a new virtual control panel
    // under this tab widget
    Fl_Group::current(cpt);  
    cps[i] = new control_panel_window( 
      cp_widget_x, cp_widget_y, main_w - 6, cp_widget_h);
    cps[i]->index = i;
    cps[i]->copy_label( labstr.c_str());
    cps[i]->labelsize( 10);
    cps[i]->resizable( cps[i]);
    cps[i]->make_widgets( cps[i]);

    // End the group here so that we can create new plot windows 
    // at the top level, then set the pointer to the current group
    // to the top level.
    cps[i]->end();
    Fl_Group::current(0); 

    // If this is an initialization or resize operation, create or
    // restore windows.  NOTE: If this code was executed during a
    // read or reload operation, it would cause a segementation fault.
    if( uInitialize || ( nplots != nplots_old && nplots_old > 0)) {
    if( i>=nplots_old) {
      pws[i] = new plot_window( pw_w, pw_h);
      pws[i]->index = i;
      cps[i]->pw = pws[i];
      pws[i]->cp = cps[i];
    }
    else {
      // if( pws[i]->shown()) pws[i]->hide();  // This is the problem!
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

    // Link plot window and its associated virtual control panel
    pws[i]->index = cps[i]->index = i;
    cps[i]->pw = pws[i];
    pws[i]->cp = cps[i];

    // Determine which variables to plot in new panels.
    int ivar, jvar;
    if( i==0) {
      ivar = 0;
      jvar = 1;
      
      // When the plot window array is being created, the first 
      // plot's tab is shown and its axes are locked.
      if( o == NULL) {
        cps[i]->hide();  
      }
    }
    else plot_window::upper_triangle_incr( ivar, jvar, nvars);

    // If this is an initialization or this is a resize operation, 
    // restore variable indices and normalization styles for old 
    // panels.  Otherwise set variable indices for new panels    
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

    // If this is an initialization or resize operation, test for
    // missing data, extract data, reset panels, and make them
    // resizable.  Otherwise invoke plot_window::initialize() and 
    // draw panels
    if( uInitialize || ( nplots != nplots_old && nplots_old > 0)) {
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

    if( borderless) pws[i]->border(0);

    // Make sure the window has been shown and check again to make
    // sure it is resizable.  NOTE: pws[i]->show() with no arguments
    // is not sufficient when windows are created.
    if( !pws[i]->shown()) pws[i]->show( global_argc, global_argv);
    pws[i]->resizable( pws[i]);

    // Turn on show capability of plot_window::reset_view();
    pws[i]->do_reset_view_with_show = 1;
  }
  
  // Get rid of any superfluous plot windows
  if( nplots < nplots_old)
    for( int i=nplots; i<nplots_old; i++) pws[i]->hide();

  // Create master control panel for tabs
  create_broadcast_group ();
}

//*****************************************************************
// make_main_menu_bar() -- Make main menu bar.  NOTE: because the
// FLTK documentation recommends against manipulating the
// Fl_Menu_Item array directly, this is done via the add() method 
// of Fl_Menu_.
void make_main_menu_bar()
{
  // Instantiate the Fl_Menu_Bar object
  main_menu_bar =
    new Fl_Menu_Bar( 0, 0, main_w, 25);

  // Add File menu items
  main_menu_bar->add( 
    "File/Read ASCII file   ", 0, 
    (Fl_Callback *) read_data, (void*) ASCII);
  main_menu_bar->add( 
    "File/Read binary file   ", 0, 
    (Fl_Callback *) read_data, (void*) BINARY);
  main_menu_bar->add( 
    "File/Write binary file   ", 0, 
    (Fl_Callback *) write_data, 0, FL_MENU_DIVIDER);
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
    "View/Reload Plots   ", 0, 
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
  
  // This example is included to illustrate how awkward it can  
  // be to access elements of the Fl_Menu_Item array directly.
  // for( int i=0; i<main_menu_bar->size(); i++) {
  //   Fl_Menu_Item *pMenuItem = 
  //     (Fl_Menu_Item*) &(main_menu_bar->menu()[i]);
  //   pMenuItem->labelsize(32);
  // }
}

//*****************************************************************
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

//*****************************************************************
// close_help_window( *o, *user_data) -- Close a Help window
void close_help_window( Fl_Widget *o, void* user_data)
{
  // WARNING: No error checking is done on user_data!
  ((Fl_Window*) user_data)->hide();
}

//*****************************************************************
// step_help_view_window( *o, *user_data) -- Step through the 
// 'Help|Help' window.
void step_help_view_widget( Fl_Widget *o, void* user_data)
{
  help_topline += (int) user_data;
  if( help_topline < 0) help_topline=0;
  help_view_widget->topline( help_topline);
}

//*****************************************************************
// make_global_widgets() -- Make controls for main control panel
void make_global_widgets()
{
  // Draw 'npoints' horizontal slider at top of subpanel
  // int xpos=10, ypos=500;
  int xpos = global_widgets_x, ypos = global_widgets_y;
  npoints_slider = 
    new Fl_Hor_Value_Slider_Input( xpos+30, ypos+=25, 300-30, 20, "npts");
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
  // write_data_button = b = 
  //   new Fl_Button( xpos, ypos+=25, 20, 20, "write data");
  // b->align( FL_ALIGN_RIGHT); 
  // b->selection_color( FL_BLUE); 
  // b->callback( write_data);

  // Advance to column 2
  xpos = xpos1 + 150; ypos = ypos1;

  // Button(1,2): Chose color of non-selcted points
  choose_color_deselected_button = b = 
    new Fl_Button( xpos, ypos+=25, 20, 20, "unselected color");
  b->align( FL_ALIGN_RIGHT); 
  b->selection_color( FL_BLUE); 
  b->callback( (Fl_Callback*)choose_color_deselected);

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

  // Button(5,2): Reload plot window array
  reload_plot_window_array_button = b = 
    new Fl_Button( xpos, ypos+=25, 20, 20, "reload plots");
  b->align( FL_ALIGN_RIGHT); 
  b->selection_color( FL_BLUE); 
  b->callback( manage_plot_window_array);

  // Button(6,2): Read ASCII data file
  // read_data_button = b = 
  //   new Fl_Button( xpos, ypos+=25, 20, 20, "read data");
  // b->align( FL_ALIGN_RIGHT); 
  // b->selection_color( FL_BLUE); 
  // b->callback( read_data);
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
    // only change axes for plots whose x or y axis is unlocked
    if( !(cps[i]->lock_axis1_button->value() && cps[i]->lock_axis2_button->value()))
      pws[i]->change_axes( 0);
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
  dfm.write_binary_file_with_headers();
}

//*****************************************************************
// reset_all_plots() -- Reset all plots.  Invoked by main control 
// panel.
void reset_all_plots()
{
  for( int i=0; i<nplots; i++) pws[i]->reset_view();
}

//*****************************************************************
// read_data( o, user_data) -- Widget to open and read data from 
// an ASCII file.  Class FL_File_Chooser is used in preference to
// the fl_file_chooser method to obtain access to member functions 
// such as directory() and to allow the possibility of a derived 
// class with additional controls in the file_chooser window.
void read_data( Fl_Widget* o, void* user_data)
{
  // Evaluate user_data to get file format
  if( (int) user_data == BINARY) dfm.format = BINARY;
  else dfm.format = ASCII;

  // Generate text, file extensions, etc, for this file type
  char* title = NULL;
  char* pattern = NULL;
  if( dfm.format == ASCII) {
    title = "Read ASCII input from file";
    pattern = "*.{txt,lis,asc}\tAll Files (*)";
  }
  else {
    title =  "Read binary input from file";
    pattern = "*.bin\tAll Files (*)";
  }

  // Initialize read status and filespec.  NOTE: inFileSpec is
  // defined as const char* for use with Fl_File_Chooser, which 
  // means it could be destroyed by the relevant destructors!
  const char *inFileSpec = (dfm.directory()).c_str();
  int iReadStatus = 0;

  // Instantiate and show an Fl_File_Chooser widget.  NOTE: The 
  // pathname must be passed as a variable or the window will begin 
  // in some root directory.
  Fl_File_Chooser* file_chooser = 
    new Fl_File_Chooser( inFileSpec, pattern, Fl_File_Chooser::SINGLE, title);

  // Loop: Select fileSpecs until a non-directory is obtained
  while( 1) {
    if( inFileSpec != NULL) file_chooser->directory( inFileSpec);

    // Loop: wait until the file selection is done
    file_chooser->show();
    while( file_chooser->shown()) Fl::wait();
    inFileSpec = file_chooser->value();   

    // If no file was specified then quit
    if( inFileSpec == NULL) {
      cout << "No input file was specified" << endl;
      break;
    }

    // For some reason, the fl_filename_isdir method doesn't seem
    // to work, so try to open this file to see if it is a directory.
    FILE* pFile = fopen( inFileSpec, "r");
    if( pFile == NULL) {
      file_chooser->directory( inFileSpec);
      dfm.directory( (string) inFileSpec);
      continue;
    }
    fclose( pFile);
    break;         
  } 

  // If no file was specified then quit and deallocate the 
  // Fl_File_Chooser object
  if( inFileSpec == NULL) {
    cout << "No input file was specified" << endl;
    delete file_chooser;  // WARNING! Destroys inFileSpec!
    return;
  }

  // Report results
  if( dfm.format == ASCII) cout << "Reading ASCII data from <";
  else cout << "Reading binary data from <";
  cout << inFileSpec << ">" << endl;

  // Since this is presumably a new file, reinitialize the data 
  // file manager!
  dfm.initialize();

  // Invoke the load_data_file( inFileSpec) method of the data 
  // file manager to read an ASCII or BINARY file.  NOTE: Error
  // reporting is handled by the method itself.
  iReadStatus = dfm.load_data_file( (string) inFileSpec);

  // If only one or fewer records are available then quit before 
  // something terrible happens!
  if( npoints <= 1) {
    cout << "Insufficient data, " << npoints
         << " samples.  Loading default data." << endl;
    dfm.create_default_data( 10);
    npoints_slider->bounds(1,npoints);
    npoints_slider->value(npoints);
    return;
  }
  else {
    cout << "Loaded " << npoints
         << " samples with " << nvars << " fields" << endl;
  }
  
  // Resize slider
  npoints_slider->bounds(1,npoints);
  npoints_slider->value(npoints);

  // Fewer points -> bigger starting pointsize
  pointsize = max( 1.0, 6.0 - (int) log10f( (float) npoints));

  // Clear children of tab widget and reload plot window array
  manage_plot_window_array( o);

  // Deallocate file_chooser
  delete file_chooser;  // WARNING! Destroys inFileSpec!
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
// Purpose: Driver to run everything.  STEP 1: Read and parse the
//  command line.  STEP 2: Read data file or create default data 
//  set.  STEP 3: Create main control panel.  STEP 4: Create plot
//  window array.  STEP 5: Enter main execution loop.
//
// Functions:
//   main() -- main routine
//
// Author:   Creon Levit   unknown
// Modified: P. R. Gazis   04-MAY-2006
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

  // Initialize the data file manager, just in case
  dfm.initialize();

  // Loop: Invoke GETOPT_LONG to parse successive command-line 
  // arguments (Windows version of GETOPT_LONG is implemented in 
  // LIBGW32).  NOTES: 1) The possible options MUST be listed in 
  // the call to GETOPT_LONG, 2) This process does NOT effect arc 
  // and argv in any way.
  int c;
  string inFileSpec = "";
  while( 
    ( c = getopt_long( 
        argc, argv, 
        "f:n:s:o:r:c:m:i:b:h", long_options, NULL)) != -1) {
  
    // Examine command-line options and extract any optional
    // arguments
    switch( c) {

      // format: Extract format of input file
      case 'f':
        if( !strncmp( optarg, "binary", 1)) dfm.format = BINARY;
        else if( !strncmp( optarg, "ascii", 1)) dfm.format = ASCII;
        else {
          usage();
          exit( -1);
        }
        break;

      // npoints: Extract maximum number of points (samples, rows 
      // of data) to read from the data file
      case 'n':
        dfm.npoints_cmd_line = atoi( optarg);
        if( dfm.npoints_cmd_line < 1)  {
          usage();
          exit( -1);
        }
        break;
      
      // npoints: Extract maximum number of points (samples, rows 
      // of data) to read from the data file
      case 's':
        dfm.nSkipHeaderLines = atoi( optarg);
        if( dfm.nSkipHeaderLines < 0)  {
          usage();
          exit( -1);
        }
        break;
      
      // ordering: Extract the ordering of ("columnmajor or 
      // rowmajor") of a binary input file
      case 'o':
        if( !strncmp( optarg, "columnmajor", 1))
          dfm.ordering=COLUMN_MAJOR;
        else if ( !strncmp( optarg, "rowmajor", 1))
          dfm.ordering=ROW_MAJOR;
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

      // inputfile: Extract data filespec
      case 'i':
        inFileSpec.append( optarg);
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

  // If command line was used but no arguments were specified, 
  // then quit.  NOTE: If command line was not used, the GUI
  // should be invoked.  This test will work for WIN32, but
  // what about Linux and MacOS?
  if( argc == 1 && strlen( argv[ 0]) <= 2) {
    usage();
    exit( 0);
  }

  // If no data file was specified, assume the last argument is
  // the filespec.
  if( inFileSpec.length() <= 0) inFileSpec.append( argv[ argc-1]);

  // Increment pointers to the optional arguments to get the
  // last argument.
  argc -= optind;
  argv += optind;

  // Set random seed
  srand( (unsigned int) time(0));

  // Restrict format and restruct and set number of plots.  NOTE:
  // nplots will be reset by manage_plot_window_array( NULL)
  assert( dfm.format==BINARY || dfm.format==ASCII);
  assert( nrows*ncols <= MAXPLOTS);
  nplots = nrows*ncols;

  // STEP 2: Read the data file create a 10-d default data set if 
  // the read attempt fails
  if( inFileSpec.length() <= 0) dfm.create_default_data( 10);
  else {
    if( dfm.load_data_file( inFileSpec) != 0) 
      dfm.create_default_data( 10);
  }
  
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

  // Step 4: Call manage_plot_window_array with a NULL argument to
  // to initialize the plot window array.  KLUDGE ALERT: argc and 
  // argv are 'globalized' to make them available to 
  // manage_plot_window_array.
  global_argc = argc;
  global_argv = argv;
  manage_plot_window_array( NULL);

  // Now we can show the main control panel and all its subpanels
  main_control_panel->show();

  // Step 5: Set pointer to the function to call when the window is
  // idle and enter the main event loop
  Fl::add_idle( redraw_if_changing);

  // Enter the main event loop
  int result = Fl::run();
  return result;
}
