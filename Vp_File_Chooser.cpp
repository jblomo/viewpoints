// viewpoints - interactive linked scatterplots and more.
// copyright 2005 Creon Levit and Paul Gazis, all rights reserved.
//***************************************************************************
// File name: Vp_File_Chooser.cpp
//
// Class definitions:
//   Vp_File_Chooser -- File chooser window for Creon Levit's viewpoints
//
// Classes referenced:
//   Various FLTK classes
//
// Required packages: none
//
// Compiler directives:
//   Requires WIN32 to be defined
//
// Purpose: Source code for <Vp_File_Chooser.H.h>.  Based on the 
//   Fl_File_Chooser dialog for the Fast Light Tool Kit (FLTK), copyright 
//   1998-2005 by Bill Spitzak and others, for FLTK 1.1.7, but extensively
//   modified by Paul Gazis and Creon Levit for use with viewpoints.
//
// Author: Bill Spitzak and others   1998-2005
// Modified: P. R. Gazis  07-NOV-2007
//***************************************************************************

// Include header
#include "Vp_File_Chooser.H"

// Define statics to hol bitmap image for new folder button
static unsigned char idata_new[] = {
  0,0,120,0,132,0,2,1,
  1,254,1,128,49,128,49,128,
  253,128,253,128,49,128,49,128,
  1,128,1,128,255,255,0,0};
static Fl_Bitmap image_new( idata_new, 16, 16);

// Set File chooser label strings and sort function...
Fl_Preferences Vp_File_Chooser::prefs_( Fl_Preferences::USER, "fltk.org", "filechooser");
const char *Vp_File_Chooser::add_favorites_label = "Add to Favorites";
const char *Vp_File_Chooser::all_files_label = "All Files (*)";
const char *Vp_File_Chooser::custom_filter_label = "Custom Filter";
const char *Vp_File_Chooser::existing_file_label = "Please choose an existing file!";
const char *Vp_File_Chooser::favorites_label = "Favorites";
const char *Vp_File_Chooser::filename_label = "Filename:";
const char *Vp_File_Chooser::filetype_label = "File Type:";
#ifdef WIN32
  const char *Vp_File_Chooser::filesystems_label = "My Computer";
#else
  const char *Vp_File_Chooser::filesystems_label = "File Systems";
#endif   // WIN32
const char *Vp_File_Chooser::manage_favorites_label = "Manage Favorites";
const char *Vp_File_Chooser::new_directory_label = "New Directory?";
const char *Vp_File_Chooser::new_directory_tooltip = "Create a new directory.";
const char *Vp_File_Chooser::preview_label = "Preview";
const char *Vp_File_Chooser::save_label = "Save";
const char *Vp_File_Chooser::selection_label = "Write Selection Info";
const char *Vp_File_Chooser::selection_tooltip = "Include selection information with data";
const char *Vp_File_Chooser::show_label = "Show ext:";
Fl_File_Sort_F *Vp_File_Chooser::sort = fl_numericsort;

// Define various static global functions to process pathnames
static int compare_dirnames( const char *a, const char *b);
static void quote_pathname( char *, const char *, int);
static void unquote_pathname( char *, const char *, int);

// Define a static global function to replace fl_filename_isdir.  In the 
// original fl_filename_isdir.cxx, this was accomplished by including 
// #include <FL/filename.H>, but this doesn't seem to work here.
FL_EXPORT static int new_filename_isdir( const char* pathname);

//*****************************************************************************
// Vp_File_Chooser::Vp_File_Chooser( *value_in, *filter_in, type_in, 
// *title) -- Constructor.  Create windows and buttons and initialize
// various settings.
Vp_File_Chooser::Vp_File_Chooser( 
  const char *value_in, const char *filter_in, int type_in, const char *title)
{
  // Define pointer to the main double window
  Fl_Double_Window* w;

  // Double_Window scope: Create and fill the main double-buffered window to 
  // hold almost everything
  { 
    Fl_Double_Window* mainDoubleWindow = window = 
      new Fl_Double_Window( 490, 430, "Choose File");
      // new Fl_Double_Window( 490, 380, "Choose File");
    w = mainDoubleWindow;
    mainDoubleWindow->callback( (Fl_Callback*) cb_window, (void*)(this));

    // Upper Row Group scope: Create a group to hold buttons and fields
    { 
      Fl_Group* upperRowGroup = new Fl_Group( 10, 10, 470, 25);

      // 'Favorites:' Button/menu at upper left
      { 
        Fl_Menu_Button* o = favoritesButton = 
          new Fl_Menu_Button( 10, 10, 155, 25, "Favorites");
        o->down_box( FL_BORDER_BOX);
        o->callback( (Fl_Callback*) cb_favoritesButton);
        o->align( FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
        favoritesButton->label( favorites_label);
      }

      // 'New Folder' button to the right of Favorites
      { 
        Fl_Button* o = newButton = 
          new Fl_Button( 175, 10, 25, 25);
        o->image( image_new);
        o->labelsize( 8);
        o->callback( (Fl_Callback*) cb_newButton);
        o->tooltip( new_directory_tooltip);
      }

      // Box with preview checkbox above the preview window
      { 
        Fl_Group* o = new Fl_Group( 10, 10, 470, 20);
        { 
          Fl_Check_Button* o = previewButton = 
            new Fl_Check_Button( 340, 15, 73, 20, "Preview");
          o->down_box( FL_DOWN_BOX);
          o->value( 1);
          o->shortcut( 0x80070);
          o->callback( (Fl_Callback*) cb_previewButton);
          previewButton->label( preview_label);
        }
        {
          Fl_Box* o = new Fl_Box( 115, 15, 365, 20);
          Fl_Group::current()->resizable(o);
        }
        o->end();
      }
      
      upperRowGroup->end();
    }   // End of scope for the Upper Row Group
    
    // Browser Tile scope: Create a tiling to hold the file browser and 
    // preview windows
    { 
      Fl_Tile* browserTile = new Fl_Tile( 10, 45, 470, 225);
      browserTile->callback( (Fl_Callback*) cb_preview);

      // Draw File Browser on the left below the 'Show:' field
      { 
        Fl_File_Browser* o = fileBrowser = 
          new Fl_File_Browser( 10, 45, 295, 225);
        o->type( 2);
        o->callback( (Fl_Callback*) cb_fileBrowser);
        w->hotspot( o);
      }
      
      // Draw the 'Preview' box on th right below the 'Favorites' field,
      // 'New Folder' button, and 'Preview'checkbox
      { 
        Fl_Box* o = previewBox = 
          new Fl_Box( 305, 45, 175, 225, "?");
        o->box( FL_DOWN_BOX);
        o->labelsize( 100);
        o->align( FL_ALIGN_CLIP|FL_ALIGN_INSIDE);
      }
      browserTile->end();
      Fl_Group::current()->resizable( browserTile);
    }   // End of scope for the Browser Tile
    
    // Lower Controls Group scope: Create group to hold a box with the 
    // filename input field, the file type chooser, the extension chooser,
    // and a box with the 'OK' and 'Cancel' buttons.
    {
      // Fl_Group* lowerControlsGroup = new Fl_Group( 10, 275, 470, 95);
      Fl_Group* lowerControlsGroup = new Fl_Group( 10, 275, 470, 145);

      // Filename input field
      { 
        Fl_File_Input* o = fileName =
          new Fl_File_Input( 85, 280, 365, 35);
          // new Fl_File_Input( 115, 300, 365, 35);
        o->labelfont( 1);
        o->callback( (Fl_Callback*) cb_fileName);
        o->when( FL_WHEN_ENTER_KEY);
        Fl_Group::current()->resizable(o);
        fileName->when( FL_WHEN_CHANGED | FL_WHEN_ENTER_KEY_ALWAYS);
      }

      // Box with filename label
      {
        Fl_Box* o = new Fl_Box( 10, 290, 105, 25, "Filename:");
        o->labelfont( 1);
        o->align( FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
        o->label( filename_label);
      }

      // 'File Type:' File type menu
      { 
        Fl_Choice* o = fileType = 
          new Fl_Choice( 85, 320, 215, 25);
        o->down_box( FL_BORDER_BOX);
        o->labelfont( 1);
        o->add( "ASCII");
        o->add( "binary");
        o->value( 0);
        o->callback( (Fl_Callback*) cb_fileType);
        Fl_Group::current()->resizable(o);
      }

      // Box with file type label
      {
        Fl_Box* o = new Fl_Box( 10, 320, 55, 25, "File Type:");
        o->labelfont( 1);
        o->align( FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
        o->label( filetype_label);
      }

      // 'Show:' Show extension chooser
      { 
        Fl_Choice* o = showChoice = 
          new Fl_Choice( 85, 350, 215, 25);
        o->down_box( FL_BORDER_BOX);
        o->labelfont( 1);
        o->callback( (Fl_Callback*) cb_showChoice);
        Fl_Group::current()->resizable(o);
        // showChoice->label( show_label);
      }

      // Box with the show label
      {
        Fl_Box* o = new Fl_Box( 10, 350, 105, 25, "Show ext:");
        o->labelfont( 1);
        o->align( FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
        o->label( show_label);
      }

      // Write Selection Info check button
      { 
        Fl_Check_Button* o = selectionButton = 
          new Fl_Check_Button( 85, 380, 160, 20, "Include selection info");
        o->down_box( FL_DOWN_BOX);
        o->value( 0);
        o->callback( (Fl_Callback*) cb_selectionButton);
        o->tooltip( selection_tooltip);
        selectionButton->label( selection_label);
      }

      // Group with 'OK' and 'Cancel' buttons at bottom right
      {
        // Fl_Group* o = new Fl_Group( 10, 345, 470, 25);
        Fl_Group* o = new Fl_Group( 10, 395, 470, 25);
        {
          Fl_Return_Button* o = okButton = 
            new Fl_Return_Button( 313, 395, 85, 25, "OK");
            // new Fl_Return_Button( 313, 345, 85, 25, "OK");
          o->callback( (Fl_Callback*) cb_okButton);
          okButton->label( fl_ok);
        }
        {
          Fl_Button* o = cancelButton = 
            new Fl_Button( 408, 395, 72, 25, "Cancel");
            // new Fl_Button( 408, 345, 72, 25, "Cancel");
          o->callback( (Fl_Callback*) cb_cancelButton);
          o->label( fl_cancel);
        }
        {
          Fl_Box* o = new Fl_Box( 10, 395, 30, 25);
          // Fl_Box* o = new Fl_Box( 10, 345, 30, 25);
          Fl_Group::current()->resizable(o);
        }
        o->end();
      }
      lowerControlsGroup->end();
    }   // End of scope for the Lower Controls Group

    // If title is available, load it.
    if( title) window->label( title);
    mainDoubleWindow->set_modal();
    mainDoubleWindow->end();
  }   // End scope for main double window
  
  // FavWindow scope: Create the favorites window
  {
    Fl_Double_Window* o = favWindow = 
      new Fl_Double_Window( 355, 150, "Manage Favorites");
    w = o;
    o->user_data( (void*) (this));

    // FavWindow scope: Create a file browser to hold the favorites list
    {
      Fl_File_Browser* o = favList = 
        new Fl_File_Browser( 10, 10, 300, 95);
      o->type( 2);
      o->callback( (Fl_Callback*) cb_favList);
      Fl_Group::current()->resizable(o);
    }

    // FavWindow scope: Create a group to hold the favorites up, down, and 
    // delete buttons
    {
      Fl_Group* o = new Fl_Group( 320, 10, 25, 95);
      {
        Fl_Button* o = favUpButton = 
          new Fl_Button( 320, 10, 25, 25, "@8>");
        o->callback( (Fl_Callback*) cb_favUpButton);
      }
      {
        Fl_Button* o = favDeleteButton = 
          new Fl_Button( 320, 45, 25, 25, "X");
        o->labelfont( 1);
        o->callback( (Fl_Callback*) cb_favDeleteButton);
        Fl_Group::current()->resizable(o);
      }
      {
        Fl_Button* o = favDownButton = 
          new Fl_Button( 320, 80, 25, 25, "@2>");
        o->callback( (Fl_Callback*) cb_favDownButton);
      }
      o->end();
    }

    // Manage Favorites Group scope: Create a group to hold a the
    // 'Manage Favorites' window
    {
      Fl_Group* o = new Fl_Group( 10, 113, 335, 29);
      {
        Fl_Button* o = favCancelButton = 
          new Fl_Button( 273, 115, 72, 25, "Cancel");
        o->callback( (Fl_Callback*) cb_favCancelButton);
        favCancelButton->label( fl_cancel);
      }
      {
        Fl_Return_Button* o = favOkButton = 
          new Fl_Return_Button( 181, 115, 79, 25, "Save");
        o->callback( (Fl_Callback*) cb_favOkButton);
        favOkButton->label( save_label);
      }
      {
        Fl_Box* o = new Fl_Box( 10, 115, 161, 25);
        Fl_Group::current()->resizable(o);
      }
      o->end();
    }

    // Load the 'Manage Favorites' label
    favWindow->label( manage_favorites_label);
    o->set_modal();
    o->size_range( 181, 150);
    o->end();
  }   // End of scope of the 'Manage Favorites Group'

  // Set size range for the main double window
  window->size_range( window->w(), window->h(), Fl::w(), Fl::h());

  // Initialize pointers to callback function, data, and directory
  callback_ = 0;
  data_ = 0;
  directory_[ 0] = 0;

  // Invoke member functions to intialize various state variables
  type( type_in);
  filter( filter_in);
  update_favorites();
  value( value_in);
  type( type_in);  // XXX PRG: Why is this here twice?
  isAscii_ = 1;
  writeSelectionInfo_ = 0;

  // Set up the preview state and box
  int iPreview;
  prefs_.get( "preview", iPreview, 1);
  preview( iPreview);
}

//*****************************************************************************
// Vp_File_Chooser::~Vp_File_Chooser() -- Destructor.  Remove timeout 
// handler associated with the preview control button and deallocate storage 
// for main window and favorites window.
Vp_File_Chooser::~Vp_File_Chooser()
{
  Fl::remove_timeout( (Fl_Timeout_Handler) previewCB, this);
  delete window;
  delete favWindow;
}

//*****************************************************************************
// Vp_File_Chooser::callback( (*pCallback)( *, *), *pData = 0) -- Set pointer 
// to the callback function and if specified, pass a pointer to the file data.
void Vp_File_Chooser::callback( 
  void (*pCallback)( Vp_File_Chooser*, void*), void *pData)
{
  callback_ = pCallback;
  data_ = pData;
}

//*****************************************************************************
// Vp_File_Chooser::color() -- Get color of the Fl_File_Browser object.
Fl_Color Vp_File_Chooser::color()
{
  return (fileBrowser->color());
}

//*****************************************************************************
// Vp_File_Chooser::color( file_browser_color_in) -- Set color of the 
// Fl_File_Browser object.
void Vp_File_Chooser::color( Fl_Color file_browser_color_in)
{
  fileBrowser->color( file_browser_color_in);
}

//*****************************************************************************
// Vp_File_Chooser::count() - Return the number of files that have been 
// selected.
int Vp_File_Chooser::count()
{
  // Get contents of the filename input field, check file browser usage type, 
  // and check to see if the file name input field is blank.
  const char *filename;
  filename = fileName->value();
  if( !( type_ & MULTI)) {
    if( !filename || !filename[0]) return 0;
    else return 1;
  }

  // Loop: Examine successive members of fileBrowser to see if they are files 
  // or directories and increment the file count if this is not a directory.
  int i, file_count;
  for( i = 1, file_count = 0; i <= fileBrowser->size(); i++) {
    if( fileBrowser->selected( i)) {
      filename = (char*) fileBrowser->text( i);
      if( filename[ strlen( filename) - 1] != '/') file_count++;
    }
  }

  // If a file count was found, return it, otherwise return 1 or 0 depending
  // on whether or not a filename was found.
  if( file_count) return file_count;
  else if( !filename || !filename[ 0]) return 0;
  else return 1;
}

//*****************************************************************************
// Vp_File_Chooser::directory() -- Get the current directory in the file 
// chooser.
char* Vp_File_Chooser::directory()
{
  return directory_;
}

//*****************************************************************************
// Vp_File_Chooser::directory( directory_in) -- Set the current directory in 
// the file chooser.
void Vp_File_Chooser::directory( const char *directory_in)
{
  // DIAGNOSTIC
  // cout << "Vp_File_Chooser::directory( \""
  //      << ( directory_in == NULL ? "(null)" : directory_in) 
  //      << "\")" << endl;

  // If no directory was specified, set it to the local directory
  if( directory_in == NULL) directory_in = ".";

  // For WIN32, see if the filename contains backslashes and if so, do a
  // cute loop through the string to convert backslashes to slashes.
  #ifdef WIN32
  char *slash;   // Pointer to slashes
  char fixed_pathname[ 1024];  // Pathname with slashes converted
  if( strchr( directory_in, '\\')) {
    strncpy( fixed_pathname, directory_in, sizeof( fixed_pathname));
    for( slash = strchr( fixed_pathname, '\\'); 
         slash; 
         slash = strchr(slash + 1, '\\'))
      *slash = '/';
    directory_in = fixed_pathname;
  }
  #endif   // WIN32

  // If no directory was specified, set the directory string to nothing, 
  // otherwise extract and set the directory string
  if( directory_in[ 0] == '\0') directory_[0] = '\0';
  else {

    // If necessary, make the directory absolute, otherwise just copy the
    // user-specified directory string
    #if( defined(WIN32) && ! defined(__CYGWIN__)) || defined(__EMX__)
    if( directory_in[ 0] != '/' && 
        directory_in[ 0] != '\\' && 
        directory_in[ 1] != ':')
    #else
    if( directory_in[ 0] != '/' && 
        directory_in[ 0] != '\\')
    #endif /* WIN32 || __EMX__ */
      fl_filename_absolute( directory_, directory_in);
    else
      strncpy( directory_, directory_in, sizeof( directory_));

    // Set a temporary directory pointer to the end of the current directory 
    // string and strip any trailing slash.
    char *dirptr;
    dirptr = directory_ + strlen( directory_) - 1;
    if( ( *dirptr == '/' || *dirptr == '\\') && dirptr > directory_)
      *dirptr = '\0';

    // Set the temporary directory pointer to two characters before the end of 
    // the current directory string.  If we have a trailing "..", strip the 
    // trailing path...
    dirptr = directory_ + strlen( directory_) - 3;
    if( dirptr >= directory_ && strcmp( dirptr, "/..") == 0) {
      *dirptr = '\0';
      while( dirptr > directory_) {
        if( *dirptr == '/') break;
        dirptr--;
      }
      if( dirptr >= directory_ && *dirptr == '/') *dirptr = '\0';
    } 

    // ...otherwise, strip any trailing "."
    else if( ( dirptr + 1) >= directory_ && strcmp( dirptr + 1, "/.") == 0) {
      dirptr[ 1] = '\0';
    }
  }

  // If necessary, rescan the directory.
  if( shown()) rescan();
}

//*****************************************************************************
// Vp_File_Chooser::filter() -- Get the file browser filter pattern(s).
const char* Vp_File_Chooser::filter()
{
  return (fileBrowser->filter());
}

//*****************************************************************************
// Vp_File_Chooser::filter( pattern_in) -- Set the filter pattern(s) for the 
// file browser.
void Vp_File_Chooser::filter( const char *pattern_in)
{
  // Make sure a filter pattern was specified
  if( !pattern_in || !*pattern_in) pattern_in = "*";

  // Copy the pattern string and clear the choice window
  char *pattern_copy;
  pattern_copy = strdup( pattern_in);
  showChoice->clear();

  // Loop: Extract and examine successive patterns until nothing
  // remains of the string.
  char *start, *end;
  int doallfiles = 0;
  for( start = pattern_copy; start && *start; start = end) {

    // Get pointer to the next tab and set terminator
    end = strchr( start, '\t');
    if( end) *end++ = '\0';

    // If pattern is "*", add label for all files and set flag, otherwise 
    // extract the pattern string and add it to the choice window
    if( strcmp( start, "*") == 0) {
      showChoice->add( all_files_label);
      doallfiles = 1;
    } 
    else {
      char temp[ 1024];
      quote_pathname( temp, start, sizeof( temp));
      showChoice->add( temp);
      if( strstr( start, "(*)") != NULL) doallfiles = 1;
    }
  }

  // Be sure to free the copy of the pattern string
  free( pattern_copy);

  // Add labels to the choice window, initialize its value, and invoke the
  // relevant method
  if (!doallfiles) showChoice->add( all_files_label);
  showChoice->add( custom_filter_label);
  showChoice->value( 0);
  showChoiceCB();
}

//*****************************************************************************
// Vp_File_Chooser::filter_value() -- Get the value (index) of the filter in 
// the choice window.
int Vp_File_Chooser::filter_value()
{
  return showChoice->value();
}

//*****************************************************************************
// Vp_File_Chooser::filter_value( index_in) -- Set the value (index) of the 
// filter in the choice window.
void Vp_File_Chooser::filter_value( int index_in)
{
  showChoice->value( index_in);
  showChoiceCB();
}

//*****************************************************************************
// Vp_File_Chooser::hide() -- Hide the main window.
void Vp_File_Chooser::hide()
{
  window->hide();
}

//*****************************************************************************
// Vp_File_Chooser::iconsize() -- Get the icon size for the file browser.
uchar Vp_File_Chooser::iconsize()
{
  return (fileBrowser->iconsize());
}

//*****************************************************************************
// Vp_File_Chooser::iconsize( sise_in) -- Set icon size for the file browser.
void Vp_File_Chooser::iconsize( uchar size_in)
{
  fileBrowser->iconsize( size_in);
}

//*****************************************************************************
// Vp_File_Chooser::label() -- Get the label of the main window.
const char* Vp_File_Chooser::label()
{
  return (window->label());
}

//*****************************************************************************
// Vp_File_Chooser::label( l) -- Set label of the main window.
void Vp_File_Chooser::label( const char *label_in)
{
  window->label( label_in);
}

//*****************************************************************************
// Vp_File_Chooser::ok_label() -- Get the label of the 'OK' button.
const char* Vp_File_Chooser::ok_label()
{
  return (okButton->label());
}

//*****************************************************************************
// Vp_File_Chooser::ok_label( label_in) -- Set label of the 'OK' button, 
// resize the button to fit next to the cancel button, and reinitialize sizes 
// in the parent group.
void Vp_File_Chooser::ok_label( const char *label_in)
{
  okButton->label( label_in);
  int w=0, h=0;
  okButton->measure_label( w, h);
  okButton->resize(
    cancelButton->x() - 50 - w, cancelButton->y(), w + 40, 25);
  okButton->parent()->init_sizes();
}

//*****************************************************************************
// Vp_File_Chooser::preview() - Enable or disable the preview checkbox.
void Vp_File_Chooser::preview( int iPreviewState)
{
  // Get and set value of the preview checkbox
  previewButton->value( iPreviewState);
  prefs_.set( "preview", iPreviewState);

  // If preview is enabled, resize the file browser and preview box, then
  // show and update the preview...
  Fl_Group *previewBoxParent = previewBox->parent();
  if( iPreviewState) {
    int w = previewBoxParent->w() * 2 / 3;
    fileBrowser->resize(
      fileBrowser->x(), fileBrowser->y(), 
      w, fileBrowser->h());
    previewBox->resize(
      fileBrowser->x()+w, previewBox->y(), 
      previewBoxParent->w()-w, previewBox->h());
    previewBox->show();
    update_preview();
  }

  // ...otherwise resize the file browser and preview box and hide the 
  // preview.
  else {
    fileBrowser->resize(
      fileBrowser->x(), fileBrowser->y(), 
      previewBoxParent->w(), fileBrowser->h());
    previewBox->resize(
      previewBoxParent->x()+previewBoxParent->w(), 
      previewBox->y(), 0, previewBox->h());
    previewBox->hide();
  }

  // Passs sizes and redraw parents
  previewBoxParent->init_sizes();
  fileBrowser->parent()->redraw();
}

//*****************************************************************************
// Vp_File_Chooser::rescan() - Rescan the current directory.
void Vp_File_Chooser::rescan()
{
  // Clear the current filename
  char pathname[ 1024];
  strncpy( pathname, directory_, sizeof( pathname));
  if( pathname[ 0] && pathname[ strlen( pathname) - 1] != '/') {
    strncat( pathname, "/", sizeof( pathname));
  }
  // cout << "Setting fileName in Vp_File_Chooser::rescan()";
  fileName->value( pathname);

  // Depending on whether or not this is a directory, activate or deactivate 
  // the 'OK button
  if( type_ & DIRECTORY) okButton->activate();
  else okButton->deactivate();

  // Build the file list and update the preview box.  NOTE: SORT is a static
  // variable that describes the type of sort to be performed.
  fileBrowser->load( directory_, sort);
  update_preview();
}

//*****************************************************************************
// Vp_File_Chooser::show() -- Show the main window.  Set the hotspot to the 
// file browser, show the main window, call Fl::flush() to redraw windows,
// set cursor information, rescan the directory, and wait for the user to do 
// something.
void Vp_File_Chooser::show()
{
  window->hotspot( fileBrowser);
  window->show();
  Fl::flush();
  fl_cursor( FL_CURSOR_WAIT);
  rescan();
  fl_cursor( FL_CURSOR_DEFAULT);
  fileName->take_focus();
}

//*****************************************************************************
// Vp_File_Chooser::shown() -- Get the show state of the main window
int Vp_File_Chooser::shown()
{
  return window->shown();
}

//*****************************************************************************
// Vp_File_Chooser::textcolor() -- Get the file browser text color.
Fl_Color Vp_File_Chooser::textcolor()
{
  return (fileBrowser->textcolor());
}

//*****************************************************************************
// Vp_File_Chooser::textcolor( file_browser_text_color_in) -- Set the file 
// browser text color
void Vp_File_Chooser::textcolor( Fl_Color file_browser_text_color_in)
{
  fileBrowser->textcolor( file_browser_text_color_in);
}

//*****************************************************************************
// Vp_File_Chooser::textfont() -- Get the file browser text font.
uchar Vp_File_Chooser::textfont()
{
  return (fileBrowser->textfont());
}

//*****************************************************************************
// Vp_File_Chooser::textfont( file_browswer_text_font_in) -- Set the file
// browser text font.
void Vp_File_Chooser::textfont( uchar file_browser_text_font_in)
{
  fileBrowser->textfont( file_browser_text_font_in);
}

//*****************************************************************************
// Vp_File_Chooser::textsize() -- Get the file browser text size.
uchar Vp_File_Chooser::textsize()
{
  return (fileBrowser->textsize());
}

//*****************************************************************************
// Vp_File_Chooser::textsize( file_browser_text_size_in) -- Set the file
// browser text font.
void Vp_File_Chooser::textsize( uchar file_browser_text_size_in)
{
  fileBrowser->textsize( file_browser_text_size_in);
}

//*****************************************************************************
// Vp_File_Chooser::type() -- Get the type of usage for the file browser.
int Vp_File_Chooser::type()
{
  return (type_);
}

//*****************************************************************************
// Vp_File_Chooser::type( type_in) -- Set the type of usage for the file 
// browser, pass this information to the browser itself, and activate or 
// deactivate the necessary buttons.
void Vp_File_Chooser::type( int type_in)
{
  type_ = type_in;
  if( type_in & MULTI) fileBrowser->type( FL_MULTI_BROWSER);
  else fileBrowser->type( FL_HOLD_BROWSER);

  if( type_in & CREATE) newButton->activate();
  else newButton->deactivate();

  if( type_in & DIRECTORY) 
    fileBrowser->filetype( Fl_File_Browser::DIRECTORIES);
  else fileBrowser->filetype( Fl_File_Browser::FILES);
}

//*****************************************************************************
// Vp_File_Chooser::user_data() -- Get pointer to the file data
void* Vp_File_Chooser::user_data() const 
{
  return (data_);
}

//*****************************************************************************
// Vp_File_Chooser::user_data( d) -- Set pointer to the file data
void Vp_File_Chooser::user_data( void *pData)
{
  data_ = pData;
}

//*****************************************************************************
// Vp_File_Chooser::value( index) -- Return the filename for this index.
const char* Vp_File_Chooser::value( int index)
{
  // Get the current filename and return if it's missing or we're done
  const char *name;
  name = fileName->value();
  if( !( type_ & MULTI)) {
    if( !name || !name[ 0]) return NULL;
    else return name;
  }

  // Loop: Examine succesive elements in the file browser to find the file
  // for this index and load it into the pathname (directory+filename)
  static char pathname[ 1024];
  int file_count = 0;
  for( int i = 1; i <= fileBrowser->size(); i++) {

    // See if this file is a selected file/directory...
    if( fileBrowser->selected( i)) {
      name = fileBrowser->text( i);
      file_count++;

      if( file_count == index) {
        if( directory_[ 0]) {
          snprintf( pathname, sizeof( pathname), "%s/%s", directory_, name);
        } 
        // else strlcpy( pathname, name, sizeof(pathname));
        else strncpy( pathname, name, sizeof(pathname));
      }
      return pathname;
    }
  }

  // If nothing was selected, return the contents of the filename field.
  if( !name || !name[ 0]) return NULL;
  else return name;
}

//*****************************************************************************
// Vp_File_Chooser::value( filename_in) -- Set the current filename.
void Vp_File_Chooser::value( const char *filename_in)
{
  // DIAGNOSTIC
  // cout << "Vp_File_Chooser::value( \""
  //      << ( filename_in == NULL ? "(null)" : filename_in) 
  //      << "\")" << endl;

  // If the filename is NULL or empty, change the current directory to the
  // "My System" directory.
  if( filename_in == NULL || !filename_in[ 0]) {
    directory( filename_in);
    fileName->value( "");
    okButton->deactivate();
    return;
  }

  // If necessary, do a cute loop to convert backslashes to slashes...
  char *slash;  // Directory separator
  #ifdef WIN32
  char fixed_pathname[ 1024];
  if( strchr( filename_in, '\\')) {
    strncpy( fixed_pathname, filename_in, sizeof( fixed_pathname));
    for( slash = strchr( fixed_pathname, '\\'); 
         slash; 
         slash = strchr(slash + 1, '\\'))
      *slash = '/';
    filename_in = fixed_pathname;
  }
  #endif   // WIN32

  // Make a local copy of the pathname.  If it is a directory, change the 
  // display to that directory, otherwise change to current directory
  char pathname[ 1024];
  fl_filename_absolute( pathname, sizeof( pathname), filename_in);
  if( ( slash = strrchr( pathname, '/')) != NULL) {
    if( !new_filename_isdir( pathname)) *slash++ = '\0';
    directory( pathname);
    if( *slash == '/') slash = pathname;
  }
  else {
    directory( ".");
    slash = pathname;
  }

  // Set the input field to the absolute path and activate buttons
  if( slash > pathname) slash[-1] = '/';
  fileName->value( pathname);
  fileName->position( 0, strlen( pathname));
  okButton->activate();

  // Then find the file in the file list and select it...
  int fcount = fileBrowser->size();
  fileBrowser->deselect(0);
  fileBrowser->redraw();

  // Loop: Examine successive files
  for( int i = 1; i <= fcount; i ++) {
    #if defined(WIN32) || defined(__EMX__)
    if( strcasecmp( fileBrowser->text( i), slash) == 0) {
    #else
    if( strcmp( fileBrowser->text( i), slash) == 0) {
    #endif   // WIN32 || __EMX__

      // cout << "Selecting line " << i << "..." << endl;
      fileBrowser->topline(i);
      fileBrowser->select(i);
      break;
    }
  }
}

//*****************************************************************************
// Vp_File_Chooser::visible() -- Get visibility state of the main window.
int Vp_File_Chooser::visible()
{
  return window->visible();
}

//*****************************************************************************
// Vp_File_Chooser::favoritesCB( *pWidget) -- Master dialog handle to handle
// all dialogs associated with the file browser window used as a favorites
// button and pull-down menu.
void Vp_File_Chooser::favoritesCB( Fl_Widget *pWidget)
{
  // Define buffers to hold preference name and pathname
  char name[ 32];
  char pathname[ 1024];

  // If widget is NULL, load the favorites list...
  if( !pWidget) {
    favList->clear();
    favList->deselect();

    // Loop: Get favorite directory 0 to 99...
    for( int i = 0; i < 100; i ++) {
      sprintf( name, "favorite%02d", i);
      prefs_.get( name, pathname, "", sizeof( pathname));

      // Stop on the first empty favorite
      if( !pathname[0]) break;

      // Add the favorite to the list...
      favList->add(
        pathname,
        Fl_File_Icon::find( pathname, Fl_File_Icon::DIRECTORY));
    }

    // Deactivate various buttons
    favUpButton->deactivate();
    favDeleteButton->deactivate();
    favDownButton->deactivate();
    favOkButton->deactivate();

    // Set and show hotspot?
    favWindow->hotspot(favList);
    favWindow->show();
  } 

  // ..or examine the favorites list
  else if( pWidget == favList) {
    int i = favList->value();
    
    // Activate and deactivate the relevant buttons
    if( i) {
      if( i > 1) favUpButton->activate();
      else favUpButton->deactivate();
      favDeleteButton->activate();
      if (i < favList->size()) favDownButton->activate();
      else favDownButton->deactivate();
    } 
    else {
      favUpButton->deactivate();
      favDeleteButton->deactivate();
      favDownButton->deactivate();
    }
  } 
  
  // .. or process the Favorites Up button...
  else if( pWidget == favUpButton) {
    int i = favList->value();

    // Update favorites list
    favList->insert( i - 1, favList->text(i), favList->data(i));
    favList->remove( i + 1);
    favList->select( i - 1);

    // Activate and deactivate the relevant buttons
    if( i == 2) favUpButton->deactivate();
    favDownButton->activate();
    favOkButton->activate();
  } 
  
  // .. or process the Favorites Delete button...
  else if( pWidget == favDeleteButton) {
    int i = favList->value();

    // Update the favorites list
    favList->remove(i);
    if( i > favList->size()) i--;
    favList->select( i);

    // Activate and deactivate the relevant buttons
    if( i < favList->size()) favDownButton->activate();
    else favDownButton->deactivate();
    if( i > 1) favUpButton->activate();
    else favUpButton->deactivate();
    if( !i) favDeleteButton->deactivate();
    favOkButton->activate();
  } 
  
  // .. or process the Favorites Down button...
  else if( pWidget == favDownButton) {
    int i = favList->value();

    // Update the favorites list
    favList->insert( i + 2, favList->text(i), favList->data(i));
    favList->remove( i);
    favList->select( i + 1);

    // Activate and deactivate the relevant buttons
    if ( (i + 1) == favList->size()) favDownButton->deactivate();
    favUpButton->activate();
    favOkButton->activate();
  } 
  
  // .. or process the Favorites OK button...
  else if( pWidget == favOkButton) {

    // Loop: Copy the new list over and set favorite directory 0 to 99.
    int i;
    for( i = 0; i < favList->size(); i ++) {
      sprintf(name, "favorite%02d", i);
      prefs_.set(name, favList->text( i + 1));
    }

    // Loop: Clear favorite directory 0 to 99 to clear old entries as 
    // necessary.  NOTE: index 'i' is retained from the previous loop!
    for( ; i < 100; i ++) {
      sprintf( name, "favorite%02d", i);
      prefs_.get( name, pathname, "", sizeof( pathname));
      if( pathname[0]) prefs_.set(name, "");
      else break;
    }

    // Update favorites and hide window
    update_favorites();
    favWindow->hide();
  }
}

//*****************************************************************************
// Vp_File_Chooser::previewCB() - Timeout handler for the preview box.
void Vp_File_Chooser::previewCB( Vp_File_Chooser *fc)
{
  fc->update_preview();
}

//*****************************************************************************
// Vp_File_Chooser::update_favorites() -- Update the favorites button/menu.
void Vp_File_Chooser::update_favorites()
{
  // Set up the Favorites button/menue
  favoritesButton->clear();
  favoritesButton->add( "bla");
  favoritesButton->clear();
  favoritesButton->add( add_favorites_label, FL_ALT + 'a', 0);
  favoritesButton->add(
    manage_favorites_label, FL_ALT + 'm', 0, 0, FL_MENU_DIVIDER);
  favoritesButton->add( filesystems_label, FL_ALT + 'f', 0);

  // Get and display the home directory as a name in the favorites menu
  char menuname[ 2048];
  const char *home;
  if( ( home = getenv( "HOME")) != NULL) {
    quote_pathname( menuname, home, sizeof(menuname));
    favoritesButton->add( menuname, FL_ALT + 'h', 0);
  }

  // Loop: Examine successive elements of the favorites menu.  NOTE: The index,
  // i, is retained for use later.  (To deactivate the menu if it's too long?)
  char pathname[ 1024];
  int i = 0;
  for( i = 0; i < 100; i ++) {
    sprintf( menuname, "favorite%02d", i);
    prefs_.get( menuname, pathname, "", sizeof( pathname));
    if ( !pathname[ 0]) break;

    quote_pathname( menuname, pathname, sizeof( menuname));

    if( i < 10) favoritesButton->add( menuname, FL_ALT + '0' + i, 0);
    else favoritesButton->add( menuname);
  }
  if( i == 100) ( (Fl_Menu_Item*) favoritesButton->menu())[0].deactivate();
}

//*****************************************************************************
// Vp_File_Chooser::update_preview() -- Update the preview box
void Vp_File_Chooser::update_preview()
{
  // If button is unchecked then quit
  if( !previewButton->value()) return;

  // Get filename and set new image
  const char *filename;
  Fl_Shared_Image *image;
  if( ( filename = value()) == NULL || new_filename_isdir( filename)) 
    image = NULL;
  else {
    window->cursor( FL_CURSOR_WAIT);
    Fl::check();
    image = Fl_Shared_Image::get( filename);
    if( image) {
      window->cursor( FL_CURSOR_DEFAULT);
      Fl::check();
    }
  }

  // Set and release the old image
  Fl_Shared_Image *oldimage;
  oldimage = (Fl_Shared_Image*) previewBox->image();
  if( oldimage) oldimage->release();

  // Initialize the preview box?
  previewBox->image( 0);

  // If no image exists then try to get one...
  if( !image) {

    // Try to open file
    FILE *fp;
    if( filename) fp = fopen( filename, "rb");
    else fp = NULL;

    // Try reading the first 1k of data for a label...
    int bytes;
    if( fp != NULL) {
      bytes = fread( preview_text_, 1, sizeof( preview_text_) - 1, fp);
      preview_text_[ bytes] = '\0';
      fclose( fp);
    } 

    // ...or assume we can't read any data.
    else {
      preview_text_[ 0] = '\0';
    }

    // Set cursor information and check to see if anything happened
    window->cursor( FL_CURSOR_DEFAULT);
    Fl::check();

    // Loop: Set a pointer to the preview text string and loop to examine
    // it for for printable characters.  NOTE: The relevant pointer, PTR, is
    // retained for use later.
    char *ptr;
    for( ptr = preview_text_;
         *ptr && (isprint(*ptr & 255) || isspace(*ptr & 255));
         ptr ++);

    // If this is a non-printable file, just show a big ?...
    if( *ptr || ptr == preview_text_) {
      previewBox->label(filename ? "?" : 0);
      previewBox->align(FL_ALIGN_CLIP);
      previewBox->labelsize(100);
      previewBox->labelfont(FL_HELVETICA);
    } 
    
    // ...or get the height of the preview box, use this to determine a font
    // size, and show the first 1k of text.
    else {
      int size = previewBox->h() / 20;
      if( size < 6) size = 6;
      else if( size > 14) size = 14;

      previewBox->label( preview_text_);
      previewBox->align(
        (Fl_Align) 
          (FL_ALIGN_CLIP | FL_ALIGN_INSIDE | FL_ALIGN_LEFT | FL_ALIGN_TOP));
      previewBox->labelsize( (uchar) size);
      previewBox->labelfont( FL_COURIER);
    }
  } 

  // ...otherwise use the image
  else {
    int pbw = previewBox->w() - 20;
    int pbh = previewBox->h() - 20;

    if( image->w() > pbw || image->h() > pbh) {
      int w = pbw;
      int h = w * image->h() / image->w();
      if (h > pbh) {
        h = pbh;
        w = h * image->w() / image->h();
      }

      oldimage = (Fl_Shared_Image *) image->copy(w, h);
      previewBox->image( (Fl_Image *) oldimage);
      image->release();
    } 
    else {
      previewBox->image((Fl_Image *)image);
    }

    previewBox->align(FL_ALIGN_CLIP);
    previewBox->label(0);
  }

  // Make sure the preview box gets redrawn
  previewBox->redraw();
}

//*****************************************************************************
// Vp_File_Chooser:::cb_favList( o, v) -- Wrapper for callback method for the 
// favorites list
void Vp_File_Chooser::cb_favList(Fl_File_Browser* o, void* v)
{
  ( (Vp_File_Chooser*) (o->parent()->user_data()))->cb_favList_i( o, v);
}

//*****************************************************************************
// Vp_File_Chooser:::cb_favList_i( *, *) -- Callback method for the favorites 
// list.  Invokes favoritesCB
void Vp_File_Chooser::cb_favList_i( Fl_File_Browser*, void*)
{
  favoritesCB( favList);
}

//*****************************************************************************
// Vp_File_Chooser::cb_fileBrowser( o, v) -- Wrapper for the callback method 
// for the file browswer.
void Vp_File_Chooser::cb_fileBrowser( Fl_File_Browser* o, void* v)
{
  ( (Vp_File_Chooser*) 
      (o->parent()->parent()->user_data()))->cb_fileBrowser_i( o, v);
}

//*****************************************************************************
// Vp_File_Chooser::cb_fileBrowser_i( *, *) -- Callback method for the file
// browswer.  Invokes fileBroswerCB.
void Vp_File_Chooser::cb_fileBrowser_i( Fl_File_Browser*, void*)
{
  fileBrowserCB();
}

//*****************************************************************************
// Vp_File_Chooser::fileBrowserCB() - Handle clicks and double-clicks in the 
// Fl_File_Browser.
void Vp_File_Chooser::fileBrowserCB()
{
  // Set filename and return if it's empty
  char *filename;
  filename = (char*) fileBrowser->text(fileBrowser->value());
  if( !filename) return;

  // Examine name of current directory to extract, reformat, and load it into 
  // the full pathname
  char pathname[ 1024];
  if( !directory_[0]) {
    // strlcpy( pathname, filename, sizeof(pathname));
    strncpy( pathname, filename, sizeof(pathname));
  } 
  else if( strcmp(directory_, "/") == 0) {
    snprintf( pathname, sizeof(pathname), "/%s", filename);
  }
  else {
    snprintf( pathname, sizeof(pathname), "%s/%s", directory_, filename);
  }

  // Check for event clicks...
  if( Fl::event_clicks()) {

    // Deal with OS-specific formats, change directories, and reset the 
    // click count so that a click in the same spot won't be treated as a
    // triple-click.  Use a value of -1 because the next click will 
    // increment click count to 0, which is what we really want...
    #if (defined(WIN32) && ! defined(__CYGWIN__)) || defined(__EMX__)
    if( ( strlen(pathname) == 2 && pathname[1] == ':') ||
      // fl_filename_isdir( pathname)) {
      new_filename_isdir( pathname)) {
    #else
    // if( fl_filename_isdir(pathname)) {
    if( new_filename_isdir(pathname)) {
    #endif   // WIN32 || __EMX__
      directory( pathname);
      Fl::event_clicks( -1);
    }

    // ...or hide the window because file has been picked
    else {
      window->hide();
    }
  }

  // ...or if no event clicks, check if the user clicks on a directory when
  // picking files, and if so, make sure only that item is selected.
  else
  {
    filename = pathname + strlen(pathname) - 1;
    if( (type_ & MULTI) && !(type_ & DIRECTORY)) {

      // Clicked on a directory, deselect everything else...
      if( *filename == '/') {
        int i = fileBrowser->value();
        fileBrowser->deselect();
        fileBrowser->select(i);
      }

      // ...or clicked on a file so loop through fileBrowser to see if there 
      // are other directories selected, then set values and selections.
      // NOTE: Value of index 'i' is kept after loop.
      else {
        int i;
        const char *temp;
        for( i = 1; i <= fileBrowser->size(); i ++) {
          if( i != fileBrowser->value() && fileBrowser->selected(i)) {
            temp = fileBrowser->text(i);
            temp += strlen(temp) - 1;
            if( *temp == '/') break;  // Yes, selected directory
          }
        }
        if( i <= fileBrowser->size()) {
          i = fileBrowser->value();
        fileBrowser->deselect();
          fileBrowser->select( i);
        }
      }
    }

    // Strip any trailing slash from the directory name...
    if( *filename == '/') *filename = '\0';

    // Set pathname
    // puts("Setting fileName from fileBrowserCB...");
    fileName->value( pathname);

    // Update the preview box...
    Fl::remove_timeout( (Fl_Timeout_Handler) previewCB, this);
    Fl::add_timeout( 1.0, (Fl_Timeout_Handler) previewCB, this);

    // Do any callback that is registered...
    if( callback_) (*callback_)( this, data_);

    // Activate the OK button as needed...
    // if( !fl_filename_isdir( pathname) || ( type_ & DIRECTORY))
    if( !new_filename_isdir( pathname) || ( type_ & DIRECTORY))
      okButton->activate();
    else
      okButton->deactivate();
  }
}

//*****************************************************************************
// Vp_File_Chooser::cb_fileName( o, v) -- Wrapper for the callback method for 
// the file name field.
void Vp_File_Chooser::cb_fileName( Fl_File_Input* o, void* v) {
  ( (Vp_File_Chooser*) 
      (o->parent()->parent()->user_data()))->cb_fileName_i( o, v);
}

//*****************************************************************************
// Vp_File_Chooser::cb_fileName_i( *, *) -- Callback method for the file name
// field.  Invokes filenameCB to handle the dialog.
void Vp_File_Chooser::cb_fileName_i( Fl_File_Input*, void*)
{
  fileNameCB();
}

//*****************************************************************************
// Vp_File_Chooser::fileNameCB() - Handle dialog for the file name field.
void Vp_File_Chooser::fileNameCB()
{
  char *slash; // Pointer to trailing slash
  char pathname[ 1024];  // Full pathname to file
  char matchname[ 256];  // Matching filename
  
  int min_match;   // Minimum number of matching chars
  int max_match;   // Maximum number of matching chars
  int num_files;   // Number of files in directory
  int first_line;   // First matching line
  // const char *file;   // File from directory

  // Diagnostic
  // cout << "fileNameCB()";

  // Get the filename from the text field.  If none was specified,
  // deactivate button and return
  char *filename;
  filename = (char*) fileName->value();
  if( !filename || !filename[ 0]) {
    okButton->deactivate();
    return;
  }

  // Expand ~ and $ variables as needed...
  if( strchr(filename, '~') || strchr(filename, '$')) {
    fl_filename_expand( pathname, sizeof( pathname), filename);
    filename = pathname;
    value( pathname);
  }

  // If current directory is defined, make sure we have an absolute path...
  #if (defined(WIN32) && !defined(__CYGWIN__)) || defined(__EMX__)
  if( directory_[0] != '\0' && filename[0] != '/' &&
      filename[0] != '\\' &&
      !(isalpha(filename[0] & 255) && (!filename[1] || filename[1] == ':'))) {
  #else
  if( directory_[0] != '\0' && filename[0] != '/') {
  #endif   // WIN32 || __EMX__
    fl_filename_absolute( pathname, sizeof( pathname), filename);
    value( pathname);
    fileName->mark( fileName->position()); // no selection after expansion
  } 

  // ...or set pathname to make sure we have a writable copy...
  else if( filename != pathname) {
    // strlcpy( pathname, filename, sizeof(pathname));
    strncpy( pathname, filename, sizeof(pathname));
  }
  filename = pathname;

  // Process things according to the key pressed.
  // Process Fl_Enter or FL_KP_Enter...
  if( Fl::event_key() == FL_Enter || Fl::event_key() == FL_KP_Enter) {

    // Enter pressed - select or change directory...
    #if (defined(WIN32) && ! defined(__CYGWIN__)) || defined(__EMX__)
    if( ( isalpha( pathname[0] & 255) && pathname[1] == ':' && !pathname[2]) ||
      // fl_filename_isdir( pathname) &&
      new_filename_isdir( pathname) &&
      compare_dirnames( pathname, directory_)) {
    #else
    // if( fl_filename_isdir( pathname) &&
    if( new_filename_isdir( pathname) &&
        compare_dirnames(pathname, directory_)) {
    #endif   // WIN32 || __EMX__
      directory( pathname);
    } 

    // ...or update the preview box, o any callback that is registered, and
    // hide the window to signal things are done...
    else if( ( type_ & CREATE) || access( pathname, 0) == 0) {
      // if( !fl_filename_isdir( pathname) || ( type_ & DIRECTORY)) {
      if( !new_filename_isdir( pathname) || ( type_ & DIRECTORY)) {
        update_preview();
        if( callback_) (*callback_)( this, data_);
        window->hide();
      }
    } 
    
    // ..or file doesn't exist, so beep at and alert the user...
    else {
      fl_alert(existing_file_label);
    }
  }   // Fl_Enter or FL_KP_Enter...

  // ...or process !FL_Delete and !FL_BackSpace
  else if( Fl::event_key() != FL_Delete &&
           Fl::event_key() != FL_BackSpace) {

    // Check to see if the user has entered a directory.
    if( ( slash = strrchr( pathname, '/')) == NULL)
      slash = strrchr( pathname, '\\');
    if( !slash) return;

    // If they have, change directories if necessary...
    *slash++ = '\0';
    filename = slash;

    #if defined(WIN32) || defined(__EMX__)
    if( strcasecmp( pathname, directory_) &&
        ( pathname[0] || strcasecmp("/", directory_))) {
    #else
    if( strcmp(pathname, directory_) &&
        ( pathname[0] || strcasecmp("/", directory_))) {
    #endif   // WIN32 || __EMX__
      int p = fileName->position();
      int m = fileName->mark();
      directory(pathname);

      // Add file name if present
      if( filename[ 0]) {
        char tempname[1024];
        snprintf( tempname, sizeof(tempname), "%s/%s", directory_, filename);
        fileName->value(tempname);
        // strlcpy( pathname, tempname, sizeof(pathname));
        strncpy( pathname, tempname, sizeof(pathname));
      }
      fileName->position(p, m);
    }

    // Other key pressed - do filename completion as possible...
    num_files  = fileBrowser->size();
    min_match  = strlen(filename);
    max_match  = min_match + 1;
    first_line = 0;

    // Loop: Examine successive files to pick the best match
    for( int i = 1; i <= num_files && max_match > min_match; i ++) {
      const char *file;
      file = fileBrowser->text(i);

      // If match is better than MIN_MATCH, examine it
      #if (defined(WIN32) && ! defined(__CYGWIN__)) || defined(__EMX__)
      if( strncasecmp( filename, file, min_match) == 0) {
      #else
      if( strncmp( filename, file, min_match) == 0) {
      #endif   // WIN32 || __EMX__
 
        // If first line is empty, copy stuff over
        if( !first_line) {
          // strlcpy( matchname, file, sizeof(matchname));
          strncpy( matchname, file, sizeof(matchname));
          max_match = strlen(matchname);

          // Strip trailing /, if any...
          if( matchname[ max_match - 1] == '/') {
            max_match --;
          matchname[max_match] = '\0';
          }

          // And then make sure that the item is visible
          fileBrowser->topline(i);
          first_line = i;
        }

        // ...otherwise, compare succesize matches to find the maximum 
        // string match.
        else {
          while( max_match > min_match)
            #if (defined(WIN32) && ! defined(__CYGWIN__)) || defined(__EMX__)
            if( strncasecmp( file, matchname, max_match) == 0)
            #else
          if( strncmp(file, matchname, max_match) == 0)
            #endif   // WIN32 || __EMX__
              break;
            else max_match --;

          // Truncate the string as needed...
          matchname[ max_match] = '\0';
        }
      }   // if( match better than MIN_MATCH) loop
    }   // for( int i...) loop

    // Add any matches to the input field...
    // ...if this is the only possible match...
    if( first_line > 0 && min_match == max_match &&
        max_match == (int) strlen( fileBrowser->text( first_line))) {
      fileBrowser->deselect(0);
      fileBrowser->select(first_line);
      fileBrowser->redraw();
    } 

    // ...or sdd the matching portion and highlight it with the cursor at the 
    // end of the selection so one can press the right arrow to accept the 
    // selection.  (Tab and End also do this for both cases.)
    else if( max_match > min_match && first_line) {
      fileName->replace(
        filename - pathname, filename - pathname + min_match, matchname);
      fileName->position(
        filename - pathname + max_match, filename - pathname + min_match);

    //  ...or just turn things off
    } else if( max_match == 0) {
      fileBrowser->deselect(0);
      fileBrowser->redraw();
    }

    // As necesssary, enable or deactivate the OK button...
    // if( ( ( type_ & CREATE) || !access( fileName->value(), 0)) &&
    //     ( !fl_filename_isdir( fileName->value()) || (type_ & DIRECTORY))) {
    if( ( ( type_ & CREATE) || !access( fileName->value(), 0)) &&
        ( !new_filename_isdir( fileName->value()) || (type_ & DIRECTORY))) {
      okButton->activate();
    }
    else {
      okButton->deactivate();
    }
  }   // !FL_Delete and !FL_BackSpace

  // ...or process FL_Delete or FL_BackSpace
  else {
    fileBrowser->deselect(0);
    fileBrowser->redraw();
    // if( ( ( type_ & CREATE) || !access(fileName->value(), 0)) &&
    //     ( !fl_filename_isdir(fileName->value()) || (type_ & DIRECTORY))) {
    if( ( ( type_ & CREATE) || !access(fileName->value(), 0)) &&
        ( !new_filename_isdir(fileName->value()) || (type_ & DIRECTORY))) {
      okButton->activate();
    } 
    else {
      okButton->deactivate();
    }
  }   // FL_Delete or FL_BackSpace
}

//*****************************************************************************
// Vp_File_Chooser::cb_preview( o, v) -- Wrapper for the callback method for
// the preview window.
void Vp_File_Chooser::cb_preview( Fl_Tile* o, void* v)
{
  ( (Vp_File_Chooser*) (o->parent()->user_data()))->cb_preview_i( o, v);
}

//*****************************************************************************
// Vp_File_Chooser::cb_preview_i( *, *) -- Callback method for the preview
// window.  Invokes update_preview() to handle the dialog.
void Vp_File_Chooser::cb_preview_i( Fl_Tile*, void*)
{
  update_preview();
}

//*****************************************************************************
// Vp_File_Chooser::cb_fileType( o, v) -- Wrapper for the callback 
// method for the file type choice button
void Vp_File_Chooser::cb_fileType( Fl_Choice* o, void* v)
{
  ( (Vp_File_Chooser*) 
      (o->parent()->parent()->user_data()))->cb_fileType_i( o,v);
}

//*****************************************************************************
// Vp_File_Chooser::cb_fileType_i( *, *) -- Callback method for the 
// file type choice button.  Invokes fileTypeCB() to handle the dialog.
void Vp_File_Chooser::cb_fileType_i( Fl_Choice*, void*)
{
  fileTypeCB();
}

//*****************************************************************************
// Vp_File_Chooser::fileTypeCB() -- Handle file type choice selections.
void Vp_File_Chooser::fileTypeCB()
{
  // Get the selected item
  const char *item;
  item = fileType->text( fileType->value());
  
  // Get default patterns
  if( strcmp( item, "ASCII") == 0) {
    strcpy( pattern_, "*.{txt,lis,asc}\tAll Files (*)");
    fileBrowser->filter( "*.{txt,lis,asc}");
    isAscii_ = 1;
  }
  else {
    strcpy( pattern_, "*.bin\tAll Files (*)");
    fileBrowser->filter( "*.bin");
    isAscii_ = 0;
  }

  // Set the pattern
  filter( pattern_);
  
  // If necessary, rescan the directory.
  if( shown()) rescan();
}

//*****************************************************************************
// Vp_File_Chooser::cb_showChoice( o, v) -- Wrapper for the callback method
// for the show choice button
void Vp_File_Chooser::cb_showChoice( Fl_Choice* o, void* v)
{
  ( (Vp_File_Chooser*) 
      (o->parent()->parent()->user_data()))->cb_showChoice_i( o,v);
}

//*****************************************************************************
// Vp_File_Chooser::cb_showChoice_i( *, *) -- Callback method for the show 
// choice button.  Invokes showChoiceCB() to handle the dialog.
void Vp_File_Chooser::cb_showChoice_i( Fl_Choice*, void*)
{
  showChoiceCB();
}

//*****************************************************************************
// Vp_File_Chooser::showChoiceCB() -- Handle show choice selections.
void Vp_File_Chooser::showChoiceCB()
{
  // Get the selected item
  const char *item;
  item = showChoice->text( showChoice->value());

  // Define buffers used to compare the item with the pattern and
  // perform the comparison
  const char *patstart;
  char *patend;
  char temp[ 1024];
  if( strcmp( item, custom_filter_label) == 0) {
    if( ( item = fl_input( custom_filter_label, pattern_)) != NULL) {
      strncpy( pattern_, item, sizeof(pattern_));

      quote_pathname( temp, item, sizeof(temp));
      showChoice->add( temp);
      showChoice->value( showChoice->size() - 2);
    }
  } 
  else if( ( patstart = strchr( item, '(')) == NULL) {
    strncpy( pattern_, item, sizeof(pattern_));
  } 
  else {
    strncpy( pattern_, patstart + 1, sizeof( pattern_));
    if( ( patend = strrchr( pattern_, ')')) != NULL) *patend = '\0';
  }

  // Load the pattern into the file browser
  fileBrowser->filter( pattern_);

  // If necessary, rescan the directory.
  if( shown()) {
    rescan();
  }
}

//*****************************************************************************
// Vp_File_Chooser::cb_window( o, v) -- Wrapper for the callback method for
// the main (double) window.
void Vp_File_Chooser::cb_window( Fl_Double_Window* o, void* v)
{
  ( (Vp_File_Chooser*) (o->user_data()))->cb_window_i( o, v);
}

//*****************************************************************************
// Vp_File_Chooser::cb_window_i( *, *) -- Callback method for the main 
// (double) window.
void Vp_File_Chooser::cb_window_i( Fl_Double_Window*, void*)
{
  fileName->value( "");
  fileBrowser->deselect();
  Fl::remove_timeout( (Fl_Timeout_Handler) previewCB, this);
  window->hide();
}

//*****************************************************************************
// Vp_File_Chooser:::cb_cancelButton( o, v) -- Wrapper for the callback
// method for the cancel button.
void Vp_File_Chooser::cb_cancelButton( Fl_Button* o, void* v)
{
  ( (Vp_File_Chooser*) 
    (o->parent()->parent()->parent()->user_data()))->cb_cancelButton_i( o, v);
}

//*****************************************************************************
// Vp_File_Chooser::cb_cancelButton_i( *, *) -- Callback method for the 
// cancel button.
void Vp_File_Chooser::cb_cancelButton_i( Fl_Button*, void*)
{
  fileName->value( "");
  fileBrowser->deselect();
  Fl::remove_timeout( (Fl_Timeout_Handler) previewCB, this);
  window->hide();
}

//*****************************************************************************
// Vp_File_Chooser:::cb_favCancelButton( o, v) -- Wrapper for the callback
// method to for the 'Favorites Cancel' button.
void Vp_File_Chooser::cb_favCancelButton( Fl_Button* o, void* v)
{
  ( (Vp_File_Chooser*) 
    (o->parent()->parent()->user_data()))->cb_favCancelButton_i( o, v);
}

//*****************************************************************************
// Vp_File_Chooser:::cb_favCancelButton_i( *, *) -- Callback method for the 
// 'Favorites Cancel' button.
void Vp_File_Chooser::cb_favCancelButton_i( Fl_Button*, void*)
{
  favWindow->hide();
}

//*****************************************************************************
// Vp_File_Chooser:::cb_favDeleteButton( o, v) -- Wrapper for the callback
// method to for the 'Favorites Delete' button.
void Vp_File_Chooser::cb_favDeleteButton( Fl_Button* o, void* v)
{
  ( (Vp_File_Chooser*) 
    (o->parent()->parent()->user_data()))->cb_favDeleteButton_i( o, v);
}

//*****************************************************************************
// Vp_File_Chooser:::cb_favDeleteButton_i( *, *) -- Callback method for the 
// 'Favorites Delete' button.  Invokes favoritesCB to handle the dialog.
void Vp_File_Chooser::cb_favDeleteButton_i( Fl_Button*, void*)
{
  favoritesCB( favDeleteButton);
}

//*****************************************************************************
// Vp_File_Chooser:::cb_favDownButton( o, v) -- Wrapper for the callback
// method to for the 'Favorites Down' button.
void Vp_File_Chooser::cb_favDownButton(Fl_Button* o, void* v) 
{
  ( (Vp_File_Chooser*) 
    (o->parent()->parent()->user_data()))->cb_favDownButton_i( o, v);
}

//*****************************************************************************
// Vp_File_Chooser:::cb_favDownButton_i( *. *) -- Callback method for the 
// 'Favorites Down' button.  Invokes favoritesCB to handle the dialog.
void Vp_File_Chooser::cb_favDownButton_i( Fl_Button*, void*)
{
  favoritesCB( favDownButton);
}

//*****************************************************************************
// Vp_File_Chooser:::cb_favOkButton( o, v) -- Wrapper for the callback
// method to for the 'Favorites OK' button.
void Vp_File_Chooser::cb_favOkButton( Fl_Return_Button* o, void* v)
{
  ( (Vp_File_Chooser*) 
    (o->parent()->parent()->user_data()))->cb_favOkButton_i( o, v);
}

//*****************************************************************************
// Vp_File_Chooser:::cb_favOkButton_i( *, *) -- Callback method for the 
// 'Favorites OK' button.  Invokes favoritesCB to handle the dialog.
void Vp_File_Chooser::cb_favOkButton_i( Fl_Return_Button*, void*)
{
  favoritesCB( favOkButton);
}

//*****************************************************************************
// Vp_File_Chooser:::cb_favUpButton( o, v) -- Wrapper for the callback
// method to for the 'Favorites Up' button.
void Vp_File_Chooser::cb_favUpButton( Fl_Button* o, void* v)
{
  ( (Vp_File_Chooser*) 
    (o->parent()->parent()->user_data()))->cb_favUpButton_i( o, v);
}

//*****************************************************************************
// Vp_File_Chooser:::favUpButton_i( *, *) -- Callback method for the 
// 'Favorites Up' button.  Invokes favoritesCB to handle the dialog.
void Vp_File_Chooser::cb_favUpButton_i( Fl_Button*, void*)
{
  favoritesCB( favUpButton);
}

//*****************************************************************************
// Vp_File_Chooser::cb_favoritesButton( o, v) -- Wrapper for the callback
// method to for the Favorites button and pull-down menu.
void Vp_File_Chooser::cb_favoritesButton( Fl_Menu_Button* o, void* v)
{
  ( (Vp_File_Chooser*)
    (o->parent()->parent()->user_data()))->cb_favoritesButton_i( o, v);
}

//*****************************************************************************
// Vp_File_Chooser::cb_favoritesButton_i( *, *) -- Callback method for the 
// Favorites button and pull-down menu..  Invokes favoritesCB to handle the 
// dialog.
void Vp_File_Chooser::cb_favoritesButton_i( Fl_Menu_Button*, void*)
{
  favoritesButtonCB();
}

//*****************************************************************************
// Vp_File_Chooser::favoritesButtonCB() - Handle all dialog for the favorites 
// button and pull-down menu.
void Vp_File_Chooser::favoritesButtonCB()
{
  // Get current selection
  int selectionValue = favoritesButton->value();

  // If the current selection is empty, add the current directory to the
  // favorites list and revise the relevant menu buttons
  char pathname[ 1024];
  char menuname[ 2048];
  if( !selectionValue) {
    if( getenv("HOME")) selectionValue = favoritesButton->size() - 5;
    else selectionValue = favoritesButton->size() - 4;

    sprintf( menuname, "favorite%02d", selectionValue);
    prefs_.set( menuname, directory_);

    quote_pathname( menuname, directory_, sizeof(menuname));
    favoritesButton->add( menuname);

    if( favoritesButton->size() > 104) {
      ( (Fl_Menu_Item *) favoritesButton->menu())[ 0].deactivate();
    }
  } 

  // ...or invoke the favorites dialog with a null value which should
  // reinitialize the list...
  else if( selectionValue == 1) {
    favoritesCB( 0);
  } 

  //... or set current directory to Filesystems/My Computer
  else if( selectionValue == 2) {
    directory( "");
  } 

  // ..or remove quotes and add the current pathname
  else {
    unquote_pathname( 
      pathname, favoritesButton->text( selectionValue), 
      sizeof( pathname));
    directory( pathname);
  }
}

//*****************************************************************************
// Vp_File_Chooser::cb_newButton( o, v) -- Wrapper for the callback method 
// for the New Folder button.
void Vp_File_Chooser::cb_newButton( Fl_Button* o, void* v)
{
  ( (Vp_File_Chooser*) 
    (o->parent()->parent()->user_data()))->cb_newButton_i( o, v);
}

//*****************************************************************************
// Vp_File_Chooser::cb_newButton_i( o, v) -- Callback method for the New
// Folder button.  Invokes newdir() to handle dialog.
void Vp_File_Chooser::cb_newButton_i( Fl_Button*, void*)
{
  newdir();
}

//*****************************************************************************
// Vp_File_Chooser::newdir() -- Diaslog to make a new folder.  Used only by
// cb_newButton_i.
void Vp_File_Chooser::newdir()
{
  char pathname[ 1024];   // Full path of directory

  // Get a directory name from the user
  const char *dir;
  if( ( dir = fl_input(new_directory_label, NULL)) == NULL) return;

  // Make it relative to the current directory as needed...
  #if (defined(WIN32) && ! defined (__CYGWIN__)) || defined(__EMX__)
  if( dir[ 0] != '/' && dir[ 0] != '\\' && dir[1] != ':')
  #else
  if( dir[ 0] != '/' && dir[ 0] != '\\')
  #endif   // WIN32 || __EMX__
    snprintf( pathname, sizeof( pathname), "%s/%s", directory_, dir);
  // else strlcpy( pathname, dir, sizeof(pathname));
  else strncpy( pathname, dir, sizeof(pathname));

  // Create the directory; ignore EEXIST errors...
  #if defined(WIN32) && ! defined (__CYGWIN__)
  if( mkdir( pathname))
  #else
  if( mkdir( pathname, 0777))
  #endif   // WIN32
    if( errno != EEXIST) {
      fl_alert( "%s", strerror(errno));
      return;
    }

  // Show the new directory...
  directory( pathname);
}

//*****************************************************************************
// Vp_File_Chooser:::cb_okButton( o, v) -- Wrapper for the callback method 
// for the OK button.
void Vp_File_Chooser::cb_okButton( Fl_Return_Button* o, void* v)
{
  ( (Vp_File_Chooser*) 
      (o->parent()->parent()->parent()->user_data()))->cb_okButton_i( o, v);
}

//*****************************************************************************
// Vp_File_Chooser::cb_okButton_i( *, *) -- Callback method for the OK 
// button.  Does any callback tat is registered, then hides window.
void Vp_File_Chooser::cb_okButton_i( Fl_Return_Button*, void*) 
{
  if( callback_) (*callback_)(this, data_);
  window->hide();
}

//*****************************************************************************
// Vp_File_Chooser::cb_previewButton( o, v) -- Wrapper for the callback 
// method for the Preview checkbox.
void Vp_File_Chooser::cb_previewButton( Fl_Check_Button* o, void* v)
{
  ( (Vp_File_Chooser*) 
    (o->parent()->parent()->parent()->user_data()))->cb_previewButton_i( o, v);
}

//*****************************************************************************
// Vp_File_Chooser::cb_previewButton_i( *. *) -- Callback method for the 
// Preview checkbox.  Invokes preview() with the appropriate value to handle 
// the dialog.
void Vp_File_Chooser::cb_previewButton_i( Fl_Check_Button*, void*)
{
  preview( previewButton->value());
}

//*****************************************************************************
// Vp_File_Chooser::cb_selectionButton( o, v) -- Wrapper for the callback 
// method for the 'Write Selection Info' checkbox.
void Vp_File_Chooser::cb_selectionButton( Fl_Check_Button* o, void* v)
{
  ( (Vp_File_Chooser*) 
    (o->parent()->parent()->parent()->user_data()))->cb_selectionButton_i( o, v);
}

//*****************************************************************************
// Vp_File_Chooser::cb_selectionButton_i( *. *) -- Callback method for the 
// 'Write Selection Info' checkbox. 
void Vp_File_Chooser::cb_selectionButton_i( Fl_Check_Button*, void*)
{
  // if( selectionButton->value() == 0) writeSelectionInfo_ = 0;
  // else writeSelectionInfo_ = 1;
}

//*****************************************************************************
// Static (global?) functions.  NOTE: Unless it is anticipated that these will
// be used elsewhere, it may be desirable to make them member functions of
// this class.
//

//*****************************************************************************
// compare_dirnames( a, b) -- Compare two directory names.
static int compare_dirnames( const char *a, const char *b)
{
  // Get length of each string and return if one is empty
  int alen = strlen(a) - 1;
  int blen = strlen(b) - 1;
  if( alen < 0 || blen < 0) return alen - blen;

  // Check for trailing slashes...
  if( a[ alen] != '/') alen ++;
  if( b[ blen] != '/') blen ++;

  // If the lengths aren't the same, then return the difference...
  if( alen != blen) return alen - blen;

  // Do a comparison of the first N chars (alen == blen at this point)...
  #ifdef WIN32
  return strncasecmp( a, b, alen);
  #else
  return strncmp( a, b, alen);
  #endif    // WIN32
}

//*****************************************************************************
// quote_pathname( *dst, *src, dstsize) -- Given a destination string, DST, a
// surce string, SRC, and a size, DSTSIZE, for the destination string, quote a 
// pathname for a menu.
static void quote_pathname( char *dst, const char *src, int dstsize)
{
  // Loop: Move through the strings
  dstsize--;
  while( *src && dstsize > 1) {

    // Convert backslash to forward slash...
    if( *src == '\\') {
      *dst++ = '\\';
      *dst++ = '/';
      src ++;
    } 
    else {
      if( *src == '/') *dst++ = '\\';
      *dst++ = *src++;
    }
  }
  *dst = '\0';
}

//*****************************************************************************
// unquote_pathname( *dst, *src, dstsize) - Given a destination string, DST, a
// source string, SRC, and a size, DSTSIZE, for the destination string, 
// unquote a pathname for a menu.
static void unquote_pathname( char *dst, const char *src, int dstsize)
{
  // Loop: Move through the strings
  dstsize--;
  while( *src && dstsize > 1) {
    if( *src == '\\') src ++;
    *dst++ = *src++;
  }
  *dst = '\0';
}

//*****************************************************************************
// isdirsep( c) -- A simple inline function to recognize directory separators 
// under Windows or a for-real OS.
#if defined(WIN32) || defined(__EMX__) && !defined(__CYGWIN__)
  static inline int isdirsep( char c) { return c=='/' || c=='\\';}
#else
  static inline int isdirsep( char c) { return c=='/';}
#endif

//*****************************************************************************
// new_filename_isdir( pathname) -- Method to determine if PATHNAME
// corresponds to a directory.  
static int new_filename_isdir( const char* pathname) 
{
  // Do a quick optimization for filenames with a trailing slash...
  if( *pathname && isdirsep( pathname[ strlen( pathname) - 1])) return 1;

  // If this is Windows, define a temporary buffer with which to edit the
  // pathname, make sure the pathname is short enough, then edit it to
  // remove the foul taint of Redmond.
  #ifdef WIN32
  char temp[ 1024];
  int length = strlen( pathname);
  if( length < (int)(sizeof( temp) - 1)) {

    // If the pathname is shorter than four characters and begins with a 
    // letter and colon (example, "D:"), assume it's merely a drive letter
    // and modify it to terminate with a slash (example, "D:/")...
    if( length < 4 && isalpha( pathname[ 0]) && pathname[ 1] == ':' &&
        ( isdirsep( pathname[ 2]) || !pathname[ 2])) {
      temp[ 0] = pathname[ 0];
      strcpy( temp + 1, ":/");
      pathname = temp;
    } 

    // ...otherwise if the pathname terminates with a directory separator.
    // strip this from the name and make sure the string is null-terminated.
    else if( length > 0 && isdirsep( pathname[ length - 1])) {
      length--;
      memcpy( temp, pathname, length);
      temp[ length] = '\0';
      pathname = temp;
    }
  }
  #endif   // WIN32

  // The original return statement that loads the stat structure and tests 
  // the result in one fell and somewhat opaque swoop.
  // struct stat thisStat;
  // return !stat( pathname, &thisStat) && (thisStat.st_mode&0170000)==0040000;

  // Define and load a STAT structure and get the return value.  If this 
  // was non-zero, the load failed so return a zero
  struct stat thisStat;
  int iStatResult = stat( pathname, &thisStat);
  if( iStatResult != 0) return 0;

  // Examine the content of thisStat.st_mode to determine if PATHNAME 
  // corresponds to a directory and not a file.  This should be composed of 
  // the following values:
  // _S_IFREG -- Set if path refers to an ordinary file, not a directory. 
  // _S_IREAD -- Set if path refers to a readable file or directory. 
  // _S_IWRITE -- Set if path refers to a writable file or directory. 
  // _S_IFDIR -- Set if path refers to a directory. 
  // _S_IEXEC -- Set if path refers to an executable file or a directory.
  // The simlest way to combine them would be with something like:
  // unsigned uModeTest = 
  //   ( ( thisStat.st_mode & _S_IFDIR) != 0) &&
  //   ( ( thisStat.st_mode & _S_IFREG) == 0) &&
  //   ( ( thisStat.st_mode & _S_IREAD) == 0) &&
  //   ( ( thisStat.st_mode & _S_IWRITE) == 0) &&
  //   ( ( thisStat.st_mode & _S_IEXEC) == 0);
  // But since _S_IMFT is the file type mask, the following should work.
  // NOTE: from sys\stat.h, #define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
  #ifndef WIN32
  unsigned uModeTest = S_ISDIR(thisStat.st_mode);
  #else // WIN32
  unsigned uModeTest = ( thisStat.st_mode & _S_IFMT) == _S_IFDIR;
  #endif // WIN32
  return uModeTest;
}
