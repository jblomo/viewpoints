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
//   1) This class is still under development and member variables are
//      left public rather than wrapped in access methods
//
// Author: Creon Levit    14-AUG-2007
// Modified: P. R. Gazis  08-JUL-2008
//***************************************************************************

// Protection to make sure this header is not included twice
#ifndef BRUSH_H
#define BRUSH_H 1

#include <FL/Fl_Pixmap.H>

#include "include_libraries_vp.h"
#include "global_definitions_vp.h"
#include "Vp_Color_Chooser.h"

//***************************************************************************
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
//   Brush() -- Default constructor
//   Brush( int x, int y, int w, int h) -- Constructor
//   serialize( &ar, iFileVersion) -- Perform serialization
//   make_state() -- Generate state parameters for this brush
//   copy_state( *brush_save) -- Copy state parameters from another brush
//   load_state() -- Load state parameters into widgets
//
//   make_widgets( Brush *bw) -- Build control panel
//   brush_changed() -- Redraw bush if it changed
//   change_color() -- Change color
//   clear_now() --
//   reset() -- Reset brush
//   build_symbol_menu( void) -- Build symbol menu
//
// Static functions for access by Fl_Button::callback?
//   static_brush_changed( Fl_Widget *w, Brush *brush) -- Callback
//   static_change_color( Fl_Widget *w, Brush *brush) -- Callback
//   static_clear_now( Fl_Widget *w, Brush *brush) -- Callback
//   static_reset( Fl_Widget *w, Brush *brush) -- Callback
//
// Author: Creon Levit    14-AUG-2007
// Modified: P. R. Gazis  08-JUL-2008
//***************************************************************************
class Brush : public Fl_Group
{
  protected:
    // Need this declaration to grant the serialization library access to 
    // private member variables and functions.
#ifdef SERIALIZATION
    friend class boost::serialization::access;
#endif // SERIALIZATION
    
    // Define buffers to save state
    int brush_symbol_save;
    float brush_size_save;
    float alpha_save, cutoff_save, lum1_save, lum2_save;
    double red_value_save, green_value_save, blue_value_save;

    // When the class Archive corresponds to an output archive, the &
    // operator is defined similar to <<.  Likewise, when the class Archive 
    // is a type of input archive the & operator is defined similar to >>.
    // It is easiest to define this serialize method inline.
#ifdef SERIALIZATION
    template<class Archive>
    void serialize( Archive & ar, const unsigned int /* file_version */)
    {
      // Use a dynamic_cast to determine if this is an output operation
      if( (dynamic_cast<boost::archive::xml_oarchive *> (&ar))) make_state();

      // Embed serialization in a try-catch loop so we can pass exceptions
      try{
        ar & boost::serialization::make_nvp( "index", index);
        ar & boost::serialization::make_nvp( "brush_size", brush_size_save);
        ar & boost::serialization::make_nvp( "brush_symbol", brush_symbol_save);
        ar & boost::serialization::make_nvp( "alpha", alpha_save);
        ar & boost::serialization::make_nvp( "cutoff", cutoff_save);
        ar & boost::serialization::make_nvp( "luminosity_1", lum1_save);
        ar & boost::serialization::make_nvp( "luminosity_2", lum2_save);
        ar & boost::serialization::make_nvp( "red_value", red_value_save);
        ar & boost::serialization::make_nvp( "green_value", green_value_save);
        ar & boost::serialization::make_nvp( "blue_value", blue_value_save);
      }
      catch( exception &e) {}
    }
#endif //SERIALIZATION

  public:
    Brush();
    Brush( int x, int y, int w, int h);
    void make_state();
    void copy_state( Brush* brush_save);
    void load_state();

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

    Fl_Button *add_to_selection, *paint;

    Fl_Button *clear_now_button;
    void clear_now ();
    static void static_clear_now( Fl_Widget *w, Brush *brush)
    { brush->clear_now(); }

    Fl_Button *reset_button;
    void reset ();
    static void static_reset( Fl_Widget *w, Brush *brush)
    { brush->reset(); }

    // Symbol menu with points, round points, crosses, etc.
    Fl_Choice *symbol_menu;
    static Fl_Menu_Item symbol_menu_items[];
    static Fl_Pixmap* symbol_images[];
    void build_symbol_menu( void);
    int previous_symbol;
    
    // Static member Fl_Pixmap objects to hold pixel maps of symbols
    static Fl_Pixmap image_0;
    static Fl_Pixmap image_line;
    static Fl_Pixmap image_1;
    static Fl_Pixmap image_2;
    static Fl_Pixmap image_3;
    static Fl_Pixmap image_4;
    static Fl_Pixmap image_5;
    static Fl_Pixmap image_6;
    static Fl_Pixmap image_7;
    static Fl_Pixmap image_8;
    static Fl_Pixmap image_9;
    static Fl_Pixmap image_10;
    static Fl_Pixmap image_11;
    static Fl_Pixmap image_12;
    static Fl_Pixmap image_13;
    static Fl_Pixmap image_14;
    static Fl_Pixmap image_18;
    static Fl_Pixmap image_19;

    // Static member Fl_Pixmap objects to hold pixel maps of numbers and 
    // letters
    static Fl_Pixmap image_osaka_21;
    static Fl_Pixmap image_osaka_22;
    static Fl_Pixmap image_osaka_23;
    static Fl_Pixmap image_osaka_24;
    static Fl_Pixmap image_osaka_25;
    static Fl_Pixmap image_osaka_26;
    static Fl_Pixmap image_osaka_27;
    static Fl_Pixmap image_osaka_28;
    static Fl_Pixmap image_osaka_29;
    static Fl_Pixmap image_osaka_30;
    static Fl_Pixmap image_osaka_38;
    static Fl_Pixmap image_osaka_39;
    static Fl_Pixmap image_osaka_40;
    static Fl_Pixmap image_osaka_41;
    static Fl_Pixmap image_osaka_42;
    static Fl_Pixmap image_osaka_43;
    static Fl_Pixmap image_osaka_44;
    static Fl_Pixmap image_osaka_45;
    static Fl_Pixmap image_osaka_46;
    static Fl_Pixmap image_osaka_47;
    static Fl_Pixmap image_osaka_48;
    static Fl_Pixmap image_osaka_49;
    static Fl_Pixmap image_osaka_50;
    static Fl_Pixmap image_osaka_51;
    static Fl_Pixmap image_osaka_52;
    static Fl_Pixmap image_osaka_53;
    static Fl_Pixmap image_osaka_54;
    static Fl_Pixmap image_osaka_55;
    static Fl_Pixmap image_osaka_56;
    static Fl_Pixmap image_osaka_57;
    static Fl_Pixmap image_osaka_58;
    static Fl_Pixmap image_osaka_59;
    static Fl_Pixmap image_osaka_60;
    static Fl_Pixmap image_osaka_61;
    static Fl_Pixmap image_osaka_62;
    static Fl_Pixmap image_osaka_63;
};

#endif   // BRUSH_H
