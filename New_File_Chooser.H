// viewpoints - interactive linked scatterplots and more.
// copyright 2005 Creon Levit, all rights reserved.
//***************************************************************************
// File name: New_File_Chooser.h
//
// Class definitions:
//   New_File_Chooser -- File chooser window for Creon Levit's viewpoints
//
// Classes referenced:
//   Various FLTK classes
//
// Required packages: none
//
// Compiler directives:
//   Requires WIN32 to be defined
//
// Purpose: File chooser for Creon Levit's viewpoints.  Based on the 
//   Fl_File_Chooser dialog for the Fast Light Tool Kit (FLTK), copyright 
//   1998-2005 by Bill Spitzak and others, for FLTK 1.1.7.  This code has
//   been extensively modified by Paul Gazis and Creon Levit to improve 
//   clarity and maintainability, change the functionality for use with 
//   viewpoints, and eliminate several Windows-specific bugs
//
// General design philosophy:
//   1) The binary and ASCII read operations are sufficiently different that 
//      they should be handled by entirely separate methods.
//   2) Data are read into global variables.
//   3) Arguments are passed as strings rather than const char* under the 'if 
//      only it were JAVA' approach to C++ style and to take advantage of 
//      STL's powerful string manipulation tools.  Unfortunately, this means 
//      that c_str() must be used to pass some of these strings on to other 
//      methods.
//   4) NOTE: There is considerable duplicate code here.  In particular, the 
//      code to read headers and the calls to Fl_File_Chooser here and in 
//      vp.cpp could be consolidated.
//
// Author: Bill Spitzak and others   1998-2005
// Modified: P. R. Gazis  23-MAR-2007
//***************************************************************************

// Protection to make sure this header is not included twice
#ifndef NEW_FILE_CHOOSER_H
#define NEW_FILE_CHOOSER_H

// Include necessary C libraries
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

// Include string funsctions implemented in FLTK, including strlcpy() and 
// strlcat() as replacements for strncpy() and strncat().  NOTE: This is
// in the FLTK source directory rather than the usual header directory
// #include "flstring.h"

// Include the all-important header for the 'stat' methods and structure.
#include <sys/stat.h>

// Include necessary C++ libraries
#include <iostream>
#include <string>
using namespace std;

// Additional WIN32-specific includes from Fl_File_Chooser2.cxx
#if defined(WIN32) && ! defined (__CYGWIN__)
  # include <direct.h>
  # include <io.h>
  
  // Visual C++ 2005 incorrectly displays a warning about the use of POSIX 
  // APIs on Windows, which is supposed to be POSIX compliant...
  # define access _access
  # define mkdir _mkdir

  // Apparently Borland C++ defines DIRECTORY in <direct.h>, which 
  // interferes with the Fl_File_Icon enumeration of the same name.
  # ifdef DIRECTORY
  #    undef DIRECTORY
  # endif   // DIRECTORY
#else
  # include <unistd.h>
  # include <pwd.h>
#endif   // WIN32 and !__CYGWIN__

// Include the necessary FLTK libraries
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Preferences.H>
#include <FL/Fl_Tile.H>
#include <FL/Fl_File_Browser.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_File_Input.H>
#include <FL/Fl_Return_Button.H>
#include <FL/fl_ask.H>

// Include necessary FLTK libraries mentioned in the source code
#include <FL/Fl_Bitmap.H>
#include <FL/fl_draw.H>

// Include necessary FLTK libraries that aren't mentioned anywhere
#include <FL/Fl_Image.H>
#include <FL/Fl_Shared_Image.H>

//***************************************************************************
// File name: New_File_Chooser.h
//
// Class definitions:
//   New_File_Chooser -- File chooser window for Creon Levit's viewpoints
//
// Classes referenced:
//   Various FLTK classes
//
// Purpose: File chooser for Creon Levit's viewpoints.
//
// Functions:
//   New_File_Chooser( *value_in, *filter_in, type_in, *title) -- Const
//   ~New_File_Chooser() -- Destructor
//
//    callback( (*pCallback)( *, *), *pData = 0) -- Set callback function
//    color() -- Get file browset color
//    color( file_browser_color_in) -- Set file browser color
//    count() -- Get number of selected files
//    directory() -- Get directory in browser
//    directory( *directory_in) -- Set directory in browser
//    filter() -- Get the file browswer filter pattern(s)
//    filter( pattern_in) -- Set the file browswer filter pattern(s)
//    filter_value() -- Get index of filter in choice window
//    filter_value( index_in) -- Set index of filter in choice window
//    hide() -- Hide main window
//    iconsize() -- Get icon size
//    iconsize( size_in) -- Set icon size
//    char* label() -- Get label of main window
//    label( const char *label_in) -- Set label of main window
//    ok_label() -- Get label of the 'OK' button
//    ok_label( label_in) -- Set label and resize the 'OK' button
//    preview() -- Get value of the preview checkbox
//    preview( iPreviewState) -- Set and enable or disable preview
//    rescan() -- Rescan the directory in the file browser
//    show() -- Set buffers and states and show main window.
//    shown() -- Get show state of main window
//    textcolor() -- Get file browswer text color
//    textcolor( file_browser_text_color_in) -- Set file browser text color
//    textfont() -- Get file browser text font
//    textfont( file_browser_text_font_in) -- Get file browser text font
//    textsize()-- Get file browser text size
//    textsize( file_browser_text_size_in) -- Get file browser text size
//    type() -- Get type of usage for the file browser
//    type( type_in) Set type of usage for the file broswer
//    user_data() -- Get pointer to the data
//    user_data( pData) -- Set pointer to the data
//    value( index) -- Return filename for this index
//    value( filename) -- Set filename
//    visible() -- Get visibility state for main window
//
//    favoritesCB( *w) -- Handle favorites dialog
//    previewCB( *fc) -- Handle timeouts for the preview box
//    update_favorites()-- Update the favorites menu
//    update_preview() -- Update the preview box
//
//    cb_favList( *o, *v) -- Wrapper for callback method for favorites list
//    cb_favList_i( *, *) -- Callback method for the favorites list
//    cb_fileBrowser( *o, *v) -- Wrapper for callback for the file browswer.
//    cb_fileBrowser_i( *, *) -- Callback method for the file browswer
//    fileBrowserCB() -- Handle file broswer dialog
//    cb_fileName( *o, *v) -- Wrapper for callback for file name field.
//    cb_fileName_i( *, *) -- Callback method for the file name field
//    fileNameCB() -- Handle file name field dialog
//    cb_preview( *o, *v) -- Wrapper for callback for preview window.
//    cb_preview_i( *, *) -- Callback method for the preview window
//    cb_showChoice( *o, *v) -- Wrapper for callback for show choice button
//    cb_showChoice_i( *, *) -- Callback method for the show choice button
//    showChoiceCB() -- Handle show choice dialog
//    cb_window( *o, *v) -- Wrapper for callback for main window
//    cb_window_i( *, *) -- Callback method for the main window
//
//    cb_cancelButton( *o, *v) -- Wrapper for callback for cancel button.
//    cb_cancelButton_i( *, *) -- Callback method for the cancel button.
//    cb_favCancelButton( *o, *v) -- Wrapper for callback for Favorites Cancel
//    cb_favCancelButton_i( *, *) -- Callback method for Favorites Cancel
//    cb_favDeleteButton( *o, *v) -- Wrapper for callback for Favorites Delete
//    cb_favDeleteButton_i( *, *) -- Callback method for Favorites Delete 
//    cb_favDownButton( *o, *v) -- Wrapper for callback for Favorites Down
//    cb_favDownButton_i( *, *) -- Callback method for Favorites Down
//    cb_favOkButton( *o, *v) -- Wrapper for callback for Favorites OK
//    cb_favOkButton_i( *, *) -- Callback method for the Favorites OK button
//    cb_favUpButton( *o, *v) -- Wrapper for callback for Favorites Up button
//    cb_favUpButton_i( *, *) -- Callback method for the Favorites Up button
//    cb_favoritesButton( *o, *v) -- Wrapper for callback for Favorites
//    cb_favoritesButton_i( *, *) -- Callback method for Favorites button/menu
//    favoritesButtonCB() -- Handle all dialog for Favorites button/menu
//    cb_newButton( *o, *v) -- Wrapper for callback for New Folder button
//    cb_newButton_i( *, *) -- Callback method for the New Folder button
//    newdir() -- Handle New Folder dialog
//    cb_okButton( *o, *v) -- Wrapper for callback for the OK button
//    cb_okButton_i( *, *) -- Callback method for the OK button
//    cb_previewButton( *o, *v) -- Wrapper for callback for previw checkbox
//    cb_previewButton_i( *, *) -- Callback method for the preview checkbox
//
// Author: Bill Spitzak and others   1998-2005
// Modified: P. R. Gazis  23-MAR-2007
//***************************************************************************
class FL_EXPORT New_File_Chooser
{
  // NOTE: In the original Fl_File_Chooser code, these methods and variables 
  // were all made private.  They are left protected here for development 
  // purposes.
  protected:
    // General methods used in other methods
    void favoritesCB( Fl_Widget *pWidget);
    static void previewCB( New_File_Chooser *fc);
    void update_favorites();
    void update_preview();

    // Non-button callbacks, grouped by purpose
    static void cb_favList( Fl_File_Browser*, void*);
    void cb_favList_i( Fl_File_Browser*, void*);
    static void cb_fileBrowser( Fl_File_Browser*, void*);
    void cb_fileBrowser_i( Fl_File_Browser*, void*);
    void fileBrowserCB();
    static void cb_fileName( Fl_File_Input*, void*);
    void cb_fileName_i( Fl_File_Input*, void*);
    void fileNameCB();
    static void cb_preview( Fl_Tile*, void*);
    void cb_preview_i( Fl_Tile*, void*);
    static void cb_showChoice( Fl_Choice*, void*);
    void cb_showChoice_i( Fl_Choice*, void*);
    void showChoiceCB();
    static void cb_window( Fl_Double_Window*, void*);
    void cb_window_i( Fl_Double_Window*, void*);

    // Button-related callbacks, grouped by button
    static void cb_cancelButton( Fl_Button*, void*);
    void cb_cancelButton_i( Fl_Button*, void*);
    static void cb_favCancelButton( Fl_Button*, void*);
    void cb_favCancelButton_i( Fl_Button*, void*);
    static void cb_favDeleteButton( Fl_Button*, void*);
    void cb_favDeleteButton_i( Fl_Button*, void*);
    static void cb_favDownButton( Fl_Button*, void*);
    void cb_favDownButton_i( Fl_Button*, void*);
    static void cb_favOkButton( Fl_Return_Button*, void*);
    void cb_favUpButton_i( Fl_Button*, void*);
    static void cb_favUpButton( Fl_Button*, void*);
    void cb_favoritesButton_i( Fl_Menu_Button*, void*);
    static void cb_favoritesButton( Fl_Menu_Button*, void*);
    void cb_favOkButton_i( Fl_Return_Button*, void*);
    void favoritesButtonCB();
    static void cb_newButton( Fl_Button*, void*);
    void cb_newButton_i( Fl_Button*, void*);
    void newdir();
    static void cb_okButton( Fl_Return_Button*, void*);
    void cb_okButton_i( Fl_Return_Button*, void*);
    static void cb_previewButton( Fl_Check_Button*, void*);
    void cb_previewButton_i( Fl_Check_Button*, void*);

    // Pointer to callback function and other buffers
    void (*callback_)( New_File_Chooser*, void *);
    void *data_;
    char directory_[ 1024];
    char pattern_[ 1024];
    char preview_text_[ 2048];
    int type_;

    // FLTK windows, boxes, and fields
    Fl_File_Browser *favList;
    Fl_Double_Window *favWindow;
    Fl_File_Browser *fileBrowser;
    Fl_File_Input *fileName;
    Fl_Box *previewBox;
    Fl_Choice *showChoice;
    Fl_Double_Window *window;
    
    // FLTK Buttons
    Fl_Button *cancelButton;
    Fl_Menu_Button *favoritesButton;
    Fl_Button *favCancelButton;
    Fl_Button *favDownButton;
    Fl_Button *favDeleteButton;
    Fl_Return_Button *favOkButton;
    Fl_Button *favUpButton;
    Fl_Return_Button *okButton;

    // Static variables
    static Fl_Preferences prefs_;
        
  public:
    New_File_Chooser(   // Constructor
      const char *value_in, const char *filter_in, 
      int type_in, const char *title);
    ~New_File_Chooser();   // Destructor

    // Access functions
    void callback(
      void (*pCallback)( New_File_Chooser*, void*), void *pData = 0);
    Fl_Color color();
    void color( Fl_Color file_browser_color_in);
    int count();
    char* directory();
    void directory( const char *directory_in);
    const char* filter();
    void filter( const char *pattern_in);
    int filter_value();
    void filter_value( int index_in);
    void hide();
    uchar iconsize();
    void iconsize( uchar size_in);
    const char* label();
    void label( const char *label_in);
    const char* ok_label();
    void ok_label( const char *label_in);
    int preview() const { return previewButton->value();};
    void preview( int iPreviewState);
    void rescan();
    void show();
    int shown();
    Fl_Color textcolor();
    void textcolor( Fl_Color file_browswer_text_color_in);
    uchar textfont();
    void textfont( uchar file_browswer_text_font_in);
    uchar textsize();
    void textsize( uchar file_browswer_text_size_in);
    int type();
    void type( int type_in);
    void user_data( void *pData);
    void* user_data() const;
    const char* value( int index = 1);
    void value( const char *filename_in);
    int visible();

    // Enumeration to hold file browser states
    enum { SINGLE = 0, MULTI = 1, CREATE = 2, DIRECTORY = 4 };

    // Publicly-accessible buttons
    Fl_Button *newButton;   // New folder
    Fl_Check_Button *previewButton;   // Preview box checkbutton

    // Public static variables to hold various labels and the sort mode
    static const char *add_favorites_label;
    static const char *all_files_label;
    static const char *custom_filter_label;
    static const char *existing_file_label;
    static const char *favorites_label;
    static const char *filename_label;
    static const char *filesystems_label;
    static const char *manage_favorites_label;
    static const char *new_directory_label;
    static const char *new_directory_tooltip;
    static const char *preview_label;
    static const char *save_label;
    static const char *show_label;
    static Fl_File_Sort_F *sort;
};

// Additional global function definitions.  NOTE: These do not seem to be used, and for
// some reason the compiler dies horribly when these are specified beginning with 
// FL_EXPORT, so these lines have been commented out.
// FL_EXPORT char *new_dir_chooser( const char *message, const char *fname, int relative=0);
// FL_EXPORT char *new_File_Chooser( const char *message, const char *pat, const char *fname, int relative=0);
// FL_EXPORT void new_File_Chooser_callback( void (*cb)( const char*));
// FL_EXPORT void new_File_Chooser_ok_label( const char*l);
#endif
