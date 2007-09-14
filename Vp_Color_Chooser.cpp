// viewpoints - interactive linked scatterplots and more.
// copyright 2005 Creon Levit and Paul Gazis, all rights reserved.
//***************************************************************************
// File name: Vp_Color_Chooser.cpp
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
// Purpose: Source code for <Vp_Color_Chooser.h>
//
// Author: Bill Spitzak and others   1998-2005
// Modified: Creon Levitt  14-Aug-2007
//***************************************************************************
// modified version of Fl_Color_Chooser by creon

#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/math.h>
#include <stdio.h>

#include "Vp_Color_Chooser.h"

// The "hue box" can be a circle or rectilinear.
// You get a circle by defining this:
#define CIRCLE 1
// And the "hue box" can auto-update when the value changes
// you get this by defining this:
#define UPDATE_HUE_BOX 1

//*****************************************************************************
// Vp_Color_Chooser::hsv2rgb( H, S, V, R, G, B) -- Convert HSV to RGB
void Vp_Color_Chooser::hsv2rgb(
	double H, double S, double V, double& R, double& G, double& B)
{
  if (S < 5.0e-6) {
    R = G = B = V;
  }
  else {
    int i = (int)H;  
    double f = H - (float)i;
    double p1 = V*(1.0-S);
    double p2 = V*(1.0-S*f);
    double p3 = V*(1.0-S*(1.0-f));
    switch (i) {
    case 0: R = V;   G = p3;  B = p1;  break;
    case 1: R = p2;  G = V;   B = p1;  break;
    case 2: R = p1;  G = V;   B = p3;  break;
    case 3: R = p1;  G = p2;  B = V;   break;
    case 4: R = p3;  G = p1;  B = V;   break;
    case 5: R = V;   G = p1;  B = p2;  break;
    }
  }
}

//*****************************************************************************
// Vp_Color_Chooser::hsv2rgb( H, S, V, R, G, B) -- Convert RGB to HSV
void Vp_Color_Chooser::rgb2hsv(
	double R, double G, double B, double& H, double& S, double& V)
{
  double maxv = R > G ? R : G; if (B > maxv) maxv = B;
  V = maxv;
  if (maxv>0) {
    double minv = R < G ? R : G; if (B < minv) minv = B;
    S = 1.0 - double(minv)/maxv;
    if (maxv > minv) {
      if (maxv == R) {H = (G-B)/double(maxv-minv); if (H<0) H += 6.0;}
      else if (maxv == G) H = 2.0+(B-R)/double(maxv-minv);
      else H = 4.0+(R-G)/double(maxv-minv);
    }
  }
}

// Enumeration and static variable to hold SV and RGB mosed
enum {M_RGB, M_HSV}; // modes
static Fl_Menu_Item mode_menu[] = {
  {"RGB"},
  {"HSV"},
  {0}
};

//*****************************************************************************
// Flcc_Value_Input::format -- Pass format to Fl_Valuator
int Flcc_Value_Input::format(char* buf) {
  return Fl_Valuator::format(buf);
}

//*****************************************************************************
// Vp_Color_Chooser::set_valuators() -- Set valuators for HSV or RGB modes
void Vp_Color_Chooser::set_valuators()
{
  switch (mode()) {
  case M_RGB:
    rvalue.range(0,1); rvalue.step(1,1000); rvalue.value(r_);
    gvalue.range(0,1); gvalue.step(1,1000); gvalue.value(g_);
    bvalue.range(0,1); bvalue.step(1,1000); bvalue.value(b_);
    break;
  case M_HSV:
    rvalue.range(0,6); rvalue.step(1,1000); rvalue.value(hue_);
    gvalue.range(0,1); gvalue.step(1,1000); gvalue.value(saturation_);
    bvalue.range(0,1); bvalue.step(1,1000); bvalue.value(value_);
    break;
  }
}

//*****************************************************************************
// Vp_Color_Chooser::rgb( R, G, B) -- Set RGB values
int Vp_Color_Chooser::rgb(double R, double G, double B)
{
  if (R == r_ && G == g_ && B == b_) return 0;
  r_ = R; g_ = G; b_ = B;
  double ph = hue_;
  double ps = saturation_;
  double pv = value_;
  rgb2hsv(R,G,B,hue_,saturation_,value_);
  set_valuators();
  set_changed();
  if (value_ != pv) {
#ifdef UPDATE_HUE_BOX
    huebox.damage(FL_DAMAGE_SCROLL);
#endif
    valuebox.damage(FL_DAMAGE_EXPOSE);}
  if (hue_ != ph || saturation_ != ps) {
    huebox.damage(FL_DAMAGE_EXPOSE); 
    valuebox.damage(FL_DAMAGE_SCROLL);
  }
  return 1;
}

//*****************************************************************************
// Vp_Color_Chooser::hsv( R, G, B) -- Set HSV values
int Vp_Color_Chooser::hsv(double H, double S, double V)
{
  H = fmod(H,6.0); if (H < 0.0) H += 6.0;
  if (S < 0.0) S = 0.0; else if (S > 1.0) S = 1.0;
  if (V < 0.0) V = 0.0; else if (V > 1.0) V = 1.0;
  if (H == hue_ && S == saturation_ && V == value_) return 0;
  double ph = hue_;
  double ps = saturation_;
  double pv = value_;
  hue_ = H; saturation_ = S; value_ = V;
  if (value_ != pv) {
#ifdef UPDATE_HUE_BOX
    huebox.damage(FL_DAMAGE_SCROLL);
#endif
    valuebox.damage(FL_DAMAGE_EXPOSE);}
  if (hue_ != ph || saturation_ != ps) {
    huebox.damage(FL_DAMAGE_EXPOSE); 
    valuebox.damage(FL_DAMAGE_SCROLL);
  }
  hsv2rgb(H,S,V,r_,g_,b_);
  set_valuators();
  set_changed();
  return 1;
}

////////////////////////////////////////////////////////////////

//*****************************************************************************
// tohs( x, y, h, s) -- Global method
static void tohs(double x, double y, double& h, double& s) {
#ifdef CIRCLE
  x = 2*x-1;
  y = 1-2*y;
  s = sqrt(x*x+y*y); if (s > 1.0) s = 1.0;
  h = (3.0/M_PI)*atan2(y,x);
  if (h<0) h += 6.0;
#else
  h = fmod(6.0*x,6.0); if (h < 0.0) h += 6.0;
  s = 1.0-y; if (s < 0.0) s = 0.0; else if (s > 1.0) s = 1.0;
#endif
}

//*****************************************************************************
// Flcc_HueBox::handle( e) -- 
int Flcc_HueBox::handle( int e)
{
  static double ih, is;
  Vp_Color_Chooser* c = (Vp_Color_Chooser*)parent();
  switch (e) {
  case FL_PUSH:
    if (Fl::visible_focus()) {
      Fl::focus(this);
      redraw();
    }
    ih = c->hue();
    is = c->saturation();
  case FL_DRAG: {
    double Xf, Yf, H, S;
    Xf = (Fl::event_x()-x()-Fl::box_dx(box()))/double(w()-Fl::box_dw(box()));
    Yf = (Fl::event_y()-y()-Fl::box_dy(box()))/double(h()-Fl::box_dh(box()));
    tohs(Xf, Yf, H, S);
    if (fabs(H-ih) < 3*6.0/w()) H = ih;
    if (fabs(S-is) < 3*1.0/h()) S = is;
    if (Fl::event_state(FL_CTRL)) H = ih;
    if (c->hsv(H, S, c->value())) c->do_callback();
    } return 1;
  case FL_FOCUS :
  case FL_UNFOCUS :
    if (Fl::visible_focus()) {
      redraw();
      return 1;
    }
    else return 1;
  case FL_KEYBOARD :
    return handle_key(Fl::event_key());
  default:
    return 0;
  }
}

//*****************************************************************************
// generate_image( void* vv, X, Y, W, buf) -- global static method
static void generate_image(void* vv, int X, int Y, int W, uchar* buf) 
{
  Flcc_HueBox* v = (Flcc_HueBox*)vv;
  int iw = v->w()-Fl::box_dw(v->box());
  double Yf = double(Y)/(v->h()-Fl::box_dh(v->box()));
#ifdef UPDATE_HUE_BOX
  const double V = ((Vp_Color_Chooser*)(v->parent()))->value();
#else
  const double V = 1.0;
#endif
  for (int x = X; x < X+W; x++) {
    double Xf = double(x)/iw;
    double H,S; tohs(Xf,Yf,H,S);
    double r=0,g=0,b=0;
    Vp_Color_Chooser::hsv2rgb(H,S,V,r,g,b);
    *buf++ = uchar(255*r+.5);
    *buf++ = uchar(255*g+.5);
    *buf++ = uchar(255*b+.5);
  }
}

//*****************************************************************************
// Flcc_HueBox::handle_key( key) -- 
int Flcc_HueBox::handle_key(int key)
{
  int w1 = w()-Fl::box_dw(box())-6;
  int h1 = h()-Fl::box_dh(box())-6;
  Vp_Color_Chooser* c = (Vp_Color_Chooser*)parent();

#ifdef CIRCLE
  int X = int(.5*(cos(c->hue()*(M_PI/3.0))*c->saturation()+1) * w1);
  int Y = int(.5*(1-sin(c->hue()*(M_PI/3.0))*c->saturation()) * h1);
#else
  int X = int(c->hue()/6.0*w1);
  int Y = int((1-c->saturation())*h1);
#endif

  switch (key) {
    case FL_Up :
      Y -= 3;
      break;
    case FL_Down :
      Y += 3;
      break;
    case FL_Left :
      X -= 3;
      break;
    case FL_Right :
      X += 3;
      break;
    default :
      return 0;
  }

  double Xf, Yf, H, S;
  Xf = (double)X/(double)w1;
  Yf = (double)Y/(double)h1;
  tohs(Xf, Yf, H, S);
  if (c->hsv(H, S, c->value())) c->do_callback();

  return 1;
}

//*****************************************************************************
// Flcc_HueBox::draw() -- 
void Flcc_HueBox::draw()
{
  if (damage()&FL_DAMAGE_ALL) draw_box();
  int x1 = x()+Fl::box_dx(box());
  int yy1 = y()+Fl::box_dy(box());
  int w1 = w()-Fl::box_dw(box());
  int h1 = h()-Fl::box_dh(box());
  if (damage() == FL_DAMAGE_EXPOSE) fl_clip(x1+px,yy1+py,6,6);
  fl_draw_image(generate_image, this, x1, yy1, w1, h1);
  if (damage() == FL_DAMAGE_EXPOSE) fl_pop_clip();
  Vp_Color_Chooser* c = (Vp_Color_Chooser*)parent();
#ifdef CIRCLE
  int X = int(.5*(cos(c->hue()*(M_PI/3.0))*c->saturation()+1) * (w1-6));
  int Y = int(.5*(1-sin(c->hue()*(M_PI/3.0))*c->saturation()) * (h1-6));
#else
  int X = int(c->hue()/6.0*(w1-6));
  int Y = int((1-c->saturation())*(h1-6));
#endif
  if (X < 0) X = 0; else if (X > w1-6) X = w1-6;
  if (Y < 0) Y = 0; else if (Y > h1-6) Y = h1-6;
  //  fl_color(c->value()>.75 ? FL_BLACK : FL_WHITE);
  draw_box(FL_UP_BOX,x1+X,yy1+Y,6,6,Fl::focus() == this ? FL_FOREGROUND_COLOR : FL_GRAY);
  px = X; py = Y;
}

////////////////////////////////////////////////////////////////

//*****************************************************************************
// Flcc_ValueBox::handle( e) -- 
int Flcc_ValueBox::handle( int e)
{
  static double iv;
  Vp_Color_Chooser* c = (Vp_Color_Chooser*)parent();
  switch (e) {
  case FL_PUSH:
    if (Fl::visible_focus()) {
      Fl::focus(this);
      redraw();
    }
    iv = c->value();
  case FL_DRAG: {
    double Yf;
    Yf = 1-(Fl::event_y()-y()-Fl::box_dy(box()))/double(h()-Fl::box_dh(box()));
    if (fabs(Yf-iv)<(3*1.0/h())) Yf = iv;
    if (c->hsv(c->hue(),c->saturation(),Yf)) c->do_callback();
    } return 1;
  case FL_FOCUS :
  case FL_UNFOCUS :
    if (Fl::visible_focus()) {
      redraw();
      return 1;
    }
    else return 1;
  case FL_KEYBOARD :
    return handle_key(Fl::event_key());
  default:
    return 0;
  }
}

//*****************************************************************************
// generate_vimage( void* vv, X, Y, W, buf) -- staic global method
static double tr, tg, tb;
static void generate_vimage(void* vv, int X, int Y, int W, uchar* buf) {
  Flcc_ValueBox* v = (Flcc_ValueBox*)vv;
  double Yf = 255*(1.0-double(Y)/(v->h()-Fl::box_dh(v->box())));
  uchar r = uchar(tr*Yf+.5);
  uchar g = uchar(tg*Yf+.5);
  uchar b = uchar(tb*Yf+.5);
  for (int x = X; x < X+W; x++) {
    *buf++ = r; *buf++ = g; *buf++ = b;
  }
}

//*****************************************************************************
// Flcc_ValueBox::draw() -- 
void Flcc_ValueBox::draw() {
  if (damage()&FL_DAMAGE_ALL) draw_box();
  Vp_Color_Chooser* c = (Vp_Color_Chooser*)parent();
  c->hsv2rgb(c->hue(),c->saturation(),1.0,tr,tg,tb);
  int x1 = x()+Fl::box_dx(box());
  int yy1 = y()+Fl::box_dy(box());
  int w1 = w()-Fl::box_dw(box());
  int h1 = h()-Fl::box_dh(box());
  if (damage() == FL_DAMAGE_EXPOSE) fl_clip(x1,yy1+py,w1,6);
  fl_draw_image(generate_vimage, this, x1, yy1, w1, h1);
  if (damage() == FL_DAMAGE_EXPOSE) fl_pop_clip();
  int Y = int((1-c->value()) * (h1-6));
  if (Y < 0) Y = 0; else if (Y > h1-6) Y = h1-6;
  draw_box(FL_UP_BOX,x1,yy1+Y,w1,6,Fl::focus() == this ? FL_FOREGROUND_COLOR : FL_GRAY);
  py = Y;
}

//*****************************************************************************
// Flcc_ValueBox::handle_key( key) -- 
int Flcc_ValueBox::handle_key(int key) {
  int h1 = h()-Fl::box_dh(box())-6;
  Vp_Color_Chooser* c = (Vp_Color_Chooser*)parent();

  int Y = int((1-c->value()) * h1);
  if (Y < 0) Y = 0; else if (Y > h1) Y = h1;

  switch (key) {
    case FL_Up :
      Y -= 3;
      break;
    case FL_Down :
      Y += 3;
      break;
    default :
      return 0;
  }

  double Yf;
  Yf = 1-((double)Y/(double)h1);
  if (c->hsv(c->hue(),c->saturation(),Yf)) c->do_callback();

  return 1;
}

////////////////////////////////////////////////////////////////

//*****************************************************************************
// Vp_Color_Chooser::rgb_cb( Fl_Widget* o, void*) -- 
void Vp_Color_Chooser::rgb_cb(Fl_Widget* o, void*)
{
  Vp_Color_Chooser* c = (Vp_Color_Chooser*)(o->parent());
  double R = c->rvalue.value();
  double G = c->gvalue.value();
  double B = c->bvalue.value();
  if (c->mode() == M_HSV) {
    if (c->hsv(R,G,B)) c->do_callback();
    return;
  }
  if (c->mode() != M_RGB) {
    R = R/255;
    G = G/255;
    B = B/255;
  }
  if (c->rgb(R,G,B)) c->do_callback();
}

//*****************************************************************************
// Vp_Color_Chooser::mode_cb( Fl_Widget* o, void*) -- 
void Vp_Color_Chooser::mode_cb(Fl_Widget* o, void*)
{
  Vp_Color_Chooser* c = (Vp_Color_Chooser*)(o->parent());
  // force them to redraw even if value is the same:
  c->rvalue.value(-1);
  c->gvalue.value(-1);
  c->bvalue.value(-1);
  c->set_valuators();
}

////////////////////////////////////////////////////////////////

//*****************************************************************************
// Vp_Color_Chooser::Vp_Color_Chooser( X, Y, W, H, L) -- Constructor
Vp_Color_Chooser::Vp_Color_Chooser(int X, int Y, int W, int H, const char* L)
  : Fl_Group(0,0,195,115,L),
    huebox(0,0,115,115),
    valuebox(115,0,20,115),
    choice(140,0,55,25),
    rvalue(140,30,55,25),
    gvalue(140,60,55,25),
    bvalue(140,90,55,25),
    resize_box(0,0,115,115)
{
  end();
  resizable(resize_box);
  resize(X,Y,W,H);
  r_ = g_ = b_ = 0;
  hue_ = 0.0;
  saturation_ = 0.0;
  value_ = 0.0;
  huebox.box(FL_DOWN_FRAME);
  valuebox.box(FL_DOWN_FRAME);
  choice.menu(mode_menu);
  set_valuators();
  rvalue.callback(rgb_cb);
  gvalue.callback(rgb_cb);
  bvalue.callback(rgb_cb);
  rvalue.textsize(10);
  gvalue.textsize(10);
  bvalue.textsize(10);
  choice.callback(mode_cb);
  choice.box(FL_NO_BOX);
  choice.textfont(FL_HELVETICA_BOLD);
  choice.textsize(11);
  choice.clear_visible_focus();
}

////////////////////////////////////////////////////////////////
// fl_color_chooser():

#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Return_Button.H>

//*****************************************************************************
// Class: ColorChip
//
// Class definitions:
//   ColorChip
//
// Classes referenced:
//
// Purpose: Derived class of Fl_Widget
//
// Functions:
//   ColorChip( X, Y, W, H)
//   draw()
//
// Author: Creon Levit    14-Aug-2007
// Modified: P. R. Gazis  15-Aug-2007
//*****************************************************************************
class ColorChip : public Fl_Widget {
  void draw();
public:
  uchar r,g,b;
  ColorChip(int X, int Y, int W, int H) : Fl_Widget(X,Y,W,H) {
    box(FL_ENGRAVED_FRAME);}
};

//*****************************************************************************
// ColorChip::draw() -- 
void ColorChip::draw() {
  if (damage()&FL_DAMAGE_ALL) draw_box();
  fl_rectf(x()+Fl::box_dx(box()),
	   y()+Fl::box_dy(box()),
	   w()-Fl::box_dw(box()),
	   h()-Fl::box_dh(box()),r,g,b);
}

//*****************************************************************************
// chooser_cb( Fl_Object* o, void* vv) -- Global static method
static void chooser_cb(Fl_Object* o, void* vv)
{
  Vp_Color_Chooser* c = (Vp_Color_Chooser*)o;
  ColorChip* v = (ColorChip*)vv;
  v->r = uchar(255*c->r()+.5);
  v->g = uchar(255*c->g()+.5);
  v->b = uchar(255*c->b()+.5);
  v->damage(FL_DAMAGE_EXPOSE);
}

extern const char* fl_ok;
extern const char* fl_cancel;

//*****************************************************************************
// vp_color_chooser( name, r, g, b) -- Global method
int vp_color_chooser( const char* name, double& r, double& g, double& b)
{
  Fl_Window window(215,200,name);
  Vp_Color_Chooser chooser(10, 10, 195, 115);
  ColorChip ok_color(10, 130, 95, 25);
  Fl_Return_Button ok_button(10, 165, 95, 25, fl_ok);
  ColorChip cancel_color(110, 130, 95, 25);
  cancel_color.r = uchar(255*r+.5); ok_color.r = cancel_color.r;
  ok_color.g = cancel_color.g = uchar(255*g+.5);
  ok_color.b = cancel_color.b = uchar(255*b+.5);
  Fl_Button cancel_button(110, 165, 95, 25, fl_cancel);
  window.resizable(chooser);
  chooser.rgb(r,g,b);
  chooser.callback(chooser_cb, &ok_color);
  window.end();
  window.set_modal();
  window.hotspot(window);
  window.show();
  while (window.shown()) {
    Fl::wait();
    for (;;) {
      Fl_Widget* o = Fl::readqueue();
      if (!o) break;
      if (o == &ok_button) {
	r = chooser.r();
	g = chooser.g();
	b = chooser.b();
	return 1;
      }
      if (o == &window || o == &cancel_button) return 0;
    }
  }
  return 0;
}

//*****************************************************************************
// vp_color_chooser( name, r, g, b) -- Global method
int vp_color_chooser( const char* name, uchar& r, uchar& g, uchar& b)
{
  double dr = r/255.0;
  double dg = g/255.0;
  double db = b/255.0;
  if (vp_color_chooser(name,dr,dg,db)) {
    r = uchar(255*dr+.5);
    g = uchar(255*dg+.5);
    b = uchar(255*db+.5);
    return 1;
  }
  return 0;
}
