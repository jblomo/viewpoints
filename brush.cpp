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
// Modified: P. R. Gazis  08-JUL-2008
//***************************************************************************

// Include the necessary system and 3rd party header files
#include "include_libraries_vp.h"

// Include globals
#include "global_definitions_vp.h"

#include "plot_window.h"
#include "Vp_Color_Chooser.h"
#include "brush.h"
#include "symbol_menu.h"

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
  
// Define arrays to hold menu items for symbol menu.  Each one gets its
// image() set to one of the above 16x16 grayscale Fl_Pixmaps.
Fl_Menu_Item Brush::symbol_menu_items[ NSYMBOLS+1];
Fl_Pixmap* Brush::symbol_images[ NSYMBOLS+1];

// number of brushes created
int Brush::nbrushes = 0;

// Instantiate global Fl_Pixmap objects to hold images of symbols for
// use in the symbol menu.
Fl_Pixmap Brush::image_0( Symbol_Menu::idata_0);
Fl_Pixmap Brush::image_line( Symbol_Menu::idata_line);
Fl_Pixmap Brush::image_1( Symbol_Menu::idata_1);
Fl_Pixmap Brush::image_2( Symbol_Menu::idata_2);
Fl_Pixmap Brush::image_3( Symbol_Menu::idata_3);
Fl_Pixmap Brush::image_4( Symbol_Menu::idata_4);
Fl_Pixmap Brush::image_5( Symbol_Menu::idata_5);
Fl_Pixmap Brush::image_6( Symbol_Menu::idata_6);
Fl_Pixmap Brush::image_7( Symbol_Menu::idata_7);
Fl_Pixmap Brush::image_8( Symbol_Menu::idata_8);
Fl_Pixmap Brush::image_9( Symbol_Menu::idata_9);
Fl_Pixmap Brush::image_10( Symbol_Menu::idata_10);
Fl_Pixmap Brush::image_11( Symbol_Menu::idata_11);
Fl_Pixmap Brush::image_12( Symbol_Menu::idata_12);
Fl_Pixmap Brush::image_13( Symbol_Menu::idata_13);
Fl_Pixmap Brush::image_14( Symbol_Menu::idata_14);
Fl_Pixmap Brush::image_18( Symbol_Menu::idata_18);
Fl_Pixmap Brush::image_19( Symbol_Menu::idata_19);

// Instantiate global Fl_Pixmap objects to hold images of numbers and 
// letters for use in the symbol menu.
Fl_Pixmap Brush::image_osaka_21( Symbol_Menu::idata_osaka_21);
Fl_Pixmap Brush::image_osaka_22( Symbol_Menu::idata_osaka_22);
Fl_Pixmap Brush::image_osaka_23( Symbol_Menu::idata_osaka_23);
Fl_Pixmap Brush::image_osaka_24( Symbol_Menu::idata_osaka_24);
Fl_Pixmap Brush::image_osaka_25( Symbol_Menu::idata_osaka_25);
Fl_Pixmap Brush::image_osaka_26( Symbol_Menu::idata_osaka_26);
Fl_Pixmap Brush::image_osaka_27( Symbol_Menu::idata_osaka_27);
Fl_Pixmap Brush::image_osaka_28( Symbol_Menu::idata_osaka_28);
Fl_Pixmap Brush::image_osaka_29( Symbol_Menu::idata_osaka_29);
Fl_Pixmap Brush::image_osaka_30( Symbol_Menu::idata_osaka_30);
Fl_Pixmap Brush::image_osaka_38( Symbol_Menu::idata_osaka_38);
Fl_Pixmap Brush::image_osaka_39( Symbol_Menu::idata_osaka_39);
Fl_Pixmap Brush::image_osaka_40( Symbol_Menu::idata_osaka_40);
Fl_Pixmap Brush::image_osaka_41( Symbol_Menu::idata_osaka_41);
Fl_Pixmap Brush::image_osaka_42( Symbol_Menu::idata_osaka_42);
Fl_Pixmap Brush::image_osaka_43( Symbol_Menu::idata_osaka_43);
Fl_Pixmap Brush::image_osaka_44( Symbol_Menu::idata_osaka_44);
Fl_Pixmap Brush::image_osaka_45( Symbol_Menu::idata_osaka_45);
Fl_Pixmap Brush::image_osaka_46( Symbol_Menu::idata_osaka_46);
Fl_Pixmap Brush::image_osaka_47( Symbol_Menu::idata_osaka_47);
Fl_Pixmap Brush::image_osaka_48( Symbol_Menu::idata_osaka_48);
Fl_Pixmap Brush::image_osaka_49( Symbol_Menu::idata_osaka_49);
Fl_Pixmap Brush::image_osaka_50( Symbol_Menu::idata_osaka_50);
Fl_Pixmap Brush::image_osaka_51( Symbol_Menu::idata_osaka_51);
Fl_Pixmap Brush::image_osaka_52( Symbol_Menu::idata_osaka_52);
Fl_Pixmap Brush::image_osaka_53( Symbol_Menu::idata_osaka_53);
Fl_Pixmap Brush::image_osaka_54( Symbol_Menu::idata_osaka_54);
Fl_Pixmap Brush::image_osaka_55( Symbol_Menu::idata_osaka_55);
Fl_Pixmap Brush::image_osaka_56( Symbol_Menu::idata_osaka_56);
Fl_Pixmap Brush::image_osaka_57( Symbol_Menu::idata_osaka_57);
Fl_Pixmap Brush::image_osaka_58( Symbol_Menu::idata_osaka_58);
Fl_Pixmap Brush::image_osaka_59( Symbol_Menu::idata_osaka_59);
Fl_Pixmap Brush::image_osaka_60( Symbol_Menu::idata_osaka_60);
Fl_Pixmap Brush::image_osaka_61( Symbol_Menu::idata_osaka_61);
Fl_Pixmap Brush::image_osaka_62( Symbol_Menu::idata_osaka_62);
Fl_Pixmap Brush::image_osaka_63( Symbol_Menu::idata_osaka_63);

//***************************************************************************
// Brush::Brush() --  Default constructor with no arguments.  Used only for 
// serialization.  Do nothing except call the constructor for the parent 
// class, Fl_Group, with dummy arguments.
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

  // Invoke make_widgets() method to generate the brush control panel, then 
  // end the group
  make_widgets(this);
  end();

  // Set colors and clear focus
  double c[3] = {
    Brush::initial_colors[index][0],
    Brush::initial_colors[index][1],
    Brush::initial_colors[index][2]};
  color_chooser->rgb(c[0], c[1], c[2]);
  labelcolor(fl_rgb_color((uchar)(c[0]*255), (uchar)(c[1]*255), (uchar)(c[2]*255)));
  clear_visible_focus();

  // XXX someday a brush's tab's label will show a colored image of the brush's current symbol...
  // XXX (probably after we switch to FLTK 2)
  // image( symbol_images[0]);
  // int fl_draw_pixmap( char **data, int X, int Y, Fl_Color = FL_GRAY)
}

//***************************************************************************
// Brush::make_state() -- Examine widgets to generate and store state
// parameters during serialization.  Used only by the serialzation method.
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
// that this might be a default object without any widgets.
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
// Brush::brush_changed() -- If brush was changed, redraw plots.
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
// Brush::change_color() -- Change brush color.
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
// Brush::reset() -- Reset brush to default colors, symbol, size, and
// alpha-cutoff and luminosity parameters.
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
void Brush::clear_now()
{
  // (selected - index) == 0 iff currently selected by this brush:
  selected = where(selected-index,selected,0);
  previously_selected = selected;
  // previously_selected = 0;

  // redraw based on changed selection.  Candidate for "pull out method(s)" refactoring?
  pws[1]->color_array_from_selection();
  pws[1]->redraw_all_plots(0);
}

//***************************************************************************
// Brush::set_sizes( size) -- Loopp trough all brishes to set the pointsize 
// of all brushes (called when (re)initializing all brushes)
void Brush::set_sizes( float size)
{
  // printf("--- npoints = %d, setting default pointsize to %f\n", npoints, size);
  cout << "--- npoints = " << npoints
       << ",, setting default pointsize to " << size << endl;
  for( int i=0; i<nbrushes; i++) {
    brushes[i]->pointsize->value(size);
  }
}

//***************************************************************************
// Brush::make_widgets( *bw) -- Make widgets to hold brush control panel
void Brush::make_widgets( Brush *bw)
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
  pointsize->tooltip( "change symbol size for this brush");

  // reset all parameters for this brush
  reset_button = new Fl_Button( xpos+pointsize->w()+85, ypos, 20, 20, "reset brush");
  reset_button->align( FL_ALIGN_LEFT); 
  reset_button->selection_color( FL_BLUE); 
  reset_button->callback((Fl_Callback*)static_reset, this);
  reset_button->value( 0);
  reset_button->tooltip( "restore default parameters for this brush");

  // Alpha slider
  alpha = new Fl_Hor_Value_Slider_Input( xpos, ypos+=25, bw->w()-155, 20, "alpha");
  alpha->align(FL_ALIGN_LEFT);
  alpha->callback((Fl_Callback*)static_brush_changed, this);
  alpha->step(0.0001);
  alpha->bounds(0.0,1.0);
  alpha->value(1.0);
  alpha->tooltip( "change opacity of this brush");

  // Define symbol types menu for this brush
  symbol_menu = new Fl_Choice(xpos+alpha->w()+45, ypos, 60, 20);

  // Call the Symbol_Menu::build_symbol_menu() method to do the dirty work 
  // of setting up all the glyphs for the symbols menu.
  build_symbol_menu();
  symbol_menu->textsize( 12);
  symbol_menu->down_box( FL_NO_BOX);
  symbol_menu->clear_visible_focus(); // MCL XXX - I think this should be set for all Fl_Choice widgets and perhaps more.
  symbol_menu->color( FL_WHITE);
  symbol_menu->menu( symbol_menu_items);
  symbol_menu->label( "symb");
  symbol_menu->value( 0);
  symbol_menu->callback( (Fl_Callback*) static_brush_changed, this);
  symbol_menu->tooltip( "select symbol for this brush");

  // Alpha cutoff slider
  cutoff = new Fl_Hor_Value_Slider_Input( xpos, ypos+=25, bw->w()-155, 20, "cutoff");
  cutoff->align(FL_ALIGN_LEFT);
  cutoff->callback((Fl_Callback*)static_brush_changed, this);
  cutoff->step(0.0001);
  cutoff->bounds(0.0,1.0);
  cutoff->value(0.0);
  cutoff->tooltip( "change alpha cutoff for this brush");

  // Initial luminosity slider
  lum1 = new Fl_Hor_Value_Slider_Input( xpos, ypos+=25, bw->w()-50, 20, "lum1");
  lum1->align(FL_ALIGN_LEFT);
  lum1->callback((Fl_Callback*)static_brush_changed, this);
  lum1->step(0.0001);
  lum1->bounds(0.0,1.0);
  lum1->value(0.2);  // !!!
  lum1->tooltip( "change initial luminosity for this brush");

  // Luminosity accumulation factor slider
  lum2 = new Fl_Hor_Value_Slider_Input( xpos, ypos+=25, bw->w()-50, 20, "lum2");
  lum2->align(FL_ALIGN_LEFT);
  lum2->callback((Fl_Callback*)static_brush_changed, this);
  lum2->step(0.0001);
  lum2->bounds(0.0,2.0); 
  lum2->value(1.0);
  lum2->tooltip( "change accumulated luminosity when overplotting this brush");

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
  clear_now_button->tooltip( "de-select all points currently selected by this brush");

  add_to_selection = new Fl_Button( xpos, ypos+=20, 20, 20, "extend selection");
  add_to_selection->align( FL_ALIGN_RIGHT); 
  add_to_selection->selection_color( FL_BLUE); 
  add_to_selection->type( FL_TOGGLE_BUTTON);
  add_to_selection->value( index?0:1);  // all brushes default this to off, except brush 0.
  add_to_selection->tooltip( "disable smart auto-clear");

  paint = new Fl_Button( xpos, ypos+=20, 20, 20, "paint");
  paint->align( FL_ALIGN_RIGHT); 
  paint->selection_color( FL_BLUE); 
  paint->type( FL_TOGGLE_BUTTON);
  paint->tooltip( "dribble paint (do not erase) when dragging or shift-dragging");
}

//***************************************************************************
// Brush::build_symbol_menu() -- Build symbol menu.
void Brush::build_symbol_menu()
{
  // Loop: Build menu of symbols
  for( int i=0; i<=NSYMBOLS; i++) {
    // Fl_Menu_Item *m = &Brush::symbol_menu_items[ i];
    Fl_Menu_Item *m = &symbol_menu_items[ i];

    // MCL XXX shouldn't we do this by calling the member functions instead?
    m->shortcut_ = 0;
    m->callback_ = 0;
    m->flags = 0;
    m->labelfont_ = 0;
    m->labelcolor_ = 0;
    // last item == 0 signals end of menu items.
    if( i < NSYMBOLS) {
      m->labeltype_ = FL_NORMAL_LABEL;
      m->labelsize_ = 14;
      m->user_data_ = (void *)i;
    }
    else {
      m->labeltype_ = 0;
      m->labelsize_ = 0;
      m->user_data_ = (void *)0;
    }
  }

  // The following was hand-transformed from code generated by fluid.
  // Load images of symbols into the array of symbol menu items.  
  symbol_menu_items[0].image( image_0); 
  symbol_images[0] = &image_0;
  symbol_menu_items[1].image( image_line);
  symbol_menu_items[2].image( image_1);
  symbol_menu_items[3].image( image_2);
  symbol_menu_items[4].image( image_3);
  symbol_menu_items[5].image( image_4);
  symbol_menu_items[6].image( image_5);
  symbol_menu_items[7].image( image_6);
  symbol_menu_items[8].image( image_7);
  symbol_menu_items[9].image( image_8);
  symbol_menu_items[10].image( image_9);
  symbol_menu_items[11].image( image_10);
  symbol_menu_items[12].image( image_11);
  symbol_menu_items[13].image( image_12);
  symbol_menu_items[14].image( image_13);
  symbol_menu_items[15].image( image_14);
  symbol_menu_items[16].image( image_18);
  symbol_menu_items[17].image( image_19);
  
  // Load images of numbers and letters into the array of symbol menu items.  
  symbol_menu_items[18].image( image_osaka_21);
  symbol_menu_items[19].image( image_osaka_22);
  symbol_menu_items[20].image( image_osaka_23);
  symbol_menu_items[21].image( image_osaka_24);
  symbol_menu_items[22].image( image_osaka_25);
  symbol_menu_items[23].image( image_osaka_26);
  symbol_menu_items[24].image( image_osaka_27);
  symbol_menu_items[25].image( image_osaka_28);
  symbol_menu_items[26].image( image_osaka_29);
  symbol_menu_items[27].image( image_osaka_30);
  symbol_menu_items[28].image( image_osaka_38);
  symbol_menu_items[29].image( image_osaka_39);
  symbol_menu_items[30].image( image_osaka_40);
  symbol_menu_items[31].image( image_osaka_41);
  symbol_menu_items[32].image( image_osaka_42);
  symbol_menu_items[33].image( image_osaka_43);
  symbol_menu_items[34].image( image_osaka_44);
  symbol_menu_items[35].image( image_osaka_45);
  symbol_menu_items[36].image( image_osaka_46);
  symbol_menu_items[37].image( image_osaka_47);
  symbol_menu_items[38].image( image_osaka_48);
  symbol_menu_items[39].image( image_osaka_49);
  symbol_menu_items[40].image( image_osaka_50);
  symbol_menu_items[41].image( image_osaka_51);
  symbol_menu_items[42].image( image_osaka_52);
  symbol_menu_items[43].image( image_osaka_53);
  symbol_menu_items[44].image( image_osaka_54);
  symbol_menu_items[45].image( image_osaka_55);
  symbol_menu_items[46].image( image_osaka_56);
  symbol_menu_items[47].image( image_osaka_57);
  symbol_menu_items[48].image( image_osaka_58);
  symbol_menu_items[49].image( image_osaka_59);
  symbol_menu_items[50].image( image_osaka_60);
  symbol_menu_items[51].image( image_osaka_61);
  symbol_menu_items[52].image( image_osaka_62);
  symbol_menu_items[53].image( image_osaka_63);
}
