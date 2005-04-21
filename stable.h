/* Add C includes here */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>

#ifdef __APPLE__
#include <vecLib/vBLAS.h>
#elif linux
//extern "C" {
//# include <cblas.h>
//}
#endif
        
#if defined __cplusplus
/* Add C++ includes here */
#include <complex>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <vector>
#include <string>
#include <sstream>
 
// FLTK 
#include <FL/math.h>
#include <FL/gl.h>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Hor_Value_Slider.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Gl_Window.H>

// flews (FLTK extenstion) extras
#include <FL/Fl_flews.h>
#include <FL/Fl_Value_Slider_Input.H>
#include "Fl_Hor_Value_Slider_Input.H"

// Blitz++
#include <blitz/array.h>

#endif


