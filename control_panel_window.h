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
// Author: Creon Levit    2005-2006
// Modified: P. R. Gazis  26-OCT-2006
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

// Include associated headers and source code.  NOTE: not needed if
// this class has already been declared
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
//   maybe_redraw() -- Set redraw flag nicely
//   make_widgets( *cpw) -- Make widgets for this tab
//   extract_and_redraw() -- extract a variable, renormalize it, etc.
//
// Static functions for access by Fl_Button::callback
//   choose_color_selected( *w, *cpw) -- Color of selected points
//   static_extract_and_redraw( *w, *cpw) -- extract a variable, renormalize it, etc.
//   static_maybe_redraw( *w, *cpw) -- Set redraw flag nicely.
//   replot( *w, *cpw) -- set redraw flag.
//   reset_view( *w, *cpw) -- Reset one plot's view
//   redraw_one_plot( *w, *cpw) -- Redraw one plot
//
//   This comment also conveys nothing.
//
// Author: Creon Levitt   unknown
// Modified: P. R. Gazis  08-MAR-2006
//*****************************************************************
class control_panel_window : public Fl_Group
{
  protected:
    void maybe_redraw();

  public:
    control_panel_window( int x, int y, int w, int h);
    void make_widgets( control_panel_window *cpw);
    void extract_and_redraw();

    // Static functions for access by Fl Widget callbacks
    static void broadcast_change( Fl_Widget *global_widget);
    static void choose_color_selected( Fl_Widget *w, control_panel_window *cpw)
    { cpw->pw->choose_color_selected() ;}
    static void static_extract_and_redraw( Fl_Widget *w, control_panel_window *cpw)
    { cpw->extract_and_redraw(); }
    static void static_maybe_redraw( Fl_Widget *w, control_panel_window *cpw)
    { cpw->maybe_redraw() ;}
    static void replot( Fl_Widget *w, control_panel_window *cpw)
    { cpw->pw->needs_redraw=1;}
    static void reset_view( Fl_Widget *w, control_panel_window *cpw)
    { cpw->pw->reset_view() ;}
    static void redraw_one_plot( Fl_Widget *w, control_panel_window *cpw)
    { cpw->pw->redraw_one_plot();}

    // Pointers to sliders & menus
    Fl_Hor_Value_Slider_Input *pointsize_slider, *selected_pointsize_slider;
    Fl_Hor_Value_Slider_Input *Bkg, *Lum, *Lum2, *Alph;
    Fl_Hor_Value_Slider_Input *rot_slider;
    Fl_Hor_Value_Slider_Input *nbins_slider;
    Fl_Choice *varindex1, *varindex2, *varindex3;
    Fl_Button *lock_axis1_button, *lock_axis2_button, *lock_axis3_button;
  
    // Pointers to buttons
    Fl_Button *reset_view_button;
    Fl_Button *spin, *dont_clear, *show_points, 
              *show_deselected_points, 
              *show_axes, *show_grid, *show_labels, 
              *show_histogram;
    Fl_Button *show_scale;
    Fl_Button *choose_selection_color_button;
    Fl_Button *z_bufferring_button;
    // Fl_Button *x_equals_delta_x, *y_equals_delta_x;
    Fl_Group *transform_style;
    Fl_Button *sum_vs_difference, *polar, *no_transform;
    Fl_Choice *x_normalization_style, 
              *y_normalization_style, 
              *z_normalization_style;

    // points, round points, crosses, etc.
    Fl_Choice *symbol_menu;
    enum symbol_type {
        SQUARE_POINTS,
        SMOOTH_POINTS,
        SPRITES
    };
    static Fl_Menu_Item symbol_menu_items[];

    // Pointer to and index of the plot window associated with 
    // this control panel tab.  Each plot window has the same 
    // color and index as its associated control panel tab.
    plot_window *pw;
    int index;  
    
    // constants that describe normalization styles.  
    // MCL XXX this stuff is now full of icky magic numbers like 11 and 12.  Fix!
    // use enums.
    static const int control_panel_window::n_normalization_styles = 11;
    static const int control_panel_window::NORMALIZATION_NONE   = 0;
    static const int control_panel_window::NORMALIZATION_MINMAX = 1;
    static const int control_panel_window::NORMALIZATION_ZEROMAX = 2;
    static const int control_panel_window::NORMALIZATION_MAXABS = 3;
    static const int control_panel_window::NORMALIZATION_TRIM_1E2 = 4;
    static const int control_panel_window::NORMALIZATION_TRIM_1E3 = 5;
    static const int control_panel_window::NORMALIZATION_THREESIGMA = 6;
    static const int control_panel_window::NORMALIZATION_LOG10 = 7;
    static const int control_panel_window::NORMALIZATION_SQUASH = 8;
    static const int control_panel_window::NORMALIZATION_RANK = 9;
    static const int control_panel_window::NORMALIZATION_GAUSSIANIZE = 10;

    static Fl_Menu_Item varindex_menu_items[]; 

    // Static variables that use and apply different normalization 
    // styles.
    static Fl_Menu_Item normalization_style_menu_items[];
    static int normalization_styles[];  
    static char *normalization_style_labels[];

};

#endif   // CONTROL_PANEL_WINDOW_H
