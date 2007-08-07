// Include the necessary system and 3rd party header files
#include "include_libraries_vp.h"


// Include globals
#include "global_definitions_vp.h"

#include "plot_window.h"
#include "brush.h"

// default RGBA starting colors for brushes
const GLdouble Brush::initial_colors[NBRUSHES][4] = {
  {1,0,0,1},
  {0,1,0,1},
  {0,0,1,1},
  {0,1,1,1},
  {1,0,1,1},
  {1,1,0,1},
  {0.5,0.5,0.5,1},
  {0,0,0,0}
};
  

// Array of menu items for fixed color selection.  There are 7 fixed
// colors (R, G, B, C, M, Y, Grey).
//Fl_Menu_Item Brush::fixed_colors[7+1] = { Fl_Menu_Item()};

//***************************************************************************
// Brush::Brush( x, y, w, h) --  Default 
// constructor.  Do nothing except call the constructor for the parent 
// class, Fl_Group.
Brush::Brush(int x, int y, int w, int h) : Fl_Group( x, y, w, h)
{
  count = 0;
	// XXX color should be initialized differently for each brush
  color[0] = 1.0; color[1] = 0.0; color[2] = 0.0; color[3] = 0.5; 
}

// could this be replaced by redraw_all_plots() ?
void Brush::brush_changed() {
  Plot_Window::redraw_all_plots(0);
}

void Brush::change_color () {
  (void) fl_color_chooser( "brush color", color[0], color[1], color[2]);
  brush_changed();
}

void Brush::make_widgets(Brush *bw)
{
  // Since these (virtual) control panels are really groups inside a tab 
  // inside a window, set their child widget's coordinates relative to 
  // their enclosing window's position. 
  int xpos = this->x()+50;
  int ypos = this->y()+20;

  Fl_Button *b;

  // point size slider for this brush
  pointsize = new Fl_Hor_Value_Slider_Input( xpos, ypos, bw->w()-145, 20, "size");
  pointsize->align(FL_ALIGN_LEFT);
  pointsize->value(default_pointsize);
  pointsize->step(0.25);
  pointsize->bounds(1.0,50.0);
  pointsize->callback((Fl_Callback*)static_brush_changed, this);

  // symbol types menu for this brush
  symbol_menu = new Fl_Choice(xpos+pointsize->w()+45, ypos, 45, 20);
  // call a method to do the dirty work of setting up all the glyphs for the symbols menu.
  build_symbol_menu ();
  symbol_menu->textsize(12);
  symbol_menu->down_box(FL_NO_BOX);
  symbol_menu->clear_visible_focus(); // MCL XXX - I think this should be set for all widgets
  symbol_menu->color(FL_WHITE);
  symbol_menu->menu(symbol_menu_items);
  symbol_menu->label("sym");
  symbol_menu->value(0);
  symbol_menu->callback( (Fl_Callback*)static_brush_changed, this);

  // Initial luminosity slider
  lum = new Fl_Hor_Value_Slider_Input( xpos, ypos+=25, bw->w()-60, 20, "lum1");
  lum->align(FL_ALIGN_LEFT);
  lum->callback((Fl_Callback*)static_brush_changed, this);
  lum->step(0.0001);
  lum->bounds(0.0,1.0);
  lum->value(0.04);  // !!!

  // Luminosity accumulation factor slider
  lum2 = new Fl_Hor_Value_Slider_Input( xpos, ypos+=25, bw->w()-60, 20, "lum2");
  lum2->align(FL_ALIGN_LEFT);
  lum2->callback((Fl_Callback*)static_brush_changed, this);
  lum2->step(0.0001);
  lum2->bounds(0.0,5.0);
  lum2->value(1.0);

  // Button (1,3): Pop up color a chooser for this brush
  change_color_button = b = new Fl_Button(xpos, ypos+=25, 20, 20, "change color");
  b->align(FL_ALIGN_RIGHT); 
  b->selection_color(FL_BLUE); 
  b->callback((Fl_Callback*)static_change_color, this);
}
