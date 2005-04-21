#ifndef Fl_Hor_Value_Slider_Input_H
#define Fl_Hor_Value_Slider_Input_H

#include <FL/Fl_Value_Slider.H>

class Fl_Hor_Value_Slider_Input : public Fl_Value_Slider_Input {
public:
    Fl_Hor_Value_Slider_Input(int X,int Y,int W,int H,const char *l=0)
	: Fl_Value_Slider_Input(X,Y,W,H,l)
  {
    type(FL_HOR_NICE_SLIDER);
    textsize(10);
    textboxsize(50);
  }
};

#endif
