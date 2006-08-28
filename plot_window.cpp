// viewpoints - interactive linked scatterplots and more.
// copyright 2005 Creon Levit, all rights reserved.
//*****************************************************************
// File name: plot_window.cpp
//
// Class definitions:
//   plot_window -- Plot window
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
// Purpose: Source code for <plot_window.h>
//
// Author: Creon Levit   unknown
// Modified: P. R. Gazis  18-JUL-2006
//*****************************************************************

// Include the necessary include libraries
#include "include_libraries_vp.h"

// Include globals
#include "global_definitions_vp.h"

// Include associated headers and source code
#include "plot_window.h"
#include "control_panel_window.h"

// initialize static data members for class plot_window::
//

// Initial number of plot windows
int plot_window::count = 0;

// Initial fraction of the window to be used for showing (normalized) data
float const plot_window::initial_pscale = 0.8; 

// color for points (modified per point by texture rgba)
GLfloat plot_window::pointscolor[4] = { 1, 1, 1, 1};

// Initialize color for deselected points
double plot_window::r_deselected=1.0;
double plot_window::g_deselected=0.01;
double plot_window::b_deselected=0.01;

// "color tables" used for coloring points based on which plot (if any) they are selected in.
// see: plot_window::color_array_from_selection()
blitz::Array<GLfloat,2> plot_window::colors_show_deselected(MAXPLOTS+1,4); // used when deselected points are shown
blitz::Array<GLfloat,2> plot_window::colors_hide_deselected(MAXPLOTS+1,4); // used when deselected points are not shown

//GLfloat plot_window::texenvcolor[ 4] = { 1, 1, 1, 1};
//GLuint plot_window::texnames[ 2] = { };
int plot_window::textures_initialized = 0;

//*****************************************************************
// plot_window::plot_window( w, h) -- Constructor.  Increment
// count of plot wndows, resize arrays, and set mode.
plot_window::plot_window( int w, int h) : Fl_Gl_Window( w, h),
  do_reset_view_with_show( 0)
{
  // Update count and invoke initialzation method
  count++;
  initialize();
}

//*****************************************************************
// plot_window::initialize -- Initialize window parameters.  Set
// flags, set colors, resize arrays, and set mode.
void plot_window::initialize()
{
  do_reset_view_with_show = 0;
  show_center_glyph = 0;
  r_selected=0.01, g_selected=0.01, b_selected=1.0;

  // Resize arrays
  vertices.resize( npoints, 3);
  //vertex_ranks.resize( npoints, 3);
  x_rank.resize( npoints);
  y_rank.resize( npoints);
  z_rank.resize( npoints);
  nbins = nbins_default;
  counts.resize( nbins_max, 3);
  counts_selected.resize( nbins_max, 3);

  // Set mode
  if( can_do(FL_RGB|FL_DOUBLE|FL_ALPHA|FL_DEPTH)) {
    mode( FL_RGB|FL_DOUBLE|FL_ALPHA|FL_DEPTH);
  } else if( can_do(FL_RGB8|FL_DOUBLE|FL_ALPHA|FL_DEPTH)) {
    mode( FL_RGB8|FL_DOUBLE|FL_DEPTH|FL_ALPHA);
  } else if( can_do(FL_RGB|FL_DOUBLE|FL_ALPHA)) {
    cout << "Warning: depth buffering not enabled" << endl;
    mode( FL_RGB|FL_DOUBLE|FL_DEPTH|FL_ALPHA);
  } else if( can_do(FL_RGB8|FL_DOUBLE|FL_ALPHA)) {
    cout << "Warning: depth buffering not enabled" << endl;
    mode( FL_RGB8|FL_DOUBLE|FL_ALPHA);
  } else {
    cerr << "Error: could not allocate double buffered RGBA window" << endl;
    exit (-1);
  }
}

//*****************************************************************
// choose_color_selected() -- Choose color of selected points.
// Could this become a static member function of plot_window?
void plot_window::choose_color_selected()
{
  (void) fl_color_chooser( "selected", r_selected, g_selected, b_selected);
  update_textures();
}

//*****************************************************************
// plot_window::change_axes() -- Change axes for a plot to new axes
// which are far enough away so they are (probably) not duplicates
// and (probably) don't skip any combinations.
// void plot_window::change_axes()
void plot_window::change_axes( int nchange)
{
  // int nchange = 0;

  // Loop: Examine control panel tabs and increment axis counts
  // only for plots with x or y axis unlocked.  This is not ideal.
  for( int i=0; i<nplots; i++) {
    if( !cps[i]->lock_axis1_button->value() || !cps[i]->lock_axis2_button->value())
      nchange++;
  }
  // cout << "for window " << index << " nchange=" << nchange << endl;

  // this seems a little verbose.....
  int i=cp->varindex1->value();
  int j=cp->varindex2->value();
  // cout << "  (i,j) before = (" << i << "," << j << ")" << endl;
  if (!cp->lock_axis1_button->value() && !cp->lock_axis2_button->value()) {
	  for( int k=0; k<nchange; k++)
		  upper_triangle_incr( i, j, nvars);
	  cp->varindex1->value(i);
	  cp->varindex2->value(j);
  } else if (!cp->lock_axis1_button->value()) {
	  for( int k=0; k<nchange; k++) {
		  i = (i+1)%nvars;
		  cp->varindex1->value(i);
	  }
  } else if (!cp->lock_axis2_button->value()) {
	  for( int k=0; k<nchange; k++) {
		  j = (j+1)%nvars;
		  cp->varindex2->value(j);
	  }
  }
  cp->extract_and_redraw();
}

//*****************************************************************
// plot_window::update_linked_transforms() -- Use current plot's 
// scale and offset to update all the others that show (any of) 
// the same axes (using the same normalization).
void plot_window::update_linked_transforms()
{
  if( !link_all_axes_button->value()) return;

  // get this plot's axis indices and normalization styles
  int axis1=cp->varindex1->value(); 
  int style1 = cp->x_normalization_style->value();
  int axis2=cp->varindex2->value(); 
  int style2 = cp->y_normalization_style->value();

  // Loop: Find other plot windows that have any of the same axis 
  // indices active and the same normalization style, update the 
  // appropriate translation and scale values for them.
  for( int i=0; i<nplots; i++) {
    plot_window *p = pws[i];

    // don't need to update ourself
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

//*****************************************************************
// plot_window::handle( event) -- Main event handler.
int plot_window::handle( int event)
{
  // Current plot window (mouse drags, etc) must get redrawn before 
  // others so that selections get colored correctly.  Ugh.
  switch(event) {
    case FL_PUSH:
      DEBUG(cout << "FL_PUSH at " << xprev << ", " << yprev << endl);

      // Show the control panel associated with this plot window.
      cpt->value(cps[this->index]);	
      xprev = Fl::event_x();
      yprev = Fl::event_y();

      // middle button pushed = start zoom
      if( (Fl::event_state() == FL_BUTTON2) || (Fl::event_state() == (FL_BUTTON1 | FL_CTRL))) {
          // XXX wish this worked
		  #if 0
          xzoomcenter = (float)xprev;
          xzoomcenter = + (2.0*(xzoomcenter/(float)w()) -1.0) ; // window -> [-1,1]
          yzoomcenter = (float)yprev;
          yzoomcenter = - (2.0*(yzoomcenter/(float)h()) -1.0) ; // window -> [-1,1]
		  #endif
      }

      // right button pushed = start translating
      else if( Fl::event_state(FL_BUTTON3) || (Fl::event_state() == (FL_BUTTON1 | FL_ALT)) ) {
        show_center_glyph = 1;
        needs_redraw = 1;
      }

      // left button pushed = start new selection, or extend or move old selection
      else if( Fl::event_state() == FL_BUTTON1) {
        static int previous_window, current_window = -1;
        previous_window = current_window;
        current_window = index;
        if( current_window != previous_window)
          previously_selected( blitz::Range(0,npoints-1)) = selected( blitz::Range( 0, npoints-1));
        
        // no shift key = new selection
        if(! (Fl::event_key(FL_Shift_L) || Fl::event_key(FL_Shift_R))) {
          extend_selection = 0;

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
          redraw_all_plots (index);

        }
        else {
          // previously_selected( blitz::Range( 0, npoints-1)) = 0;
        }
      }

      return 1;

    case FL_DRAG:
      DEBUG (printf ("FL_DRAG, event_state: %x\n", Fl::event_state()));
      xcur = Fl::event_x();
      ycur = Fl::event_y();

      xdragged = xcur - xprev;
      ydragged = -(ycur - yprev);
      xprev = xcur;
      yprev = ycur;

      // translate = drag with right mouse (or alt-left-mouse)
      if( Fl::event_state(FL_BUTTON3) || (Fl::event_state(FL_BUTTON1) && Fl::event_state(FL_ALT))) {
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

      // scale = drag with middle-mouse (or c-left-mouse)
      else if( Fl::event_state(FL_BUTTON2) || (Fl::event_state(FL_BUTTON1) && Fl::event_state(FL_CTRL))) {
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

      // continue selection = drag with left mouse
      else if( Fl::event_state(FL_BUTTON1)) {
        // right key down = move selection
        // left shift down = extend selection (bug on OSX - no left key events)
        if( Fl::event_key(FL_Shift_L) || Fl::event_key(FL_Shift_R)) {
          xdown += xdragged*(1/xscale)*(2.0/w());
          ydown += ydragged*(1/yscale)*(2.0/h());
          xtracked += xdragged*(1/xscale)*(2.0/w());
          ytracked += ydragged*(1/yscale)*(2.0/h());
          if (Fl::event_key(FL_Shift_R)) {
            extend_selection = 0;
          }
          else {
            extend_selection = 1;
          }
        }
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
          redraw_all_plots (index);
        }
      }
      screen_to_world (-1, -1, wmin[0], wmin[1]);
      screen_to_world (+1, +1, wmax[0], wmax[1]);
      return 1;

    // Mouse up
    case FL_RELEASE:   
      DEBUG (cout << "FL_RELEASE at " << Fl::event_x() << ", " << Fl::event_y() << endl);
      // selection_changed = 0;
      if( show_center_glyph) {
        show_center_glyph = 0;
      }
      redraw_all_plots (index);
      return 1;

    // keypress, key is in Fl::event_key(), ascii in 
    // Fl::event_text().  Return 1 if you understand/use the 
    // keyboard event, 0 otherwise...
    case FL_KEYDOWN:
      DEBUG ( cout << "FL_KEYDOWN, event_key() = " << Fl::event_key() << endl);

      // XXX should figure out how to share shortcuts between plot 
      // windows and control panels... later
      switch( Fl::event_key()) {

        case 'q':   // exit
        case '\027':  // quit
          exit( 0);

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
          clear_selection( (Fl_Widget *) NULL);
          return 1;
          
        // Don't display / display deselected dots
        case 'd':
          toggle_display_deselected( (Fl_Widget *) NULL);
          return 1;

        // Extract data for these axes and redraw plot
        case 'r':
          extract_data_points();
          //redraw();
          return 1;

        case 'h':
          scale_histogram=1;
          return 1;

        default:
          return 0;
      }

    case FL_KEYUP:
      DEBUG ( cout << "FL_KEYUP" << endl);
      switch( Fl::event_key()) {
        case 'h':
          scale_histogram=0;
          return 1;

        default:
          return 0;
      }

    // Shortcut, key is in Fl::event_key(), ascii in 
    // Fl::event_text().  Return 1 if you understand/use the 
    // shortcut event, 0 otherwise...
    case FL_SHORTCUT:
      return 0;

    // Pass other events to the base class...
    default:
      return Fl_Gl_Window::handle( event);}
} 


//*****************************************************************
// plot_window::reset_selection_box() -- Reset selection box.
void plot_window::reset_selection_box()
{
  xdragged = ydragged = 0.0;
  xzoomcenter = yzoomcenter = zzoomcenter = 0.0;
  xdown = ydown = xtracked = ytracked = 0.0;
  xprev = yprev = xcur = ycur = 0;
}

//*****************************************************************
// plot_window::redraw_one_plot() -- Invoke member functions to
// redraw one plot.
void plot_window::redraw_one_plot ()
{
  DEBUG( cout << "in redraw_one_plot" << endl ) ;
  compute_histograms();
  redraw();
  Fl::flush();
  needs_redraw = 0;
}

//*****************************************************************
// plot_window::reset_view() -- Reset pan, zoom, and angle.
void plot_window::reset_view()
{
  // Get third axis, if any
  int axis2 = (int)(cp->varindex3->mvalue()->user_data());

  // Regenerate axis scales
  xscale = 2.0 / (wmax[0]-wmin[0]);
  yscale = 2.0 / (wmax[1]-wmin[1]);
  if (axis2 != nvars) zscale = 2.0 / (wmax[2]-wmin[2]);
  else zscale = 1.0;
	
  // Initiallly, datapoints only span 0.8 of the window dimensions, 
  // which allows room around the edges for labels, tickmarks, 
  // histograms....
  xscale *= initial_pscale; 
  yscale *= initial_pscale; 
  zscale *= initial_pscale; 

  // Get axis centers
  xcenter = (wmin[0]+wmax[0]) / 2.0;
  ycenter = (wmin[1]+wmax[1]) / 2.0;
  if( axis2 != nvars) zcenter = (wmin[2]+wmax[2]) / 2.0;
  else zcenter = 0.0;

  // Get histogram scales
  xhscale = 1.0;
  yhscale = 1.0;

  // Reset angle and stop any spin.
  angle = 0.0;
  cp->spin->value(0);
  cp->rot_slider->value(0.0);
  cp->dont_clear->value(0);

  // Reset selection box and flag window as needing redraw
  reset_selection_box ();
  if( count ==1) {
    // color_array_from_selection (); // HUH????
  }
  needs_redraw = 1;

  // Make sure the window is visible and resizable.  NOTE: For 
  // some reason, it is necessary to turn this off when a new plot
  // window array is created or the windows will not be resizable!
  if( do_reset_view_with_show & !visible()) {
    this->show();
    this->resizable( this);
  }
}

//*****************************************************************
// plot_window::draw() -- Main draw method that calls others.
void plot_window::draw() 
{
  DEBUG (cout << "in draw: " << xcenter << " " << ycenter << " " << xscale << " " << yscale << endl);

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
    glEnableClientState(GL_COLOR_ARRAY);

    // this next idiom is necessary, per window, to map 
    // texture coordinate values to [0..1] for texturing.
    // glMatrixMode(GL_TEXTURE);
    // glLoadIdentity();	
    // glScalef( 1.0/(float)MAXPLOTS, 1.0/(float)MAXPLOTS, 1.0/(float)MAXPLOTS); 
    // glMatrixMode(GL_MODELVIEW);

    #ifdef FAST_APPLE_VERTEX_EXTENSIONS
      glEnableClientState(GL_VERTEX_ARRAY_RANGE_APPLE);
    #endif // FAST_APPLE_VERTEX_EXTENSIONS

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
  glTranslatef (-xcenter, -ycenter, -zcenter);
  glTranslatef (-xzoomcenter, -yzoomcenter, -zzoomcenter);

  if( cp->dont_clear->value() == 0) {
    // glClearColor(0.0,0.0,0.0,0.0);
    glClearColor( cp->Bkg->value(), cp->Bkg->value(), cp->Bkg->value(), 0.0);
    glClearDepth (0.0);
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    draw_grid();
  }

  if( selection_changed) {
    handle_selection ();
  }
  draw_data_points();
  draw_center_glyph();
  draw_histograms ();
  draw_axes();
}

//*****************************************************************
// plot_window::draw_grid() -- Draw a grid.
void plot_window::draw_grid()
{
  glBlendFunc(GL_ONE, GL_ZERO);
  // glBlendFunc(sfactor, dfactor);
  // glEnable(GL_LINE_SMOOTH);
  // glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
  // glLineWidth(0.5);
  glLineWidth(1.0);
  if( cp->show_grid->value()) {
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
  }
}

//*****************************************************************
// plot_window::screen_to_world( xscreen, yscreen, xworld, yworld)
// -- Convert from screen to world co-ordinates?
void plot_window::screen_to_world( 
  float xscreen, float yscreen, float &xworld, float &yworld)
{
  // cout << "screen_to_world" << endl;
  // cout << "  before" << xscreen << " " << yscreen
  //      << " " << xworld << " " << yworld << endl;
  xworld = ( xscreen*initial_pscale / xscale) + xcenter;
  yworld = ( yscreen*initial_pscale / yscale) + ycenter;
  //cout << "  after " << xscreen << " " << yscreen
  //     << " " << xworld << " " << yworld << endl;
}

//*****************************************************************
// plot_window::draw_axes() -- If requested, draw and label the
// axes
void plot_window::draw_axes ()
{
  // If requested draw axes
  if( cp->show_axes->value()) {
    glPushMatrix();
    glLoadIdentity();

    // Define the extra (relative) distance that axes extend past 
    // leftmost and rightmost tickmarks and set initial pscale
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

    // Define a buffer used to lable tic marks and set the 
    // offset factor for tic mark length. b<1 -> inwards, 
    // b>1 -> outwards, b==1 -> no tick.
    char buf[ 1024];
    float b = 1.5;

    // If requested, draw tic marks to show scale	
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

      //  offset for scale values. b<1 -> inwards, 
      // b>1 -> outwards, b==1 -> on axis.
      b = 2;  

      // lower X-axis scale value
      snprintf( buf, sizeof(buf), "%+.3g", wmin[0]); 
      gl_draw( 
        (const char *)buf, 
        -1.0-gl_width((const char *)buf)/(w()), -(1+b*a));

      // upper X-axis scale value
      snprintf(buf, sizeof(buf), "%+.3g", wmax[0]); 
      gl_draw(
        (const char *)buf, 
        +1.0-gl_width((const char *)buf)/(w()), -(1+b*a));

      // This value of b is used for...?
      b = 2.4;

      // lower Y-axis scale value
      snprintf( buf, sizeof(buf), "%+.3g", wmin[1]);
      gl_draw( (const char *)buf, -(1+b*a), -1.0f+a/4);

      // upper Y-axis scale value
      snprintf( buf, sizeof(buf), "%+.3g", wmax[1]);
      gl_draw( (const char *)buf, -(1+b*a), +1.0f+a/4);
    }

    // If requested, draw tic mark labels
    if( cp->show_labels->value()) {

      // offset for axis labels values. b<1 -> inwards, 
      // b>1 -> outwards, b==1 -> on axis.
      b = 2; 

      float wid = gl_width(xlabel.c_str())/(float)(w());
      gl_draw( (const char *)(xlabel.c_str()), -wid, -(1+b*a));	

      b = 1.5;
      gl_draw( (const char *)(ylabel.c_str()), -(1+b*a), 1+b*a);
    }

    glPopMatrix();
  }
}

//*****************************************************************
// plot_window::draw_center_glyph() -- Draw a glyph in the center
// of a selected region?)
void plot_window::draw_center_glyph ()
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

//*****************************************************************
// plot_window::print_selection_stats() -- Write statistics for
// selection to current plot window.
void plot_window::print_selection_stats ()
{
  glDisable( GL_DEPTH_TEST);
  glEnable( GL_COLOR_LOGIC_OP);
  glLogicOp( GL_INVERT);
  glPushMatrix();
  glLoadIdentity ();
  gl_font( FL_COURIER, 10);
  glBlendFunc( GL_ONE, GL_ZERO);
  glColor4f( 0.7,0.7,0.7,0.0);

  // Define character buffer to allocate storage for printing
  char buf[ 1024];

  // print selection statistics to top of plot window
  gl_font( FL_HELVETICA, 10);
  snprintf( buf, sizeof(buf), "%8d/%d (%5.2f%%) selected", nselected, npoints, 100.0*nselected/(float)npoints);
  gl_draw( (const char *)buf, -0.4f, 0.9f);

  glPopMatrix(); // back to world coordinates, to render strings at selection box boundaries
  
  // print ranges at appropriate sides of selection box
  snprintf (buf, sizeof(buf), "%# 7.4g", xdown);
  gl_draw( (const char *)buf, xdown-2*gl_width(buf)/(w()*xscale), ((ydown+ytracked)/2)-(0.5f*gl_height())/(h()*yscale) );
  if (xtracked != xdown) {
      snprintf (buf, sizeof(buf), "%#-7.4g", xtracked);
      gl_draw( (const char *)buf, xtracked, ((ydown+ytracked)/2)-(0.5f*gl_height())/(h()*yscale) );
  }
  
  snprintf (buf, sizeof(buf), "%# 7.4g", ydown);
  gl_draw( (const char *)buf, (xdown+xtracked)/2-gl_width(buf)/(w()*xscale), ydown+(0.5f*gl_height())/(h()*yscale) );
  if (ytracked != ydown) {
      snprintf (buf, sizeof(buf), "%# 7.4g", ytracked);
      gl_draw( (const char *)buf, (xdown+xtracked)/2-gl_width(buf)/(w()*xscale), ytracked-(1.5f*gl_height())/(h()*yscale) );
  }

  glDisable( GL_COLOR_LOGIC_OP);
}

//*****************************************************************
// plot_window::handle_selection() -- Handler to handle selection
// operations.
void plot_window::handle_selection ()
{
  if (selection_is_inverted)
    invert_selection();

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
  blitz::Range NPTS( 0, npoints-1);	

  // Identify newly-selected points
  newly_selected( NPTS) = where( 
    ( vertices( NPTS, 0)>fmaxf( xdown, xtracked) || vertices( NPTS, 0)<fminf( xdown, xtracked) ||
      vertices( NPTS, 1)>fmaxf( ydown, ytracked) || vertices( NPTS, 1)<fminf( ydown, ytracked)),
	0, index+1);

  // Add newly-selected points to existing or previous selection
  if( add_to_selection_button->value()) {
    selected( NPTS) = where( newly_selected( NPTS), newly_selected( NPTS), selected( NPTS));
  } 
  else {
    selected( NPTS) = where( newly_selected( NPTS), newly_selected( NPTS), previously_selected( NPTS));
  }

  // Determine and print selection statistics
  nselected = blitz::count( selected( NPTS)>0);
  // there should be a gui element controlling this?
  print_selection_stats();
  color_array_from_new_selection ();

  // done flagging selection for this plot
  selection_changed = 0;
}

//*****************************************************************
// plot_window::update_textures() -- Update textures.
void plot_window::update_textures ()
{
  // New color for selected points (selection in this window only)
  colors_show_deselected(index+1,0) = colors_hide_deselected(index+1,0) = r_selected;
  colors_show_deselected(index+1,1) = colors_hide_deselected(index+1,1) = g_selected;
  colors_show_deselected(index+1,2) = colors_hide_deselected(index+1,2) = b_selected;
  colors_show_deselected(index+1,3) = colors_hide_deselected(index+1,3) = 1.0;

  // color for de-selected points when they are displayed
  colors_show_deselected(0,0) = r_deselected;
  colors_show_deselected(0,1) = g_deselected;
  colors_show_deselected(0,2) = b_deselected;
  colors_show_deselected(0,3) = 1.0;

  // color for de-selected points when they are not displayed
  colors_hide_deselected(0,0) = 0.0;
  colors_hide_deselected(0,1) = 0.0;
  colors_hide_deselected(0,2) = 0.0;
  colors_hide_deselected(0,3) = 0.0;

}

//*****************************************************************
// plot_window::color_array_from_selection() -- fill the array of colors that
// we will pass to openGL to color each vertex in *every* plot
//
// XXX this was rewritten to not use textures so textures could be used instead, later, to
// render the points using point sprites, so we can have huge smooth points, symbols, etc.
// note: this would probably be much faster (and much clearer) if we used openGL's color index mode.
// however, however, color index mode disables all sorts of other opengl features that we depend on.
// note: the reason for the ugly casts and memcpy() is that blitz does not have enough powers
// of indirection to compile the epression I wanted to use to fake my own color indices.  The expression
// I want would be, basically:
//  colors(NPTS,RGBA) = colors_show_deselected(selected(NPTS),RGBA);
// note: Another way to implement this would be to run through the selected() array, which an integer specifying
// the number of the plot_window that (most recently) selected each point (+1, since 0 means "not selected
// in any window") and pack all the *indices* of points with selected==0 into one array, selected==1 into another
// array, etc.  Then these indices could be used with glDrawElements(), glDrawRangeElements(), and friends to render
// each selected set (and the non-selected set) separately, with one call to glColor() (& glPointsize(), glBlendfunc(),
// glPointSprite(), .....) for each set, dispensing with the color array entirely.  This would allow us to, e.g., force
// selected points to overplot non-selected points, etc.
// 
void plot_window::color_array_from_selection()
{
  update_textures();  // update "color tables" if the user requested a color change

  GLfloat *src = 0;
  if( show_deselected_button->value() && cp->show_deselected_points->value()) {  // XXX need to decide - global or local?
      src = (GLfloat *)(colors_show_deselected.data());
  } else {
      src = (GLfloat *)(colors_hide_deselected.data());
  }
  GLfloat *dest = (GLfloat *)(colors.data());
  int *offset = (int *)(selected.data());
  for (int i=0; i<npoints; i++) {
          memcpy ((void *)dest, (void *)(src+(*offset*4)), 4*sizeof(GLfloat));
          dest += 4;
          offset +=1;
  }
}

//*****************************************************************
// plot_window::color_array_from_new_selection() -- Invoke
// color_array_from_selection to color new selection.
void plot_window::color_array_from_new_selection()
{
  color_array_from_selection ();
}

//*****************************************************************
// plot_window::draw_data_points() -- If requested, draw the data
void plot_window::draw_data_points()
{
  // cout << "pw[" << index << "]: draw_data_points() " << endl;
  if ( !cp->show_points->value())return;

  glEnable( GL_DEPTH_TEST);
  glDepthFunc (GL_GEQUAL);

  float const_color[4];

  const_color[0] = const_color[1] = const_color[2] = cp->Lum->value(); 
  const_color[3] = cp->Alph->value();
  glBlendColor( const_color[0], const_color[1], const_color[2], const_color[3]);

  glPointSize( cp->pointsize_slider->value());

  if (cp->smooth_points_button->value()) { 
    glEnable(GL_POINT_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT,GL_NICEST);
    glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA); // anti-aliased points blendfunc for log. saturation overplotting
    // which is the same as
    // glBlendFuncSeparate(GL_SRC_ALPHA, GL_DST_ALPHA, GL_SRC_ALPHA, GL_DST_ALPHA);
    //  or play with things like:
    // glBlendFuncSeparate(GL_CONSTANT_COLOR, GL_DST_ALPHA, GL_SRC_ALPHA, GL_DST_ALPHA);
    glBlendFuncSeparate(GL_CONSTANT_COLOR, GL_DST_ALPHA, GL_CONSTANT_ALPHA, GL_DST_ALPHA);
  } else {
    glBlendFunc(GL_CONSTANT_COLOR, GL_DST_ALPHA); // aliased points blendfunc for log. saturation overplotting
  }

  // Tell the GPU where to find the correct colors for each vertex.
  GLfloat *colorp = (GLfloat *) colors.data();
  glColorPointer (4, GL_FLOAT, 0, colorp);

  int alpha_test_enabled = 0;

  // XXX need to resolve local/global controls issue
  //  - partially done.  can now get rid of show_deselected_button.
  if( !(show_deselected_button->value() && cp->show_deselected_points->value())) {
    // Cull any deselected points (alpha==0.0), whatever the 
    // blendfunc:
    glEnable( GL_ALPHA_TEST);
    glAlphaFunc( GL_GREATER, 0.0);  
    alpha_test_enabled = 1;
  }

  // Tell the GPU where to find the vertices;
  GLfloat *vertexp = (GLfloat *)vertices.data();

  // Are we plotting in two dimensions or three?  axis3 == 
  // "-nothing-" means 2D plotting
  if( cp->varindex3->value() == nvars) 
    glVertexPointer (2, GL_FLOAT, 3*sizeof(GL_FLOAT), vertexp); 
  else
    glVertexPointer (3, GL_FLOAT, 0, vertexp); 

  #ifdef FAST_APPLE_VERTEX_EXTENSIONS
    // for static data
    glVertexArrayParameteriAPPLE( GL_VERTEX_ARRAY_STORAGE_HINT_APPLE, GL_STORAGE_CACHED_APPLE);  

    // for dynamic data
    //  glVertexArrayParameteriAPPLE( GL_VERTEX_ARRAY_STORAGE_HINT_APPLE, GL_STORAGE_SHARED_APPLE); 

    glVertexArrayRangeAPPLE(3*npoints*sizeof(GLfloat),(GLvoid *)vertexp);
  #endif // FAST_APPLE_VERTEX_EXTENSIONS

  // tell the GPU to draw the vertices.
  glDrawArrays( GL_POINTS, 0, npoints);

  if( alpha_test_enabled ) {
	  glDisable(GL_ALPHA_TEST);
	  alpha_test_enabled = 0;
  }
  
  if (cp->smooth_points_button->value()) {
    glDisable(GL_POINT_SMOOTH);
  }

  //  glDisable( GL_TEXTURE_1D);
  glDisable( GL_DEPTH_TEST);
}

#if 0
//*****************************************************************
// plot_window::compute_histogram( axis) -- If requested, compute 
// equi-depth histogram for axis 'axis'.
void plot_window::compute_histogram( int axis)
{
  if( !(cp->show_histogram->value())) return;

  nbins = (int)(cp->nbins_slider->value());
  blitz::Range BINS(0,nbins-1);
  counts(BINS,axis) = 0.0;
  counts_selected( BINS, axis) = 0.0;
  float range = amax[axis]-amin[axis];

  // Loop: Sum over all data points
  for( int i=0; i<npoints; i++) {
    float x = vertices(i,axis);
    int bin=(int)(nbins*((x-amin[axis])/range));
    if( bin < 0) bin = 0;
    if( bin >= nbins) bin=nbins-1;
    counts(bin,axis)++;
    if( selected( i) > 0.5) counts_selected( bin, axis)++;
  }
  
  // Normalize results.  NOTE: This will die horribly if there is no data
  counts( BINS, axis) = 
    (5.0*nbins/(float)nbins_default)*counts(BINS,axis)/((float)(npoints));
  counts_selected(BINS,axis) = 
    (5.0*nbins/(float)nbins_default)*counts_selected(BINS,axis)/((float)(npoints));
}

// #endif 0
#endif

//*****************************************************************
// plot_window::compute_histogram( axis) -- If requested, compute 
// equi-width histogram for axis 'axis'.
void plot_window::compute_histogram( int axis)
{
  if( !(cp->show_histogram->value())) return;

  nbins = (int)(cp->nbins_slider->value());
  blitz::Range BINS( 0, nbins-1);
  counts( BINS, axis) = 0.0;
  counts_selected( BINS, axis) = 0.0;
  float range = amax[axis] - amin[axis];

  // Loop: Sum over all data points
  for( int i=0; i<npoints; i++) {
    float x = vertices( i, axis);
    int bin=(int)( nbins * ( ( x - amin[axis]) / range));
    if( bin < 0) bin = 0;
    if( bin >= nbins) bin = nbins-1;
    counts( bin, axis)++;
    if( selected( i) > 0.5) counts_selected( bin, axis)++;
  }
  
  // Normalize results.  NOTE: This would die horribly if there was no data
  if( npoints > 0) {
    counts( BINS, axis) = 
      ( 5.0*nbins / (float) nbins_default) * counts( BINS, axis) / 
      ( (float) (npoints));
    counts_selected( BINS, axis) = 
      ( 5.0*nbins / (float) nbins_default) * counts_selected( BINS, axis) / 
      ( (float)(npoints));
  }
}

//*****************************************************************
// plot_window::compute_histograms() -- Invoke compute_histogram to
// compute histograms for axes 0 and 1.
void plot_window::compute_histograms()
{
  compute_histogram(0);
  compute_histogram(1);
}

//*****************************************************************
// plot_window::draw_histogram() -- If requested, draw histograms.
void plot_window::draw_histograms()
{
  if( !(cp->show_histogram->value())) return;

  // histograms base is this far from edge of window
  float hoffset = 0.01; 

  glPushMatrix();

  // x-axis histograms
  glLoadIdentity();
  glTranslatef( xzoomcenter*xscale, 0.0, 0);
  glScalef( xscale, yhscale, 1.0);
  glTranslatef( -xcenter, -1.0/yhscale, 0.0);
  glTranslatef( -xzoomcenter, 0.0, 0);
  glTranslatef( 0, hoffset, 0);

  // histograms cover pointclouds
  glTranslatef (0.0, 0.0, 0.1);
  float xwidth = (amax[0]-amin[0]) / (float)(nbins);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Loop: Draw x-axis histogram (all points)
  float x = amin[0];
  glColor4f( 0.0, 1.0, 0.0, 0.5);
  glBegin( GL_LINE_STRIP);
  glVertex2f( x, 0.0);
  for( int bin=0; bin<nbins; bin++) {
    glVertex2f( x, counts(bin,0));   // left edge
	glVertex2f( x+xwidth, counts(bin,0));	  // Top edge
    //glVertex2f( x+xwidth, 0.0);   // Right edge
    x += xwidth;
  }
  glVertex2f(x,0.0);					
  glEnd();

  // If points were selected, refactor selected points of the
  // x-axis histogram?
  if( nselected > 0) {
    x = amin[0];
    glColor4f( 0.25, 1.0, 0.25, 1.0);
    glBegin( GL_LINE_STRIP);
    glVertex2f( x, 0.0);
    for( int bin=0; bin<nbins; bin++) {
      glVertex2f( x, counts_selected( bin, 0));   // left edge
      glVertex2f( x+xwidth, counts_selected( bin, 0));   // top edge
      // glVertex2f(x+xwidth,0.0);   // right edge 
      x += xwidth;
    }
    glVertex2f(x,0.0);
    glEnd();
  }
	
  // y-axis histograms
  glLoadIdentity();
  glTranslatef( 0.0, yzoomcenter*yscale, 0);
  glScalef( xhscale, yscale, 1.0);
  glTranslatef( -1.0/xhscale, -ycenter, 0.0);
  glTranslatef( 0.0, -yzoomcenter, 0);
  glTranslatef( hoffset, 0, 0);
  float ywidth = (amax[1]-amin[1]) / (float)(nbins);

  // Loop: draw y-axis histogram (all points)
  float y = amin[1];
  glColor4f( 0.0, 1.0, 0.0, 0.5);
  glBegin( GL_LINE_STRIP);
  glVertex2f( 0.0, y);					
  for( int bin=0; bin<nbins; bin++) {
    glVertex2f(counts(bin,1),y);   // bottom
    glVertex2f(counts(bin,1), y+ywidth);   // right edge
    // glVertex2f(0.0, y+ywidth);   // top edge 
    y+=ywidth;
  }
  glVertex2f(0.0,y);					
  glEnd();

  // If points were selected, refactor selected points of the
  // y-axis histogram?
  if( nselected > 0) {
    y = amin[1];
    glColor4f( 0.25, 1.0, 0.25, 1.0);
    glBegin( GL_LINE_STRIP);
    glVertex2f( 0.0, y);
    for( int bin=0; bin<nbins; bin++) {
      glVertex2f(counts_selected( bin, 1),y);   // bottom
      glVertex2f(counts_selected( bin, 1), y+ywidth);   // right edge
      // glVertex2f(0.0, y+ywidth);   // top edge 
      y+=ywidth;
    }
    glVertex2f(0.0,y);					
    glEnd();
  }

  glPopMatrix();
}

//*****************************************************************
// plot_window::transform_2d() -- If requested, transform data to
// 2D sum-vs-difference or polar coordinates.
int plot_window::transform_2d()
{
  if( cp->no_transform->value()) return 1;  // no transform
	
  blitz::Range NPTS(0,npoints-1);

  blitz::Array <float,1> tmp1(npoints), tmp2(npoints);
  tmp1 = vertices(NPTS,0);
  tmp2 = vertices(NPTS,1);

  if( cp->sum_vs_difference->value()) {
    vertices(NPTS,0) = (sqrt(2.0)/2.0) * (tmp1 + tmp2);
    vertices(NPTS,1) = (sqrt(2.0)/2.0) * (tmp1 - tmp2);
  }
  else if( cp->polar->value()) {
    vertices(NPTS,0) = atan2(tmp1, tmp2);
    vertices(NPTS,1) = sqrt(pow2(tmp1)+pow2(tmp2));
  }
  return 1;
}

//*****************************************************************
// plot_window::normalize( a, a_rank, style, axis_index) --  Apply
// normalization of the requested style.
int plot_window::normalize(
  blitz::Array<float,1> a, 
  blitz::Array<int,1> a_rank, 
  int style, int axis_index)
{
  blitz::Range NPTS(0,npoints-1);

  #ifdef CHECK_FOR_NANS_IN_NORMALIZATION
    blitz::Array<int,1> inrange(npoints);
    inrange = where(
      ((a(NPTS) < MAXFLOAT) && (a(NPTS) > -MAXFLOAT)), 1, 0);
    float tmin = min(where(inrange,a(NPTS), MAXFLOAT));
    float tmax = max(where(inrange,a(NPTS),-MAXFLOAT));
  #else // CHECK_FOR_NANS_IN_NORMALIZATION
    float tmin = a(a_rank(0));
    float tmax = a(a_rank(npoints-1));
    blitz::Array<float, 1> tmp(npoints);
  #endif // CHECK_FOR_NANS_IN_NORMALIZATION

  float mu,sigma;
  
  switch( style) {
    case control_panel_window::NORMALIZATION_NONE:
      wmin[axis_index] = -1;
      wmax[axis_index] = +1;
      return 1;

    case control_panel_window::NORMALIZATION_MINMAX:
      wmin[axis_index] = tmin;
      wmax[axis_index] = tmax;
      return 1;

    // all positive data fits in window, zero at "left" of axis.
    case control_panel_window::NORMALIZATION_ZEROMAX: 
      wmin[axis_index] = 0.0;
      wmax[axis_index] = tmax;
      return 1;

    // all data fits in window w/zero at center of axis
    case control_panel_window::NORMALIZATION_MAXABS:  
      tmax = fmaxf(fabsf(tmin),fabsf(tmax));
      if( tmax != 0.0) {
        wmin[axis_index] = -tmax;
        wmax[axis_index] = tmax;
      }
      return 1;

    // median at center of axis, axis extends to include at 
    // least 99% of data
    case control_panel_window::NORMALIZATION_TRIM_1E2:
      {
        float trim = 1e-2;
        wmin[axis_index] = 
          a(a_rank((int)((0.0 + (0.5*trim))*npoints)));
        wmax[axis_index] = 
          a(a_rank((int)((1.0 - (0.5*trim))*npoints)));
		return 1;
      }

    // median at center of axis, axis extends to include at 
    // least 99.9% of data
    case control_panel_window::NORMALIZATION_TRIM_1E3:  
      {
        float trim = 1e-3;
        wmin[axis_index] = 
          a(a_rank((int)((0.0 + (0.5*trim))*npoints)));
        wmax[axis_index] = 
          a(a_rank((int)((1.0 - (0.5*trim))*npoints)));
        return 1;
      }

    // mean at center of axis, axis extends to +/- 3*sigma
    case control_panel_window::NORMALIZATION_THREESIGMA:  
      mu = mean(a(NPTS));
      sigma = sqrt((1.0/(float)npoints)*sum(pow2(a(NPTS)-mu)));
      DEBUG (cout << "mu, sigma = " << mu << ", " << sigma << endl);
      if( finite(mu) && (sigma!=0.0)) {
        wmin[axis_index] = mu - 3*sigma;
        wmax[axis_index] = mu + 3*sigma;
      }
      return 1;

    // negative numbers get assigned a log of zero.
    case control_panel_window::NORMALIZATION_LOG10: 
      if( tmin <= 0.0) {
        cerr << "Warning: "
             << "attempted to take logarithms of nonpositive "
             << " numbers. Those logs were set to zero." 
             << endl;
      }
      // find smallest positive element
      a(NPTS) = where( a(NPTS) > 0, log10(a(NPTS)), 0);
      wmin[axis_index] = min(a(NPTS));
      wmax[axis_index] = a(a_rank(npoints-1));
      return 1;

    // simple sigmoid, (-inf,0,+inf) -> (-1,0,+1)
    case control_panel_window::NORMALIZATION_SQUASH: 
      a(NPTS) = a(NPTS)/(1+abs(a(NPTS)));
      wmin[axis_index] = a(a_rank(0));
      wmax[axis_index] = a(a_rank(npoints-1));
      return 1;

    // replace each item with its rank, normalized from 0 to 1
    case control_panel_window::NORMALIZATION_RANK:
      for( int i=0; i<npoints; i++) {
        a( a_rank(i)) = float(i) / ((float)npoints-1);
      }
      wmin[axis_index] = 0;
      wmax[axis_index] = 1;
      return 1;
      
    // Gaussianize the data, with the cnter of the gaussian 
    // at the median.
    case control_panel_window::NORMALIZATION_GAUSSIANIZE: 
      for( int i=0; i<npoints; i++) {
        a( a_rank(i)) = 
          (1.0/5.0) *
          (float)gsl_cdf_ugaussian_Pinv((double)(float(i+1) / 
          (float)(npoints+2)));
      }
      wmin[axis_index] = -1.0;
      wmax[axis_index] = +1.0;
      return 1;
    
    // Default: do nothing
    default:
    return 0;
  }
}

//*****************************************************************
// plot_window::compute_rank() -- Order data for normalization or
// generation of histograms
void plot_window::compute_rank(
  blitz::Array<float,1> a, 
  blitz::Array<int,1> a_rank, 
  int var_index)
{
  blitz::Range NPTS(0,npoints-1);
  if( !ranked( var_index)) {
    if( !a.isStorageContiguous()) {
      cerr << "Warning: sorting with non-contiguous data." << endl;
    }
    if( !a_rank.isStorageContiguous()) {
      cerr << "Warning: sorting with non-contiguous rank." << endl;
    }
    a_rank(NPTS) = identity(NPTS);
    
    tmp_points.reference(a);
    int *lo = a_rank.data(), *hi = lo + npoints;
    std::stable_sort(lo, hi, myCompare());

    ranked(var_index) = 1;  // now we are ranked
	ranked_points(var_index,NPTS) = a_rank(NPTS);  // and our rank is cached!
    // cout << "  cache STORE at index " << var_index << endl;
  }
  else {
    a_rank=ranked_points(var_index,NPTS);// use previously cached rank!
    // cout << "  CACHE HIT   at index " << var_index << endl;
  }
}

//*****************************************************************
// plot_window::extract_data_points() -- Extract column labels and 
// data for a set of axes, rank (order) and normalize and scale
// data, compute histograms, and compute axes scales.
int plot_window::extract_data_points ()
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
  cout << "plot " << row << ", " << column << endl;
  cout << " pre-normalization: " << endl;

  compute_rank( points( axis0, NPTS), x_rank, axis0);
  cout << "  min: " << xlabel << "(" << x_rank(0) << ") = " << points( axis0, x_rank(0));
  cout << "  max: " << xlabel << "(" << x_rank(npoints-1) << ") = " << points( axis0, x_rank(npoints-1)) << endl;
  
  compute_rank( points( axis1, NPTS), y_rank,axis1);
  cout << "  min: " << ylabel << "("  << y_rank(0) << ") = " << points(axis1,y_rank(0));
  cout << "  max: " << ylabel << "(" << y_rank(npoints-1) << ") = " << points( axis1, y_rank(npoints-1)) << endl;

  if( axis2 != nvars) {
    compute_rank( points(axis2,NPTS),z_rank,axis2);
    cout << "  min: " << zlabel << "(" << z_rank(0) << ") = " << points(axis2,z_rank(0));
    cout << "  max: " << zlabel << "(" << z_rank(npoints-1) << ") = " << points(axis2,z_rank(npoints-1)) << endl;
  }

  // Load vertices and points for the x-axis.
  // This copies the data...
  vertices( NPTS, 0) = points( axis0, NPTS);  
  // ...but this doesn't. See blitz++ manual.
  blitz::Array<float,1> xpoints = vertices( NPTS, 0); 
  
  // Load vertices and points for the y-axis.
  vertices( NPTS, 1) = points( axis1, NPTS);
  blitz::Array<float,1> ypoints = vertices( NPTS, 1);

  // Load vertices and points, if any, for the z-axis.  
  if( axis2 != nvars)
    vertices( NPTS, 2) = points( axis2, NPTS);
  else
    vertices( NPTS, 2) = 0;
  blitz::Array<float,1> zpoints = vertices(NPTS,2);

  // Apply the normalize() method to normalize and scale the data 
  // and report results
  cout << " post-normalization: " << endl;
  (void) normalize( xpoints, x_rank, cp->x_normalization_style->value(), 0);
  amin[0] = xpoints(x_rank(0));
  amax[0] = xpoints(x_rank(npoints-1));
  cout << "  min: " << xlabel << "(" << x_rank(0) << ") = " << xpoints(x_rank(0));
  cout << "  max: " << xlabel << "(" << x_rank(npoints-1) << ") = " << xpoints(x_rank(npoints-1)) << endl;
    
  // Normalize and scale the y-axis
  (void) normalize( ypoints, y_rank, cp->y_normalization_style->value(), 1);
  amin[1] = ypoints(y_rank(0));
  amax[1] = ypoints(y_rank(npoints-1));
  cout << "  min: " << ylabel << "(" << y_rank(0) << ") = " << ypoints(y_rank(0));
  cout << "  max: " << ylabel << "(" << y_rank(npoints-1) << ") = " << ypoints(y_rank(npoints-1)) << endl;

  // Normalize and scale the z-axis
  if( axis2 != nvars) {
    (void) normalize( zpoints, z_rank, cp->z_normalization_style->value(), 2);
    amin[2] = zpoints(z_rank(0));
    amax[2] = zpoints(z_rank(npoints-1));
    cout << "  min: " << zlabel << "(" << z_rank(0) << ") = " << zpoints(z_rank(0));
    cout << "  max: " << zlabel << "(" << z_rank(npoints-1) << ") = " << zpoints(z_rank(npoints-1)) << endl;
  } 
  else {
    amin[2] = -1.0;
    amax[2] = +1.0;
  }

  // Reset pan, zoom, and view-angle
  reset_view();
  (void) transform_2d();

  // XXX need to refactor this.  This is needed to make sure the
  // scale marks on the axis are updated
  screen_to_world( -1, -1, wmin[0], wmin[1]);
  screen_to_world( +1, +1, wmax[0], wmax[1]);

  compute_histograms();
  return 1;
}

//*****************************************************************
// Define STATIC methods

//*****************************************************************
// plot_window::upper_triangle_incr( i, j, n) -- STATIC method
// to increment the row and column indices, (i,j), to traverse an 
// upper triangular matrix by moving "down and to the right" with
// wrapping.  A static method used by plot_window::change_axes and
// in the body of the main routine to select axis labels.
void plot_window::upper_triangle_incr( 
  int &i, int &j, const int n)
{
  // cout << "  upper_triangle_incr before: i, j = " << " " << i << " " << j << endl;
  // diagonals get incremented together, with wrapping
  if (i==j)
  {
    i=(i+1)%n;
    j = i;
  }
  // upper triangle gets incremented "down and to the right" with diagonal wrapping
  else if (i<j) {
    if (i<n-2 && j<n-1)
    {
      i++;
      j++;
    } else {
      j = (n-i);
      if (j>n-1) j=1;
      i = 0;
    }
  }
  // lower triangle gets treated as upprt triangle, with the two axes swapped.
  else if (i>j) {
    upper_triangle_incr( j, i, n);
  }
  // cout << "  upper_triangle_incr after:  i, j = " << " " << i << " " << j << endl;
  assert( i >= 0);
  assert( j >= 0);
  assert( i < n);
  assert( j < n);
}

//*****************************************************************
// plot_window::redraw_all_plots( p) -- STATIC method that invokes 
// methods to redraw all plots cylically, starting with plot p.  
// This is a static method used by class plot_window and by the
// npoints_changed method in the main routine.
void plot_window::redraw_all_plots( int p)
{
  DEBUG( cout << "in redraw_all_plots(" << p << ")" << endl ) ;

  // redraw all plots, cyclically, starting with plot p.  This p 
  // is important, since the draw() routine for a plot handles 
  // the selection region, and the active plot (the one where we 
  // are making the selection) must update the selected set and 
  // the color/texture arrays *before* all the other plots get 
  // redrawn.  Ugh.
  for( int i=0; i<nplots; i++) {
    int j=(p+i)%nplots;
    pws[j]->compute_histograms();
    pws[j]->redraw();
    Fl::flush();
    pws[j]->needs_redraw = 0;
  }
}

//*****************************************************************
// plot_window::delete_selection( p) -- STATIC method to delete
// selected points.  This is a static method used only by class 
// plot_window.
void plot_window::delete_selection( Fl_Widget *o)
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
    npoints_slider->bounds(1,npoints);
    npoints_slider->value(npoints);

    clear_selection( (Fl_Widget *) NULL);
	
    for( int j=0; j<nplots; j++) {
      cps[j]->extract_and_redraw();
    }
  }
}

//*****************************************************************
// plot_window::invert_selection() -- STATIC method to invert 
// selected and nonselected points.  This is a static method used 
// only by class plot_window.
void plot_window::invert_selection ()
{
  if (!selection_is_inverted) {
      // save "true" selection
      saved_selection = selected;
      // create something like an inverse in its place
      selected = where(selected==0, 1, 0);
      selection_is_inverted = true;
      cout << "selection inverted" << endl;
  } else {
      // restore what we saved last time
      selected = saved_selection;
      selection_is_inverted = false;
      cout << "selection restored" << endl;
  }

  nselected = npoints-nselected;

  // recolor all points using the new selection and redraw
  pws[ 0]->color_array_from_selection();
  redraw_all_plots(0);
}

//*****************************************************************
// plot_window::toggle_display_selected( *o) -- STATIC method to 
// toggle colors of selected and unselected points. This is a 
// static method used only by class plot_window.
void plot_window::toggle_display_deselected( Fl_Widget *o)
{
  // Toggle the value of the button manually, but only if we were 
  // called via a keypress in a plot window
  // Shouldn't there be an easier way?
  if( o == NULL)
    show_deselected_button->value( 1 - show_deselected_button->value());

  // recolor all points using the correct "color table" and redraw
  pws[0]->color_array_from_selection();
  redraw_all_plots (0);
}

//*****************************************************************
// plot_window::initialize_selection() -- STATIC method to clear
// selection without doing anything else that might lose the 
// context.  This is a static method used from main() during 
// intialization and by plot_window::clear_selection.
void plot_window::initialize_selection()
{
  // Loop: Loop through all the plots
  for( int i=0; i<nplots; i++) {
    pws[i]->reset_selection_box();
  }
  newly_selected = 0;
  selected = 0;
  previously_selected = 0;
  saved_selection = 0;
  nselected = 0;
  selection_is_inverted = false;
}

//*****************************************************************
// plot_window::clear_selection( *o) -- STATIC method to clear 
// selection, reset color array, and redraw all plots.  This is a 
// static method used only by class plot_window.
void plot_window::clear_selection( Fl_Widget *o)
{
  initialize_selection();
  pws[0]->color_array_from_selection(); // So, I'm lazy.
  redraw_all_plots (0);
}

//*****************************************************************
// plot_window::initialize_textures() -- STATIC method to 
// initialize textures.  This is a static method used only by 
// class plot_window.  It makes use of a global flag variable, 
// textures_initialized. 
void plot_window::initialize_textures()
{
  if( textures_initialized) return;

#if 0
  // Generate texture names
  // glPixelStorei( GL_UNPACK_ALIGNMENT, 1);
  // glGenTextures( 2, texnames);

  // Color for de-selected points when they are displayed
  texture_images[0][0] = r_deselected;
  texture_images[0][1] = g_deselected;
  texture_images[0][2] = b_deselected;
  texture_images[0][3] = 1.0; 

  // Color for de-selected points when they are not displayed
  texture_images[1][0] = 0.00; 
  texture_images[1][1] = 0.00; 
  texture_images[1][2] = 0.00; 
  texture_images[1][3] = 0.0;

  // Loop: Set color(s) for selected points.  Remember that 
  // the 0th color is reserved for deselected points.
  for( int i=0; i<nplots; i++) {
    int j=4*(i+1);  

    // Initial colors of selected points
    texture_images[0][j+0] = 
      texture_images[1][j+0] = pws[0]->r_selected;  // need to fix this.
    texture_images[0][j+1] = 
      texture_images[1][j+1] = pws[0]->g_selected;
    texture_images[0][j+2] = 
      texture_images[1][j+2] = pws[0]->b_selected;
    texture_images[0][j+3] = 
      texture_images[1][j+3] = 1.0; 
  }

  // Loop: Set textures?
  for( unsigned int i=0; 
       i < sizeof(texnames)/sizeof(texnames[0]); i++) {
    glBindTexture( GL_TEXTURE_1D, texnames[ i]);
    glTexParameteri( GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri( GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri( GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexImage1D( GL_TEXTURE_1D, 0, GL_RGBA8, MAXPLOTS, 0, GL_RGBA, GL_FLOAT, texture_images[i]);
  }
  
#endif 0

  // Set flag to indicate that textures have been initialized
  textures_initialized = 1;
}
