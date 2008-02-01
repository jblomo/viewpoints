// viewpoints - interactive linked scatterplots and more.
// copyright 2005 Creon Levit and Paul Gazis, all rights reserved.
//***************************************************************************
// File name: Vp_Value_Input_Spin.H
//
// Class definitions:
//   Vp_Value_Input_Spin -- Input spinner for Creon Levit's viewpoints
//
// Classes referenced:
//   Various FLTK classes
//
// Required packages: none
//
// Compiler directives:
//   Requires WIN32 to be defined
//
// Purpose: Input spinner for Creon Levit's viewpoints.
//     Based on the Fl_Value_Input_Spin dialog for the Fast Light Tool Kit 
//   (FLTK), copyright 1998-2005 by Bill Spitzak and others, for FLTK 1.1.7.  
//
// General design philosophy:
//   1) TBD
//
// Author: Bill Spitzak and others   1998-2005
// Modified: Creon Levitt  31-Jan-2008
//***************************************************************************

// Protection to make sure this header is not included twice
#ifndef __Vp_Value_Input_Spin__
#define __Vp_Value_Input_Spin__
#ifndef FLTK_1_0_COMPAT
#define FLTK_1_0_COMPAT
#endif

#include <FL/Fl.H>
#include <FL/Fl_Input.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Valuator.H>
#include <math.h>

//*****************************************************************
// Class: Vp_Value_Input_Spin
//
// Class definitions:
//   Vp_Value_Input_Spin
//
// Classes referenced:
//   Fl_Valuator
//
// Purpose: Derived class of Fl_Valuator
//
// Functions:
//   Vp_Value_Input_Spin( x, y, w, h, *l = 0) -- Constructor
//  ~Vp_Value_Input_Spin() -- Destructor
//
//   draw() -- Draw widget
//   handle(int) --
//   resize( x, y, w, h) -- Resize widget
//
//   nput_cb( Fl_Widget*, void*) --
//   value_damage() --
//   repeat_callback( void *) --
//   void increment_cb() --
//
//   soft(char x) -- Access function
//   soft() -- Access function
//   textfont() --
//   textfont(uchar s) --
//   textsize() --
//   textsize(uchar s) --
//   textcolor() --
//   textcolor(uchar n) --
//   cursor_color() --
//   cursor_color(uchar n) --
//   buttonssize() --
//   buttonssize(int s) --
//
// Author: Creon Levit    31-Jan-2008
// Modified: P. R. Gazis  31-Jan-2008
//*****************************************************************
class Vp_Value_Input_Spin : public Fl_Valuator {
private:
  int ix, iy, drag, indrag,sldrag;
  int delta,deltadir;
  char soft_;
  uchar mouseobj;
  int butsize;

  static FL_EXPORT void input_cb(Fl_Widget*,void*);
  virtual FL_EXPORT void value_damage(); // cause damage() due to value() changing
  static FL_EXPORT void repeat_callback(void *);
  FL_EXPORT void increment_cb(); 
  
public:
  Fl_Input input;

  FL_EXPORT Vp_Value_Input_Spin(int x,int y,int w,int h, const char *l = 0);
  ~Vp_Value_Input_Spin();

  FL_EXPORT void draw();
  FL_EXPORT int handle(int);
  FL_EXPORT void resize(int,int,int,int);

  void soft(char x) {soft_ = x;}
  char soft() const {return soft_;}

  Fl_Font textfont() const {return input.textfont();}
  void textfont(uchar s) { input.textfont(s);}
  uchar textsize() const {return input.textsize();}
  void textsize(uchar s) {input.textsize(s);}
  Fl_Color textcolor() const {return input.textcolor();}
  void textcolor(uchar n) {input.textcolor(n);}
  Fl_Color cursor_color() const {return input.cursor_color();}
  void cursor_color(uchar n) {input.cursor_color(n);}
  int buttonssize() const {return butsize;}
  void buttonssize(int s) { butsize=s;}
};

#endif
