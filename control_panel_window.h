// viewpoints - interactive linked scatterplots and more.
// copyright 2005 Creon Levit and Paul Gazis, all rights reserved.
//*****************************************************************
// File name: control_panel_window.h
//
// Class definitions:
//   Control_Panel_Window -- Control panel window
//
// Classes referenced:
//   Plot_Window -- Plot window
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
//      normalization schemes used here and by class Plot_Windows.
//
// Author: Creon Levit    2005-2006
// Modified: P. R. Gazis  15-DEC-2007
//*****************************************************************

// Protection to make sure this header is not included twice
#ifndef CONTROL_PANEL_WINDOW_H
#define CONTROL_PANEL_WINDOW_H 1

// Include the extra BOOST library required to split SERIALIZE 
// into SAVE and LOAD
// #include <boost/serialization/split_member.hpp>

// Include the necessary include libraries
#include "include_libraries_vp.h"

// Include globals
#include "global_definitions_vp.h"

// Declare class Plot_Window here so it can be referenced
// class Plot_Window;

// Include associated headers and source code.  NOTE: not needed if
// this class has already been declared
#include "plot_window.h"

//*****************************************************************
// Class: Control_Panel_Window
//
// Class definitions:
//   Control_Panel_Window
//
// Classes referenced:
//   Plot_Window -- Maintain and manage plot window
//
// Purpose: Derived class of Fl_Gl_Window to construct, draw,
//   and manage a plot window
//
// Functions:
//   Control_Panel_Window() -- Default Constructor
//   Control_Panel_Window( x, y, w, h) -- Constructor
//   serialize( &ar, iFileVersion) -- Perform serialization
//   make_state() -- Generate and save state parameters for this window
//   copy_state( *cp) -- Copy state parameters from another window
//   load_state() -- Load state parameters into widgets
//
//   maybe_redraw() -- Set redraw flag nicely
//   make_widgets( *cpw) -- Make widgets for this tab
//   extract_and_redraw() -- extract a variable, renormalize it, etc.
//
//   transform_style_value() -- Get y-axis transform style
//   transform_style_value( transform_style_in) -- Set y-axis transform style
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
// Author: Creon Levit    2005-2006
// Modified: P. R. Gazis  15-DEC-2007
//*****************************************************************
class Control_Panel_Window : public Fl_Group
{
  protected:
    // Need this to grant the serialization library access to private member 
    // variables and functions.
    friend class boost::serialization::access;
    
    // Define state parameters used by serialization
    int ivar_save, jvar_save, kvar_save;
    int ix_style, jy_style, kz_style;
    int ix_lock, jy_lock, kz_lock;
    float background_save, luminosity_save, point_size_save;
    int scale_points_save;
    int transform_style_save;

    // When the class Archive corresponds to an output archive, the &
    // operator is defined similar to <<.  Likewise, when the class Archive 
    // is a type of input archive the & operator is defined similar to >>.
    // It is easiest to define this method inline.
    template<class Archive>
    void serialize( Archive & ar, const unsigned int /* file_version */)
    {
      // Use a dynamic_cast to determine if this is an output operation.
      // If it is, then call make_state() to set the state parameters
      if( (dynamic_cast<boost::archive::xml_oarchive *> (&ar))) make_state();

      // Embed serialization in a try-catch loop so we can pass exceptions
      try{
        ar & boost::serialization::make_nvp( "index", index);
        ar & boost::serialization::make_nvp( "varindex1", ivar_save);
        ar & boost::serialization::make_nvp( "varindex2", jvar_save);
        ar & boost::serialization::make_nvp( "varindex3", kvar_save);
        ar & boost::serialization::make_nvp( "x_normalization_style", ix_style);
        ar & boost::serialization::make_nvp( "y_normalization_style", jy_style);
        ar & boost::serialization::make_nvp( "z_normalization_style", kz_style);
        ar & boost::serialization::make_nvp( "lock_axis1_button", ix_lock);
        ar & boost::serialization::make_nvp( "lock_axis2_button", jy_lock);
        ar & boost::serialization::make_nvp( "lock_axis3_button", kz_lock);
        ar & boost::serialization::make_nvp( "background", background_save);
        ar & boost::serialization::make_nvp( "luminosity", luminosity_save);
        ar & boost::serialization::make_nvp( "point_size", point_size_save);
        ar & boost::serialization::make_nvp( "scale_points", scale_points_save);
        ar & boost::serialization::make_nvp( "transform_style", transform_style_save);
      }
      catch( exception &e) {}
    }
    
    void maybe_redraw();

  public:
    Control_Panel_Window();
    Control_Panel_Window( int x, int y, int w, int h);
    void make_state();
    void copy_state( Control_Panel_Window* cp);
    void load_state();

    void make_widgets( Control_Panel_Window *cpw);
    void extract_and_redraw();

    // Access functions
    int transform_style_value();
    void transform_style_value( int transform_style_in);
    
    // Static functions for access by Fl Widget callbacks
    static void broadcast_change( Fl_Widget *global_widget);
    static void static_extract_and_redraw( Fl_Widget *w, Control_Panel_Window *cpw)
    { cpw->extract_and_redraw(); }
    static void static_maybe_redraw( Fl_Widget *w, Control_Panel_Window *cpw)
    { cpw->maybe_redraw() ;}
    static void replot( Fl_Widget *w, Control_Panel_Window *cpw)
    { cpw->pw->needs_redraw=1;}
    static void reset_view( Fl_Widget *w, Control_Panel_Window *cpw)
    { cpw->pw->reset_view() ;}
    static void redraw_one_plot( Fl_Widget *w, Control_Panel_Window *cpw)
    { cpw->pw->redraw_one_plot();}

    // Pointers to sliders & menus
    // Fl_Hor_Value_Slider_Input *pointsize_slider, *selected_pointsize_slider;
    Fl_Hor_Value_Slider_Input *Bkg, *lum;
    Fl_Hor_Value_Slider_Input *rot_slider, *size;
    Fl_Hor_Value_Slider_Input *nbins_slider[3], *hscale_slider[3];
    Fl_Choice *varindex1, *varindex2, *varindex3;
    Fl_Button *lock_axis1_button, *lock_axis2_button, *lock_axis3_button;
    Vp_Value_Input_Spin *offset[3];

    // Pointers to buttons
    Fl_Button *reset_view_button;
    Fl_Button *scale_points;
    Fl_Button *spin, *dont_clear, *show_points, *show_deselected_points;
    Fl_Button *show_axes, *show_grid, *show_labels;

    Fl_Menu_Button *show_histogram[3];
    enum histogram_styles {
        HISTOGRAM_MARGINAL = 0,
        HISTOGRAM_SELECTION,   
        HISTOGRAM_CONDITIONAL,
        HISTOGRAM_WEIGHTED
    };

    Fl_Button *show_scale;
    Fl_Button *choose_selection_color_button;
    Fl_Button *z_bufferring_button;
    // Fl_Button *x_equals_delta_x, *y_equals_delta_x;
    Fl_Group *transform_style;
    Fl_Button *sum_vs_difference, *cond_prop, *fluctuation, *no_transform;

    Fl_Choice *x_normalization_style, 
              *y_normalization_style, 
              *z_normalization_style;

    enum normalization_style {
      NORMALIZATION_NONE = 0,
      NORMALIZATION_MINMAX,
      NORMALIZATION_ZEROMAX,
      NORMALIZATION_MAXABS,
      NORMALIZATION_TRIM_1E2,
      NORMALIZATION_TRIM_1E3,
      NORMALIZATION_THREESIGMA,
      NORMALIZATION_LOG10,
      NORMALIZATION_SQUASH,
      NORMALIZATION_RANK,
      NORMALIZATION_PARTIAL_RANK,
      NORMALIZATION_GAUSSIANIZE,
      NORMALIZATION_RANDOMIZE
    };
    static Fl_Menu_Item normalization_style_menu_items[];

    static Fl_Menu_Item varindex_menu_items[]; 

    Fl_Choice *blend_menu;
    enum blend_styles {
        BLEND_OVERPLOT = 0,
        BLEND_OVERPLOT_WITH_ALPHA,
        BLEND_BRUSHES_SEPARATELY,
        BLEND_ALL_BRUSHES,
        BLEND_ALL2,
        BLEND_ALL3
    };

    // Pointer to and index of the plot window associated with 
    // this control panel tab.  Each plot window has the same 
    // color and index as its associated control panel tab.
    Plot_Window *pw;
    int index;  
};

#endif   // CONTROL_PANEL_WINDOW_H
