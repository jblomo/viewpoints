// Protection to make sure this header is not included twice
#ifndef BRUSH_H
#define BRUSH_H 1

#include "include_libraries_vp.h"
#include "global_definitions_vp.h"

class Brush : public Fl_Group
{
  public:
    Brush( int x, int y, int w, int h);

    // index of this brush in brushes[] array.
    int index;

    // number of points selected by this brush
    unsigned int count;

    // color used to render this brush's points.
    // MCL XXX it would be good to change these to GLfloat
    GLdouble color[4];

    // the rest of the brush's attributes (pointsize, luminance, symbol, etc...)
    // are stored in the value() fields of the appropriate widget
    void make_widgets( Brush *bw);

    // Static functions for access by Fl Widget callbacks
    void brush_changed();
    static void static_brush_changed( Fl_Widget *w, Brush *brush)
    {
        brush->brush_changed();
    }

    void change_color();
    static void static_change_color( Fl_Widget *w, Brush *brush)
    { brush->change_color();}

    // Pointers to sliders & menus
    Fl_Hor_Value_Slider_Input *pointsize;
    Fl_Hor_Value_Slider_Input *lum, *lum2;
    Fl_Button *change_color_button;

    // points, round points, crosses, etc.
    Fl_Choice *symbol_menu;
    static Fl_Menu_Item symbol_menu_items[];
    void build_symbol_menu (void);
};

#endif   // BRUSH_H

