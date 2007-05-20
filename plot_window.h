// viewpoints - interactive linked scatterplots and more.
// copyright 2005 Creon Levit, all rights reserved.
//***************************************************************************
// File name: Plot_window.h
//
// Class definitions:
//   Plot_Window -- Plot window
//
// Classes referenced:
//   Control_panel_window -- Control panel window
//   MyCompare -- Member class used by std::stable_sort
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
// Purpose: Plot window class for Creon Levitt's 
//   viewpoints
//
// General design philosophy:
//   1) Review and add comments!
//
// Author: Creon Levit    2005-2006
// Modified: P. R. Gazis  23-APR-2007
//***************************************************************************

// Protection to make sure this header is not included twice
#ifndef PLOT_WINDOW_H
#define PLOT_WINDOW_H 1

// Include the necessary include libraries
#include "include_libraries_vp.h"

// Include globals
#include "global_definitions_vp.h"

// Declare class control_panel_window here so it can be referenced
class Control_Panel_Window;

//***************************************************************************
// Class: Plot_Window
//
// Class definitions:
//   Plot_Window -- Maintain and manage plot window
//
// Classes referenced:
//   Control_panel_window
//
// Purpose: Derived class of Fl_Gl_Window to construct, draw, and manage a 
//   plot window.  The Plot_Window class is subclass of an ftlk openGL window 
//   that also handles certain keyboard and mouse events.  It is where data is
//   displayed.  There are usually several open at one time.
//
// Functions:
//   Plot_Window( w, h) -- Constructor
//   initialize() -- Initialization method
//
//   initialize_VBO() -- Initialize VBO for this window
//   fill_VBO() -- Fill the VBO for this window
//   initialize_indexVBO( int) -- Initialize the 'index VBO'
//   initialize_indexVBOs() -- Initialize 'index VBOs'
//   fill_indexVBOs() -- Fill 'index VBO' for this window
//
//   draw() -- Draw plot
//   draw_grid() -- Draw grid
//   void draw_axes() -- Draw axes
//   draw_data_points() -- Draw data points
//   void draw_center_glyph() -- ???
//   void update_linked_transforms() -- ???
//
//   handle( event) -- Main event handler
//   handle_selection() -- Handle this selection?
//
//   void screen_to_world( xs, ys, x, y) -- Screen to word coords?
//   void print_selection_stats() -- ???
//
//   compute_histogram( int) -- Compute a histogram
//   draw_histograms() -- Draw histograms
//
//   compute_rank(int var_index);
//   compute_histograms () -- Compute histograms
//   normalize() -- Normalize data
//
//   extract_data_points() -- Extract data for these axes
//   transform_2d() -- Transofmr to 2D coords
//   reset_selection_box() -- Reset selection box
//   color_array_from_new_selection() -- ???
//   color_array_from_selection() -- Fill index arrays
//   update_selection_color_table() -- change the rgba tables used for
//     selected and/or deselected points
//   choose_color_selected() -- Choose color of selcted data
//   reset_view() -- Reset plot
//   redraw_one_plot() -- Redraw one plot
//   change_axes() -- Change axes of this plot
//
// Static functions:
//   upper_triangle_incr( i, j, nvars) -- Traverse upper triangle
//   redraw_all_plots( p) -- Redraw all plots
//   delete_selection( *o) -- Delete selcted points
//   invert_selection() -- Invert selected and nonselcted points
//   toggle_display_delected( *o) -- Toggle colors
//   initialize_selection() -- Clear selection
//   clear_selection( *o) -- Clear selection and redraw plots
//   initialize_sprites() -- initial setup of rgba used for selected 
//     and deselected points when rendered as openGL point sprites.
//
// Author: Creon Levit    2005-2006
// Modified: P. R. Gazis  23-APR-2007
//***************************************************************************
class Plot_Window : public Fl_Gl_Window
{
  protected:
    // If they are available, use vertex buffer objects (VBOs)
    // have we initialized the openGL vertex buffer object?
    int VBOinitialized;
    void initialize_VBO();
    // and have we filled it with our chunk of vertex data yet?
    bool VBOfilled;
    void fill_VBO();
    
    // have we initialized the shared openGL index vertex buffer objects?
    static int indexVBOsinitialized;
    void initialize_indexVBOs();
    // and are they filled with the latest index data?
    static int indexVBOsfilled;
    void fill_indexVBOs();
    
    // Draw routines
    void draw();
    void draw_grid();
    void draw_axes();
    void draw_selection_information();
    void draw_data_points();
    void draw_center_glyph();
    void update_linked_transforms();
    void enable_sprites(symbol_type);
    void enable_regular_points();
    void enable_antialiased_points();
    void disable_sprites();

    // Event handlers
    int handle( int event);
    void handle_selection();
    void run_timing_test();
    
    void screen_to_world(float xs, float ys, float &x, float &y);
    void print_selection_stats();

    // Event parameters
    int xprev, yprev, xcur, ycur;
    float xdragged, ydragged;
    float xcenter, ycenter, zcenter;
    float xscale, yscale, zscale;
    float xzoomcenter, yzoomcenter, zzoomcenter;
    float xdown, ydown, xtracked, ytracked;
    int extend_selection;

    // Arrays and routines for histograms
    int nbins[3];
    blitz::Array<float,2> counts, counts_selected;
    float xhscale, yhscale;
    void compute_histogram( int);
    void draw_histograms();
    void density_1D (blitz::Array<float,1>a, const int axis);

    int show_center_glyph;
    int selection_changed;

    // Number of plot windows
    static int count; // MCL XXX isn't this the same as nplots?  is it consistent?

  public:
    Plot_Window( int w, int h, int new_index);   // Constructor
    void initialize();

    // Initialize and fill index VBO for this window
    void initialize_indexVBO(int);
    void fill_indexVBO(int);

    // min and max for data's bounding box in x, y, and z;
    float amin[3], amax[3];
    // data values at edges of "window"
    // (the X and Y axes lines bound the left and bottom edges of this "window")
    float wmin[3], wmax[3];

    // openGL vertices of points to be plotted
    blitz::Array<float,2> vertices;
    // indices of points when ranked according to their x, y, or z coordinate respectively
    blitz::Array<int,1> x_rank, y_rank, z_rank;

    // constants for histogramming
    static const int nbins_default = 128;
    static const int nbins_max = 1024;

    // Routines to compute histograms and normalize data
    void compute_rank(int var_index);
    void compute_histograms();
    int normalize( 
      blitz::Array<float,1> a, 
      blitz::Array<int,1> a_rank, 
      int style, int axis_index);

    // Define strings to hold axis labels
    std::string xlabel, ylabel, zlabel;

    // Pointer to and index of the control panel tab associated with this plot 
    // window.  Each plot window has the same index as its associated tab.
    Control_Panel_Window *cp;
    int index;

    // Initial ordering of windows on screen.  Upper left is (1,1)
    int row, column;

    // More plot routines
    int extract_data_points();
    int transform_2d();

    // Routines and variables to handle point colors and selection
    void reset_selection_box();
    void color_array_from_new_selection();
    void color_array_from_selection();
    void update_selection_color_table ();
    void choose_color_selected ();
    double r_selected, g_selected, b_selected;
    static double r_deselected, g_deselected, b_deselected;

    // Routines to redraw plots
    void reset_view();
    void redraw_one_plot();
    void change_axes( int nchange);
    float angle;
    int needs_redraw;
    unsigned do_reset_view_with_show;
    
    // Static methods moved here from vp.cpp
    static void upper_triangle_incr( int &i, int &j, const int nvars);
    static void redraw_all_plots( int p);
    static void delete_selection( Fl_Widget *o);
    static void invert_selection();
    static void toggle_display_deselected( Fl_Widget *o);
    static void initialize_selection();
    static void clear_selection( Fl_Widget *o);
    static void initialize_sprites();
    
    // Static variable to hold he initial fraction of the window to be used 
    // for data to allow room for axes, labels, etc.
    static const float initial_pscale; 

    // Specify how the RGB and alpha source and destination blending factors 
    // are computed.
    static int sfactor;
    static int dfactor;

    // Count of points in each plot's selected set, index zero reserved for 
    // nonselected.
    // number_selected[1] = count of points selected in 1st plot (i.e, pws[0])....
    // number_selected[n+1] = count of points selected in nth plot (pws[n])
    // number_selected[0] = count of nonselected points
    // sum(number_selected)==npoints;
    static blitz::Array<unsigned int,1> number_selected; 

    // Indices of points for rendering, packed acording to selection state;
    static blitz::Array<unsigned int,2> indices_selected; 

    // Colors to use deselected points are visible or invisible
    static blitz::Array<GLfloat,2> colors_show_deselected;
    static blitz::Array<GLfloat,2> colors_hide_deselected;
    static GLfloat pointscolor[ 4];

    // point sprites-specific data
    static int sprites_initialized;

    // number of distinct sprites available for plotting symbols
    static const int Nsprites=5;

    // openGL "texture names" to bind with sprites
    static GLuint spriteTextureID[Nsprites];

	// Buffers to hold sprite (texture image) data
    static const GLsizei spriteWidth = 8, spriteHeight = 8, spriteDepth  = 2;
    static const GLsizei spriteSize = spriteWidth*spriteHeight*spriteDepth;

    // The GLContextshared by all Plot_Windows
    static void *global_GLContext; 
};

#endif   // PLOT_WINDOW_H
