// viewpoints - interactive linked scatterplots and more.
// copyright 2005 Creon Levit, all rights reserved.
//*****************************************************************
// File name: control_panel_window.cpp
//
// Class definitions:
//   control_panel_window -- Control panel window
//
// Classes referenced:
//   plot_window -- Plot window
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
// Purpose: Source code for <control_panel_window.h>
//
// Author: Creon Levitt   unknown
// Modified: P. R. Gazis  27-MAR-2006
//*****************************************************************

// Include the necessary include libraries
#include "include_libraries_vp.h"

// Include globals
#include "global_definitions_vp.h"

// Include associated headers and source code
#include "plot_window.h"
#include "control_panel_window.h"

//*****************************************************************
// control_panel_window::control_panel_window( w, h) -- 
// Default constructor.  Do nothing.
control_panel_window::control_panel_window(
  int x, int y, int w, int h) : Fl_Group( x, y, w, h)
{}

//*****************************************************************
// control_panel_window::maybe_draw() -- Check plot windows to see
// if they need to be redrawn.
void control_panel_window::maybe_redraw() 
{
  // kludge.  Avoid double redraw when setting "don't clear".
  if( dont_clear->value()) return;

  //pw->redraw();
  pw->needs_redraw = 1;
}

//*****************************************************************
// plot_window::extract_and_redraw() --
void control_panel_window::extract_and_redraw ()
{
  if( pw->extract_data_points()) {

    #ifdef FAST_APPLE_VERTEX_EXTENSIONS
      GLvoid *vertexp = (GLvoid *)pw->vertices.data();
      glFlushVertexArrayRangeAPPLE(
        3*npoints*sizeof(GLfloat), vertexp);
    #endif // FAST_APPLE_VERTEX_EXTENSIONS

    //pw->redraw ();
    pw->needs_redraw = 1;
  }
}

//*****************************************************************
// control_panel_window::make_widgets( cpw) -- Make widgets
void control_panel_window::make_widgets( control_panel_window *cpw)
{
  // since these (virtual) control panels are really groups inside 
  // a tab inside a window, set their child widget's coordinates 
  // relative to their enclosing window's position.  (I think ;-)
  int xpos = this->x()+50;
  int ypos = this->y()+20;

  Fl_Button *b;

  pointsize_slider = 
    new Fl_Hor_Value_Slider_Input(
      xpos, ypos, cpw->w()-60, 20, "size");
  pointsize_slider->align(FL_ALIGN_LEFT);
  pointsize_slider->value(pointsize);
  pointsize_slider->step(0.25);
  pointsize_slider->bounds(0.1,20.0);
  pointsize_slider->callback((Fl_Callback*)replot, this);

  Bkg = 
    new Fl_Hor_Value_Slider_Input(
      xpos, ypos+=25, cpw->w()-60, 20, "Bkg");
  Bkg->align(FL_ALIGN_LEFT);
  Bkg->step(0.0001);
  Bkg->bounds(0.0,1.0);
  Bkg->callback((Fl_Callback*)replot, this);
  Bkg->value(0.0);

  Lum = 
    new Fl_Hor_Value_Slider_Input(
      xpos, ypos+=25, cpw->w()-60, 20, "Lum");
  Lum->align(FL_ALIGN_LEFT);
  Lum->callback((Fl_Callback*)replot, this);
  Lum->step(0.0001);
  Lum->bounds(0,1.0);
  Lum->value(1.0);

  Alph = 
    new Fl_Hor_Value_Slider_Input( 
      xpos, ypos+=25, cpw->w()-60, 20, "Alph");
  Alph->align(FL_ALIGN_LEFT);
  Alph->callback((Fl_Callback*)replot, this);
  Alph->step(0.0001);
  Alph->bounds(0.25,0.5);
  Alph->value(0.5);

  rot_slider = 
    new Fl_Hor_Value_Slider_Input( 
      xpos, ypos+=25, cpw->w()-60, 20, "rot");
  rot_slider->align(FL_ALIGN_LEFT);
  rot_slider->callback((Fl_Callback*)replot, this);
  rot_slider->value(0.0);
  rot_slider->step(0.001);
  rot_slider->bounds(-180.0, 180.0);

  nbins_slider = 
    new Fl_Hor_Value_Slider_Input(
      xpos, ypos+=25, cpw->w()-60, 20, "nbins");
  nbins_slider->align(FL_ALIGN_LEFT);
  nbins_slider->callback((Fl_Callback*)redraw_one_plot, this);
  nbins_slider->value(plot_window::nbins_default);
  nbins_slider->step(1);
  nbins_slider->bounds(2,plot_window::nbins_max);

  // dynamically build the variables menu
  // cout << "starting menu build, nvars = " << nvars << endl;
  for( int i=0; i<=nvars; i++) {
    // cout << "label " << i 
    //      << " = " << column_labels[i].c_str() << endl;
    varindex_menu_items[i].label(
      (const char *)(column_labels[i].c_str()));
    varindex_menu_items[i].user_data((void *)i);
  }
  varindex_menu_items[nvars+1].label(0);

  xpos = 10;

  varindex1 = 
    new Fl_Choice (xpos, ypos+=45, 100, 25, "axis 1");
  varindex1->align(FL_ALIGN_TOP);
  varindex1->textsize(12);
  varindex1->menu( varindex_menu_items);
  varindex1->callback(
    (Fl_Callback*)static_extract_and_redraw, this);

  varindex2 = 
    new Fl_Choice (xpos+100, ypos, 100, 25, "axis 2");
  varindex2->align(FL_ALIGN_TOP);
  varindex2->textsize(12);
  varindex2->menu( varindex_menu_items);
  varindex2->callback(
    (Fl_Callback*)static_extract_and_redraw, this);

  varindex3 = 
    new Fl_Choice (xpos+200, ypos, 100, 25, "axis 3");
  varindex3->align(FL_ALIGN_TOP);
  varindex3->textsize(12);
  varindex3->menu( varindex_menu_items);
  varindex3->value(nvars);  // initially, axis3 == "-nothing-"
  varindex3->callback(
    (Fl_Callback*)static_extract_and_redraw, this);

  // NLoop: Genenerate normalization style menu
  for( int i=0; i<n_normalization_styles; i++) {
    normalization_style_menu_items[i].label(
      normalization_style_labels[i]);
    normalization_style_menu_items[i].user_data(
      (void *)normalization_styles[i]);
  }
  normalization_style_menu_items[n_normalization_styles].label(0);

  x_normalization_style = 
    new Fl_Choice (xpos, ypos+=45, 100, 25, "normalize x");
  x_normalization_style->align(FL_ALIGN_TOP);
  x_normalization_style->textsize(12);
  x_normalization_style->menu(normalization_style_menu_items);
  x_normalization_style->value(NORMALIZATION_TRIM_1E3);
  x_normalization_style->callback(
    (Fl_Callback*)static_extract_and_redraw, this);
 
  y_normalization_style =
    new Fl_Choice (xpos+100, ypos, 100, 25, "normalize y");
  y_normalization_style->align(FL_ALIGN_TOP);
  y_normalization_style->textsize(12);
  y_normalization_style->menu(normalization_style_menu_items);
  y_normalization_style->value(NORMALIZATION_TRIM_1E3); 
  y_normalization_style->callback(
    (Fl_Callback*)static_extract_and_redraw, this);
 
  z_normalization_style = 
    new Fl_Choice (xpos+200, ypos, 100, 25, "normalize z");
  z_normalization_style->align(FL_ALIGN_TOP);
  z_normalization_style->textsize(12);
  z_normalization_style->menu(normalization_style_menu_items);
  z_normalization_style->value(NORMALIZATION_TRIM_1E3); 
  z_normalization_style->callback(
    (Fl_Callback*)static_extract_and_redraw, this);
 
  int xpos2 = xpos;
  int ypos2 = ypos;
  
  reset_view_button = b = 
    new Fl_Button(xpos2, ypos+=25, 20, 20, "reset view ");
  b->align(FL_ALIGN_RIGHT); b->selection_color(FL_BLUE);
  b->callback((Fl_Callback*) reset_view, this);

  spin = b= new Fl_Button(xpos2, ypos+=25, 20, 20, "spin");
  b->align(FL_ALIGN_RIGHT); b->selection_color(FL_BLUE);
  b->type(FL_TOGGLE_BUTTON);

  dont_clear = 
    new Fl_Button(xpos2, ypos+=25, 20, 20, "don't clear");
  dont_clear->align(FL_ALIGN_RIGHT);
  dont_clear->type(FL_TOGGLE_BUTTON);
  dont_clear->selection_color(FL_BLUE);
  dont_clear->callback(
    (Fl_Callback*)static_maybe_redraw, this);

  transform_style = 
    new Fl_Group (xpos2-1, ypos+25-1, 20+2, 4*25+2);

  no_transform = b = 
    new Fl_Button(xpos2, ypos+=25, 20, 20, "identity");
  b->callback((Fl_Callback*)static_extract_and_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_RADIO_BUTTON); 
  b->selection_color(FL_BLUE);

  sum_vs_difference = b = 
    new Fl_Button(xpos2, ypos+=25, 20, 20, "sum vs. diff.");
  b->callback((Fl_Callback*)static_extract_and_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_RADIO_BUTTON); 
  b->selection_color(FL_BLUE);
	
  polar = b = 
    new Fl_Button(xpos2, ypos+=25, 20, 20, "polar");
  b->callback((Fl_Callback*)static_extract_and_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_RADIO_BUTTON); 
  b->selection_color(FL_BLUE);
	
  transform_style->end();
  no_transform->setonly();

  ypos=ypos2;
  xpos=xpos2+100;

  show_points = b = 
    new Fl_Button(xpos, ypos+=25, 20, 20, "points");
  b->callback((Fl_Callback*)static_maybe_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);	
  b->value(1);

  show_deselected_points = b = 
    new Fl_Button(xpos, ypos+=25, 20, 20, " unselected");
  b->callback((Fl_Callback*)static_maybe_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);	
  b->value(1);

  show_axes = b = 
    new Fl_Button(xpos, ypos+=25, 20, 20, "axes");
  b->callback((Fl_Callback*)static_maybe_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);	
  b->value(1);

  show_labels = b = 
    new Fl_Button(xpos, ypos+=25, 20, 20, "labels");
  b->callback((Fl_Callback*)static_maybe_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);	
  b->value(1);

  show_scale = b = 
    new Fl_Button(xpos, ypos+=25, 20, 20, "scales");
  b->callback((Fl_Callback*)static_maybe_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);	
  b->value(1);

  show_grid = b = 
    new Fl_Button(xpos, ypos+=25, 20, 20, "grid");
  b->callback((Fl_Callback*)static_maybe_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);
  b->value(0);

  show_histogram = b = 
    new Fl_Button(xpos, ypos+=25, 20, 20, "histograms");
  b->callback((Fl_Callback*)redraw_one_plot, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);	
  b->value(0);

  ypos=ypos2;
  xpos=xpos2+200;

  b = new Fl_Button(xpos, ypos+=25, 20, 20, "selection color");
  b->align(FL_ALIGN_RIGHT); 
  b->selection_color(FL_BLUE); 
  b->callback((Fl_Callback*)choose_color_selected, this);

  lock_axes_button = b = 
    new Fl_Button(xpos, ypos+=25, 20, 20, "lock axes");
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);
  b->value(0);
}
