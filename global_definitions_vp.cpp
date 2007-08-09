// viewpoints - interactive linked scatterplots and more.
// copyright 2005 Creon Levit and Paul Gazis, all rights reserved.
//***************************************************************************
// File name: global_definitions_vp.cpp
//
// Class definitions: none
//   myCompare -- Member class used by std::stable_sort
//
// Classes referenced:
//   Various BLITZ templates
//
// Purpose: Source code for global methods declared in 
//   <global_definitions_vp.h>
//
// General design philosophy:
//   1) Initializers and #define EXTERN are used so that this code
//      can be used with separate compilation units.
//   2) Global variables can be evil.  Move as many of these as
//      possible into specific classes or to the main routine
//   3) Consider putting these variables into a NAMESPACE?
//
// Functions:
//   make_confirmation_window( text) -- Make confirmation window
//
// Author: Creon Levit    2005-2006
// Modified: P. R. Gazis  13-JUL-2007
//***************************************************************************

// Include the necessary include libraries
#include "include_libraries_vp.h"

// Include globals
#include "global_definitions_vp.h"

//***************************************************************************
// make_confirmation_window( text, mode) -- Make and manage the confirmation 
// window.  Result of 1,0,-1 => Yes, No, Cancel.
int make_confirmation_window( const char* text, int nButtons)
{
  // Destroy any existing window
  // MCL XXX rule #2: "Compile cleanly at high warning levels." 
  if( confirmation_window != NULL) confirmation_window->hide();
  
  // Create the confirmation window
  Fl::scheme( "plastic");  // optional
  confirmation_window = new Fl_Window( 400, 100, "Confirmation Window");
  confirmation_window->begin();
  confirmation_window->selection_color( FL_BLUE);
  confirmation_window->labelsize( 10);
  
  // Compose text. NOTE use of @@ in conjunction with label()
  string sMessage = "";
  sMessage.append( text);

  // Write text to box label and align it inside box
  Fl_Box* output_box = new Fl_Box( 5, 5, 390, 60, sMessage.c_str());
  // output_box->box( FL_SHADOW_BOX);
  output_box->box( FL_NO_BOX);
  output_box->color( 7);
  output_box->selection_color( 52);
  output_box->labelfont( FL_HELVETICA);
  output_box->labelsize( 15);
  output_box->align( FL_ALIGN_TOP|FL_ALIGN_CENTER|FL_ALIGN_INSIDE);

  // Define buttons and invoke callback functions to handle them
  Fl_Button* yes_button = new Fl_Button( 90, 70, 60, 25, "&Yes");
  Fl_Button* no_button = new Fl_Button( 170, 70, 60, 25, "&No");
  Fl_Button* cancel_button = new Fl_Button( 250, 70, 60, 25, "&Cancel");

  // Revise format if this is not the three-button mode
  if( nButtons == 1) {
    yes_button->hide();
    no_button->label( "&OK");
    cancel_button->hide();
  }

  // Finish creating and show the confirmation window.  Make sure it is 
  // 'modal' to prevent events from being delivered to the other windows.
  confirmation_window->resizable( confirmation_window);
  confirmation_window->end();
  confirmation_window->set_modal();
  confirmation_window->show();
  
  // Loop: While the window is open, wait and check the read queue until the 
  // right widget is activated
  while( confirmation_window->shown()) {
    Fl::wait();
    for( ; ;) {   // Is this loop needed?
      Fl_Widget* o = Fl::readqueue();
      if( !o) break;

      // Has the window been closed or a button been pushed?
      if( o == yes_button) {
        confirmation_window->hide();
        return 1;
      }
      else if( o == no_button) {
        confirmation_window->hide();
        return 0;
      }
      else if( o == cancel_button) {
        confirmation_window->hide();
        return -1;
      }
      else if( o == confirmation_window) {

        // Don't need to hide window because user has already deleted it
        // confirmation_window->hide();
        return -1;
      }
    }
  }

  // When in doubt, do nothing
  return -1;
}

float pow2(float x)
{
    return(x*x);
}
