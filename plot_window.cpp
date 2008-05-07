// viewpoints - interactive linked scatterplots and more.
// copyright 2005 Creon Levit and Paul Gazis, all rights reserved.
//***************************************************************************
// File name: Plot_window.cpp
//
// Class definitions:
//   Plot_Window -- Plot window
//
// Classes referenced:
//   control_panel_window -- Control panel window
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
// Purpose: Source code for <Plot_Window.h>
//
// Author: Creon Levit    2005-2006
// Modified: P. R. Gazis  15-DEC-2007
//***************************************************************************

// Include the necessary include libraries
#include "include_libraries_vp.h"

// Include globals
#include "global_definitions_vp.h"

// Include associated headers and source code
#include "plot_window.h"
#include "control_panel_window.h"
#include "brush.h"

// experimental
#define ALPHA_TEXTURE

#define CHECK_GL_ERROR(msg)                    \
    {                                          \
        GLenum TMerrCode = glGetError();       \
        if (TMerrCode != GL_NO_ERROR) {        \
            printf("%s: %s %s at %s:%d\n",     \
                   __FUNCTION__,               \
                   gluErrorString(TMerrCode),  \
                   (msg), __FILE__, __LINE__); \
            abort();                           \
        }                                      \
    }

// initialize static data members for class Plot_Window::

// Initial number of plot windows
int Plot_Window::count = 0;

// Initial fraction of the window to be used for showing (normalized) data
float const Plot_Window::initial_pscale = 0.8; 

//GLfloat Plot_Window::texenvcolor[ 4] = { 1, 1, 1, 1};

// 2D array that holds indices of vertices for each brush
blitz::Array<unsigned int,2> Plot_Window::indices_selected(NBRUSHES,1); 

GLuint Plot_Window::spriteTextureID[NSYMBOLS];
GLubyte* Plot_Window::spriteData[NSYMBOLS];
int Plot_Window::sprites_initialized = 0;

void *Plot_Window::global_GLContext = NULL;
int Plot_Window::indexVBOsinitialized = 0;
int Plot_Window::indexVBOsfilled = 0;
#define BUFFER_OFFSET(vbo_offset) ((char *)NULL + (vbo_offset))

// Declarations for global methods defined and used by class Plot_Window.
// NOTE: Is it a good idea to do this here rather than global_definitions.h?
void moving_average( 
  blitz::Array<float,1> a, const blitz::Array<int,1> indices, 
  const int half_width);
void cummulative_conditional(
  blitz::Array<float,1> a, const blitz::Array<int,1> indices, 
  const int half_width);
void fluctuation(
  blitz::Array<float,1> a, const blitz::Array<int,1> indices, 
  const int half_width);

//***************************************************************************
// Plot_Window::Plot_Window() -- Default constructor, used only in
// association with serialzation!
Plot_Window::Plot_Window() : Fl_Gl_Window( 10, 10),
  index( 0),
  x_save( 0), y_save( 0), w_save( 0), h_save( 0),
  do_reset_view_with_show( 0)
{}

//***************************************************************************
// Plot_Window::Plot_Window( w, h) -- Constructor.  Increment count of plot 
// windows, resize arrays, and set mode.  This constructor sets a flag to
// tell the package to show the windows as part on a RESET_VIEW operation.
Plot_Window::Plot_Window( int w, int h, int new_index) : 
  Fl_Gl_Window( w, h), do_reset_view_with_show( 0)
{
  // Set flag, update count, and invoke initialzation method
  count++;
  index = new_index;
  initialize();
}

//***************************************************************************
// Plot_Window::initialize -- Initialize window parameters.  Set flags, set 
// set framebufer modes, set up openGL context.
void Plot_Window::initialize()
{
  do_reset_view_with_show = 0;
  show_center_glyph = 0;
  selection_changed = 0;

  VBOinitialized = 0;
  VBOfilled = false;

  // Resize arrays
  vertices.resize( npoints, 3);
  nbins[0] = nbins[1] = nbins[2] = nbins_default;
  counts.resize( nbins_max+2, 3);
  counts_selected.resize( nbins_max+2, 3);

  if( can_do(FL_RGB|FL_DOUBLE|FL_ALPHA|FL_DEPTH|FL_STENCIL)) {
    mode( FL_RGB|FL_DOUBLE|FL_ALPHA|FL_DEPTH|FL_STENCIL);
    cout << " mode: FL_RGB|FL_DOUBLE|FL_ALPHA|FL_DEPTH|FL_STENCIL" << endl;
  }
  else if( can_do(FL_RGB|FL_DOUBLE|FL_ALPHA|FL_DEPTH)) {
    mode( FL_RGB|FL_DOUBLE|FL_ALPHA|FL_DEPTH);
    cout << " mode: FL_RGB|FL_DOUBLE|FL_ALPHA|FL_DEPTH" << endl;
  }
  else if( can_do(FL_RGB8|FL_DOUBLE|FL_ALPHA|FL_DEPTH)) {
    mode( FL_RGB8|FL_DOUBLE|FL_DEPTH|FL_ALPHA);
    cout << " mode: FL_RGB8|FL_DOUBLE|FL_ALPHA|FL_DEPTH" << endl;
  }
  else if( can_do(FL_RGB|FL_DOUBLE|FL_ALPHA)) {
    cout << "Warning: depth buffering not enabled" << endl;
    mode( FL_RGB|FL_DOUBLE|FL_ALPHA);
    cout << " mode: FL_RGB|FL_DOUBLE|FL_ALPHA" << endl;
  }
  else if( can_do(FL_RGB8|FL_DOUBLE|FL_ALPHA)) {
    cout << "Warning: depth buffering not enabled" << endl;
    mode( FL_RGB8|FL_DOUBLE|FL_ALPHA);
    cout << " mode: FL_RGB8|FL_DOUBLE|FL_ALPHA" << endl;
  }
  else {
    cerr << "Error: can not allocate double buffered RGBA window for plot window " << index << endl;
    exit (-1);
  }
    
  indexVBOsinitialized=0;
  sprites_initialized=0;

}

//***************************************************************************
// Plot_Window::make_state() -- Examine widgets to generate and save state
// parameters.
void Plot_Window::make_state()
{
  x_save = x();
  y_save = y();
  w_save = w();
  h_save = h();
}

//***************************************************************************
// Plot_Window::copy_state( &pw) -- Copy state parameters from another 
// object that has been created by serialization.
void Plot_Window::copy_state( Plot_Window* pw)
{
  index = pw->index;
  x_save = pw->x_save;
  y_save = pw->y_save;
  w_save = pw->w_save;
  h_save = pw->h_save;
}

//***************************************************************************
// Plot_Window::load_state() -- Load state parameters into widgets.  
// WARNING: There is no proetction against bad state parameters or the
// possibility that this might be a default object without widgets.
void Plot_Window::load_state()
{
  // x( x_save);
  // y( y_save);
  // w( w_save);
  // h( h_save);
  position( x_save, y_save);
  size( w_save, h_save);
}

//***************************************************************************
// Plot_Window::change_axes() -- Change axes for a plot to new axes which are 
// (probably) not duplicates.
void Plot_Window::change_axes( int nchange)
{
  // int nchange = 0;

  // Loop: Examine control panel tabs and increment axis counts only for plots 
  // with x or y axis unlocked.  This is not ideal.
  for( int i=0; i<nplots; i++) {
    if( !cps[i]->lock_axis1_button->value() || !cps[i]->lock_axis2_button->value())
      nchange++;
  }
  // cout << "for window " << index << " nchange=" << nchange << endl;

  // Get variable indices for this panel, then change variable indices for 
  // axes that are not locked.  MCL observes that this code seems a little 
  // verbose
  int i=cp->varindex1->value();
  int j=cp->varindex2->value();
  // cout << "  (i,j) before = (" << i << "," << j << ")" << endl;

  if (!cp->lock_axis1_button->value() && !cp->lock_axis2_button->value()) {
    for( int k=0; k<nchange; k++) upper_triangle_incr( i, j, nvars);
    cp->varindex1->value(i);
    cp->varindex2->value(j);
  } 
  else if (!cp->lock_axis1_button->value()) {
    for( int k=0; k<nchange; k++) {
      i = (i+1)%nvars;
      cp->varindex1->value(i);
    }
  }
  else if (!cp->lock_axis2_button->value()) {
    for( int k=0; k<nchange; k++) {
      j = (j+1)%nvars;
      cp->varindex2->value(j);
    }
  }

  // Extract, renormalize, and draw variables
  cp->extract_and_redraw();
}

//***************************************************************************
// Plot_Window::update_linked_transforms() -- Use current plot's scale and 
// offset to update all the others that show (any of) the same axes (using 
// the same normalization).
void Plot_Window::update_linked_transforms()
{
  if( !link_all_axes_button->value()) return;

  // Get this plot's axis indices and normalization styles
  int axis1=cp->varindex1->value(); 
  int style1 = cp->x_normalization_style->value();
  int axis2=cp->varindex2->value(); 
  int style2 = cp->y_normalization_style->value();

  // Loop: Find other plot windows that have any of the same axis indices 
  // active and the same normalization style, update the appropriate 
  // translation and scale values for them.
  for( int i=0; i<nplots; i++) {
    Plot_Window *p = pws[i];

    // Don't need to update ourself
    if( p == this) continue; 

    // Finally, figure out what me may want to change and how
    if( p->cp->varindex1->value() == axis1 && 
        p->cp->x_normalization_style->value() == style1) {
      p->xscale = xscale; 
      p->xcenter = xcenter;
      p->needs_redraw = 1;
    }
    else if( p->cp->varindex1->value() == axis2 && 
             p->cp->x_normalization_style->value() == style2) {
      p->xscale = yscale; 
      p->xcenter = ycenter;
      p->needs_redraw = 1;
    }

    if( p->cp->varindex2->value() == axis1 && 
        p->cp->y_normalization_style->value() == style1) {
      p->yscale = xscale; 
      p->ycenter = xcenter;
      p->needs_redraw = 1;
    }
    else if( p->cp->varindex2->value() == axis2 && 
             p->cp->y_normalization_style->value() == style2) {
      p->yscale = yscale; 
      p->ycenter = ycenter;
      p->needs_redraw = 1;
    }

    // This is needed to make sure the scale marks on the axis are 
    // updated
    p->screen_to_world(-1, -1, p->wmin[0], p->wmin[1]);
    p->screen_to_world(+1, +1, p->wmax[0], p->wmax[1]);
  }
}

//***************************************************************************
// Plot_Window::handle( event) -- Main event handler.
int Plot_Window::handle( int event)
{
  // Current plot window (button pushes, mouse drags, etc) must get redrawn 
  // before others so that selections get colored correctly.  Ugh.
  switch(event) {

    active_plot = index;

    // Mouse button push
    case FL_PUSH:
      DEBUG(cout << "FL_PUSH at " << xprev << ", " << yprev << endl);

      // Show the control panel associated with this plot window.
      cpt->value(cps[this->index]);  
      xprev = Fl::event_x();
      yprev = Fl::event_y();

      // middle button pushed => start zoom
      if( (Fl::event_state() == FL_BUTTON2) || 
          (Fl::event_state() == (FL_BUTTON1 | FL_CTRL))) {
        // XXX wish this worked
        #if 0
        xzoomcenter = (float)xprev;
        xzoomcenter = + (2.0*(xzoomcenter/(float)w()) -1.0) ; // window -> [-1,1]
        yzoomcenter = (float)yprev;
        yzoomcenter = - (2.0*(yzoomcenter/(float)h()) -1.0) ; // window -> [-1,1]
        #endif
      }

      // right button pushed => start translating
      else if( Fl::event_state(FL_BUTTON3) || 
               (Fl::event_state() == (FL_BUTTON1 | FL_ALT)) ) {
        show_center_glyph = 1;
        needs_redraw = 1;
      }

      // left button pushed => start new selection, or start translating the 
      // old selection
      else if( Fl::event_state() == FL_BUTTON1) {

        // determine current and previous active plot windows
        static Plot_Window *previous_plot, *current_plot = (Plot_Window *)NULL;
        previous_plot = current_plot;
        current_plot = this;

        // determine current and previous active brushes
        static Brush *previous_brush, *current_brush = (Brush *)NULL;
        previous_brush = current_brush;
        current_brush =  dynamic_cast <Brush*> (brushes_tab->value());
        assert (current_brush);

        newly_selected(blitz::Range(0,npoints-1)) = 0;
        // extend the selection under the following circumstances, otherwise replace.
        if (current_brush->add_to_selection->value() || // current_brush->paint->value() || ???
            current_brush != previous_brush ||
            current_plot != previous_plot) {
          previously_selected( blitz::Range(0,npoints-1)) = selected( blitz::Range( 0, npoints-1));
        }

        xdown = (float)xprev;
        xdown = + (2.0*(xdown/(float)w()) -1.0) ; // window -> [-1,1]
        xdown = xdown / xscale;
        xdown = xdown + xcenter;
      
        ydown = (float)yprev;
        ydown = - (2.0*(ydown/(float)h()) -1.0) ; // window -> [-1,1]
        ydown = ydown/yscale;
        ydown = ydown + ycenter;

        xtracked = xdown;
        ytracked = ydown;
        selection_changed = 1;
        handle_selection ();
        redraw_all_plots (index);
      }

      return 1;

    // Mouse drag
    case FL_DRAG:
      DEBUG (printf ("FL_DRAG, event_state: %x\n", Fl::event_state()));
      xcur = Fl::event_x();
      ycur = Fl::event_y();

      xdragged = xcur - xprev;
      ydragged = -(ycur - yprev);
      xprev = xcur;
      yprev = ycur;

      // drag with right mouse (or alt-left-mouse) => translate the view
      if( Fl::event_state(FL_BUTTON3) || 
          (Fl::event_state(FL_BUTTON1) && Fl::event_state(FL_ALT))) {
        float xmove = xdragged*(1/xscale)*(2.0/w());
        float ymove = ydragged*(1/yscale)*(2.0/h());
        xcenter -= xmove;
        ycenter -= ymove;
        DEBUG ( cout << "translating (xcenter, ycenter) = (" << xcenter << ", " << ycenter << ")" << endl);
        // redraw ();
        show_center_glyph = 1;
        needs_redraw = 1;
        update_linked_transforms ();
      }

      // drag with middle-mouse (or c-left-mouse) => scale the view
      else if( Fl::event_state(FL_BUTTON2) || 
               (Fl::event_state(FL_BUTTON1) && Fl::event_state(FL_CTRL))) {
        if( scale_histogram) {
          xhscale *= 1 + xdragged*(2.0/w());
          yhscale *= 1 + ydragged*(2.0/h());
        }
        else {
          xscale *= 1 + xdragged*(2.0/w());
          yscale *= 1 + ydragged*(2.0/h());
          zscale *= 1 + 0.5 * (xdragged*(2.0/w()) + ydragged*(2.0/h()));  // XXX hack.
          DEBUG ( cout << "scaling (xscale, yscale) = (" << xscale << ", " << yscale << ")" << endl);
        }
        // redraw();
        needs_redraw = 1;
        update_linked_transforms ();
      }

      // drag with left mouse => continue selecting
      else if( Fl::event_state(FL_BUTTON1)) {
        // shift key down => move entire selection
        if( Fl::event_key(FL_Shift_L) || Fl::event_key(FL_Shift_R)) {
          xdown += xdragged*(1/xscale)*(2.0/w());
          ydown += ydragged*(1/yscale)*(2.0/h());
          xtracked += xdragged*(1/xscale)*(2.0/w());
          ytracked += ydragged*(1/yscale)*(2.0/h());
        }
        // no shift key => move corner of selection
        else {
          xtracked = + (2.0*(xcur/(float)w()) -1.0) ; // window -> [-1,1]
          xtracked = xtracked / xscale;
          xtracked = xtracked + xcenter;
        
          ytracked = - (2.0*(ycur/(float)h()) -1.0) ; // window -> [-1,1]
          ytracked = ytracked/yscale;
          ytracked = ytracked + ycenter;
        }
        
        // printf ("FL_DRAG & FL_BUTTON1, event_state: %x  isdrag = %d  xdragged=%f  ydragged=%f\n", Fl::event_state(), isdrag, xdragged, ydragged);
        if((fabs(xdragged)+fabs(ydragged))>0 ){
          selection_changed = 1;
          handle_selection ();
          redraw_all_plots (index);
        }
      }
      screen_to_world (-1, -1, wmin[0], wmin[1]);
      screen_to_world (+1, +1, wmax[0], wmax[1]);
      return 1;

    // Mouse button up
    case FL_RELEASE:   
      DEBUG (cout << "FL_RELEASE at " << Fl::event_x() << ", " << Fl::event_y() << endl);
      if( show_center_glyph) {
        show_center_glyph = 0;
      }
      redraw_one_plot();
      return 1;

    // keypress, key is in Fl::event_key(), ascii in Fl::event_text().  Return 
    // 1 if you understand/use the keyboard event, 0 otherwise...
    case FL_KEYDOWN:
      DEBUG ( cout << "FL_KEYDOWN, event_key() = " << Fl::event_key() << endl);

      // XXX should figure out how to share shortcuts between plot windows and 
      // control panels... later
      switch( Fl::event_key()) {

				// exit
        case 'q':  
          if( expert_mode || make_confirmation_window( "Quit?  Are you sure?") > 0)
            exit( 0);
          else
            return 1;

        // delete selected points from all future processing
        case 'x':
        case FL_Delete:
          delete_selection( (Fl_Widget *) NULL);
          return 1;

        // Invert or restore (uninvert) selected and nonselected
        case 'i':
          invert_selection();
          return 1;

        // Clear selection
        case 'c':
          clear_selections( (Fl_Widget *) NULL);
          return 1;

        // Don't display / display deselected dots
        case 'd':
          toggle_display_deselected( (Fl_Widget *) NULL);
          return 1;

        // Reset view tranform for this plot
        case 'r':
          reset_view();
          return 1;

        // hold down 'h' and middle mouse drag to scale histogram bin height.
        // there should really be a better way....
        case 'h':
          scale_histogram=1;
          return 1;

        // run a timing test, for performancw tuning & profiling 
        case '9':
          run_timing_test();
          return 1;

        default:
          return 0;
      }

    // Keyboard key up
    case FL_KEYUP:
      DEBUG ( cout << "FL_KEYUP" << endl);
      switch( Fl::event_key()) {
        case 'h':
          scale_histogram=0;
          return 1;

        default:
          return 0;
      }

    // Shortcut, key is in Fl::event_key(), ascii in Fl::event_text().  Return 
    // 1 if you understand/use the shortcut event, 0 otherwise...
    case FL_SHORTCUT:
      return 0;

    // Pass other events to the base class...
    default:
      return Fl_Gl_Window::handle( event);}
} 

//***************************************************************************
// Plot_Window::reset_selection_box() -- Reset the selection box.
void Plot_Window::reset_selection_box()
{
  xdragged = ydragged = 0.0;
  xzoomcenter = yzoomcenter = zzoomcenter = 0.0;
  xdown = ydown = xtracked = ytracked = 0.0;
  xprev = yprev = xcur = ycur = 0;
}

//***************************************************************************
// Plot_Window::redraw_one_plot() -- Invoke member functions to compute 
// histograms and redraw one plot.
void Plot_Window::redraw_one_plot ()
{
  DEBUG( cout << "in redraw_one_plot" << endl ) ;
  compute_histograms();
  redraw();
  //Fl::flush();
  needs_redraw = 0;
}

//***************************************************************************
// Plot_Window::reset_view() -- Reset pan, zoom, and angle.
void Plot_Window::reset_view()
{
  // XXX this, and so much else, needs to be cleaned up, generalized to any & all axes.
  // every possible c-style array should be replaced in viewpoints with a std::vector or
  // a boost::array.
   for (int i=0; i<3; i++) {
     wmin[i] = amin[i];
     wmax[i] = amax[i];
   }
  
  // Get third axis, if any
  int axis2 = (int)(cp->varindex3->mvalue()->user_data());

  // Regenerate axis scales
  xscale = 2.0 / (wmax[0]-wmin[0]);
  yscale = 2.0 / (wmax[1]-wmin[1]);
  if (axis2 != nvars) zscale = 2.0 / (wmax[2]-wmin[2]);
  else zscale = 1.0;
  
  // Initially, datapoints only span 0.8 of the window dimensions, which 
  // allows room around the edges for labels, tickmarks, histograms....
  xscale *= initial_pscale; 
  yscale *= initial_pscale; 
  zscale *= initial_pscale; 
  initial_scale = (xscale+yscale)/2.0;
  magnification = 1.0;

  // Get axis centers
  xcenter = (wmin[0]+wmax[0]) / 2.0;
  ycenter = (wmin[1]+wmax[1]) / 2.0;
  if( axis2 != nvars) zcenter = (wmin[2]+wmax[2]) / 2.0;
  else zcenter = 0.0;

  // Get histogram scales (max height in window coords)
  xhscale = 0.25;
  yhscale = 0.25;

  // Reset angle and stop any spin.
  angle = 0.0;
  cp->spin->value(0);
  cp->rot_slider->value(0.0);
  cp->dont_clear->value(0);

  // Reset selection box and flag window as needing redraw
  reset_selection_box ();
  needs_redraw = 1;

  // Make sure the window is visible and resizable.  NOTE: For some reason, it
  // is necessary to turn this off when a new plot window array is created or 
  // the windows will not be resizable!
  if( do_reset_view_with_show & !visible()) {
    this->show();
    this->resizable( this);
  }
}

//***************************************************************************
// Plot_Window::draw() -- Main draw method that calls others.
void Plot_Window::draw() 
{
  DEBUG (cout << "in draw: " << xcenter << " " << ycenter << " " << xscale << " " << yscale << " " << wmin[0] << " " << wmax[0] << endl);

  // the valid() property can avoid reinitializing matrix for 
  // each redraw:
  if( !valid()) {
    valid(1);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, 1000, -1000);
    glViewport(0, 0, w(), h());
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glEnableClientState(GL_VERTEX_ARRAY);

    // glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    // glEnableClientState(GL_COLOR_ARRAY);

    // this next idiom is necessary, per window, to map texture coordinate 
    // values to [0..1] for texturing.
    // glMatrixMode(GL_TEXTURE);
    // glLoadIdentity();  
    // glScalef( 1.0/(float)MAXPLOTS, 1.0/(float)MAXPLOTS, 1.0/(float)MAXPLOTS); 
    // glMatrixMode(GL_MODELVIEW);
  }
  
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef( xzoomcenter*xscale, yzoomcenter*yscale, zzoomcenter*zscale);
  if( cp->spin->value())
    angle += cp->rot_slider->value()/100.0;
  else
    angle = cp->rot_slider->value();
  glRotatef(angle, 0.0, 1.0, 0.1);
  glScalef (xscale, yscale, zscale);
  //magnification = ((xscale+yscale)/2.0) / initial_scale;
  magnification = sqrt(xscale*yscale) / initial_scale;
  glTranslatef (-xcenter, -ycenter, -zcenter);
  glTranslatef (-xzoomcenter, -yzoomcenter, -zzoomcenter);

  if( cp->dont_clear->value() == 0) {
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClearDepth (0.0);
    glClearStencil (0);
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    draw_grid();
  }

  if (use_VBOs) {
    if (!VBOinitialized) initialize_VBO();
    if (!VBOfilled) fill_VBO();
    if (!indexVBOsinitialized) initialize_indexVBOs();
    if (!indexVBOsfilled) fill_indexVBOs();
  }

  draw_data_points();
  if( selection_changed) {
    draw_selection_information();
  }
  draw_center_glyph();
  draw_axes();
  draw_histograms ();
  draw_background ();
}

// If the background is anything besides black, draw it.  Last.
// Why last?  Because we always blend against a black background initially so
// that the overplotting comes out OK.  Other backgrounds would interfere.
void Plot_Window::draw_background ()
{
  if( cp->dont_clear->value() == 0) {
    float bk = cp->Bkg->value();
    if (bk > 0.0) {
      glPushMatrix();
      glLoadIdentity();
      glEnable( GL_DEPTH_TEST);
      glDepthFunc( GL_GEQUAL);
      glColor4f(bk,bk,bk,1.0);
      glBegin(GL_QUADS);
      glVertex3f(-2,-2,-2);
      glVertex3f(+2,-2,-2);
      glVertex3f(+2,+2,-2);
      glVertex3f(-2,+2,-2);
      glEnd();
      glPopMatrix();
    }
  }
}

//***************************************************************************
// Plot_Window::draw_grid() -- Draw a grid.
void Plot_Window::draw_grid()
{
  glBlendFunc(GL_ONE, GL_ZERO);
  // glBlendFunc(sfactor, dfactor);
  // glEnable(GL_LINE_SMOOTH);
  // glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
  // glLineWidth(0.5);
  glLineWidth(1.0);
  if( cp->show_grid->value()) {
    glPushMatrix();
    glLoadIdentity();

    float c = initial_pscale;
    glScalef( c, c, c);

    if( cp->Bkg->value() <= 0.2)
      glColor4f(0.2,0.2,0.2,0.0);
    else
      glColor4f( 
        0.8*cp->Bkg->value(), 0.8*cp->Bkg->value(), 
        0.8*cp->Bkg->value(), 0.0);

    // draw the grid here
    glBegin( GL_LINES);
    for( int k=-1; k<=1; k+=2) {
      for( int i=1; i<=10; i++) {

        // XY plane
        glVertex3f (-1.0, 0.1*i*k, 0.0); 
        glVertex3f (+1.0, 0.1*i*k, 0.0);
        glVertex3f (0.1*i*k, -1.0, 0.0); 
        glVertex3f (0.1*i*k, +1.0, 0.0);

        // YZ plane
        glVertex3f (0.0, -1.0, 0.1*i*k); 
        glVertex3f (0.0, +1.0, 0.1*i*k);
        glVertex3f (0.0, 0.1*i*k, -1.0); 
        glVertex3f (0.0, 0.1*i*k, +1.0);

        // XZ plane
        glVertex3f (-1.0, 0.0, 0.1*i*k); 
        glVertex3f (+1.0, 0.0, 0.1*i*k);
        glVertex3f (0.1*i*k, 0.0, -1.0); 
        glVertex3f (0.1*i*k, 0.0, +1.0);
      }
    }
    glEnd();
    glPopMatrix();
  }
}

//***************************************************************************
// Plot_Window::screen_to_world( xscreen, yscreen, xworld, yworld) -- Convert 
// from screen to world co-ordinates?
void Plot_Window::screen_to_world( 
  float xscreen, float yscreen, float &xworld, float &yworld)
{
  DEBUG (cout << "screen_to_world" << endl
         << "  before xscreen: " << xscreen << " yscreen: " << yscreen
         << " xworld: " << xworld << " yworld: " << yworld << endl
         << " pscale: " << initial_pscale
         << " xscale: " << xscale << " xcenter: " << xcenter
         << " yscale: " << yscale << " ycenter: " << ycenter
         << endl) ;

  xworld = ( xscreen*initial_pscale / xscale) + xcenter;
  yworld = ( yscreen*initial_pscale / yscale) + ycenter;

  DEBUG (cout << "  after xscreen: " << xscreen << " yscreen: " << yscreen
         << " xworld: " << xworld << " yworld: " << yworld << endl);
}

//***************************************************************************
// Plot_Window::draw_axes() -- If requested, draw and label the axes
void Plot_Window::draw_axes()
{
  // If requested draw axes
  if( cp->show_axes->value() || cp->show_scale->value() ) {
    glPushMatrix();
    glLoadIdentity();

    // Define the extra (relative) distance that axes extend past leftmost 
    // and rightmost tickmarks and set initial pscale
    float a = 0.1; 
    float c = initial_pscale;
    glScalef( c, c, c);

    gl_font( FL_HELVETICA, 10);
    glBlendFunc( GL_ONE, GL_ZERO);
    if( cp->Bkg->value() <= 0.4)
      glColor4f( 0.7,0.7,0.0,0.0);
    else
      glColor4f( 
        0.4*cp->Bkg->value(), 
        0.4*cp->Bkg->value(), 
        0.0*cp->Bkg->value(), 0.0);

    if (cp->show_axes->value()) {
      // Draw the axes
      glBegin( GL_LINES);
      
      // X data
      glVertex3f( -(1+a), -(1+a), -(1+a));
      glVertex3f( +(1+a), -(1+a), -(1+a));
      
      // Y data
      glVertex3f( -(1+a), -(1+a), -(1+a));
      glVertex3f (-(1+a), +(1+a), -(1+a));
      
      // Z axis
      glVertex3f( -(1+a), -(1+a), -(1+a));
      glVertex3f( -(1+a), -(1+a), +(1+a));

      glEnd();

      // draw axes labels
      {
        
        // offset for axis labels values. b<1 -> inwards, 
        // b>1 -> outwards, b==1 -> on axis.
        float b = 2; 
        
        gl_font( FL_HELVETICA_BOLD, 11);
        float wid = gl_width(xlabel.c_str())/(float)(w());
        gl_draw( (const char *)(xlabel.c_str()), -wid, -(1+b*a));  
        
        b = 1.5;
        gl_draw( (const char *)(ylabel.c_str()), -(1+b*a), 1+b*a);
      }
      
      // Define a buffer used to lable tic marks and set the offset factor for 
      // tic mark length. b<1 -> inwards, b>1 -> outwards, b==1 -> no tick.
      char buf[ 1024];
      float b = 1.5;

      // If requested, draw tic marks and numbers to show scale  
      if( cp->show_scale->value()) {

        glBegin( GL_LINES);

        // lower X-axis tick
        glVertex3f( -1, -(1+a), -(1+a)); 
        glVertex3f( -1, -(1+b*a), -(1+a));

        // upper X-axis tick
        glVertex3f( +1, -(1+a), -(1+a)); 
        glVertex3f( +1, -(1+b*a), -(1+a));

        // lower Y-axis tick
        glVertex3f( -(1+a), -1, -(1+a)); 
        glVertex3f( -(1+b*a), -1, -(1+a)); 

        // upper Y-axis tick
        glVertex3f( -(1+a), +1, -(1+a)); 
        glVertex3f( -(1+b*a), +1, -(1+a)); 

        // XXX Z-axis ticks clutter 2D plots
        b = 1; 

        // lower Z-axis tick
        glVertex3f( -(1+a), -(1+a), -1); 
        glVertex3f( -(1+b*a), -(1+a), -1);

        // upper Z-axis tick
        glVertex3f (-(1+a), -(1+a), +1); 
        glVertex3f (-(1+b*a), -(1+a), +1); 

        glEnd();

        // Offset for drawing tick marks'.  Numeric values are conrolled by "b" 
        // as follows:
        //  b<1  -> draw it inside of the axis, 
        //  b>1  -> draw it outside of the axis,
        //  b==1 -> draw it on the axis.
        b = 2;

        // draw lower X-axis tic mark's numeric value
        snprintf( buf, sizeof(buf), "%+.3g", wmin[0]); 
        gl_draw( 
          (const char *)buf, 
          -1.0-gl_width((const char *)buf)/(w()), -(1+b*a));

        // draw upper X-axis tic mark's numeric value
        snprintf(buf, sizeof(buf), "%+.3g", wmax[0]); 
        gl_draw(
          (const char *)buf, 
          +1.0-gl_width((const char *)buf)/(w()), -(1+b*a));

        b = 2.4;

        // draw lower Y-axis tic mark's numeric value
        snprintf( buf, sizeof(buf), "%+.3g", wmin[1]);
        gl_draw( (const char *)buf, -(1+b*a), -1.0f+a/4);

        // draw upper Y-axis tic mark's numeric value
        snprintf( buf, sizeof(buf), "%+.3g", wmax[1]);
        gl_draw( (const char *)buf, -(1+b*a), +1.0f+a/4);
      }

    }

    glPopMatrix();
  }
}

//***************************************************************************
// Plot_Window::draw_center_glyph() -- Draw a glyph in the center of the 
// window, as an aid for positioning in preparation to zooming.
void Plot_Window::draw_center_glyph()
{
  if( !show_center_glyph) return;

  glDisable( GL_DEPTH_TEST);
  glEnable( GL_COLOR_LOGIC_OP);
  glLogicOp( GL_INVERT);
  
  glPushMatrix ();
  glLoadIdentity();
  glBlendFunc( GL_ONE, GL_ZERO);
  glBegin( GL_LINES);

  glColor4f(0.7,0.7,0.7,0.0);

  glVertex3f( -0.025, 0.0, 0.0); 
  glVertex3f( 0.025, 0.0, 0.0);

  glVertex3f( 0.0, -0.025, 0.0); 
  glVertex3f( 0.0, 0.025, 0.0);

  glEnd();
  glPopMatrix();
  glDisable( GL_COLOR_LOGIC_OP);
}

//***************************************************************************
// Plot_Window::print_selection_stats() -- dynamically write statistics for 
// the totality of the selection(s) as well as the numeric values of bounding 
// box edges to the current plot window while brushing.
void Plot_Window::print_selection_stats ()
{
  glDisable( GL_DEPTH_TEST);
  glEnable( GL_COLOR_LOGIC_OP);
  glLogicOp( GL_XOR);
  glBlendFunc( GL_ONE, GL_ZERO);
  glColor4f( 0.9,0.9,0.9,1.0);

  // Define character buffer to allocate storage for printing
  char buf[ 1024];

  // Print selection statistics, centered, near the top of the window
  // LR-centered, upper 95th percentile of the window
  snprintf( buf, sizeof(buf), 
    "%8d (%5.2f%%) selected", nselected, 100.0*nselected/(float)npoints);
  gl_font( FL_HELVETICA_BOLD, 11);
  glWindowPos2i( (w()-(int)gl_width(buf))/2, 95*h()/100);
  gl_draw( (const char *) buf);

  // Print x-ranges at left and right sides of selection box
  gl_font( FL_HELVETICA, 10);
  snprintf( buf, sizeof(buf), "%# 7.4g", xdown);
  gl_draw( (const char *) buf, 
    xdown-2*gl_width(buf)/(w()*xscale), 
    ((ydown+ytracked)/2)-(0.5f*gl_height())/(h()*yscale));
  if (xtracked != xdown) {
    snprintf( buf, sizeof(buf), "%#-7.4g", xtracked);
    gl_draw( (const char *) buf, 
      xtracked+4.0f/(w()*xscale), 
      ((ydown+ytracked)/2)-(0.5f*gl_height())/(h()*yscale) );
  }
  
  // Print y-ranges at top and bottom sides of selection box
  snprintf( buf, sizeof(buf), "%# 7.4g", ydown);
  gl_draw( (const char *) buf, 
    (xdown+xtracked)/2-gl_width(buf)/(w()*xscale), 
    ydown+(0.75f*gl_height())/(h()*yscale) );
  if (ytracked != ydown) {
    snprintf( buf, sizeof(buf), "%# 7.4g", ytracked);
    gl_draw( (const char *) buf, 
      (xdown+xtracked)/2-gl_width(buf)/(w()*xscale), 
      ytracked-(1.5f*gl_height())/(h()*yscale) );
  }

  glDisable( GL_COLOR_LOGIC_OP);
}

//***************************************************************************
// Plot_Window::handle_selection() -- Handler to handle selection operations.
// Does not draw anything (since openGL functions cannot be called from 
// within a handle() method).  Selection information (e.g. bounding box, 
// statistics) are drawn from the draw() method for the window making the 
// selection by calling draw_selection_information().
void Plot_Window::handle_selection ()
{
  blitz::Range NPTS( 0, npoints-1);  

  if (xdown==xtracked && ydown==ytracked) return;

  // Identify newly-selected points.
  // XXX could be a bool array?  faster?
  // XXX could use binary search on sorted values.  Much faster?
  Brush  *current_brush = (Brush *)NULL;
  current_brush =  dynamic_cast <Brush*> (brushes_tab->value());
  assert (current_brush);

  
  // switch (current_brush->footprint->value()) {

  enum footprint {BRUSH_BOX, BRUSH_CIRCLE};
  footprint xxx = BRUSH_BOX;
  switch (xxx) {
  case BRUSH_BOX:
    inside_footprint(NPTS) =
      where( ( vertices( NPTS, 0)>fmaxf( xdown, xtracked) || 
               vertices( NPTS, 0)<fminf( xdown, xtracked) ||
               vertices( NPTS, 1)>fmaxf( ydown, ytracked) || 
               vertices( NPTS, 1)<fminf( ydown, ytracked)),
             0, 1);
    break;
  case BRUSH_CIRCLE:
    {
      float xs = xscale*w(), ys=yscale*h();
      float dist2 = pow2((xtracked-xdown)*xs) + pow2((ytracked-ydown)*ys);
      inside_footprint(NPTS) =
        where( ( (pow2((vertices(NPTS,0)-xdown)*xs) + pow2((vertices(NPTS,1)-ydown)*ys)) > dist2),
               0, 1);
    }
    break;
  default:
    assert(!"Impossible brush footprint");
  }

  if (current_brush->paint->value()) {
    newly_selected( NPTS) |= inside_footprint(NPTS);
  } else {
    newly_selected( NPTS) = inside_footprint(NPTS);
  }

  // then tag them with the appropriate integer (index of current brush) so that later
  // they'll get sorted correctly and drawn with the correct color and texture.
  Brush *bp = dynamic_cast <Brush*> (brushes_tab->value());
  assert (bp);
  int brush_index = bp->index; 
  if (mask_out_deselected->value() && brush_index>0) {
    // MCL this should be called "and with selection" or some such.
    selected( NPTS) = where( newly_selected( NPTS) && selected(NPTS), brush_index, previously_selected( NPTS));
  } else if (mask_out_deselected->value() && brush_index==0) {
    // MCL XXX this is a bogus hack to implement an "inverse" brush that deselects everything outside of it,
    // and leaves unchanged whatever is inside it.  Invoke using brush zero while "mask_out_deselected" is turned on.
    selected( NPTS) = where( !newly_selected( NPTS), brush_index, previously_selected( NPTS));
  } else {
    selected( NPTS) = where( newly_selected( NPTS), brush_index, previously_selected( NPTS));
  }

  // pack (gather) the new index arrays for later rendering
  color_array_from_selection ();
}

//***************************************************************************
// Plot_Window::color_array_from_selection() -- Fill the index arrays and 
// their associated counts.  Each array of indices will be rendered later 
// preceded by its own single call to glColor().
// 
// MCL XXX note this could be redone so that it all lives in handle_selection, 
// conceptually.  The updating can be done in one pass, I think.
//
void Plot_Window::color_array_from_selection()
{
  for (int i=0; i<NBRUSHES; i++) {
    brushes[i]->count = 0;
  }
  // Loop: Examine successive points to fill the index arrays and their
  // associated counts
  int set, count=0;
  for( int i=0; i<npoints; i++) {
    set = selected(i);
    count = brushes[set]->count++;
    indices_selected( set, count) = i;
  }
  nselected = npoints - brushes[0]->count;
  // assert(sum(number_selected(blitz::Range(0,nplots))) == (unsigned int)npoints);
  indexVBOsfilled = 0;
}

//***************************************************************************
// Plot_Window::draw_selection_information() -- Draw decorations for the 
// selected set in the window where the user is making the selection.
void Plot_Window::draw_selection_information()
{
  int draw_selection_box = 1;
  if( draw_selection_box) {
    glBlendFunc( GL_ONE, GL_ZERO);
    glLineWidth( 1.0);
    glColor4f( 0.25,0.25,0.75,0.0);
    glBegin( GL_LINE_LOOP);

    glVertex2f( xdown, ydown);
    glVertex2f( xtracked, ydown);
    glVertex2f( xtracked, ytracked);
    glVertex2f( xdown, ytracked);

    glEnd();
  }

  // Print selection statistics.  Should there should be a gui 
  // element controlling this?
  print_selection_stats();

  // done flagging selection for this plot
  selection_changed = 0;
}

//***************************************************************************
// Plot_Window::draw_data_points() -- If requested, draw the data
void Plot_Window::draw_data_points()
{
  // cout << "pw[" << index << "]: draw_data_points() " << endl;
  if ( !cp->show_points->value())return;

#ifdef ALPHA_TEXTURE
  glEnable(GL_ALPHA_TEST);
#endif //ALPHA_TEXTURE

  int z_bufferring_enabled = 0;
  int current_sprite = 0;

  // Are we plotting in two dimensions or three?
  if( cp->varindex3->value() != nvars) {
    if (cp->z_bufferring_button->value()) {
      glEnable( GL_DEPTH_TEST);
      glDepthFunc( GL_GREATER);
      z_bufferring_enabled = 1;
    }
  }

  // Tell the GPU where to find the vertices for this plot.
  if (use_VBOs) {
    // bind VBO for vertex data
    glBindBuffer(GL_ARRAY_BUFFER, index+1);

    // If the variables we are plotting were changed, then the vertex VBO must 
    // be updated to contain the correct vertex data, and it has to be done 
    // here, where the correct window and context are active.
    if (!VBOfilled) fill_VBO();

    glVertexPointer (3, GL_FLOAT, 0, BUFFER_OFFSET(0));
  } else {
    glVertexPointer (3, GL_FLOAT, 0, (GLfloat *)vertices.data()); 
  }

  // set the blending mode for this plot
  int blending_mode = cp->blend_menu->value();
  switch (blending_mode) {
  case Control_Panel_Window::BLEND_OVERPLOT:
    glBlendFunc(GL_ONE, GL_ZERO);
    break;
  case Control_Panel_Window::BLEND_OVERPLOT_WITH_ALPHA:
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,GL_SRC_ALPHA,GL_ONE_MINUS_DST_COLOR);
    break;
  case Control_Panel_Window::BLEND_BRUSHES_SEPARATELY:
    glEnable(GL_STENCIL_TEST);
    clear_stencil_buffer();
    clear_alpha_planes();
    glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
    break; 
  case Control_Panel_Window::BLEND_ALL_BRUSHES:
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);	// very good when compositing on black bkg w/ALPHA_TEXTURE #defined
    break;
#if 0
  case Control_Panel_Window::BLEND_ALL2:
    glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA); // old default, use with ALPHA_TEXTURE #undefined
    break;
  case Control_Panel_Window::BLEND_ALL3:
    // next is almost right for bending against light backgrounds with ALPHA_TEXTURE #defined
    // and it is OK (but not as good as the above with ALPHA_TEXTURE #undefined) for black backgrounds.
    // needs glAlphaFunc(GL_GREATER, 0.5);
    glBlendFuncSeparate(GL_SRC_COLOR, GL_DST_ALPHA, GL_SRC_ALPHA, GL_ONE);  
    break;
#endif
  default:
    assert (!"Impossible blending mode");
    break;
  }

  // Loop: Draw points for each brush, using that brush's properties.

  int first_brush=0, brush_step=+1;

  // we draw brushes in reverse order iff we are using stencil buffers to blend separately
  if (blending_mode == Control_Panel_Window::BLEND_BRUSHES_SEPARATELY) {
    first_brush=NBRUSHES-1; brush_step=-1;
  }
    
  for( int brush_num=0, brush_index=first_brush; brush_num<NBRUSHES; brush_num++, brush_index+=brush_step) {

    // don't draw nonselected points (brush[0]) if we are hiding nonselected points in this plot
    if (brush_index == 0 && (!show_deselected_button->value() || !cp->show_deselected_points->value())) {
      continue;
    }

    Brush *brush = brushes[brush_index];
    unsigned int count = brush->count;
    
    // If some points were selected in this set, render them
    if(count > 0) {

      // if we are using stencils (and hence drawing brushes in reverse order)...
      if (blending_mode == Control_Panel_Window::BLEND_BRUSHES_SEPARATELY) {
        glStencilFunc (GL_GEQUAL, brush_index+1, 0xFFFF);
        glStencilOp (GL_KEEP, GL_KEEP, GL_REPLACE);
      }

      // Set the pointsize for this brush (hard limit from 1 to 100)
      // note this is a combination of the brush's size and per-plot scaling
      float size = brush->pointsize->value() * powf(2.0, cp->size->value());
      if (cp->scale_points->value()) {
        size = magnification*size;
      }
      size = min(max(size,1.0F),100.0F);

      // alpa cutoff, usefull for soft brushes on light backgrounds.  This should perhaps
      // be per plot instead of per brush, or better yet it should go away.
      glAlphaFunc(GL_GREATER, brush->cutoff->value());
      
      // Set the sprite for this brush - the symbol used for plotting points.
      current_sprite = brush->symbol_menu->value();
      assert ((current_sprite >= 0) && (current_sprite < NSYMBOLS));
      switch (current_sprite) {
      case 0:
        enable_regular_points();
        glPointSize(size);
        break;
#if 0 // this should be executed based on run-time test iff GL_POINT_SPRITE is absent.
      case 1:
        enable_antialiased_points();
        glPointSize(size);
        break;
#endif
      default:
        enable_sprites(current_sprite);
        glPointSize(size+2); // sprites cover fewer pixels, in general
        break;
      }

      // set the color for this set of points
      float lum0 = cp->lum->value();
      float lum1 = pow2(brush->lum1->value()), lum2 = pow2(brush->lum2->value());
      float alpha = brush->alpha->value();
      // float alpha0 = brush->alpha0->value();
      double r = lum0*lum2*(brush->color_chooser->r()+lum1);
      double g = lum0*lum2*(brush->color_chooser->g()+lum1);
      double b = lum0*lum2*(brush->color_chooser->b()+lum1);
      double a = alpha;
      // cout << "plot " << index << ", brush " << brush->index << ", (r,g,b,a) = (" << r << ", " << g << ", " << b << ", " << a << ")" << endl;
      glColor4d(r,g,b,a);

      // then render the points
      if (use_VBOs) {
        assert (VBOinitialized && VBOfilled && indexVBOsinitialized && indexVBOsfilled) ;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, MAXPLOTS+1+brush_index); 
        glDrawElements( GL_POINTS, (GLsizei)count, GL_UNSIGNED_INT, BUFFER_OFFSET(0));
        // glDrawRangeElements( GL_POINTS, 0, npoints, count, GL_UNSIGNED_INT, BUFFER_OFFSET(0)); // why not use this, instead?
        // make sure we succeeded 
        CHECK_GL_ERROR("drawing points from VBO");
      }
      else {
        // Create an alias to slice
        blitz::Array<unsigned int, 1> tmpArray = indices_selected(brush_index, blitz::Range(0,npoints-1));
        unsigned int *indices = (unsigned int *) (tmpArray.data());
        // glDrawElements( GL_POINTS, count, GL_UNSIGNED_INT, indices);
        glDrawRangeElements( GL_POINTS, 0, npoints, count, GL_UNSIGNED_INT, indices);
      }
    }
  }
  // potentially turn off various gl state variables that are specific to this routine.
  if (z_bufferring_enabled) {
    glDisable( GL_DEPTH_TEST);
  }
  if (blending_mode == Control_Panel_Window::BLEND_BRUSHES_SEPARATELY) {
    glDisable( GL_STENCIL_TEST);
  }
  if (current_sprite > 0) {
    disable_sprites();
  }
#ifdef ALPHA_TEXTURE
  glDisable(GL_ALPHA_TEST);
#endif // ALPHA_TEXTURE
}

//***************************************************************************
// Plot_Window::compute_histogram( axis) -- If requested, compute equi-width 
// histogram for axis 'axis'.
//
// MCL XXX this should be split into two routines, one for computing the 
// histogram for all the points, the other for selected points, and _neither_ 
// of them needs to be called as often as they are.  The first only needs to 
// be called in extract_and_redraw(), and the second only when the selection 
// changes (but it must be called for all plots in that case), or when {x,y}bins 
// changes (in that case, only for one plot).  The way it is, a lot of extra 
// time is burned here if any histograms are being shown.
// Note - we could experiment with openGL histograms...
//
// MCL XXX also note that if we want to get rid of the vertices() instance 
// variable to save memory, which should be doable since vertices are copied 
// into VBOs, then we will have to do something else here.
void Plot_Window::compute_histogram( int axis)
{
  int marginal    = cp->show_histogram[axis]->menu()[Control_Panel_Window::HISTOGRAM_MARGINAL].value();
  int selection   = cp->show_histogram[axis]->menu()[Control_Panel_Window::HISTOGRAM_SELECTION].value();
  int conditional = cp->show_histogram[axis]->menu()[Control_Panel_Window::HISTOGRAM_CONDITIONAL].value();
  // MCL XXX presently, weighting is based on the z-axis variable.  Weighting variable should really be a pulldown of is own.
  int weighted    = cp->show_histogram[axis]->menu()[Control_Panel_Window::HISTOGRAM_WEIGHTED].value();
  if (!(marginal || selection || conditional)) {
    return;
  }

  // Get number of bins, initialize arrays, and set range
  int nbins = (int)(exp2(cp->nbins_slider[axis]->value()));
  if (nbins <= 0) return;
  blitz::Range BINS( 0, nbins-1);
  counts( BINS, axis) = 0.0;

  counts_selected( BINS, axis) = 0.0;
  // range is tweaked by (n+1)/n to get the "last" point into the correct bin.
  float range = (amax[axis] - amin[axis]) * ((float)(npoints+1)/(float)npoints); 

  // only count points that are being selected by the most recent brush
  Brush *bp = dynamic_cast <Brush*> (brushes_tab->value());
  assert (bp);
  int brush_index = bp->index; 

  for( int i=0; i<npoints; i++) {
    float x = vertices( i, axis);
    // MCL XXX presently, weighting is based on the z-axis variable.  Weighting variable should really be a pulldown of is own.
    float weight = weighted?vertices(i,2):1.0;
    int bin = (int)(floorf( nbins * ( ( x - amin[axis]) / range)));
    if( bin < 0) bin = 0;
    if( bin > nbins-1) bin = nbins-1;
    counts( bin, axis) += weight;
    if( selected( i) == brush_index) counts_selected( bin, axis)+=weight;
  }
  float maxcount = max(max(counts(BINS,axis)), 1.0f);
  
  // Normalize results.  
  if( npoints > 0) {
    counts(BINS,axis) = counts(BINS,axis) / (float)maxcount;
    counts_selected(BINS,axis) = counts_selected(BINS,axis) / (float)maxcount;
  }
}

//***************************************************************************
// Plot_Window::compute_histograms() -- Invoke compute_histogram to compute
// histograms for axes 0 and 1.
void Plot_Window::compute_histograms()
{
  compute_histogram(0);
  compute_histogram(1);
}

//***************************************************************************
// Plot_Window::draw_x_histogram( bin_counts, nbins) -- Draw x histogram
void Plot_Window::draw_x_histogram(const blitz::Array<float,1> bin_counts, const int nbins) {
  float x = amin[0];
  float xwidth = (amax[0]-amin[0]) / (float)(nbins);
  glBegin( GL_LINE_STRIP);
  for( int bin=0; bin<nbins; bin++, x+=xwidth) {
    glVertex2f( x, 0.0); // lower left corner
    glVertex2f( x, bin_counts( bin, 0));   // left edge
    glVertex2f( x+xwidth, bin_counts( bin, 0));   // top edge
    glVertex2f( x+xwidth,0.0);   // right edge 
  }
  glEnd();
}

//***************************************************************************
// Plot_Window::draw_y_histogram( bin_counts, nbins) -- Draw x histogram
void Plot_Window::draw_y_histogram(const blitz::Array<float,1> bin_counts, const int nbins) {
  float y = amin[1];
  float ywidth = (amax[1]-amin[1]) / (float)(nbins);
  glBegin( GL_LINE_STRIP);
  for( int bin=0; bin<nbins; bin++) {
    glVertex2f( 0.0, y);          
    glVertex2f(bin_counts(bin),y);   // bottom
    glVertex2f(bin_counts(bin), y+ywidth);   // right edge
    glVertex2f(0.0, y+ywidth);   // top edge 
    y+=ywidth;
  }
  glEnd();
}

//***************************************************************************
// Plot_Window::draw_histogram() -- If requested, draw histograms.
void Plot_Window::draw_histograms()
{
  // the various kinds of histograms we might show:
  int x_marginal    = cp->show_histogram[0]->menu()[Control_Panel_Window::HISTOGRAM_MARGINAL].value();
  int x_selection   = cp->show_histogram[0]->menu()[Control_Panel_Window::HISTOGRAM_SELECTION].value();
  int x_conditional = cp->show_histogram[0]->menu()[Control_Panel_Window::HISTOGRAM_CONDITIONAL].value();

  int y_marginal    = cp->show_histogram[1]->menu()[Control_Panel_Window::HISTOGRAM_MARGINAL].value();
  int y_selection   = cp->show_histogram[1]->menu()[Control_Panel_Window::HISTOGRAM_SELECTION].value();
  int y_conditional = cp->show_histogram[1]->menu()[Control_Panel_Window::HISTOGRAM_CONDITIONAL].value();

  // if no axis has any histograms enabled, return immediately
  if (! (x_marginal || x_selection || x_conditional || y_marginal || y_selection || y_conditional)) {
    return;
  }

  // note use of slider to control log_2 bincount
  int xbins = (int)exp2(cp->nbins_slider[0]->value());
  int ybins = (int)exp2(cp->nbins_slider[1]->value());

  // if no axes has bin count > zero, return immediately
  if (xbins <= 0 && ybins <= 0)
    return;

  // histograms base is this far from edge of window
  float hoffset = 0.1; 

  glPushMatrix();
  // histograms should cover pointclouds
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // x-axis histograms
  if ((x_marginal || x_selection || x_conditional) && xbins > 0) {
    blitz::Range BINS( 0, xbins-1);
    glLoadIdentity();
    glTranslatef( xzoomcenter*xscale, hoffset, 0);
    glScalef( xscale, yhscale*cp->hscale_slider[0]->value(), 1.0);
    glTranslatef( -xcenter, -1.0/(yhscale*cp->hscale_slider[0]->value()), 0.0);
    glTranslatef( -xzoomcenter, 0.0, 0);
    glTranslatef (0.0, 0.0, hoffset);
    
    // Draw x-axis histogram of all points.
    if (x_marginal) {
      glColor4f( 0.0, 0.5, 0.5, 1.0);
      draw_x_histogram (counts(BINS,0), xbins);
    }
    // Draw x-axis histogram of selected points
    if( nselected > 0 && x_selection) {
      glColor4f( 0.25, 1.0, 1.0, 1.0);
      draw_x_histogram (counts_selected(BINS,0), xbins);
    }
    // Draw scaled x-axis histogram of selected points ("conditional");
    if( nselected > 0 && x_conditional) {
      float yscale = max(counts(BINS,0))/max(counts_selected(BINS,0));
      blitz::Array<float,1> scaled_bin_counts(xbins);
      scaled_bin_counts = yscale*counts_selected(BINS,0);
      glColor4f( 0.5, 1.0, 1.0, 1.0);
      draw_x_histogram (scaled_bin_counts, xbins);
    }
  }

  // y-axis histograms
  if ((y_marginal || y_selection || y_conditional) && ybins > 0) {
    blitz::Range BINS( 0, ybins-1);
    glLoadIdentity();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glTranslatef( hoffset, yzoomcenter*yscale, 0);
    glScalef( xhscale*cp->hscale_slider[1]->value(), yscale, 1.0);
    glTranslatef( -1.0/(xhscale*cp->hscale_slider[1]->value()), -ycenter, 0.0);
    glTranslatef( 0.0, -yzoomcenter, 0);

    // Draw y-axis histogram of all points.
    if (y_marginal) {
      glColor4f( 0.0, 0.5, 0.5, 1.0);
      draw_y_histogram (counts(BINS,1), ybins);
    }
    // Draw y-axis histogram of selected points
    if( nselected > 0 && y_selection) {
      glColor4f( 0.25, 1.0, 1.0, 1.0);
      draw_y_histogram (counts_selected(BINS,1), ybins);
    }
    // Draw scaled y-axis histogram of selected points ("conditional");
    if( nselected > 0 && y_conditional) {
      float yscale = max(counts(BINS,1))/max(counts_selected(BINS,1));
      blitz::Array<float,1> scaled_bin_counts(ybins);
      scaled_bin_counts = yscale*counts_selected(BINS,1);
      glColor4f( 0.5, 1.0, 1.0, 1.0);
      draw_y_histogram (scaled_bin_counts, ybins);
    }

  }
  glPopMatrix();
}

//***************************************************************************
// Plot_Window::density_1D( a, axis) -- Compute marginal density estimate 
// along axis using equi-width histogram.  Input array a is over-written.
void Plot_Window::density_1D (blitz::Array<float,1> a, const int axis)
{
  // need to compute (but not necessarily display) the x-axis histogram if 
  // its not already there.
  if (!cp->show_histogram[axis]->value()) compute_histogram(axis);
  int nbins = (int)(exp2(cp->nbins_slider[axis]->value()));

  // Loop: For each point, find which bin its in (since that isn't saved in 
  // compute_histogram) and set the density estimate for the point 
  // equal to the bin count
  // range is tweaked by (n+1)/n to get the "last" point into the correct bin.
  float range = (amax[axis] - amin[axis]) * ((float)(npoints+1)/(float)npoints); 
  for( int i=0; i<npoints; i++) {
    int bin = (int)(floorf( nbins * ( ( vertices(i,axis) - amin[axis]) / range)));
    if( bin < 0) bin = 0;
    if( bin > nbins-1) bin = nbins-1;
    a(i) = counts( bin, axis)/(float)npoints;
  }
}

//***************************************************************************
// Plot_Window::transform_2d() -- If requested, transform data to 2D 
// sum-vs-difference or polar coordinates.
int Plot_Window::transform_2d()
{
  blitz::Range NPTS(0,npoints-1);

  if( cp->no_transform->value()) return 1;

  else if( cp->sum_vs_difference->value()) {
    blitz::Array <float,1> tmp1(npoints), tmp2(npoints);
    tmp1 = vertices(NPTS,0);
    tmp2 = vertices(NPTS,1);
    vertices(NPTS,0) = (sqrt(2.0)/2.0) * (tmp1 + tmp2);
    vertices(NPTS,1) = (sqrt(2.0)/2.0) * (tmp1 - tmp2);
  }
  else if( cp->cond_prop->value()) {
    blitz::Array <float,1> tmp1(npoints);
    tmp1 = vertices(NPTS,1);
    int nbins = (int)(exp2(cp->nbins_slider[0]->value()));
    cummulative_conditional (tmp1, x_rank, (npoints-1)/(nbins*2));
    vertices(NPTS,1) = tmp1;
  }
  else if( cp->fluctuation->value()) {
    blitz::Array <float,1> tmp1(npoints);
    tmp1 = vertices(NPTS,1);
    int nbins = (int)(exp2(cp->nbins_slider[0]->value()));
    fluctuation (tmp1, x_rank, (npoints-1)/(nbins*2));
    vertices(NPTS,1) = tmp1;
  }
  for (int i=0; i<2; i++) {
    wmin[i] = amin[i] = min(vertices(NPTS,i));
    wmax[i] = amax[i] = max(vertices(NPTS,i));
  }
  return 1;
}

// MCL XXX This should really be a templated function for blitz::Array<T, int rank>
// Doing it this way has to be faster than dst(i) = src((i+delta+npoints)%npoints) right?
void circular_shift (blitz::Array<float,1> dst, blitz::Array<float,1> src, const int shift)
{
  int delta = 0;
  if (shift == 0) {
    dst = src;
    return;
  } else if (shift > 0) {
    delta=shift;
    dst (blitz::Range(0,npoints-(delta+1))) = src (blitz::Range(delta,npoints-1));
    dst (blitz::Range(npoints-delta,npoints-1), 0) = src (blitz::Range(0,delta-1));
    return;
  } else {
    // shift < 0
    delta=-shift;
    dst (blitz::Range(0,delta-1)) = src (blitz::Range(npoints-delta,npoints-1));
    dst (blitz::Range(delta,npoints-1)) = src (blitz::Range(0,npoints-(delta+1)));
    return;
  }
}

//***************************************************************************
// Plot_Window::normalize( a, a_rank0, style, axis_index) --  Apply
// normalization of the requested style.
int Plot_Window::normalize(
  blitz::Array<float,1> a, 
  blitz::Array<int,1> a_rank0, 
  int style, int axis_index)
{
  blitz::Range NPTS(0,npoints-1);

  int delta = (int)cp->offset[axis_index]->value();
  // a_rank holds either the shifted rank indices, or unshifted when delta==0
  // necessary for exotic normalizations and/or (time)-shifted data.
  blitz::Array<int,1> a_rank(NPTS);
  if (delta == 0) {
    a_rank.reference(a_rank0);
  } else {
    a_rank = (a_rank0 + npoints - delta) % npoints;
  }

#ifdef CHECK_FOR_NANS_IN_NORMALIZATION
  blitz::Array<int,1> inrange(npoints);
  inrange = where( ((a(NPTS) < MAXFLOAT) && (a(NPTS) > -MAXFLOAT)), 1, 0);
  float tmin = min(where(inrange,a(NPTS), MAXFLOAT));
  float tmax = max(where(inrange,a(NPTS),-MAXFLOAT));
#endif // CHECK_FOR_NANS_IN_NORMALIZATION

  float mu,sigma,partial_rank;
  
  switch( style) {
  case Control_Panel_Window::NORMALIZATION_NONE:
    amin[axis_index] = -1;
    amax[axis_index] = +1;
    return 1;

  case Control_Panel_Window::NORMALIZATION_MINMAX:
    amin[axis_index] = tmin[axis_index];
    amax[axis_index] = tmax[axis_index];
    return 1;

  // All positive data fits in window, zero at "left" of axis.
  case Control_Panel_Window::NORMALIZATION_ZEROMAX: 
    amin[axis_index] = 0.0;
    amax[axis_index] = tmax[axis_index];
    return 1;

  // All data fits in window w/zero at center of axis
  case Control_Panel_Window::NORMALIZATION_MAXABS:  
  {
    float tmaxabs = fmaxf(fabsf(tmin[axis_index]),fabsf(tmax[axis_index]));
    if( tmaxabs != 0.0) {
      amin[axis_index] = -tmaxabs;
      amax[axis_index] = tmaxabs;
    }
    return 1;
  }

  // Median at center of axis, axis extends to include at least 99% of data
  case Control_Panel_Window::NORMALIZATION_TRIM_1E2:
  {
    float trim = 1e-2;
    amin[axis_index] = a(a_rank((int) ((0.0 + (0.5*trim))*npoints)));
    amax[axis_index] = a(a_rank((int) ((1.0 - (0.5*trim))*npoints)));
    return 1;
  }

  // Median at center of axis, axis extends to include at least 99.9% of data
  case Control_Panel_Window::NORMALIZATION_TRIM_1E3:
  {
    float trim = 1e-3;
    amin[axis_index] = a(a_rank((int) ((0.0 + (0.5*trim))*npoints)));
    amax[axis_index] = a(a_rank((int) ((1.0 - (0.5*trim))*npoints)));
    return 1;
  }

  // Mean at center of axis, axis extends to +/- 3*sigma
  // MCL XXX behaves incorrectly with axis offsets
  case Control_Panel_Window::NORMALIZATION_THREESIGMA:  
    mu = mean(a(NPTS));
    sigma = sqrt((1.0/(float)npoints)*sum(pow2(a(NPTS)-mu)));
    DEBUG (cout << "mu, sigma = " << mu << ", " << sigma << endl);
    if( finite(mu) && (sigma!=0.0)) {
      amin[axis_index] = mu - 3*sigma;
      amax[axis_index] = mu + 3*sigma;
    }
    return 1;

  // Log of negative numbers get assigned a value of zero.
  case Control_Panel_Window::NORMALIZATION_LOG10: 
    if( tmin[axis_index] <= 0.0) {
      cerr << "Warning: "
           << "attempted to take logarithms of nonpositive "
           << " numbers. Their logs were set to zero." 
           << endl;
    }
    a(NPTS) = where( a(NPTS) > 0, log10(a(NPTS)), 0);
    amin[axis_index] = min(a(NPTS));
    amax[axis_index] = a(a_rank(npoints-1));
    return 1;

  // Simple sigmoid, (-inf,0,+inf) -> (-1,0,+1)
  case Control_Panel_Window::NORMALIZATION_SQUASH: 
    a(NPTS) = a(NPTS)/(1+abs(a(NPTS)));
    amin[axis_index] = a(a_rank(0));
    amax[axis_index] = a(a_rank(npoints-1));
    return 1;

  // Replace each value with its rank, equal values get sequential rank
  // according to original input order
  case Control_Panel_Window::NORMALIZATION_RANK:
    for( int i=0; i<npoints; i++) {
      a(a_rank(i)) = (float)(i+1);
    }
    amin[axis_index] = 1.0;
    amax[axis_index] = (float)(npoints);
    return 1;
      
  // Replace each value with its rank, equal values get equal rank
  case Control_Panel_Window::NORMALIZATION_PARTIAL_RANK:
  {
    partial_rank = 1.0;
    float previous = a(a_rank(0)); 

    // Loop: Subsequent values, if equal, get equal rank.  Otherwise, they 
    // get previous + 1.
    for( int i=0; i<npoints; i++) {
      if ( a(a_rank(i)) > previous ) {
        previous = a(a_rank(i));
        partial_rank += 1.0;
      }
      a(a_rank(i)) = partial_rank;
    }
    amin[axis_index] = 1.0;
    amax[axis_index] = partial_rank;
    return 1;
  }
      
  // Gaussianize the data, mapping the old median to 0 in the new Gaussian N(0,1).
  case Control_Panel_Window::NORMALIZATION_GAUSSIANIZE: 
    for( int i=0; i<npoints; i++) {
      a(a_rank(i)) = (1.0/5.0) * (float)gsl_cdf_ugaussian_Pinv((double)(float(i+1) / (float)(npoints+2)));
    }
    amin[axis_index] = -1.0;
    amax[axis_index] = +1.0;
    return 1;

  // randomize the data - randomly shuffle the points.
  case Control_Panel_Window::NORMALIZATION_RANDOMIZE: 
  {
    // this is crufty because gsl doesn't know about 1D arrays with non-unit strides.
    blitz::Array<float,1> acopy = a;
    // initialize the temporary indices to be sequential.  
    blitz::firstIndex ident;   
    blitz::Array<int,1> tmp_indices(npoints);
    tmp_indices = ident;

    // make a random permutation of a();
    gsl_ran_shuffle( vp_gsl_rng, tmp_indices.data(), npoints, sizeof(int));
    for( int i=0; i<npoints; i++) {
      a(i) = acopy(tmp_indices(i));
    }
  }
  return 1;

  // Default: do nothing
  default:
    return 0;
  }
}

//***************************************************************************
// Plot_Window::compute_rank() -- Order data for normalization and generation 
// of histograms
void Plot_Window::compute_rank(int var_index)
{
  if (ranked(var_index)) {
    // We have a rank "cache hit"
    return; 
  }
  else {
    blitz::Range NPTS(0,npoints-1);

    // The blitz copy constructor aliases the RHS,
    // So this next statement just creates a new view of the rhs.
    blitz::Array<int,1> a_ranked_indices = ranked_points(var_index, NPTS); 
    
    // initializealize the ranked indices to be sequential.  The sort (following) will
    // permute them into the correct order.
    blitz::firstIndex ident;    // MCL XXX don't we have a global holding this?
    a_ranked_indices = ident;

    // the sort method myCompare() needs a global alias (tmp_points) to the 
    // array data holding the sort key.  We can't use the copy contructor here 
    // since tmp_points was constructed in pre-main.
    // Lucky for us, blitz provides the reference() method for this purpose.
    tmp_points.reference(points(var_index, NPTS));
    int *lo = a_ranked_indices.data(), *hi = lo + npoints;
    std::stable_sort(lo, hi, MyCompare());
    ranked(var_index) = 1;  // now we are ranked
    return;
  }
}



//***************************************************************************
// Plot_Window::extract_data_points() -- Extract column labels and data for a 
// set of axes, rank (order) and normalize and scale data, compute histograms, 
// and compute axes scales.
//
// MCL XXX this routine (and others) could be refactored to loop over the axes
// instead of having so much code replicated for each axis.
//
// MCL XXX this routine needs to be split into separate routines that:
//  1) change one (or more) of the axes for a plot
//  2) change the normalization of an axis (or axes) but only when necessary
//  3) change the axis "offset(s)" when ploting when necessary
//  4) keep track of the ranked version of (potentially offset) axes.
// This will not only make the code cleaner, but is necessary to avoid spurious
// renormalizations (which waste time), spurious calls to reset_view() (which
// can change the view for no good reason) and breakage in the rank-dependent
// normalizations and transformations when operating on an axis with offset != 0.
// Also, offsets should get reset to zero when an axis is changed.
int Plot_Window::extract_data_points ()
{
  // Get the labels for the plot's axes
  int axis0 = (int)(cp->varindex1->mvalue()->user_data());
  int axis1 = (int)(cp->varindex2->mvalue()->user_data());
  int axis2 = (int)(cp->varindex3->mvalue()->user_data());

  xlabel = column_labels[ axis0];
  ylabel = column_labels[ axis1];
  if( axis2 != nvars) zlabel = column_labels[ axis2];
  else zlabel = "";
  
  // Define a Range operator with which to extract subarrays
  blitz::Range NPTS( 0, npoints-1);

  // Order data to prepare for normalization and scaling and 
  // report progress
  if (be_verbose) {
    cout << "Plot_Window::extract_data_points: plot[ " 
         << row << ", " << column << "]" <<endl;
    cout << " pre-normalization: " << endl;
  }
  // Rank points by x-axis value
  compute_rank(axis0);
  x_rank.reference(ranked_points(axis0, NPTS));
  tmin[0] = points( axis0, x_rank( 0));
  tmax[0] = points( axis0, x_rank( npoints-1));
  if (be_verbose) {
    cout << "  x-axis( " << xlabel
         << "): min[ " << x_rank( 0) << "] = "
         << tmin[0]
         << ", max[ " << x_rank( npoints-1) << "] = "
         << tmax[0] << endl;
  }
  // Rank points by y-axis value
  compute_rank(axis1);
  y_rank.reference(ranked_points(axis1, NPTS));
  tmin[1] = points( axis1, y_rank( 0));
  tmax[1] = points( axis1, y_rank( npoints-1));
  if (be_verbose) {
    cout << "  y-axis( " << ylabel
         << "): min[ " << y_rank( 0) << "] = "
         << tmin[1]
         << ", max[ " << y_rank( npoints-1) << "] = "
         << tmax[1] << endl;
  }
  // If z-axis was specified, rank points by z-axis value
  if( axis2 != nvars) {
    compute_rank(axis2);
    z_rank.reference(ranked_points(axis2, NPTS));
    tmin[2] = points( axis2, z_rank( 0));
    tmax[2] = points( axis2, z_rank( npoints-1));
    if (be_verbose) {
      cout << "  z-axis( " << zlabel
           << "): min[ " << z_rank( 0) << "] = "
           << tmin[2]
           << ", max[ " << z_rank( npoints-1) << "] = "
           << tmax[2] << endl;
    }
  }
  if (be_verbose) cout << endl;

  // OpenGL vertices, vertex arrays, and VBOs need to have their x, y, and z coordinates
  // interleaved i.e. stored in adjacent memory locations:  x[0],y[0],z[0],x[1],y[1],z[1],.....
  // This is not, unfortunately, how the raw data is stored in the blitz points() array.
  // So we copy the appropriate data "columns" from the points() array into the appropriate
  // components of the vertex() array.
  //
  // Though this copying takes up time and memory, it is OK since we almost certainly want to
  // normalize and/or transform the vertices prior rendering them.  Since we don't want to
  // transform and/or normalize (i.e. clobber) the "original" data, we transform and/or normalize
  // using aliases to the vertex data.  Since the vertex data are copies (not aliases) of the
  // original uncorrupted points() data, this works out fine.

  // MCL XXX - there is no need to copy and normalize all axes if only one has changed... Oh, well...

  // copy (via assignment) the appropriate columns of points() data to corresponding components of vertex() array,
  // with circular offset(s), if requested (experimental).
  // MCL XXX need to do away with axis0, axis1, and axis2 & replace with axis_index[].
  int delta = (int)cp->offset[0]->value();
  if (delta == 0) {
    vertices( NPTS, 0) = points( axis0, NPTS);
  } else {
    circular_shift (vertices(NPTS,0), points(axis0,NPTS), delta);
  }
  delta = (int)cp->offset[1]->value();
  if (delta == 0) {
    vertices( NPTS, 1) = points( axis1, NPTS);
  } else {
    circular_shift (vertices(NPTS,1), points(axis1,NPTS), delta);
  }

  // if z-axis is set to "-nothing-" (which it is, by default), then all z=0.
  if( axis2 == nvars)    {
    vertices( NPTS, 2) = 0.0;
  } else {
    // MCL XXX offset not supported for z axis (yet).
    vertices( NPTS, 2) = points( axis2, NPTS);
  }
  
  // create aliases to newly copied vertex data for normalization & transformation.
  blitz::Array<float,1> xpoints = vertices( NPTS, 0); 
  blitz::Array<float,1> ypoints = vertices( NPTS, 1);
  blitz::Array<float,1> zpoints = vertices( NPTS, 2);

  // Apply the normalize() method to normalize and scale the plot's data 
  (void) normalize( xpoints, x_rank, cp->x_normalization_style->value(), 0);
  (void) normalize( ypoints, y_rank, cp->y_normalization_style->value(), 1);
  if( axis2 != nvars) {
    (void) normalize( zpoints, z_rank, cp->z_normalization_style->value(), 2);
  } else {
    amin[2] = -1.0;
    amax[2] = +1.0;
  }

  // VBO will have to be updated to hold the new vertices in draw_data_points(), 
  // so we set a flag.  We can't update the VBO now, since we can't call openGL 
  // functions from within an fltk callback.
  VBOfilled = false;

  // Apply 2D data transformations, if any are active.
  (void) transform_2d();

  // Since we're showing new data, make sure none of it gets clipped.
  // MCL XXX - this should be done on a per-axis basis and only when necessary
  // and not at all when simply changeing an axis offset.
  reset_view();

  compute_histograms();
  return 1;
}

//***************************************************************************
// Define STATIC methods

//***************************************************************************
// Plot_Window::upper_triangle_incr( i, j, n) -- STATIC method to increment 
// the row and column indices, (i,j), to traverse an upper triangular matrix 
// by moving "down and to the right" with wrapping.  A static method used by 
// Plot_Window::change_axes and in the body of the main routine to pick
// new axis automatically (and stupidly) e.g. when the "change axes" button is pressed.
void Plot_Window::upper_triangle_incr( int &i, int &j, const int n)
{
  i++;
  j++;
  if (i==n && j==n) 
    {
      i = 0;
      j = 1;
    }
  else if (j==n) 
    {
      int d = j-i;
      d++;
      i = 0;
      j = d;
      if (j>=n) 
        {
          i = n-1;
          j = 0;
        }
    }
  else if (i==n) 
    {
      int d = i-j;
      d--;
      i = d;
      j = 0;
    }
  assert( i >= 0);
  assert( j >= 0);
  assert( i < n);
  assert( j < n);
}

//***************************************************************************
// Plot_Window::redraw_all_plots( p) -- STATIC method that invokes methods to 
// redraw all plots cylically, starting with plot p.  This is a static method 
// used by class Plot_Window and by the npoints_changed method in the main 
// routine.
void Plot_Window::redraw_all_plots( int p)
{
  DEBUG( cout << "in redraw_all_plots(" << p << ")" << endl ) ;

  // Redraw all plots cyclically, starting with plot p.  This p is important, 
  // since the draw() routine for a plot handles the selection region, and the 
  // active plot (the one where we are making the selection) must update the 
  // selected set and set arrays *before* all the other plots get redrawn.  
  // Ugh.  Also, they are queued in reverse order, since is the order in which
  // fltk will actually draw() them (most recently defined gets drawn first).
  for( int i=0; i<nplots; i++) {
    int j = (p+i)%nplots;
    assert (j>=0);
    assert (j<nplots);
    pws[j]->compute_histograms();
    pws[j]->redraw();
    pws[j]->needs_redraw = 0;
  }
  
  // R100_FIXES: Fix for WIN32 'slow-handler' bug.  If we could get rid of 
  // this call to Fl::flush, the WIN32 version would be faster.
  #ifdef __WIN32__
    Fl::flush();
  #endif // __WIN32__
}

//***************************************************************************
// Plot_Window::run_timing_test() -- Acquire information for timing tests
void Plot_Window::run_timing_test()
{
  const int nframes = 10;
  struct timeval tp;

  (void) gettimeofday(&tp, (struct timezone *)0);
  double start_time = (double)tp.tv_sec + 1.0E-6*(double)tp.tv_usec;
  
  for (int i=0; i<nframes; i++) {
    invert_selection();
    Fl::check();  // this flushes all the pending redraws.
  }
  
  (void) gettimeofday(&tp, (struct timezone *)0);
  double end_time = (double)tp.tv_sec + 1.0E-6*(double)tp.tv_usec;

  double elapsed_time = end_time - start_time;
  double fps = (double)nframes/elapsed_time;
  cout << "Timing test results: " <<  fps 
       << " frames/sec, " << (double)npoints*nplots*fps 
       << " vertices/sec " << endl;
}

//***************************************************************************
// Plot_Window::delete_selection( p) -- STATIC method to delete selected 
// points.  This is a static method used only by class Plot_Window.
void Plot_Window::delete_selection( Fl_Widget *o)
{
  blitz::Range NVARS(0,nvars-1);
  int ipoint=0;
  for( int n=0; n<npoints; n++) {
    if( selected( n) < 0.5) {
      points( NVARS, ipoint) = points( NVARS, n);
      ipoint++;
    }
  }

  // KLUDGE: If no points remain, reload the first two points
  // to avoid overflows
  if( ipoint < 2) {
    points( NVARS, 0) = points( NVARS, 0);
    points( NVARS, 1) = points( NVARS, 1);
    ipoint = 2;
    cerr << " -WARNING: tried to delete every data point, first two points retained." << endl;
    sErrorMessage = "Tried to delete every data point, first two points retained.";
  }
  
  // If some point(s) got deleted, everyone's ranking needs to 
  // be recomputed
  if( ipoint != npoints)  {
    ranked = 0;  

    npoints = ipoint;
    // npoints_slider->bounds(1,npoints);
    // npoints_slider->value(npoints);

    clear_selections( (Fl_Widget *) NULL);
  
    for( int j=0; j<nplots; j++) {
      cps[j]->extract_and_redraw();
    }
  }
}

//***************************************************************************
// Plot_Window::invert_selection() -- STATIC method to invert selected and 
// nonselected points.  This is a static method used only by class 
// Plot_Window.
void Plot_Window::invert_selection ()
{
  if (!selection_is_inverted) {
    // save "true" selection
    saved_selection = selected;

    // create something like an inverse in its place:
    //  paint nonselected points with current brush index, unless
    //  current brush is 0, in which case paint them with 1.
    Brush  *current_brush = (Brush *)NULL;
    current_brush =  dynamic_cast <Brush*> (brushes_tab->value());
    assert (current_brush);
    if (current_brush->index != 0) {
      selected = where(selected, 0, current_brush->index);
    } else {
      selected = where(selected, 0, 1);
    }
    selection_is_inverted = true;
    // cout << "selection inverted" << endl;
  } 
  else {
    // restore what we saved when inverting
    // XXX this clobbers any brushing done while the selection is inverted
    selected = saved_selection;
    selection_is_inverted = false;
    // cout << "selection restored" << endl;
  }

  previously_selected = selected;
  nselected = npoints-nselected;

  // Recolor all points using the new selection and redraw
  pws[ 0]->color_array_from_selection();
  redraw_all_plots(0);
}

//***************************************************************************
// Plot_Window::toggle_display_selected( *o) -- STATIC method to toggle colors 
// of selected and unselected points. This is a static method used only by 
// class Plot_Window.
void Plot_Window::toggle_display_deselected( Fl_Widget *o)
{
  // Toggle the value of the button manually, but only if we were called via a
  // keypress in a plot window.  Shouldn't there be an easier way?
  if( o == NULL)
    show_deselected_button->value( 1 - show_deselected_button->value());

  // recolor all points using the correct "color table" and redraw
  pws[0]->color_array_from_selection();
  redraw_all_plots (0);
}

//***************************************************************************
// Plot_Window::initialize_selection() -- STATIC method to clear all selections
// without doing anything else that might lose the context.  This is a static 
// method used from main() during intialization and by 
// Plot_Window::clear_selections.
void Plot_Window::initialize_selection()
{
  // Loop: Reset selection box for successive plots.
  for( int i=0; i<nplots; i++) {
    pws[i]->reset_selection_box();
  }
  // all points start out unselected (i.e. rendered using brush[0]);
  brushes[0]->count = npoints; 
  // no other brushes render anything to start out.
  for (int i=1; i<NBRUSHES; i++) {
    brushes[i]->count = 0;
  }
  reset_selection_arrays ();
}

//***************************************************************************
// Plot_Window::clear_selections( *o) -- STATIC method to clear all selections, 
// resetall selected sets, and redraw all plots.  This is a static method used 
// only by class Plot_Window.
void Plot_Window::clear_selections( Fl_Widget *o)
{
  initialize_selection();
  pws[0]->color_array_from_selection(); // So, I'm lazy.
  redraw_all_plots (0);
}


// Methods to enable drawing with points (as opposed to point sprites)

//***************************************************************************
// Plot_Window::enable_regular_points() --
void Plot_Window::enable_regular_points ()
{
  disable_sprites();
  glDisable( GL_POINT_SMOOTH);
}

//***************************************************************************
// Plot_Window::enable_antialiased_points() --
void Plot_Window::enable_antialiased_points ()
{
  disable_sprites();
  glEnable( GL_POINT_SMOOTH);
  glHint( GL_POINT_SMOOTH_HINT,GL_NICEST);
}


//***************************************************************************
// Initialize variables and state for use with point sprites

//***************************************************************************
// Plot_Window::initialize_sprites() -- Invoke OpenGL routines to initialize
// sprites
void Plot_Window::initialize_sprites()
{
  glEnable( GL_TEXTURE_2D);
  glGenTextures( NSYMBOLS, spriteTextureID);
  make_sprite_textures ();
  for (int i=0; i<NSYMBOLS; i++) {
#ifdef ALPHA_TEXTURE
    GLfloat rgb2rgba[16] = {
      1, 0, 0, 1/3.0,
      0, 1, 0, 1/3.0,
      0, 0, 1, 1/3.0,
      0, 0, 0, 0
    };
    glMatrixMode(GL_COLOR);
    glLoadMatrixf(rgb2rgba);
    glMatrixMode(GL_MODELVIEW);
    glBindTexture( GL_TEXTURE_2D, spriteTextureID[i]);
    gluBuild2DMipmaps( GL_TEXTURE_2D, GL_INTENSITY, spriteWidth, spriteHeight, GL_RGB, GL_UNSIGNED_BYTE, spriteData[i]);
#else // ALPHA_TEXTURE
    glBindTexture( GL_TEXTURE_2D, spriteTextureID[i]);
    gluBuild2DMipmaps( GL_TEXTURE_2D, GL_LUMINANCE_ALPHA, spriteWidth, spriteHeight, GL_RGB, GL_UNSIGNED_BYTE, spriteData[i]);
#endif // ALPHA_TEXTURE
    CHECK_GL_ERROR( "initializing sprite texture mipmaps");
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    CHECK_GL_ERROR( "initializing sprite texture parameters");
  }
  sprites_initialized = 1;
  cout << "Textures initialized!" << endl;
  glDisable( GL_TEXTURE_2D);
}
    
//***************************************************************************
// Plot_Window::enable_sprites() -- Invoke OpenGL routines to enable sprites
void Plot_Window::enable_sprites(int sprite)
{
  glDisable( GL_POINT_SMOOTH);
  if (!sprites_initialized)
    initialize_sprites();
  glEnable( GL_TEXTURE_2D);
  glEnable( GL_POINT_SPRITE);
  assert ((sprite >= 0) && (sprite < NSYMBOLS));
  glBindTexture( GL_TEXTURE_2D, spriteTextureID[sprite]);
  glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glTexEnvf( GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE );
}

//***************************************************************************
// Plot_Window::clear_alpha_planes() -- Those filthy alpha planes!  It seems 
// that no matter how hard you try, you just can't keep them clean!
void Plot_Window::clear_alpha_planes()
{
  glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
  glClearColor( 0.0, 0.0, 0.0, 0.0);
  glClear( GL_COLOR_BUFFER_BIT);
  glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

//***************************************************************************
// Plot_Window::clear_stencil_buffer() -- Clear the stebcil buffer
void Plot_Window::clear_stencil_buffer()
{
#if 0
  // XXX HAH!  Here's the bug:  Plot 0 has 8 stencil bits.  The others have none!
  GLint sbits = 0;
  glGetIntegerv(GL_STENCIL_BITS, &sbits );
  cout << "clearing (" << sbits << " bit) stencil buffer for plot " << index << endl;
#endif
  glClearStencil((GLint)0);
  glClear(GL_STENCIL_BUFFER_BIT);
}

//***************************************************************************
// Plot_Window::disable_sprites() -- Invoke OpenGL routines to disable sprites
void Plot_Window::disable_sprites()
{
  glDisable( GL_TEXTURE_2D);
  glDisable( GL_POINT_SPRITE);
}

//***************************************************************************
// Define methods to use vertex buffer objects (VBOs)

//***************************************************************************
// Plot_Window::initialize_VBO() -- Initialize VBO for this window
void Plot_Window::initialize_VBO()
{
  // Create a VBO. Index 0 is reserved.
  if (!VBOinitialized) {
    glBindBuffer(GL_ARRAY_BUFFER, index+1);  
    CHECK_GL_ERROR ("creating VBO");

    // Reserve enough space in openGL server memory VBO to hold all the 
    // vertices, but do not initilize it.
    glBufferData( GL_ARRAY_BUFFER, (GLsizeiptr) npoints*3*sizeof(GLfloat), (void *)NULL, GL_DYNAMIC_DRAW);

    // Make sure we succeeded 
    CHECK_GL_ERROR ("initializing VBO");
    cerr << " initialized VBO for plot window " << index << endl;
    VBOinitialized = 1;
  }
}
 
//***************************************************************************
// Plot_Window::fill_VBO() -- Fill the VBO for this window
void Plot_Window::fill_VBO()
{
  if (!VBOfilled) {
    glBindBuffer(GL_ARRAY_BUFFER, index+1);  
    void *vertexp = (void *)vertices.data();
    glBufferSubData( GL_ARRAY_BUFFER, (GLintptr) 0, (GLsizeiptr) (npoints*3*sizeof(GLfloat)), vertexp);
    CHECK_GL_ERROR("filling VBO");
    VBOfilled = true;
  }
}

//***************************************************************************
// Plot_Window::initialize_indexVBO() -- Initialize the 'index VBO' that
// holds indices of selected (or non-selected) points.
// MCL XXX index VBOs hould probably be handled by the Brush class.
void Plot_Window::initialize_indexVBO(int set)
{
  // There is one shared set of index VBOs for all plots.
  //  indexVBO bound to MAXPLOTS+1 holds indices of nonselected (brushes[0]) points
  //  indexVBO bound to MAXPLOTS+2 holds indices of points selected by brushes[1], etc.
  glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, MAXPLOTS+set+1);  // a safe place....
  glBufferData( GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr) (npoints*sizeof(GLuint)), (void*) NULL, GL_DYNAMIC_DRAW);
}

//***************************************************************************
// Plot_Window::initialize_indexVBOs() -- Initialize set of index VBOs
void Plot_Window::initialize_indexVBOs() 
{
  if (!indexVBOsinitialized) {
    for (int set=0; set<NBRUSHES; set++) {
      initialize_indexVBO(set);
    }
    indexVBOsinitialized = 1;
  }
}

//***************************************************************************
// Plot_Window::fill_indexVBO() -- Fill the index VBO
void Plot_Window::fill_indexVBO(int set)
{
  if (brushes[set]->count > 0) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, MAXPLOTS+set+1);
    // Create an alias to slice
    blitz::Array<unsigned int, 1> tmpArray = indices_selected( set, blitz::Range(0,npoints-1));
    unsigned int *indices = (unsigned int *) (tmpArray.data());
    glBufferSubData( GL_ELEMENT_ARRAY_BUFFER, (GLintptr) 0, (GLsizeiptr) (brushes[set]->count*sizeof(GLuint)), indices);
    // make sure we succeeded 
    CHECK_GL_ERROR("filling index VBO");
  }
}

//***************************************************************************
// Plot_Window::fill_indexVBOs() -- Fill all the index VBOs
void Plot_Window::fill_indexVBOs() 
{
  if (!indexVBOsfilled) {
    for (int set=0; set<NBRUSHES; set++) {
      fill_indexVBO(set);
    }
    indexVBOsfilled = 1;
  }
}

//***************************************************************************
// Define global methods.  NOTE: Is it a good idea to do this here rather 
// than global_definitions.h?

//***************************************************************************
// moving_average( a, indices, half_width) -- Global method to calculate 
// moving averages of BLITZ arrays
void moving_average( 
  blitz::Array<float,1> a, const blitz::Array<int,1> indices, 
  const int half_width)
{
  blitz::Array<float,1> tmp(npoints), a2(npoints);
  tmp = 0;

  // Loop: permute a into a2 using the order specified by the indices array
  for( int i=0; i<npoints; i++) a2(i) = a(indices(i));

  // Loop: form moving average (inefficiently)
  for( int i=half_width; i<npoints-half_width; i++)
    for( int j=-half_width; j<=half_width; j++)
      tmp(i) += a2(i+j);

  // Loop: clean up elements near left and right edges
  for( int i=0; i<half_width; i++) {
    tmp(i) = tmp(half_width);
    tmp(npoints-(i+1)) = tmp(npoints-(half_width+1));
  }
  
  // Loop: unpermute and return moving average in a()
  for( int i=0; i<npoints; i++)
    a(indices(i)) = tmp(i)/(float)(2*half_width+1);
}

//***************************************************************************
// cummulative_conditional( a, indices, half_width) -- Global method to
// approximate the cummulative conditional probability of one array using 
// the (rank of) another array as the conditioning variable.  Input array a 
// is over-written.
// Note: Too slow.  There is obviously a clever, incremental way of doing 
// this more efficiently, such as by storing the elements in the current 
// window in an STL sorted container, and incrementally updating it as the 
// window slides.  (or doing the whole thing on the GPU?).
void cummulative_conditional(
  blitz::Array<float,1> a, const blitz::Array<int,1> indices, 
  const int half_width)
{
  // If parameters are bogus then quit
  if (half_width < 1 || half_width > (npoints-1)/2) return;

  // Loop: use sliding window in rank-ordered conditioning variable, window 
  // centered on index i. 
  blitz::Array<float,1> tmp(npoints);
  tmp = 0;
  for (int i=0; i<npoints; i++) {
    // find leftmost and rightmost index elements of conditioning variable
    int left =max(i-half_width,0);
    int right=min(i+half_width,npoints-1);
    
    // loop from leftmost to rightmost element in sliding window and determine 
    // conditional cummulative probablility (e.g. rank within the window) of 
    // the appropriate element from a(), which lies at the center of the 
    // window.
    float rank = 0;
    for (int j=left; j<=right; j++) {
      if (a(indices(i)) > a(indices(j)))
        rank++;
    }
    tmp(i) = rank/(float)(right-left);
    // cout << "i, left, right indices(i) rank = " << i << " " << left << " " << right << " " << rank << " " << indices(i) << endl;
  }

  // Loop: Unpermute and return in a
  for (int i=0; i<npoints; i++) a(indices(i)) = tmp(i);
}

//***************************************************************************
// fluctuation: relative difference between a(i) and local average of a.
// "local" is defined by rank passed in in indices.
void fluctuation(
  blitz::Array<float,1> a, const blitz::Array<int,1> indices, 
  const int half_width)
{
  // If parameters are bogus then quit
  if (half_width < 1 || half_width > (npoints-1)/2) return;

  // Loop: use sliding window in rank-ordered conditioning variable, window 
  // centered on index i. 
  blitz::Array<float,1> tmp(npoints);
  tmp = 0;
  for (int i=0; i<npoints; i++) {
    // find leftmost and rightmost index elements of conditioning variable
    int left =max(i-half_width,0);
    int right=min(i+half_width,npoints-1);
    
    // could do fast incremental update instead of nested loop here.
    float mean = 0;
    for (int j=left; j<=right; j++) {
      mean += a(indices(j));
    }
    mean /= (float)(right-left);
    tmp(i) = (a(indices(i))-mean)/mean;
    // cout << "i, left, right indices(i) rank = " << i << " " << left << " " << right << " " << rank << " " << indices(i) << endl;
  }

  // Loop: Unpermute and return in a
  for (int i=0; i<npoints; i++) a(indices(i)) = tmp(i);
}

