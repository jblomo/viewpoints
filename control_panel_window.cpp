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
// Author: Creon Levit    2005-2006
// Modified: P. R. Gazis  09-MAY-2006
//*****************************************************************

// Include the necessary include libraries
#include "include_libraries_vp.h"

#include <typeinfo>

// Include globals
#include "global_definitions_vp.h"

// Include associated headers and source code
#include "plot_window.h"
#include "control_panel_window.h"

// Set static data members for class control_panel_window::
//

// array to hold menu items for axis menus.  
Fl_Menu_Item 
  control_panel_window::varindex_menu_items[ MAXVARS+2] = 
  { Fl_Menu_Item()};

// array to hold menu items for normalization styles
// MCL XXX this needs to be cleaned up to avoid magic numbers like 11, 12.....
Fl_Menu_Item 
  control_panel_window::normalization_style_menu_items[ 12] =
   { Fl_Menu_Item()};

// Set the array of normalization schemes.  NOTE: If possible, 
// this should be be made CONST.
int control_panel_window::normalization_styles[ 11] = {
  NORMALIZATION_NONE, NORMALIZATION_MINMAX,
  NORMALIZATION_ZEROMAX, NORMALIZATION_MAXABS, 
  NORMALIZATION_TRIM_1E2, NORMALIZATION_TRIM_1E3, 
  NORMALIZATION_THREESIGMA, NORMALIZATION_LOG10,
  NORMALIZATION_SQUASH, NORMALIZATION_RANK, 
  NORMALIZATION_GAUSSIANIZE};

// Set the array of character arrays that describe normalization
// schemes.  NOTE: If possible, this should be be made CONST.
char *control_panel_window::normalization_style_labels[ 11] = { 
  "none", "minmax", "zeromax", "maxabs", "trim 10^-2", 
  "trim 10^-3", "threesigma", "log_10", "squash", "rank",
  "gaussianize"};

//*****************************************************************
// control_panel_window::control_panel_window( x, y, w, h) -- 
// Default constructor.  Do nothing except call the constructor 
// for the parent class, Fl_Group.
control_panel_window::control_panel_window(
  int x, int y, int w, int h) : Fl_Group( x, y, w, h)
{}

//*****************************************************************
// control_panel_window::broadcast_change (*master_widget) -- 
// broadcast an interaction from the master panel to all (unlocked) 
// panels.
void control_panel_window::broadcast_change (Fl_Widget *master_widget)
{
  const Fl_Group *master_panel = master_widget->parent();
  assert(master_panel);
  const int widget_index = master_panel->find(master_widget);
  assert(widget_index >= 0 && widget_index < master_panel->children());
  for (int i=0; i<nplots; i++)
  {
    Fl_Widget *slave_widget = cps[i]->child(widget_index);
    assert(slave_widget);
    
    // cout << "master_widget: label = " << master_widget->label() << " type = " << typeid(*master_widget).name() << endl;
    // cout << "slave_widget:  label = " << slave_widget->label() <<  " type = " << typeid(*slave_widget).name() << endl;

    // MCL XXX downcasting to dispatch on type is considered very bad form.  
    // If value() were a virtual function of Fl_Widget (like callback() and 
    // do_callback() are) it would be cleaner.  Or we could bite the bullet and 
    // use one of the fltk publish/subscribe extensions.  That could clean up 
    // all sort of things.....
    assert (typeid(master_widget) == typeid(slave_widget));
    {
      Fl_Button *gp, *lp;
      if ((gp = dynamic_cast <Fl_Button*> (master_widget)) && (lp = dynamic_cast <Fl_Button*> (slave_widget)))
        lp->value(gp->value());
    }
    {
      Fl_Valuator *gp, *lp;
      if ((gp = dynamic_cast <Fl_Valuator*> (master_widget)) && (lp = dynamic_cast <Fl_Valuator*> (slave_widget)))
        lp->value(gp->value());
    }
    {
      Fl_Choice *gp, *lp;
      if ((gp = dynamic_cast <Fl_Choice*> (master_widget)) && (lp = dynamic_cast <Fl_Choice*> (slave_widget)))
        lp->value(gp->value());
    }
    if (slave_widget->callback())
    {
      // cout << ".. doing callback for widget " << widget_index 
      //      << " in panel " << i << endl;
      slave_widget->do_callback(slave_widget, cps[i]);
    }
  }
}

//*****************************************************************
// control_panel_window::maybe_draw() -- Check plot window to see
// if they need to be redrawn.
void control_panel_window::maybe_redraw() 
{
  // kludge.  Avoid double redraw when setting "don't clear".
  if( dont_clear->value()) return;

  //pw->redraw();
  pw->needs_redraw = 1;
}

//*****************************************************************
// plot_window::extract_and_redraw() -- Extract data for these 
// (new?) axes and redraw plot.  For one local control panel only.
void control_panel_window::extract_and_redraw ()
{
  if( pw->extract_data_points()) {

    #ifdef FAST_APPLE_VERTEX_EXTENSIONS
      GLvoid *vertexp = (GLvoid *)pw->vertices.data();
      glFlushVertexArrayRangeAPPLE( 3*npoints*sizeof(GLfloat), vertexp);
    #endif // FAST_APPLE_VERTEX_EXTENSIONS

    pw->needs_redraw = 1;
  }
}

//*****************************************************************
// control_panel_window::make_widgets( cpw) -- Make widgets
void control_panel_window::make_widgets( control_panel_window *cpw)
{
  // Since these (virtual) control panels are really groups inside 
  // a tab inside a window, set their child widget's coordinates 
  // relative to their enclosing window's position. 
  int xpos = this->x()+50;
  int ypos = this->y()+20;

  Fl_Button *b;

  // Pointsize slider
  pointsize_slider = new Fl_Hor_Value_Slider_Input( xpos, ypos, cpw->w()-125, 20, "size");
  pointsize_slider->align(FL_ALIGN_LEFT);
  pointsize_slider->value(pointsize);
  pointsize_slider->step(0.5);
  pointsize_slider->bounds(0.5,30.0);
  pointsize_slider->callback((Fl_Callback*)replot, this);

  // smooth (antialiased) points button
  smooth_points_button = b = new Fl_Button(xpos+pointsize_slider->w()+5, ypos, 20, 20, "smooth");
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);
  b->align(FL_ALIGN_RIGHT);
  b->value(0);
  b->callback((Fl_Callback*)replot, this);

  // Pointsize slider for selected points
  selected_pointsize_slider = new Fl_Hor_Value_Slider_Input( xpos, ypos+=25, cpw->w()-125, 20, "size2");
  selected_pointsize_slider->align(FL_ALIGN_LEFT);
  selected_pointsize_slider->value(pointsize);
  selected_pointsize_slider->step(0.5);
  selected_pointsize_slider->bounds(0.5,30.0);
  selected_pointsize_slider->callback((Fl_Callback*)replot, this);

  // Backgrund color slider
  Bkg = new Fl_Hor_Value_Slider_Input( xpos, ypos+=25, cpw->w()-60, 20, "Bkg");
  Bkg->align(FL_ALIGN_LEFT);
  Bkg->step(0.0001);
  Bkg->bounds(0.0,1.0);
  Bkg->callback((Fl_Callback*)replot, this);
  Bkg->value(0.0);

  // Luminosity slider
  Lum = new Fl_Hor_Value_Slider_Input( xpos, ypos+=25, cpw->w()-60, 20, "Lum");
  Lum->align(FL_ALIGN_LEFT);
  Lum->callback((Fl_Callback*)replot, this);
  Lum->step(0.0001);
  Lum->bounds(0,1.0);
  Lum->value(1.0);

  // Alpha plane slider
  Alph = new Fl_Hor_Value_Slider_Input( xpos, ypos+=25, cpw->w()-60, 20, "Alph");
  Alph->align(FL_ALIGN_LEFT);
  Alph->callback((Fl_Callback*)replot, this);
  Alph->step(0.0001);
  Alph->bounds(0.25,0.5);
  Alph->value(0.5);

  // Rotation (and spin) slider
  rot_slider = new Fl_Hor_Value_Slider_Input( xpos, ypos+=25, cpw->w()-60, 20, "rot");
  rot_slider->align(FL_ALIGN_LEFT);
  rot_slider->callback((Fl_Callback*)replot, this);
  rot_slider->value(0.0);
  rot_slider->step(0.001);
  rot_slider->bounds(-180.0, 180.0);

  // Number of histogram bins slider
  nbins_slider = new Fl_Hor_Value_Slider_Input( xpos, ypos+=25, cpw->w()-60, 20, "nbins");
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

  // X-axis parameter
  varindex1 = new Fl_Choice (xpos, ypos+=45, 100, 25, "axis 1");
  varindex1->align(FL_ALIGN_TOP);
  varindex1->textsize(12);
  varindex1->copy( varindex_menu_items);
  varindex1->mode( nvars, FL_MENU_INACTIVE);  // disable "--nothing--" as a choice for axis1
  varindex1->callback( (Fl_Callback*)static_extract_and_redraw, this);

  // Lock x-axis button
  lock_axis1_button = b = new Fl_Button(xpos+80, ypos+25, 20, 20, "lock ");
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);
  b->align(FL_ALIGN_LEFT);
  b->value(0);

  // Y-axis parameter
  varindex2 = new Fl_Choice (xpos+100, ypos, 100, 25, "axis 2");
  varindex2->align(FL_ALIGN_TOP);
  varindex2->textsize(12);
  varindex2->copy( varindex_menu_items);
  varindex2->mode( nvars, FL_MENU_INACTIVE);  // disable "--nothing--" as a choice for axis2
  varindex2->callback( (Fl_Callback*)static_extract_and_redraw, this);

  // Lock y-axis button
  lock_axis2_button = b = new Fl_Button(xpos+100+80, ypos+25, 20, 20, "lock ");
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);
  b->align(FL_ALIGN_LEFT);
  b->value(0);

  // Z-axis parameter
  varindex3 = new Fl_Choice (xpos+200, ypos, 100, 25, "axis 3");
  varindex3->align(FL_ALIGN_TOP);
  varindex3->textsize(12);
  varindex3->copy( varindex_menu_items);
  varindex3->value(nvars);  // initially, axis3 == "-nothing-"
  varindex3->callback( (Fl_Callback*)static_extract_and_redraw, this);

  // Lock z-axis button
  lock_axis3_button = b = new Fl_Button(xpos+200+80, ypos+25, 20, 20, "lock ");
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);
  b->align(FL_ALIGN_LEFT);
  b->value(0);

  // NLoop: Genenerate normalization style menu
  for( int i=0; i<n_normalization_styles; i++) {
    normalization_style_menu_items[i].label(
      normalization_style_labels[i]);
    normalization_style_menu_items[i].user_data(
      (void *)normalization_styles[i]);
  }
  normalization_style_menu_items[n_normalization_styles].label(0);

  // X-axis normalization and scaling
  x_normalization_style = new Fl_Choice (xpos, ypos+=70, 100, 25, "normalize x");
  x_normalization_style->align(FL_ALIGN_TOP);
  x_normalization_style->textsize(12);
  x_normalization_style->menu(normalization_style_menu_items);
  x_normalization_style->value(NORMALIZATION_TRIM_1E3);
  x_normalization_style->callback( (Fl_Callback*)static_extract_and_redraw, this);
 
  // Y-axis normalization and scaling
  y_normalization_style = new Fl_Choice (xpos+100, ypos, 100, 25, "normalize y");
  y_normalization_style->align(FL_ALIGN_TOP);
  y_normalization_style->textsize(12);
  y_normalization_style->menu(normalization_style_menu_items);
  y_normalization_style->value(NORMALIZATION_TRIM_1E3); 
  y_normalization_style->callback( (Fl_Callback*)static_extract_and_redraw, this);
 
  // Z-axis normalization and scaling
  z_normalization_style = new Fl_Choice (xpos+200, ypos, 100, 25, "normalize z");
  z_normalization_style->align(FL_ALIGN_TOP);
  z_normalization_style->textsize(12);
  z_normalization_style->menu(normalization_style_menu_items);
  z_normalization_style->value(NORMALIZATION_TRIM_1E3); 
  z_normalization_style->callback( (Fl_Callback*)static_extract_and_redraw, this);
 
  // XXX Add some additional y-offset
  ypos+=5;

  // Initialize positions for buttons
  int xpos2 = xpos;
  int ypos2 = ypos;

  // Button (1,1) Reset view in this plot
  reset_view_button = b = new Fl_Button(xpos2, ypos+=25, 20, 20, "reset view ");
  b->align(FL_ALIGN_RIGHT); b->selection_color(FL_BLUE);
  b->callback((Fl_Callback*) reset_view, this);

  // Button (2,1): FOX News (spin) button
  spin = b= new Fl_Button(xpos2, ypos+=25, 20, 20, "spin");
  b->align(FL_ALIGN_RIGHT); b->selection_color(FL_BLUE);
  b->type(FL_TOGGLE_BUTTON);

  // Button (3,1): Don't clear button - psychedelic fun!
  dont_clear = new Fl_Button(xpos2, ypos+=25, 20, 20, "don't clear");
  dont_clear->align(FL_ALIGN_RIGHT);
  dont_clear->type(FL_TOGGLE_BUTTON);
  dont_clear->selection_color(FL_BLUE);
  dont_clear->callback(
    (Fl_Callback*)static_maybe_redraw, this);

  // Define Fl_Group to hold plot transform styles
  // XXX - this group should probably be a menu, or at least have a box around it
  // to show that they are radio buttons.
  transform_style = new Fl_Group (xpos2-1, ypos+25-1, 20+2, 4*25+2);

  // Button (4,1): No transform
  no_transform = b = new Fl_Button(xpos2, ypos+=25, 20, 20, "identity");
  b->callback((Fl_Callback*)static_extract_and_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_RADIO_BUTTON); 
  b->selection_color(FL_BLUE);

  // Button (5,1): Sum vs difference transform
  sum_vs_difference = b = new Fl_Button(xpos2, ypos+=25, 20, 20, "sum vs. diff.");
  b->callback((Fl_Callback*)static_extract_and_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_RADIO_BUTTON); 
  b->selection_color(FL_BLUE);
  
  // Button (6,1): Polar co-ords transform
  polar = b = new Fl_Button(xpos2, ypos+=25, 20, 20, "polar");
  b->callback((Fl_Callback*)static_extract_and_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_RADIO_BUTTON); 
  b->selection_color(FL_BLUE);
  
  transform_style->end();
  no_transform->setonly();

  ypos=ypos2;
  xpos=xpos2+100;

  // Button (1,2): Show points
  show_points = b = new Fl_Button(xpos, ypos+=25, 20, 20, "points");
  b->callback((Fl_Callback*)static_maybe_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);  
  b->value(1);

  // Button (2,2): Show deselected points
  show_deselected_points = b = 
    new Fl_Button(xpos, ypos+=25, 20, 20, " unselected");
  b->callback((Fl_Callback*)static_maybe_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);  
  b->value(1);

  // Button (3,2): Show axes
  show_axes = b = 
    new Fl_Button(xpos, ypos+=25, 20, 20, "axes");
  b->callback((Fl_Callback*)static_maybe_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);  
  b->value(1);

  // Button (4,2): Show axis labels
  show_labels = b = 
    new Fl_Button(xpos, ypos+=25, 20, 20, "labels");
  b->callback((Fl_Callback*)static_maybe_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);  
  b->value(1);

  // Button (4,2): Show axis scale
  show_scale = b = 
    new Fl_Button(xpos, ypos+=25, 20, 20, "scales");
  b->callback((Fl_Callback*)static_maybe_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);  
  b->value(1);

  // Button (5,2): Show grid (currently broken)
  show_grid = b = new Fl_Button(xpos, ypos+=25, 20, 20, "grid");
  b->callback((Fl_Callback*)static_maybe_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);
  b->value(0);

  // Button (6,2): Show histograms for this plot
  show_histogram = b = new Fl_Button(xpos, ypos+=25, 20, 20, "histograms");
  b->callback((Fl_Callback*)redraw_one_plot, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);  
  b->value(0);

  ypos=ypos2;
  xpos=xpos2+200;

  // Button (1,3): Choose selection color
  choose_selection_color_button = b = new Fl_Button(xpos, ypos+=25, 20, 20, "selection color");
  b->align(FL_ALIGN_RIGHT); 
  b->selection_color(FL_BLUE); 
  b->callback((Fl_Callback*)choose_color_selected, this);
}
