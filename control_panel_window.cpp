// viewpoints - interactive linked scatterplots and more.
// copyright 2005 Creon Levit, all rights reserved.
//*****************************************************************
// File name: control_panel_window.cpp
//
// Class definitions:
//   Control_Panel_Window -- Control panel window
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
// Purpose: Source code for <Control_Panel_Window.h>
//
// Author: Creon Levit    2005-2006
// Modified: P. R. Gazis  10-NOV-2006
//*****************************************************************

// Include the necessary include libraries
#include "include_libraries_vp.h"

#include <typeinfo>

// Include globals
#include "global_definitions_vp.h"

// Include associated headers and source code
#include "plot_window.h"
#include "control_panel_window.h"

// Set static data members for class Control_Panel_Window::
//

// array to hold menu items for axis menus.  
Fl_Menu_Item 
  Control_Panel_Window::varindex_menu_items[ MAXVARS+2] = 
  { Fl_Menu_Item()};

// array to hold menu items for normalization styles
// MCL XXX this needs to be cleaned up to avoid magic numbers like 11, 12.....
Fl_Menu_Item 
  Control_Panel_Window::normalization_style_menu_items[ 12] =
   { Fl_Menu_Item()};

// Set the array of normalization schemes.  NOTE: If possible, 
// this should be be made CONST.
int Control_Panel_Window::normalization_styles[ 11] = {
  NORMALIZATION_NONE, NORMALIZATION_MINMAX,
  NORMALIZATION_ZEROMAX, NORMALIZATION_MAXABS, 
  NORMALIZATION_TRIM_1E2, NORMALIZATION_TRIM_1E3, 
  NORMALIZATION_THREESIGMA, NORMALIZATION_LOG10,
  NORMALIZATION_SQUASH, NORMALIZATION_RANK, 
  NORMALIZATION_GAUSSIANIZE};

// Set the array of character arrays that describe normalization
// schemes.  NOTE: If possible, this should be be made CONST.
char *Control_Panel_Window::normalization_style_labels[ 11] = { 
  "none", "minmax", "zeromax", "maxabs", "trim 10^-2", 
  "trim 10^-3", "threesigma", "log_10", "squash", "rank",
  "gaussianize"};

// array to hold menu items for symbol menu
Fl_Menu_Item Control_Panel_Window::symbol_menu_items[] = {
  { "points", 0, 0, (void *) SQUARE_POINTS, 0, 0, 0, 0, 0},
  { "smooth points", 0, 0, (void *) SMOOTH_POINTS, 0, 0, 0, 0, 0},
  { "crosses", 0, 0, (void *) SPRITES, 0, 0, 0, 0, 0},
  { 0, 0, 0, (void *) 0, 0, 0, 0, 0, 0}
};

//*****************************************************************
// Control_Panel_Window::Control_Panel_Window( x, y, w, h) -- 
// Default constructor.  Do nothing except call the constructor 
// for the parent class, Fl_Group.
Control_Panel_Window::Control_Panel_Window(
  int x, int y, int w, int h) : Fl_Group( x, y, w, h)
{}

//*****************************************************************
// Control_Panel_Window::broadcast_change (*master_widget) -- 
// broadcast an interaction from the master panel to all (unlocked) 
// panels.
void Control_Panel_Window::broadcast_change (Fl_Widget *master_widget)
{
  // Define a pointer to the parent of the master widget and verify
  // that it exists
  const Fl_Group *master_panel = master_widget->parent();
  assert( master_panel);

  // Indentify the index of the maser widget and verify that it has
  // a plausible value
  const int widget_index = master_panel->find(master_widget);
  assert( widget_index >= 0 && widget_index < master_panel->children());

  // Loop: Apply operation defined by the master widget to 
  // succesive windows
  for (int i=0; i<nplots; i++)
  {
    // Define a pointer to the relevant widget in this window and
    // verify that it exists
    Fl_Widget *slave_widget = cps[i]->child(widget_index);
    assert( slave_widget);
    
    // cout << "master_widget: label = " << master_widget->label() << " type = " << typeid(*master_widget).name() << endl;
    // cout << "slave_widget:  label = " << slave_widget->label() <<  " type = " << typeid(*slave_widget).name() << endl;

    // Verify that master that and slave widgets are of the same type
    assert( typeid(master_widget) == typeid(slave_widget));

    // MCL XXX downcasting to dispatch on type is considered very bad form.  
    // If value() were a virtual function of Fl_Widget (like callback() and 
    // do_callback() are) it would be cleaner.  Or we could bite the bullet and 
    // use one of the fltk publish/subscribe extensions.  That could clean up 
    // all sort of things.....  Or we could take a stab at refactoring the
    // repetitive parts of the following code using templates.
    
    // If the master widget is a button then set the slave's value using
    // the master's value.
    {
      Fl_Button *gp, *lp;
      if( (gp = dynamic_cast <Fl_Button*> (master_widget)) && 
          (lp = dynamic_cast <Fl_Button*> (slave_widget)))
        lp->value(gp->value());
    }

    // See previous comment
    {
      Fl_Valuator *gp, *lp;
      if( (gp = dynamic_cast <Fl_Valuator*> (master_widget)) && 
          (lp = dynamic_cast <Fl_Valuator*> (slave_widget)))
        lp->value(gp->value());
    }

    // See previous comment
    {
      Fl_Choice *gp, *lp;
      if( (gp = dynamic_cast <Fl_Choice*> (master_widget)) && 
          (lp = dynamic_cast <Fl_Choice*> (slave_widget)))
        lp->value(gp->value());
    }

    // If the slave widget has a callback function defined, call the
    // callback (since the slave widget's value() may have just been updated).
    if( slave_widget->callback())
    {
      // cout << ".. doing callback for widget " << widget_index 
      //      << " in panel " << i << endl;
      slave_widget->do_callback( slave_widget, cps[i]);
    }
  }
}

//*****************************************************************
// Control_Panel_Window::maybe_draw() -- Check plot window to see
// if they need to be redrawn.
void Control_Panel_Window::maybe_redraw() 
{
  // kludge.  Avoid double redraw when setting "don't clear".
  if( dont_clear->value()) return;

  //pw->redraw();
  pw->needs_redraw = 1;
}

//*****************************************************************
// Plot_Window::extract_and_redraw() -- Extract data for these 
// (new?) axes and redraw plot.  For one local control panel only.
void Control_Panel_Window::extract_and_redraw ()
{
  if( pw->extract_data_points()) {
    pw->needs_redraw = 1;
  }
}

//*****************************************************************
// Control_Panel_Window::make_widgets( cpw) -- Make widgets
void Control_Panel_Window::make_widgets( Control_Panel_Window *cpw)
{
  // Since these (virtual) control panels are really groups inside 
  // a tab inside a window, set their child widget's coordinates 
  // relative to their enclosing window's position. 
  int xpos = this->x()+50;
  int ypos = this->y()+20;

  Fl_Button *b;

  // the following portion of the panel deals with axes and their properties

  xpos = 50;
  int subwidth=105;  // ~1/3 of (width - extra room)

  // label for row of variable chooser menus
  b = new Fl_Button (xpos, ypos+=5, 45, 25, ""); // cleaner look with nothing here?
  b->labelsize(14);
  b->align(FL_ALIGN_LEFT);
  b->box(FL_NO_BOX);

  // dynamically build the variables menu
  // cout << "starting menu build, nvars = " << nvars << endl;
  for( int i=0; i<=nvars; i++) {
    // cout << "label " << i 
    //      << " = " << column_labels[i].c_str() << endl;
    varindex_menu_items[i].label((const char *)(column_labels[i].c_str()));
    varindex_menu_items[i].user_data((void *)i);
  }
  varindex_menu_items[nvars+1].label(0);

  // X-axis variable selection menu
  varindex1 = new Fl_Choice (xpos, ypos, subwidth-15, 25, "X axis");
  varindex1->align(FL_ALIGN_TOP);
  varindex1->textsize(12);
  varindex1->copy( varindex_menu_items);
  varindex1->mode( nvars, FL_MENU_INACTIVE);  // disable "--nothing--" as a choice for axis1
  varindex1->callback( (Fl_Callback*)static_extract_and_redraw, this);

  // Y-axis variable selection menu
  varindex2 = new Fl_Choice (xpos+subwidth, ypos, subwidth-15, 25, "Y axis");
  varindex2->align(FL_ALIGN_TOP);
  varindex2->textsize(12);
  varindex2->copy( varindex_menu_items);
  varindex2->mode( nvars, FL_MENU_INACTIVE);  // disable "--nothing--" as a choice for axis2
  varindex2->callback( (Fl_Callback*)static_extract_and_redraw, this);

  // Z-axis variable selection menu
  varindex3 = new Fl_Choice (xpos+2*subwidth, ypos, subwidth-15, 25, "Z axis");
  varindex3->align(FL_ALIGN_TOP);
  varindex3->textsize(12);
  varindex3->copy( varindex_menu_items);
  varindex3->value(nvars);  // initially, axis3 == "-nothing-"
  varindex3->callback( (Fl_Callback*)static_extract_and_redraw, this);

  // label for row of normalization menus
  b = new Fl_Button (xpos, ypos+=35, 45, 25, "scale");
  b->labelsize(14);
  b->align(FL_ALIGN_LEFT);
  b->box(FL_NO_BOX);

  // NLoop: Genenerate normalization style menu items
  for( int i=0; i<n_normalization_styles; i++) {
    normalization_style_menu_items[i].label(normalization_style_labels[i]);
    normalization_style_menu_items[i].user_data((void *)normalization_styles[i]);
  }
  normalization_style_menu_items[n_normalization_styles].label(0);

  // X-axis normalization and scaling
  x_normalization_style = new Fl_Choice( xpos, ypos, subwidth-15, 25);
  x_normalization_style->textsize( 12);
  x_normalization_style->menu( normalization_style_menu_items);
  x_normalization_style->value( NORMALIZATION_MINMAX);
  x_normalization_style->callback( (Fl_Callback*)static_extract_and_redraw, this);
 
  // Y-axis normalization and scaling
  y_normalization_style = new Fl_Choice( xpos+subwidth, ypos, subwidth-15, 25);
  y_normalization_style->textsize(12);
  y_normalization_style->menu(normalization_style_menu_items);
  y_normalization_style->value(NORMALIZATION_MINMAX); 
  y_normalization_style->callback( (Fl_Callback*)static_extract_and_redraw, this);
 
  // Z-axis normalization and scaling
  z_normalization_style = new Fl_Choice( xpos+2*subwidth, ypos, subwidth-15, 25);
  z_normalization_style->textsize(12);
  z_normalization_style->menu(normalization_style_menu_items);
  z_normalization_style->value(NORMALIZATION_MINMAX); 
  z_normalization_style->callback( (Fl_Callback*)static_extract_and_redraw, this);
 
  // one label for row of histogram buttons
  b = new Fl_Button (xpos, ypos+=30, 85, 25, "histo");
  b->labelsize(14);
  b->align(FL_ALIGN_LEFT);
  b->box(FL_NO_BOX);

  // create three show histogram buttons, one for each axis.
  for (int i=0; i<3; i++) {
    show_histogram[i] = new Fl_Button(xpos+i*subwidth, ypos, 20, 20);
    show_histogram[i]->callback((Fl_Callback*)redraw_one_plot, this);
    show_histogram[i]->type(FL_TOGGLE_BUTTON); 
    show_histogram[i]->selection_color(FL_BLUE);  
    show_histogram[i]->value(0);
  }
  // no Z-axis histograms (yet)
  show_histogram[2]->deactivate();

  // one label for row of bin count sliders
  b = new Fl_Button (xpos, ypos+=25, 45, 25, "N bins");
  b->labelsize(14);
  b->align(FL_ALIGN_LEFT);
  b->box(FL_NO_BOX);

  // create three bin count sliders, one for each axis.
  for (int i=0; i<3; i++) {
    nbins_slider[i] = new Fl_Hor_Value_Slider_Input(xpos+i*subwidth, ypos, subwidth-15, 20);
    nbins_slider[i]->textboxsize(30);
    nbins_slider[i]->callback((Fl_Callback*)redraw_one_plot, this);
    nbins_slider[i]->range(0.0,log2((double)Plot_Window::nbins_max));
    // nbins_slider[i]->precision(0);
    nbins_slider[i]->value(log2((double)Plot_Window::nbins_default));
    nbins_slider[i]->set_changed();
  }    
  // no Z-axis histograms (yet)
  nbins_slider[2]->deactivate();
    
  // one label for row of bin count sliders
  b = new Fl_Button (xpos, ypos+=25, 45, 25, "bin dx");
  b->labelsize(14);
  b->align(FL_ALIGN_LEFT);
  b->box(FL_NO_BOX);

  // create three bin count sliders, one for each axis.
  for (int i=0; i<3; i++) {
    hshift_slider[i] = new Fl_Hor_Value_Slider_Input(xpos+i*subwidth, ypos, subwidth-15, 20);
    hshift_slider[i]->textboxsize(30);
    hshift_slider[i]->callback((Fl_Callback*)redraw_one_plot, this);
    hshift_slider[i]->range(-1.0, 1.0);
    hshift_slider[i]->value(0.0);
    hshift_slider[i]->set_changed();
  }    
  // no Z-axis histograms (yet)
  hshift_slider[2]->deactivate();
    
  // one label for row of histogram height sliders
  b = new Fl_Button (xpos, ypos+=25, 45, 25, "bin ht");
  b->labelsize(14);
  b->align(FL_ALIGN_LEFT);
  b->box(FL_NO_BOX);

  // create three bin height sliders, one for each axis.
  for (int i=0; i<3; i++) {
    hscale_slider[i] = new Fl_Hor_Value_Slider_Input(xpos+i*subwidth, ypos, subwidth-15, 20);
    hscale_slider[i]->textboxsize(30);
    hscale_slider[i]->callback((Fl_Callback*)redraw_one_plot, this);
    hscale_slider[i]->range(0.0,10.0);
    hscale_slider[i]->value(1.0);
    hscale_slider[i]->set_changed();
  }    
  // no Z-axis histograms (yet)
  hscale_slider[2]->deactivate();
    
  // one label for row of lock axis buttons
  b = new Fl_Button (xpos, ypos+=25, 65, 25, "lock");
  b->labelsize(14);
  b->align(FL_ALIGN_LEFT);
  b->box(FL_NO_BOX);

  // Lock x-axis button
  lock_axis1_button = b = new Fl_Button(xpos+0*subwidth, ypos, 20, 20);
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);
  b->value(0);

  // Lock y-axis button
  lock_axis2_button = b = new Fl_Button(xpos+1*subwidth, ypos, 20, 20);
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);
  b->value(0);

  // Lock z-axis button
  lock_axis3_button = b = new Fl_Button(xpos+2*subwidth, ypos, 20, 20);
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);
  b->value(0);

  // next portion of the panel deals with point qualities
  //
  ypos += 60;

  // Pointsize slider
  pointsize_slider = 
    new Fl_Hor_Value_Slider_Input( xpos, ypos, cpw->w()-125, 20, "size");
  pointsize_slider->align(FL_ALIGN_LEFT);
  pointsize_slider->value(pointsize);
  pointsize_slider->step(0.25);
  pointsize_slider->bounds(1.0,50.0);
  pointsize_slider->callback((Fl_Callback*)replot, this);

  // symbol types menu
  symbol_menu = new Fl_Choice(xpos+pointsize_slider->w()+5, ypos, 60, 20);
  symbol_menu->textsize(12);
  symbol_menu->menu(symbol_menu_items);
  symbol_menu->value(SQUARE_POINTS);
  symbol_menu->callback( (Fl_Callback*)replot, this);

  // size for selected point size
  selected_pointsize_slider = 
    new Fl_Hor_Value_Slider_Input( xpos, ypos+=25, cpw->w()-125, 20, "size2");
  selected_pointsize_slider->align(FL_ALIGN_LEFT);
  selected_pointsize_slider->value(pointsize);
  selected_pointsize_slider->step(0.25);
  selected_pointsize_slider->bounds(1.0,50.0);
  selected_pointsize_slider->callback((Fl_Callback*)replot, this);

  // Background color slider
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
  Lum->bounds(0.0,1.0);
  Lum->value(0.04);  // !!!

  // Luminosity slider
  Lum2 = new Fl_Hor_Value_Slider_Input( xpos, ypos+=25, cpw->w()-60, 20, "Lum2");
  Lum2->align(FL_ALIGN_LEFT);
  Lum2->callback((Fl_Callback*)replot, this);
  Lum2->step(0.0001);
  Lum2->bounds(0.0,5.0);
  Lum2->value(1.0);

  // Rotation (and spin) slider
  rot_slider = new Fl_Hor_Value_Slider_Input( xpos, ypos+=25, cpw->w()-60, 20, "rot");
  rot_slider->align(FL_ALIGN_LEFT);
  rot_slider->callback((Fl_Callback*)replot, this);
  rot_slider->value(0.0);
  rot_slider->step(0.001);
  rot_slider->bounds(-180.0, 180.0);

  // Next portion of the panel is miscellanious stuff, per plot
  // needs to be more organized.
  ypos+=30;

  // Initialize positions for buttons
  int xpos2 = 25;
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
  dont_clear->callback((Fl_Callback*)static_maybe_redraw, this);

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
  show_deselected_points = b = new Fl_Button(xpos, ypos+=25, 20, 20, " unselected");
  b->callback((Fl_Callback*)static_maybe_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);  
  b->value(1);

  // Button (3,2): Show axes
  show_axes = b = new Fl_Button(xpos, ypos+=25, 20, 20, "axes");
  b->callback((Fl_Callback*)static_maybe_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);  
  b->value(1);

  // Button (4,2): Show axis labels
  show_labels = b = new Fl_Button(xpos, ypos+=25, 20, 20, "labels");
  b->callback((Fl_Callback*)static_maybe_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);  
  b->value(1);

  // Button (4,2): Show axis tickmarks
  show_scale = b = new Fl_Button(xpos, ypos+=25, 20, 20, "ticks");
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
  
  ypos=ypos2;
  xpos=xpos2+200;

  // Button (1,3): Choose selection color
  choose_selection_color_button = b = 
    new Fl_Button(xpos, ypos+=25, 20, 20, "selection color");
  b->align(FL_ALIGN_RIGHT); 
  b->selection_color(FL_BLUE); 
  b->callback((Fl_Callback*)choose_color_selected, this);

  // Button (2,3): z-buffering control
  z_bufferring_button = b = 
    new Fl_Button(xpos, ypos+=25, 20, 20, "z-bufferring");
  b->callback((Fl_Callback*)redraw_one_plot, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE); 
}
