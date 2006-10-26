// viewpoints - interactive linked scatterplots and more.
// copyright 2005 Creon Levit, all rights reserved.
//*****************************************************************
// File name: plot_window.h
//
// Class definitions:
//   plot_window -- Plot window
//
// Classes referenced:
//   control_panel_window -- Control panel window
//   myCompare -- Member class used by std::stable_sort
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
// Author: Creon Levitt   unknown
// Modified: P. R. Gazis  02-OCT-2006
//*****************************************************************

// Protection to make sure this header is not included twice
#ifndef PLOT_WINDOW_H
#define PLOT_WINDOW_H 1

// Include the necessary include libraries
#include "include_libraries_vp.h"

// Include globals
#include "global_definitions_vp.h"

// Declare class control_panel_window here so it can be referenced
// class control_panel_window;

//*****************************************************************
// Class: plot_window
//
// Class definitions:
//   plot_window -- Maintain and manage plot window
//
// Classes referenced:
//   control_panel_window
//
// Purpose: Derived class of Fl_Gl_Window to construct, draw,
//   and manage a plot window.  Tthe plot_window class is subclass 
//   of an ftlk openGL window that also handles certain keyboard & 
//   mouse events.  It is where data is displayed.  There are 
//   usually several open at one time.
//
// Functions:
//   plot_window( w, h) -- Constructor
//   initialize() -- Initialization method
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
//   compute_rank( blitz::Array<float,1>, blitz::Array<int,1>);
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
//   initialize_textures() -- initial setup of rgba used for selected 
//     and deselected points
//
// Author: Creon Levitt    unknown
// Modified: P. R. Gazis   26-OCT-2006
//*****************************************************************
class plot_window : public Fl_Gl_Window
{
  protected:
    
    // If they are available, use vertex buffer objects (VBOs)
    #ifdef USE_VBO
      // have we initialized the openGL vertex buffer object?
      int VBOinitialized;
      void initializeVBO();
      // and have we filled it with our chunk of vertex data yet?
      int VBOfilled;
    #endif // USE_VBO

    // Draw routines
    void draw();
    void draw_grid();
    void draw_axes();
    void draw_selection_information();
    void draw_data_points();
    void draw_center_glyph();
    void update_linked_transforms();

    // Event handlers
    int handle( int event);
    void handle_selection();

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
    int nbins;
    blitz::Array<float,2> counts, counts_selected;
    float xhscale, yhscale;
    void compute_histogram( int);
    void draw_histograms();

    int show_center_glyph;
    int selection_changed;

    // Number of plot windows
    static int count;

  public:
    plot_window( int w, int h);   // Constructor
    void initialize();

    // min and max for data's bounding box in x, y, and z;
    float amin[3], amax[3];
    float wmin[3], wmax[3];

    // Define buffers to calculate histograms
    blitz::Array<float,2> vertices;
    blitz::Array<int,1> x_rank, y_rank, z_rank;
    static const int nbins_default = 128;
    static const int nbins_max = 1024;

    // Routines to compute histograms and normalize data
    void compute_rank(blitz::Array<float,1> a, blitz::Array<int,1> a_rank, int var_index);
    void compute_histograms();
    int normalize(blitz::Array<float,1> a, blitz::Array<int,1> a_rank, int style, int axis_index);

    // Define strings to hold axis labels
    std::string xlabel, ylabel, zlabel;

    // Pointer to and index of the control panel tab associated 
    // with this plot window.  Each plot window has the same index
    // as its associated tab.
    control_panel_window *cp;
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
    static void initialize_textures();
    
    // Static variable to hold he initial fraction of the window 
    // to be used for data to allow room for axes, labels, etc.
    static const float initial_pscale; 

    // Specify how the RGB and alpha source and destination 
    // blending factors are computed.
    static int sfactor;
    static int dfactor;

    static blitz::Array<GLfloat,2> colors_show_deselected; // when deselected points are visible
    static blitz::Array<GLfloat,2> colors_hide_deselected; // when deselected points are invisible
    static GLfloat pointscolor[ 4];
    static int textures_initialized;
};

#endif   // PLOT_WINDOW_H
