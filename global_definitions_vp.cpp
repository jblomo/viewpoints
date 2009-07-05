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
//   pow2 ( x) -- x*x
//
// Author: Creon Levit    2005-2006
// Modified: P. R. Gazis  02-OCT-2008
//***************************************************************************

// Include the necessary include libraries
#include "include_libraries_vp.h"

// Include globals
#include "global_definitions_vp.h"

//***************************************************************************
// make_confirmation_window( text, nButtons, nLines) -- Make and manage the 
// confirmation window.  Result of 1,0,-1 => Yes, No, Cancel.
int make_confirmation_window( const char* text, int nButtons, int nLines)
{
  // Destroy any existing window
  // MCL XXX rule #2: "Compile cleanly at high warning levels." 
  if( confirmation_window != NULL) confirmation_window->hide();

  // Generate dimensions
  int nHeight = 30 * nLines;

  // Create the confirmation window
  Fl::scheme( "plastic");  // optional
  // confirmation_window = new Fl_Window( 400, 70, "Confirmation Window");
  confirmation_window = new Fl_Window( 400, 10+nHeight, "Confirmation Window");
  confirmation_window->begin();
  confirmation_window->selection_color( FL_BLUE);
  confirmation_window->labelsize( 10);
  
  // Compose text. NOTE use of @@ in conjunction with label()
  string sMessage = "";
  sMessage.append( text);

  // Write text to box label and align it inside box
  Fl_Box* output_box = new Fl_Box( 5, 5, 390, nHeight, sMessage.c_str());
  // output_box->box( FL_SHADOW_BOX);
  output_box->box( FL_NO_BOX);
  output_box->color( 7);
  output_box->selection_color( 52);
  output_box->labelfont( FL_HELVETICA);
  output_box->labelsize( 15);
  output_box->align( FL_ALIGN_TOP|FL_ALIGN_CENTER|FL_ALIGN_INSIDE);

  // Define buttons and invoke callback functions to handle them
  Fl_Button* yes_button = new Fl_Button( 90, nHeight-20, 60, 25, "&Yes");
  Fl_Button* no_button = new Fl_Button( 170, nHeight-20, 60, 25, "&No");
  Fl_Button* cancel_button = new Fl_Button( 250, nHeight-20, 60, 25, "&Cancel");

  // Revise format if this is not the three-button mode
  if( nButtons == 2) {
    yes_button->resize( 90+40, nHeight-20, 60, 25); 
    no_button->resize( 170+40, nHeight-20, 60, 25);
    cancel_button->hide();
  }
  else if( nButtons == 1) {
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

//***************************************************************************
// pow2( x) -- For when you absolutely need x*x.
float pow2( float x)
{
    return( x*x);
}

//***************************************************************************
// make_find_window( text, result) -- Make and manage the text search 
// window.  Ignore return value, use non-empty res parameter as outcome.
int make_find_window( const char* text, char *res)
{
  // Destroy any existing window
  if( find_window != NULL) find_window->hide();

  // Generate dimensions
  int nHeight = 30 * 3;

  Fl::scheme( "plastic");  // optional
  find_window = new Fl_Window( 400, 10+nHeight, "Find text");
  find_window->begin();
  find_window->selection_color( FL_BLUE);
  find_window->labelsize( 10);
  
  // Compose text. NOTE use of @@ in conjunction with label()
  string sMessage = "";
  sMessage.append( text);

  // Write text to box label and align it inside box
  Fl_Box* output_box = new Fl_Box( 5, 5, 390, nHeight, sMessage.c_str());
  // output_box->box( FL_SHADOW_BOX);
  output_box->box( FL_NO_BOX);
  output_box->color( 7);
  output_box->selection_color( 52);
  output_box->labelfont( FL_HELVETICA);
  output_box->labelsize( 15);
  output_box->align( FL_ALIGN_TOP|FL_ALIGN_CENTER|FL_ALIGN_INSIDE);

  // Define buttons and invoke callback functions to handle them
  Fl_Button* yes_button = new Fl_Button( 90, nHeight-20, 60, 25, "&OK");
  Fl_Button* cancel_button = new Fl_Button( 150, nHeight-20, 60, 25, "&Cancel");
  Fl_Input* inp = new Fl_Input(60, 30, 140, 30, "Text:");

  Fl::focus(inp);

  // 'modal' to prevent events from being delivered to the other windows.
  find_window->resizable( find_window);
  find_window->end();
  find_window->set_modal();
  find_window->show();
  
  // Loop: While the window is open, wait and check the read queue until the 
  // right widget is activated
  while( find_window->shown()) {
    Fl::wait();
    if(Fl::event_key(FL_Enter)) {
      find_window->hide();
      strcpy(res,inp->value());
      return 1;    
    }

    for( ; ;) {   // Is this loop needed?
      Fl_Widget* o = Fl::readqueue();
      // o->tooltip( "Search for string (case sensitive)");
      if( !o) break;

      // Has the window been closed or a button been pushed?
      if( o == yes_button) {
        find_window->hide();
        strcpy(res,inp->value());
        return 1;
      }
      else if( o == cancel_button) {
        find_window->hide();
        return -1;
      }
      else if( o == find_window) {

        return -1;
      }
    }
  }

  // When in doubt, do nothing
  return -1;
}

//***************************************************************************
// shrink_widget_fonts( target_widget, rScale) -- Shrink fonts for widget 
// and its children.  Note kludge to round up values if rScale>1.
void shrink_widget_fonts( Fl_Widget* target_widget, float rScale)
{
  // Revise font size for this window
  unsigned labelsize_new = target_widget->labelsize();
  labelsize_new = (int) (rScale*labelsize_new);
  if( rScale>1) labelsize_new++;
  target_widget->labelsize( labelsize_new);  

  // Rescale text in window of Vp_Value_Input_Spin  
#if 0
  Vp_Value_Input_Spin* p_Vp_Spin_;
  if( p_Vp_Spin_ = dynamic_cast <Vp_Value_Input_Spin*> (target_widget)) {
    int textsize_new = (p_Vp_Spin_->input).textsize();
    textsize_new = (int) (rScale*textsize_new);
    if( rScale>1) textsize_new++;
    (p_Vp_Spin_->input).textsize( textsize_new);
  }
#endif //0

  // Rescale text in menus
  Fl_Menu_* p_target_menu__;
  if( p_target_menu__ = dynamic_cast <Fl_Menu_*> (target_widget)) {
    int textsize_new = p_target_menu__->textsize();
    textsize_new = (int) (rScale*textsize_new);
    if( rScale>1) textsize_new++;
    p_target_menu__->textsize( textsize_new);
  }

  // Perform a dynamic cast to determine if this is a group.  If it is,
  // then shrink its children
  Fl_Group* p_target_group_;
  if( p_target_group_ = dynamic_cast <Fl_Group*> (target_widget)) {
    int nChildren = p_target_group_->children();
    for( int i=0; i<nChildren; i++) {
      shrink_widget_fonts( p_target_group_->child(i), rScale);
    }
  }
}
