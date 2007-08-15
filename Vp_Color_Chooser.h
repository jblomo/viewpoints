// viewpoints - interactive linked scatterplots and more.
// copyright 2005 Creon Levit and Paul Gazis, all rights reserved.
//***************************************************************************
// File name: Vp_Color_Chooser.h
//
// Class definitions:
//   Flcc_HueBox  --
//   Flcc_ValueBox --
//   Flcc_Value_Input --
//   Vp_Color_Chooser -- Color chooser window for Creon Levit's viewpoints
//
// Classes referenced:
//   Various FLTK classes
//
// Required packages: none
//
// Compiler directives:
//   Requires WIN32 to be defined
//
// Purpose: Color chooser for Creon Levit's viewpoints.  Maintains the color
//   chooser object and the color chooser popup.  The popup is just a window 
//   containing a single color chooser and some boxes to indicate the current 
//   and cancelled color.
//     Based on the Fl_Color_Chooser dialog for the Fast Light Tool Kit 
//   (FLTK), copyright 1998-2005 by Bill Spitzak and others, for FLTK 1.1.7.  
//
// General design philosophy:
//   1) TBD
//
// Author: Bill Spitzak and others   1998-2005
// Modified: Creon Levitt  14-Aug-2007
//***************************************************************************

// Protection to make sure this header is not included twice
#ifndef Vp_Color_Chooser_H
#define Vp_Color_Chooser_H

#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Value_Input.H>

//*****************************************************************
// Class: Flcc_HueBox
//
// Class definitions:
//   Flcc_HueBox
//
// Classes referenced:
//   Fl_Widget
//
// Purpose: Derived class of Fl_Widget
//
// Functions:
//   Flcc_HueBox( X, Y, W, H)
//   draw()
//   handle_key( int)
//   handle( int)
//
// Author: Creon Levit    14-Aug-2007
// Modified: P. R. Gazis  15-Aug-2007
//*****************************************************************
class Flcc_HueBox : public Fl_Widget {
  int px, py;
protected:
  void draw();
  int handle_key(int);
public:
  int handle(int);
  Flcc_HueBox(int X, int Y, int W, int H) : Fl_Widget(X,Y,W,H) {
  px = py = 0;}
};

//*****************************************************************
// Class: Flcc_ValueBox
//
// Class definitions:
//   Flcc_ValueBox
//
// Classes referenced:
//   Fl_Widget
//
// Purpose: Derived class of Fl_Widget
//
// Functions:
//   Flcc_ValueBox( X, Y, W, H)
//   draw()
//   handle_key( int)
//   handle( int)
//
// Author: Creon Levit    14-Aug-2007
// Modified: P. R. Gazis  15-Aug-2007
//*****************************************************************
class Flcc_ValueBox : public Fl_Widget {
  int py;
protected:
  void draw();
  int handle_key(int);
public:
  int handle(int);
  Flcc_ValueBox(int X, int Y, int W, int H) : Fl_Widget(X,Y,W,H) {
  py = 0;}
};

//*****************************************************************
// Class: Flcc_Value_Input
//
// Class definitions:
//   Flcc_Value_Input
//
// Classes referenced:
//   Fl_Value_Input
//
// Purpose: Derived class of Fl_Value_Input
//
// Functions:
//   Flcc_Value_Input( X, Y, W, H)
//   format()
//
// Author: Creon Levit    14-Aug-2007
// Modified: P. R. Gazis  15-Aug-2007
//*****************************************************************
class Flcc_Value_Input : public Fl_Value_Input {
public:
  int format(char*);
  Flcc_Value_Input(int X, int Y, int W, int H) : Fl_Value_Input(X,Y,W,H) {}
};

//*****************************************************************
// Class: Vp_Color_Chooser
//
// Class definitions:
//   Vp_Color_Chooser
//
// Classes referenced:
//   Fl_Group
//
// Purpose: Derived class of Fl_Group
//
// Functions:
//   Vp_Color_Chooser(int,int,int,int,const char* = 0) --
//
//   mode() --
//   hue() --
//   saturation() --
//   value() --
//   r() --
//   g() --
//   b() --
//   hsv( double, double, double) --
//   rgb( double, double, double) --
//
//   hsv2rgb( double, double, double,double&,double&,double&) --
//   rgb2hsv( double, double, double,double&,double&,double&) --
//
// Author: Creon Levit    14-Aug-2007
// Modified: P. R. Gazis  15-Aug-2007
//*****************************************************************
class Vp_Color_Chooser : public Fl_Group {
  Flcc_HueBox huebox;
  Flcc_ValueBox valuebox;
  Fl_Choice choice;
  Flcc_Value_Input rvalue;
  Flcc_Value_Input gvalue;
  Flcc_Value_Input bvalue;
  Fl_Box resize_box;
  double hue_, saturation_, value_;
  double r_, g_, b_;
  void set_valuators();
  static void rgb_cb(Fl_Widget*, void*);
  static void mode_cb(Fl_Widget*, void*);

public:
  int mode() {return choice.value();}
  double hue() const {return hue_;}
  double saturation() const {return saturation_;}
  double value() const {return value_;}
  double r() const {return r_;}
  double g() const {return g_;}
  double b() const {return b_;}
  int hsv(double,double,double);
  int rgb(double,double,double);
  static void hsv2rgb(double, double, double,double&,double&,double&);
  static void rgb2hsv(double, double, double,double&,double&,double&);
  Vp_Color_Chooser(int,int,int,int,const char* = 0);
};

// Definitions for two vp_color_chooser methods
int vp_color_chooser(const char* name, double& r, double& g, double& b);
int vp_color_chooser(const char* name, uchar& r, uchar& g, uchar& b);

#endif
