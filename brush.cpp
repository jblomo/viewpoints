// Include the necessary system and 3rd party header files
#include "include_libraries_vp.h"


// Include globals
#include "global_definitions_vp.h"

#include "plot_window.h"
#include "Vp_Color_Chooser.h"
#include "brush.h"
#include <FL/Fl_Pixmap.H>

// default RGBA starting colors for brushes
const GLdouble Brush::initial_colors[NBRUSHES][4] = {
  {1,0,0,1}, // red
  {0,0,1,1}, // blue
  {0,1,0,1}, // green
  {1,0,1,1}, // magenta
  {0,1,1,1}, // cyan
  {1,1,0,1}, // yellow
  {0.5,0.5,0.5,1}, // grey
};
  
// number of brushes created
int Brush::nbrushes = 0;

Brush::Brush(int x, int y, int w, int h) : Fl_Group( x, y, w, h)
{
  index = nbrushes++;
  count = 0;
  previous_symbol = 0;

  label("@square");

  make_widgets(this);
  end();

  double c[3] = {Brush::initial_colors[index][0],Brush::initial_colors[index][1],Brush::initial_colors[index][2]};
  color_chooser->rgb(c[0], c[1], c[2]);
  labelcolor(fl_rgb_color((uchar)(c[0]*255), (uchar)(c[1]*255), (uchar)(c[2]*255)));
  clear_visible_focus();
  // XXX someday a brush's tab's label will show a colored image of the brush's current symbol...
  // XXX (probably after we switch to FLTK 2)
  // image(symbol_images[0]);
  // int fl_draw_pixmap(char **data, int X, int Y, Fl_Color = FL_GRAY)
}

void Brush::brush_changed() {
  // pointsize of 1 is too small to see symbols, but OK for standard GL points
  if (previous_symbol == 0 && symbol_menu->value() != 0) {
    if (pointsize->value() < 3) {
      pointsize->value(3);
    }
  }
  previous_symbol = symbol_menu->value();
  Plot_Window::redraw_all_plots(0);
}

void Brush::change_color () {
  double c[3] = {color_chooser->r(), color_chooser->g(), color_chooser->b()};
  labelcolor(fl_rgb_color((uchar)(c[0]*255), (uchar)(c[1]*255), (uchar)(c[2]*255)));
  redraw_label();
	// keep tab's (parent's) colored labels updated while the brush color changes and the tab is selected
  brushes_tab->labelcolor(labelcolor());
  brushes_tab->redraw_label();
  brush_changed();
}

void Brush::reset () 
{
  double c[3] = {Brush::initial_colors[index][0],Brush::initial_colors[index][1],Brush::initial_colors[index][2]};
  color_chooser->rgb(c[0], c[1], c[2]);
  change_color ();
  symbol_menu->value(0);
  pointsize->value(default_pointsize);
  alpha->value(1.0);
  lum1->value(0.2);  // !!! 
  lum2->value(1.0);
}

// clear all points that are currently selected using this brush
// leave all other brush's selections alone.
void Brush::clear_now () {
  // (selected - index) == 0 iff currently selected by this brush:
  selected = where(selected-index,selected,0);
  previously_selected = selected;
  // redraw based on changed selection.  Candidate for "pull out method(s)" refactoring?
  pws[1]->color_array_from_selection();
  pws[1]->redraw_all_plots(0);
}

void Brush::make_widgets(Brush *bw)
{
  // Since these (virtual) control panels are really groups inside a tab 
  // inside a window, set their child widget's coordinates relative to 
  // their enclosing window's position. 
  int xpos = this->x()+50;
  int ypos = this->y()+20;

  // Fl_Button *b;

  // point size slider for this brush
  pointsize = new Fl_Hor_Value_Slider_Input( xpos, ypos, bw->w()-145, 20, "size");
  pointsize->align(FL_ALIGN_LEFT);
  pointsize->value(default_pointsize);
  pointsize->step(0.25);
  pointsize->bounds(1.0,50.0);
  pointsize->callback((Fl_Callback*)static_brush_changed, this);

  // symbol types menu for this brush
  symbol_menu = new Fl_Choice(xpos+pointsize->w()+35, ypos, 45, 20);
  // call a method to do the dirty work of setting up all the glyphs for the symbols menu.
  build_symbol_menu ();
  symbol_menu->textsize(12);
  symbol_menu->down_box(FL_NO_BOX);
  symbol_menu->clear_visible_focus(); // MCL XXX - I think this should be set for all Fl_Choice widgets and perhaps more.
  symbol_menu->color(FL_WHITE);
  symbol_menu->menu(symbol_menu_items);
  symbol_menu->label("sym");
  symbol_menu->value(0);
  symbol_menu->callback( (Fl_Callback*)static_brush_changed, this);

  // Alpha slider
  alpha = new Fl_Hor_Value_Slider_Input( xpos, ypos+=25, bw->w()-60, 20, "alpha");
  alpha->align(FL_ALIGN_LEFT);
  alpha->callback((Fl_Callback*)static_brush_changed, this);
  alpha->step(0.0001);
  alpha->bounds(0.0,1.0);
  alpha->value(1.0);

  // Alpha0 slider
  alpha0 = new Fl_Hor_Value_Slider_Input( xpos, ypos+=25, bw->w()-60, 20, "alpha0");
  alpha0->align(FL_ALIGN_LEFT);
  alpha0->callback((Fl_Callback*)static_brush_changed, this);
  alpha0->step(0.0001);
  alpha0->bounds(0.0,5.0);
  alpha0->value(5.0);  // !!!
  // we don't need this control, for now.
  alpha0->hide(); ypos-=alpha0->h();
  

  // Initial luminosity slider
  lum1 = new Fl_Hor_Value_Slider_Input( xpos, ypos+=25, bw->w()-60, 20, "lum1");
  lum1->align(FL_ALIGN_LEFT);
  lum1->callback((Fl_Callback*)static_brush_changed, this);
  lum1->step(0.0001);
  lum1->bounds(0.0,1.0);
  lum1->value(0.2);  // !!!

  // Luminosity accumulation factor slider
  lum2 = new Fl_Hor_Value_Slider_Input( xpos, ypos+=25, bw->w()-60, 20, "lum2");
  lum2->align(FL_ALIGN_LEFT);
  lum2->callback((Fl_Callback*)static_brush_changed, this);
  lum2->step(0.0001);
  lum2->bounds(0.0,2.0); 
  lum2->value(1.0);

  color_chooser = new Vp_Color_Chooser(xpos-25, ypos+=25, 150, 75, ""); // XXX do not remove the "".
  color_chooser->callback((Fl_Callback*)static_change_color, this);
  color_chooser->labelfont(FL_HELVETICA);
  color_chooser->labelsize(10);

  // MCL XXX there should be a "selection" menu: extend, subtract, replace, replace always, invert?
  add_to_selection = new Fl_Button( xpos+=150, ypos, 20, 20, "extend selection");
  add_to_selection->align( FL_ALIGN_RIGHT); 
  add_to_selection->selection_color( FL_BLUE); 
  add_to_selection->type( FL_TOGGLE_BUTTON);
  add_to_selection->value( index?0:1);  // all brushes default this to off, except brush 0.

  clear_now_button = new Fl_Button( xpos, ypos+=25, 20, 20, "clear selection");
  clear_now_button->align( FL_ALIGN_RIGHT); 
  clear_now_button->selection_color( FL_BLUE); 
  clear_now_button->callback((Fl_Callback*)static_clear_now, this);
  clear_now_button->value( 0);  

  reset_button = new Fl_Button( xpos, ypos+=25, 20, 20, "reset brush");
  reset_button->align( FL_ALIGN_RIGHT); 
  reset_button->selection_color( FL_BLUE); 
  reset_button->callback((Fl_Callback*)static_reset, this);
  reset_button->value( 0);  

}


