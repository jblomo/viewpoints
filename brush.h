// Protection to make sure this header is not included twice
#ifndef BRUSH_H
#define BRUSH_H 1

#include <FL/Fl_Pixmap.H>

#include "include_libraries_vp.h"
#include "global_definitions_vp.h"
#include "Vp_Color_Chooser.h"

class Brush : public Fl_Group
{
  public:
    Brush( int x, int y, int w, int h);

	// number of brushes created
    static int nbrushes;

    // index of this brush in brushes[] array.
    int index;

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
    Fl_Hor_Value_Slider_Input *lum1, *lum2, *alpha, *alpha0;
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

