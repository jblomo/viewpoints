// viewpoints - interactive linked scatterplots and more.
// copyright 2005 Creon Levit and Paul Gazis, all rights reserved.
//***************************************************************************
// File name: brush.cpp
//
// Class definitions:
//   brush -- brush class
//
// Classes referenced:
//   Plot_Window -- Plot window
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
// Purpose: Source code for <brush.h>
//
// Author: Creon Levit    14-AUG-2007
// Modified: P. R. Gazis  15-DEC-2007
//***************************************************************************

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

//***************************************************************************
// Control_Panel_Window::Control_Panel_Window() --  Default constructor with
// no arguments.  Used only for serialization.  Do nothing except call the 
// constructor for the parent class, Fl_Group, with dummy arguments.
Brush::Brush() : Fl_Group( 10, 10, 10, 10),
  index( 0),
  brush_symbol_save( 0), brush_size_save( 0),
  alpha_save( 1.0), cutoff_save( 0.0), lum1_save( 0.2), lum2_save( 1.0),
  red_value_save( 1.0), green_value_save( 0), blue_value_save( 0)
{}

//***************************************************************************
// Brush::Brush( x, y, w, h) --  Constructor.  Set parameters and tab shape, 
// invoke the make_widgets() method to generate the control panel
Brush::Brush(int x, int y, int w, int h) : Fl_Group( x, y, w, h)
{
  // Set parametera
  index = nbrushes++;
  count = 0;
  previous_symbol = 0;

  // Set tab shape
  if( index > 0) {
    label( "@circle");
    labelsize( 15);
  } 
  else {
    label( "@square");
  }

  // Invoke make_widgets() method to generate the control panel, then end
  // the group
  make_widgets(this);
  end();

  // Set colors and clear focos
  double c[3] = {
    Brush::initial_colors[index][0],
    Brush::initial_colors[index][1],
    Brush::initial_colors[index][2]};
  color_chooser->rgb(c[0], c[1], c[2]);
  labelcolor(fl_rgb_color((uchar)(c[0]*255), (uchar)(c[1]*255), (uchar)(c[2]*255)));
  clear_visible_focus();

  // XXX someday a brush's tab's label will show a colored image of the brush's current symbol...
  // XXX (probably after we switch to FLTK 2)
  // image(symbol_images[0]);
  // int fl_draw_pixmap(char **data, int X, int Y, Fl_Color = FL_GRAY)
}

//***************************************************************************
// Brush::make_state() -- Examine widgets to generate and store state
// parameters.
void Brush::make_state()
{
  brush_size_save = pointsize->value();
  brush_symbol_save = symbol_menu->value();
  alpha_save = alpha->value();
  cutoff_save = cutoff->value();
  lum1_save = lum1->value();
  lum2_save = lum2->value();
  red_value_save = color_chooser->r();
  green_value_save = color_chooser->g();
  blue_value_save = color_chooser->b();
}

//***************************************************************************
// Brush::copy_state( &brush_save) -- Copy state parameters from another
// object that has been created by serialization.
void Brush::copy_state( Brush* brush_save)
{
  // Copy state parameters
  index = brush_save->index;
  brush_size_save = brush_save->brush_size_save;
  brush_symbol_save = brush_save->brush_symbol_save;
  alpha_save = brush_save->alpha_save;
  cutoff_save = brush_save->cutoff_save;
  lum1_save = brush_save->lum1_save;
  lum2_save = brush_save->lum2_save;
  red_value_save = brush_save->red_value_save;
  green_value_save = brush_save->green_value_save;
  blue_value_save = brush_save->blue_value_save;
}
  
//***************************************************************************
// Brush::load_state() -- Load state parameters into widgets.  WARNING: 
// There is no proetction against bad state parameters or the possibility 
// that this might be a default object without widgets.
void Brush::load_state()
{
  pointsize->value( brush_size_save);
  symbol_menu->value( brush_symbol_save);
  alpha->value( alpha_save);
  cutoff->value( cutoff_save);
  lum1->value( lum1_save);
  lum2->value( lum2_save);
  color_chooser->rgb( red_value_save, green_value_save, blue_value_save);
}

//***************************************************************************
// Brush::brush_changed() -- If bush was changed, redraw plots
void Brush::brush_changed()
{
  // pointsize of 1 is too small to see symbols, but OK for standard GL points
  // symbol==0 is regular points, symbol==1 is lines
  if (previous_symbol <= 1 && symbol_menu->value() > 1) {
    if (pointsize->value() < 3) {
      pointsize->value(3);
    }
  }
  previous_symbol = symbol_menu->value();
  Plot_Window::redraw_all_plots(0);
}

//***************************************************************************
// Brush::change_color() -- Change brush color
void Brush::change_color()
{
  double c[3] = {
    color_chooser->r(),
    color_chooser->g(),
    color_chooser->b()};
  labelcolor(fl_rgb_color((uchar)(c[0]*255), (uchar)(c[1]*255), (uchar)(c[2]*255)));
  redraw_label();

	// keep tab's (parent's) colored labels updated while the brush color 
  // changes and the tab is selected
  brushes_tab->labelcolor(labelcolor());
  brushes_tab->redraw_label();
  brush_changed();
}

//***************************************************************************
// Brush::reset() -- Reset brush
void Brush::reset () 
{
  double c[3] = {
    Brush::initial_colors[index][0],
    Brush::initial_colors[index][1],
    Brush::initial_colors[index][2]};
  color_chooser->rgb(c[0], c[1], c[2]);
  change_color ();
  symbol_menu->value(0);
  pointsize->value(default_pointsize);
  alpha->value(1.0);
  lum1->value(0.2);  // !!! 
  lum2->value(1.0);
}

//***************************************************************************
// Brush::clear_now() -- Clear all points that are currently selected using 
// this brush leaving all other brush's selections alone.
void Brush::clear_now ()
{
  // (selected - index) == 0 iff currently selected by this brush:
  selected = where(selected-index,selected,0);
  previously_selected = selected;
  // previously_selected = 0;

  // redraw based on changed selection.  Candidate for "pull out method(s)" refactoring?
  pws[1]->color_array_from_selection();
  pws[1]->redraw_all_plots(0);
}

// set the pointsize of all brushes (called when (re)initializing all brushes)
void Brush::set_sizes(float size)
{
  printf("--- npoints = %d, setting default pointsize to %f\n", npoints, size);
  for (int i=0; i<nbrushes; i++) {
    brushes[i]->pointsize->value(size);
  }
}

//***************************************************************************
// Brush::make_widgets( *bw) -- Make widgets to hold brush control panel
void Brush::make_widgets(Brush *bw)
{
  // Since these (virtual) control panels are really groups inside a tab 
  // inside a window, set their child widget's coordinates relative to 
  // their enclosing window's position. 
  int xpos = this->x()+50;
  int ypos = this->y()+10;

  // Fl_Button *b;

  // point size slider for this brush
  pointsize = new Fl_Hor_Value_Slider_Input( xpos, ypos, bw->w()-155, 20, "size");
  pointsize->align(FL_ALIGN_LEFT);
  pointsize->value(default_pointsize);
  pointsize->step(0.25);
  pointsize->bounds(1.0,50.0);
  pointsize->callback((Fl_Callback*)static_brush_changed, this);
  pointsize->tooltip("change symbol size for this brush");

  // reset all parameters for this brush
  reset_button = new Fl_Button( xpos+pointsize->w()+85, ypos, 20, 20, "reset brush");
  reset_button->align( FL_ALIGN_LEFT); 
  reset_button->selection_color( FL_BLUE); 
  reset_button->callback((Fl_Callback*)static_reset, this);
  reset_button->value( 0);
  reset_button->tooltip("restore default parameters for this brush");

  // Alpha slider
  alpha = new Fl_Hor_Value_Slider_Input( xpos, ypos+=25, bw->w()-155, 20, "alpha");
  alpha->align(FL_ALIGN_LEFT);
  alpha->callback((Fl_Callback*)static_brush_changed, this);
  alpha->step(0.0001);
  alpha->bounds(0.0,1.0);
  alpha->value(1.0);
  alpha->tooltip("change opacity of this brush");

  // symbol types menu for this brush
  symbol_menu = new Fl_Choice(xpos+alpha->w()+45, ypos, 60, 20);
  // call a method to do the dirty work of setting up all the glyphs for the symbols menu.
  build_symbol_menu ();
  symbol_menu->textsize(12);
  symbol_menu->down_box(FL_NO_BOX);
  symbol_menu->clear_visible_focus(); // MCL XXX - I think this should be set for all Fl_Choice widgets and perhaps more.
  symbol_menu->color(FL_WHITE);
  symbol_menu->menu(symbol_menu_items);
  symbol_menu->label("symb");
  symbol_menu->value(0);
  symbol_menu->callback( (Fl_Callback*)static_brush_changed, this);
  symbol_menu->tooltip("select symbol for this brush");

  // Alpha cutoff slider
  cutoff = new Fl_Hor_Value_Slider_Input( xpos, ypos+=25, bw->w()-155, 20, "cutoff");
  cutoff->align(FL_ALIGN_LEFT);
  cutoff->callback((Fl_Callback*)static_brush_changed, this);
  cutoff->step(0.0001);
  cutoff->bounds(0.0,1.0);
  cutoff->value(0.0);
  cutoff->tooltip("change alpha cutoff for this brush");

  // Initial luminosity slider
  lum1 = new Fl_Hor_Value_Slider_Input( xpos, ypos+=25, bw->w()-50, 20, "lum1");
  lum1->align(FL_ALIGN_LEFT);
  lum1->callback((Fl_Callback*)static_brush_changed, this);
  lum1->step(0.0001);
  lum1->bounds(0.0,1.0);
  lum1->value(0.2);  // !!!
  lum1->tooltip("change initial luminosity for this brush");

  // Luminosity accumulation factor slider
  lum2 = new Fl_Hor_Value_Slider_Input( xpos, ypos+=25, bw->w()-50, 20, "lum2");
  lum2->align(FL_ALIGN_LEFT);
  lum2->callback((Fl_Callback*)static_brush_changed, this);
  lum2->step(0.0001);
  lum2->bounds(0.0,2.0); 
  lum2->value(1.0);
  lum2->tooltip("change accumulated luminosity when overplotting this brush");

  color_chooser = new Vp_Color_Chooser(xpos-35, ypos+=25, 150, 75, ""); // XXX do not remove the "".
  color_chooser->callback((Fl_Callback*)static_change_color, this);
  color_chooser->labelfont(FL_HELVETICA);
  color_chooser->labelsize(10);

  // MCL Should there be a "selection" menu: clear, replace, intersect (and), add (or), replace always, paint mode, invert...
  clear_now_button = new Fl_Button( xpos+=110, ypos, 20, 20, "clear selection");
  clear_now_button->align( FL_ALIGN_RIGHT); 
  clear_now_button->selection_color( FL_BLUE); 
  clear_now_button->callback((Fl_Callback*)static_clear_now, this);
  clear_now_button->value( 0);
  clear_now_button->tooltip("de-select all points currently selected by this brush");

  add_to_selection = new Fl_Button( xpos, ypos+=20, 20, 20, "extend selection");
  add_to_selection->align( FL_ALIGN_RIGHT); 
  add_to_selection->selection_color( FL_BLUE); 
  add_to_selection->type( FL_TOGGLE_BUTTON);
  add_to_selection->value( index?0:1);  // all brushes default this to off, except brush 0.
  add_to_selection->tooltip("disable smart auto-clear");

  paint = new Fl_Button( xpos, ypos+=20, 20, 20, "paint");
  paint->align( FL_ALIGN_RIGHT); 
  paint->selection_color( FL_BLUE); 
  paint->type( FL_TOGGLE_BUTTON);
  paint->tooltip("dribble paint (do not erase) when dragging or shift-dragging");

}
