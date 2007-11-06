// viewpoints - interactive linked scatterplots and more.
// copyright 2005 Creon Levit and Paul Gazis, all rights reserved.
//***************************************************************************
// File name: brush.h
//
// Class definitions:
//   brush -- brush class
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
// Purpose: Brush class for Creon Levit's viewpoints
//
// General design philosophy:
//   1) This class is still under development, and member variables are
//      publis rather than wrapped in access methods
//
// Author: Creon Levit    14-Aug-2007
// Modified: P. R. Gazis  15-Aug-2007
//***************************************************************************

// Protection to make sure this header is not included twice
#ifndef BRUSH_H
#define BRUSH_H 1

#include <FL/Fl_Pixmap.H>

#include "include_libraries_vp.h"
#include "global_definitions_vp.h"
#include "Vp_Color_Chooser.h"

//*****************************************************************
// Class: Brush
//
// Class definitions:
//   brush
//
// Classes referenced:
//
// Purpose: Derived class of Fl_Group to manage brushes
//
// Functions:
//   Brush( int x, int y, int w, int h) -- Constructor
//
//   make_widgets( Brush *bw) --
//   brush_changed() --
//   change_color() --
//   clear_now() --
//   reset() --
//   build_symbol_menu( void) --
//
// Static functions for access by Fl_Button::callback?
//   static_brush_changed( Fl_Widget *w, Brush *brush) --
//   static_change_color( Fl_Widget *w, Brush *brush) --
//   static_clear_now( Fl_Widget *w, Brush *brush) --
//   static_reset( Fl_Widget *w, Brush *brush) --
//
// Author: Creon Levit    14-Aug-2007
// Modified: P. R. Gazis  15-Aug-2007
//*****************************************************************
class Brush : public Fl_Group
{
  public:
    Brush( int x, int y, int w, int h);

    // number of brushes created
    static int nbrushes;

    // index of this brush in brushes[] array.
    int index;

    static void set_sizes(float size);

    // number of points selected by this brush
    unsigned int count;

    // color used to render this brush's points.
    // MCL XXX it would be good to change these to GLfloat
    static const GLdouble initial_colors[NBRUSHES][4];

    // the rest of the brush's attributes (pointsize, luminance, symbol, etc...)
    // are stored in the value() fields of the appropriate widget
    void make_widgets( Brush *bw);

    // Static functions for access by Fl Widget callbacks
    void brush_changed();
    static void static_brush_changed( Fl_Widget *w, Brush *brush)
    { brush->brush_changed(); }

    void change_color();
    static void static_change_color( Fl_Widget *w, Brush *brush)
    { brush->change_color(); }

    // Pointers to sliders & menus
    Fl_Hor_Value_Slider_Input *pointsize;
    Fl_Hor_Value_Slider_Input *lum1, *lum2, *alpha, *cutoff;
    Vp_Color_Chooser *color_chooser;

    Fl_Button *add_to_selection;

    Fl_Button *clear_now_button;
    void clear_now ();
    static void static_clear_now( Fl_Widget *w, Brush *brush)
    { brush->clear_now(); }

    Fl_Button *reset_button;
    void reset ();
    static void static_reset( Fl_Widget *w, Brush *brush)
    { brush->reset(); }

    // points, round points, crosses, etc.
    Fl_Choice *symbol_menu;
    static Fl_Menu_Item symbol_menu_items[];
    static Fl_Pixmap* symbol_images[];
    void build_symbol_menu (void);
    int previous_symbol;

};

#endif   // BRUSH_H
