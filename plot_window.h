// viewpoints - interactive linked scatterplots and more.
// copyright 2005 Creon Levit and Paul Gazis, all rights reserved.
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
// Modified: P. R. Gazis  10-JUL-2008
//***************************************************************************

// Protection to make sure this header is not included twice
#ifndef PLOT_WINDOW_H
#define PLOT_WINDOW_H 1

// Include the necessary include libraries
#include "include_libraries_vp.h"

// Include globals
#include "global_definitions_vp.h"

// Declare class Control_Panel_Window so it can be used for definitions of
// member variables of this class
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
//   displayed.  There will usually be several open at one time.
//
// Functions:
//   Plot_Window() -- Default constructor
//   Plot_Window( w, h) -- Constructor
//   initialize() -- Initialization method
//   serialize( &ar, iFileVersion) -- Perform serialization
//   make_state() -- Generate and save state parameters for this window
//   copy_state( *pw) -- Copy state parameters from another window
//   load_state() -- Load state parametsr into widgets
//
//   initialize_VBO() -- Initialize VBO for this window
//   fill_VBO() -- Fill the VBO for this window
//   initialize_indexVBO( int) -- Initialize one brush's index VBO
//   initialize_indexVBOs() -- Initialize all index VBOs
//   fill_indexVBOs() -- Fill all index VBOs with the indices of the vertices they should plot.
//
//   draw() -- Draw plot
//   draw_background() -- Draw background
//   draw_grid() -- Draw grid
//   draw_selection_information() -- Draw selection information
//   void draw_axes() -- Draw axes
//   draw_data_points() -- Draw data points
//   void draw_center_glyph() -- Draw a cross at the center of a zoom.  Stolen from flashearth.com
//   void update_linked_transforms() -- Replicate scale and translatation for linked axes
//   enable_sprites(int) -- Enable sprites
//   enable_regular_points() -- Enable regular points
//   enable_antialiased_points() -- Enable anti-aliased points
//   disable_sprites() -- Diasable sprites
//   clear_alpha_planes() -- Clear those pesky alpha planes
//   clear_stencil_buffer() -- Clear stencil buffer
//
//   handle( event) -- Main event handler
//   handle_selection() -- update or change selection based on mouse position
//   run_timing_test() --
//
//   void screen_to_world( xs, ys, x, y) -- Screen to word coords (only works for 2D)
//   void print_selection_stats() -- print number of points and % of points currently selected
//
//   compute_histogram( int) -- Compute histogram bin counts for one variable
//   draw_x_histogram( bin_counts, nbins);
//   draw_y_histogram( bin_counts, nbins);
//   draw_histograms() --
//   density_1D( a, axis) --
//
//   compute_histograms () -- Compute both histograms for marginals of 2D plot
//   compute_rank(int var_index) create an array of indices that rank order a variable (basically a sort).
//   normalize() -- Normalize data based on user-selected normalization scheme
//
//   extract_data_points() -- Extract data for these axes
//   transform_2d() -- Transform all (x,y) to (f(x,y), g(x,y))
//   reset_selection_box() -- Reset selection box
//   color_array_from_selection() -- Fill index arrays 
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
// Modified: P. R. Gazis  08-JUL-2008
//***************************************************************************
class Plot_Window : public Fl_Gl_Window
{
  protected:
    // Need this declaration to grant the serialization library access to 
    // private member variables and functions.
#ifdef SERIALIZATION
    friend class boost::serialization::access;
#endif //SERIALIZATION
    
    // Define state parameters for serialization
    int x_save, y_save, w_save, h_save;

    // When the class Archive corresponds to an output archive, the &
    // operator is defined similar to <<.  Likewise, when the class Archive 
    // is a type of input archive the & operator is defined similar to >>.
    // It is easiest to define this serialize method inline.
#ifdef SERIALIZATION
    template<class Archive>
    void serialize( Archive & ar, const unsigned int /* file_version */)
    {
      // Use a dynamic_cast to determine if this is an output operation
      if( (dynamic_cast<boost::archive::xml_oarchive *> (&ar))) make_state();

      // Embed serialization in a try-catch loop so we can pass exceptions
      try{
        ar & boost::serialization::make_nvp( "index", index);
        ar & boost::serialization::make_nvp( "x_save", x_save);
        ar & boost::serialization::make_nvp( "y_save", y_save);
        ar & boost::serialization::make_nvp( "w_save", w_save);
        ar & boost::serialization::make_nvp( "h_save", h_save);
      }
      catch( exception &e) {}
    }
#endif //SERIALIZATION

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
    void draw_background();
    void draw_grid();
    void draw_axes();
    void draw_selection_information();
    void draw_data_points();
    void draw_center_glyph();
    void draw_resize_knob();
    void update_linked_transforms();
    void enable_sprites(int);
    void enable_regular_points();
    void enable_antialiased_points();
    void disable_sprites();
    void clear_alpha_planes();
    void clear_stencil_buffer();

    // Event handlers
    int handle( int event);
    void handle_selection();
    void run_timing_test();
    
    void screen_to_world(float xs, float ys, float &x, float &y);
    void update_selection_from_footprint();
    void print_selection_stats();
    void interval_to_strings (const int column, const float x1, const float x2, char *buf1, char *buf2);
    void select_on_string(const char *str,int col);
    void center_on_click(int x, int y);

    // Event parameters
    int xprev, yprev, xcur, ycur;
    float xdragged, ydragged;
    float xcenter, ycenter, zcenter;
    float xscale, yscale, zscale;
    float initial_scale;
    // approximately how much we've scaled this plot since it was first displayed
    float magnification;
    float xzoomcenter, yzoomcenter, zzoomcenter;
    float xdown, ydown, xtracked, ytracked;

    // Arrays and routines for histograms
    int nbins[3];
    blitz::Array<float,2> counts, counts_selected;
    float xhscale, yhscale;
    void compute_histogram( int);
    void draw_x_histogram(const blitz::Array<float,1>bin_counts, const int nbins);
    void draw_y_histogram(const blitz::Array<float,1>bin_counts, const int nbins);
    void draw_histograms();
    void density_1D (blitz::Array<float,1>a, const int axis);

    int show_center_glyph;
    int selection_changed;

    // Number of plot windows
    static int count; // MCL XXX isn't this the same as nplots?  is it consistent?

  public:
    Plot_Window();   // Default constructor
    Plot_Window( int w, int h, int new_index);   // Constructor
    void initialize();
    void make_state();
    void copy_state( Plot_Window* pw);
    void load_state();
    static int active_plot;

    // Initialize and fill index VBO for this window
    void initialize_indexVBO(int);
    void fill_indexVBO(int);

    // true min and max of the data before normalization and transformation
    float tmin[3], tmax[3];

    // min and max for normalized data's bounding box in x, y, and z;
    float amin[3], amax[3];
    // data values at edges of "window"
    // (the X and Y axes lines bound the left and bottom edges of this "window")
    float wmin[3], wmax[3];

    // openGL vertices of points to be plotted
    blitz::Array<float,2> vertices;

    // indices of points when ranked according to their x, y, or z coordinate 
    // respectively
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
    void color_array_from_selection();
    void update_selection_color_table ();

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
    static void clear_selections( Fl_Widget *o);
    static void initialize_sprites();
    
    // Static variable to hold he initial fraction of the window to be used 
    // for data to allow room for axes, labels, etc.
    static const float initial_pscale; 

    // Specify how the RGB and alpha source and destination blending factors 
    // are computed.
    static int sfactor;
    static int dfactor;

    // Indices of points for rendering, packed acording to selection state;
    static blitz::Array<unsigned int,2> indices_selected; 

    // point sprites-specific data
    static int sprites_initialized;

    // openGL "texture names" to bind with sprites
    static GLuint spriteTextureID[NSYMBOLS];

    // Array of texture images for point sprites
    static GLubyte* spriteData[NSYMBOLS];

    // Buffers to hold sprite (texture image) data
    static const GLsizei spriteWidth = 64, spriteHeight = 64, spriteDepth = 4;
    static const GLsizei spriteSize = spriteWidth*spriteHeight;
    static void make_sprite_textures( void);

    // The GLContextshared by all Plot_Windows
    static void *global_GLContext; 
};

#endif   // PLOT_WINDOW_H
