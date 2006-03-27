// viewpoints - interactive linked scatterplots and more.
// copyright 2005 Creon Levit, all rights reserved.
//*****************************************************************
// File name: control_panel_window.h
//
// Class definitions:
//   control_panel_window -- Control panel window
//
// Classes referenced:
//   plot_window -- Plot window
//   May require various BLITZ templates
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
// Purpose: Control panel window class for Creon Levitt's 
//   viewpoints
//
// General design philosophy:
//   1) This might be a good place to consolidate references to
//      normalization schemes used here and by class plot_windows.
//
// Author: Creon Levitt   unknown
// Modified: P. R. Gazis  27-MAR-2006
//*****************************************************************

// Protection to make sure this header is not included twice
#ifndef CONTROL_PANEL_WINDOW_H
#define CONTROL_PANEL_WINDOW_H 1

// Include the necessary include libraries
#include "include_libraries_vp.h"

// Include globals
#include "global_definitions_vp.h"

// Declare class plot_window here so it can be referenced
// class plot_window;

// Include associated headers and source code.  NOTE: not needed 
// if class has been declared
#include "plot_window.h"

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
//   maybe_redraw() -- Set redraw flag
//   make_widgets( *cpw) -- Make widgets for this tab
//   extract_and_redraw() -- ???
//
// Static functions for access by Fl_Button::callback
//   choose_color_selected( *w, *cpw) -- Color of selected points
//   static_extract_and_redraw( *w, *cpw) -- ???
//   static_maybe_redraw( *w, *cpw) -- Set redraw flag
//   replot( *w, *cpw) -- Replot data
//   reset_view( *w, *cpw) -- Reset view
//   redraw_one_plot( *w, *cpw) -- Redraw one plot
//
// Author: Creon Levitt   unknown
// Modified: P. R. Gazis  27-MAR-2006
//*****************************************************************
class control_panel_window : public Fl_Group
{
  protected:
    void maybe_redraw();

  public:
    control_panel_window( int x, int y, int w, int h);
    void make_widgets( control_panel_window *cpw);
    void extract_and_redraw();

    // Static functions for access by Fl_Button::callback
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

    // Pointers to results from sliders
    Fl_Hor_Value_Slider_Input *pointsize_slider;
    Fl_Hor_Value_Slider_Input *Bkg, *Lum, *Alph;
    Fl_Hor_Value_Slider_Input *rot_slider;
    Fl_Hor_Value_Slider_Input *nbins_slider;
    Fl_Choice *varindex1, *varindex2, *varindex3;
	
    // Pointers to results from buttons
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
    
    // Variables that describe normalization schemes.  These were
    // moved here from main routine because they are specific to
    // this class.
    static Fl_Menu_Item varindex_menu_items[ nvars_max+2]; 
    static int normalization_styles[ 11];
    static char *normalization_style_labels[ 11];
    static Fl_Menu_Item 
      normalization_style_menu_items[ n_normalization_styles+1];
};

// Set static member variable to hold menu items.  NOTE: Since
// this is an array, this is a complicated process.
Fl_Menu_Item 
  control_panel_window::varindex_menu_items[ nvars_max+2] = 
  { Fl_Menu_Item()};

// Set the array of normalization schemes.  NOTE: If possible, 
// this should be be made CONST.
int control_panel_window::normalization_styles[ 11] = {
  NORMALIZATION_NONE, NORMALIZATION_MINMAX,
  NORMALIZATION_ZEROMAX, NORMALIZATION_MAXABS, 
  NORMALIZATION_TRIM_1E2, NORMALIZATION_TRIM_1E3, 
  NORMALIZATION_THREESIGMA, NORMALIZATION_LOG10,
  NORMALIZATION_SQUASH, NORMALIZATION_RANK, 
  NORMALIZATION_GAUSSIANIZE};

// Set the array of character arrays that describe normalization
// schemes.  NOTE: If possible, this should be be made CONST.
char *control_panel_window::normalization_style_labels[ 11] = { 
  "none", "minmax", "zeromax", "maxabs", "trim 10^-2", 
  "trim 10^-3", "threesigma", "log_10", "squash", "rank",
  "gaussianize"};

// Set static member variable to hold menu of normalization 
// styles. NOTE: Since this is an array, this is a complicated 
// process.
Fl_Menu_Item 
  control_panel_window::normalization_style_menu_items[ 
    n_normalization_styles+1] =
      { Fl_Menu_Item()};

#endif   // CONTROL_PANEL_WINDOW_H
