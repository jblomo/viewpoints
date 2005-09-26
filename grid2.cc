
// C includes
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <math.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <getopt.h>

#ifdef __APPLE__
#include <float.h>
#else
#include <values.h>
#endif // __APPLE__

//  C++ includes
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
 
// FLTK 
#include <FL/math.h>
#include <FL/gl.h>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Hor_Value_Slider.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Color_Chooser.H>


// flews (FLTK extenstion) extras
#include <FL/Fl_flews.h>
#include <FL/Fl_Value_Slider_Input.H>
#include "Fl_Hor_Value_Slider_Input.H"

// OpenGL extensions
#ifdef __APPLE__
#  include <OpenGL/glext.h>
#else
#  include <GL/glext.h>
#endif

// Blitz++ (C++ array operations via template metaprogramming)
#include <blitz/array.h>

// gsl (gnu scientific library)
#include <gsl/gsl_math.h>
#include <gsl/gsl_cdf.h>

// BLAS
#ifdef __APPLE__
#include <vecLib/vBLAS.h>
#elif linux
//extern "C" {
//# include <cblas.h>
//}
#endif // __APPLE__
        
// debugging and application-specific definitions
#include "grid2.H"

using namespace std;

int format=ASCII;		// default file format

const int nvars_max = 256;  	// maximum number of columns allowed in data file
const int MAXPOINTS = 1200000;	// maximum number of rows (points, or samples) in data file
const int skip = 0;		// skip this many columns at the beginning of each row

int nrows=2, ncols=2;		// layout of plot windows
int nplots = nrows*ncols;	// number of plot windows
const int maxplots=64;		
class plot_window; 		// love those one-pass compilers.
class control_panel_window;

int npoints = MAXPOINTS;	// actual number of rows in data file
int npoints_cmd_line = 0;	// number of points, if specified, on cmd line.
int nvars = nvars_max;		// actual number of columns in data file

int scale_histogram = 0;	// global toggle between scale histgram & scale view :-(

blitz::Array<float,2> points(nvars_max,MAXPOINTS);	// main data array
blitz::Array<int,2> ranked_points;					// main data, ranked, as needed.
blitz::Array<int,1> ranked;							// flag: 1->column is ranked, 0->it is not
blitz::Array<int,1> identity;	// holds a(i)=i.

blitz::Array<int,1> newly_selected;	// true iff a point is in the newly selected set
blitz::Array<int,1> selected;	// used when adding to selection
blitz::Array<float,2> colors, altcolors;

double r_selected=0.01, g_selected=0.01, b_selected=1.0;
double r_deselected=1.0, g_deselected=0.01, b_deselected=0.01;

// temporary array (reference) for qsort
blitz::Array<float,1> tmp_points;
float *tpoints;

std::vector<std::string> column_labels; // vector of strings to hold variable names

const float initial_pscale = 0.8; // initial fraction of the window for datapoints, to allow room for axes, etc.

float xmin, xmax, gmax;

float pointsize = 1.0;

int istart = 0;

int sfactor = GL_CONSTANT_COLOR;
int dfactor = GL_DST_ALPHA;

void redraw_all_plots (int);
void reset_all_plots (void);

// Main control panel
Fl_Window *main_control_panel;

// Main control panel's top level (global) widgets:
Fl_Tabs *cpt;			// tabs to hold individual plot's virtual control panels
Fl_Hor_Value_Slider_Input *npoints_slider;	// maximum number of points to display in all plots
Fl_Button *add_to_selection_button, *clear_selection_button, *delete_selection_button;
Fl_Button *display_deselected_button, *invert_selection_button;
Fl_Button *write_data_button;
Fl_Button *choose_color_selected_button, *choose_color_deselected_button;

// the plot_window class is subclass of an ftlk openGL window that also handles
// certain keyboard & mouse events.  It is where data is displayed.
// There are usually several open at one time.
class plot_window : public Fl_Gl_Window {
protected:
    void draw();
    void draw_grid();
    void draw_axes();
    void draw_labels();
    void draw_summary();
    void draw_data_points();
	void draw_center_glyph ();
    int handle (int event);
    void handle_selection();
	void screen_to_world(float xs, float ys, float &x, float &y);
    int xprev, yprev, xcur, ycur;
    float xdragged, ydragged;
    float xcenter, ycenter, zcenter;
	float xscale, yscale, zscale;
    float xzoomcenter, yzoomcenter, zzoomcenter;
    float xdown, ydown, xtracked, ytracked;
    int selection_changed, extend_selection;
	void set_selection_colors ();
    static int count;
    // histograms
    int nbins;
    blitz::Array<float,2> counts, counts_selected;
    void compute_histogram (int);
    float xhscale, yhscale;
    void draw_histograms ();
	int show_center_glyph;
public:
    plot_window(int w, int h);
    blitz::Array<float,2> vertices;
    blitz::Array<int,1> x_rank, y_rank, z_rank;
    float amin[3], amax[3]; // min and max for data's bounding box in x, y, and z;
    float wmin[3], wmax[3]; // min and max for window's bounding box in x, y, and z;
    void compute_rank (blitz::Array<float,1> a, blitz::Array<int,1> a_rank, int var_index);
    static const int nbins_default = 128;
    static const int nbins_max = 1024;
    void compute_histograms ();
    int normalize(blitz::Array<float,1>, blitz::Array<int,1>, int, int);
    std::string xlabel, ylabel, zlabel;
    control_panel_window *cp;	// pointer to the control panel (tab) associated with this plot window
    int index;	// each plot window, and its associated control panel tab, has the same index.
	int row, column; // initial ordering of windows on screen. Upper left window is (1,1)
    int extract_data_points();
    int transform_2d();
    void reset_selection_box();
    void color_array_from_new_selection();
    void color_array_from_selection();
	GLfloat color1[4], color2[4]; // color of selected and deselected points, respectively
    void reset_view();
    void redraw_one_plot();
    float angle;
    int needs_redraw;
};

int plot_window::count = 0;

plot_window::plot_window(int w,int h) : Fl_Gl_Window(w,h) 
{
    count++;
	show_center_glyph = 0;
    vertices.resize(npoints,3);
    x_rank.resize(npoints);
    y_rank.resize(npoints);
    z_rank.resize(npoints);
    nbins = nbins_default;
    counts.resize(nbins_max,3);
    counts_selected.resize(nbins_max,3);
#if 0
    if (can_do(FL_RGB8|FL_DOUBLE|FL_ALPHA|FL_DEPTH))
		mode(FL_RGB8|FL_DOUBLE|FL_ALPHA|FL_DEPTH);  // Can't seem to make this work on PBG4 OSX 
    else
		mode(FL_RGB|FL_DOUBLE|FL_ALPHA);
#endif
    mode(FL_RGB8|FL_DOUBLE|FL_ALPHA);
}


class control_panel_window : public Fl_Group
{
protected:
    void maybe_redraw ();

public:
    control_panel_window(int x, int y, int w, int h);
    void make_widgets(control_panel_window *cpw);
    void extract_and_redraw ();
    static void static_extract_and_redraw (Fl_Widget *w, control_panel_window *cpw)
		{ cpw->extract_and_redraw(); }
    static void static_maybe_redraw(Fl_Widget *w, control_panel_window *cpw)
		{ cpw->maybe_redraw() ;}
    static void replot (Fl_Widget *w, control_panel_window *cpw)
		{ /* cpw->pw->redraw(); */ cpw->pw->needs_redraw=1;}
	static void reset_view (Fl_Widget *w, control_panel_window *cpw)
		{ cpw->pw->reset_view() ;}
	static void redraw_one_plot (Fl_Widget *w, control_panel_window *cpw)
		{ cpw->pw->redraw_one_plot();}
    Fl_Hor_Value_Slider_Input *pointsize_slider;
    Fl_Hor_Value_Slider_Input *Bkg, *Lum, *Alph;
    Fl_Hor_Value_Slider_Input *rot_slider;
    Fl_Hor_Value_Slider_Input *nbins_slider;
    Fl_Choice *varindex1, *varindex2, *varindex3;
	
	Fl_Button *reset_view_button;
    Fl_Button *spin, *dont_clear, *show_points, *show_axes, *show_grid, *show_labels, *show_histogram;
	Fl_Button *show_summary;
//	Fl_Button *x_equals_delta_x, *y_equals_delta_x;
    Fl_Group *transform_style;
    Fl_Button *sum_vs_difference, *polar, *no_transform;
    Fl_Choice *x_normalization_style, *y_normalization_style, *z_normalization_style;

    plot_window *pw;  // pointer to the plot window associated with this control panel (tab)
    int index;	// each plot window, and its associated control panel tab, has the same index.
};

control_panel_window::control_panel_window(int x, int y, int w, int h) : Fl_Group(x, y, w, h)
{
}

plot_window *pws[maxplots];
control_panel_window *cps[maxplots];

// these menu related lists should really be class variables in class control_panel_window
Fl_Menu_Item varindex_menu_items[nvars_max+2]; 

const int NORMALIZATION_NONE 	= 0;
const int NORMALIZATION_MINMAX 	= 1;
const int NORMALIZATION_ZEROMAX = 2;
const int NORMALIZATION_MAXABS	= 3;
const int NORMALIZATION_THREESIGMA = 4;
const int NORMALIZATION_RANK = 5;
const int NORMALIZATION_GAUSSIANIZE = 6;

const char *normalization_style_labels[] = { "none","minmax","zeromax","maxabs","threesigma","rank","gaussianize"};

int normalization_styles[] = 
{NORMALIZATION_NONE, NORMALIZATION_MINMAX, NORMALIZATION_ZEROMAX, NORMALIZATION_MAXABS, NORMALIZATION_THREESIGMA, NORMALIZATION_RANK, NORMALIZATION_GAUSSIANIZE};

const int n_normalization_styles = sizeof(normalization_styles)/sizeof(normalization_styles[0]);

Fl_Menu_Item normalization_style_menu_items[n_normalization_styles+1];
  
void
invert_selection ()
{
	selected(blitz::Range(0,npoints-1)) = 1-selected(blitz::Range(0,npoints-1));
	pws[0]->color_array_from_selection ();
	redraw_all_plots (0);
}

void
toggle_display_delected(Fl_Widget *o)
{
	// Toggle the value of the button manually, but only if we were called via a keypress in a plot window
	// Shouldn't there be an easier way?
	if (o == NULL)
		display_deselected_button->value(1 - display_deselected_button->value());
	redraw_all_plots (0); // something wrong here....
}

void
clear_selection (Fl_Widget *o)
{
	for (int i=0; i<nplots; i++)
	{
		pws[i]->reset_selection_box ();
	}
	newly_selected = 0;
	selected = 0;
	pws[0]->color_array_from_selection (); // So, I'm lazy.
	redraw_all_plots (0);
}

void
choose_color_selected (Fl_Widget *o)
{
	(void) fl_color_chooser("selected", r_selected, g_selected, b_selected);
}

void
choose_color_deselected (Fl_Widget *o)
{
	(void) fl_color_chooser("deselected", r_deselected, g_deselected, b_deselected);
}

void
delete_selection (Fl_Widget *o)
{
    blitz::Range NVARS(0,nvars-1);
    int ipoint=0;
    for (int n=0; n<npoints; n++)
    {
		if (!selected(n))
		{
			points(NVARS,ipoint) = points(NVARS,n);
			ipoint++;
		}
    }
	if (ipoint != npoints)  // some point(s) got deleted
	{
		ranked = 0;	// everyone's ranking needs to be recomputed

		npoints = ipoint;
		npoints_slider->bounds(1,npoints);
		npoints_slider->value(npoints);

		clear_selection ((Fl_Widget *)NULL);
	
		for (int j=0; j<nplots; j++)
		{
			cps[j]->extract_and_redraw();
		}
	}
}

int 
plot_window::handle(int event)
{
	// current plot window (getting mouse drags, etc) must get redrawn before others
	// so that selections get colored correctly.  Ugh.
    switch(event) {
    case FL_PUSH:
		DEBUG(cout << "FL_PUSH at " << xprev << ", " << yprev << endl);
		cpt->value(cps[this->index]);	// show the control panel associated with this plot window.
		xprev = Fl::event_x();
		yprev = Fl::event_y();

		if ((Fl::event_state() == FL_BUTTON2) || (Fl::event_state() == (FL_BUTTON1 | FL_CTRL)))
		{
#if 0
			// XXX wish this worked
			xzoomcenter = (float)xprev;
			xzoomcenter = + (2.0*(xzoomcenter/(float)w()) -1.0) ; // window -> [-1,1]
			
			yzoomcenter = (float)yprev;
			yzoomcenter = - (2.0*(yzoomcenter/(float)h()) -1.0) ; // window -> [-1,1]
#endif
		}

		if (Fl::event_state() & FL_BUTTON1) // left button down = start new selection
		{
			selected(blitz::Range(0,npoints-1)) = selected(blitz::Range(0,npoints-1));
			
			if (! (Fl::event_key(FL_Shift_L) || Fl::event_key(FL_Shift_R))) // not moving or extending old selection
			{
				extend_selection = 0;
				
				xdown = (float)xprev;
				xdown = + (2.0*(xdown/(float)w()) -1.0) ; // window -> [-1,1]
				xdown = xdown / xscale;
				xdown = xdown + xcenter;
			
				ydown = (float)yprev;
				ydown = - (2.0*(ydown/(float)h()) -1.0) ; // window -> [-1,1]
				ydown = ydown/yscale;
				ydown = ydown + ycenter;
			}
		}
		// start translating
		if ((Fl::event_state() == FL_BUTTON3) || (Fl::event_state() == (FL_BUTTON1 | FL_ALT)))
		{
			show_center_glyph = 1;
			needs_redraw = 1;
		}
		return 1;
    case FL_DRAG:
		DEBUG (printf ("FL_DRAG, event_state: %x\n", Fl::event_state()));
		xcur = Fl::event_x();
		ycur = Fl::event_y();
		xdragged =   xcur - xprev;
		ydragged = -(ycur - yprev);
		xprev = xcur;
		yprev = ycur;

		// translate = drag with right mouse (or alt-left-mouse)
		if ((Fl::event_state() == FL_BUTTON3) || (Fl::event_state() == (FL_BUTTON1 | FL_ALT)))
		{
			float xmove = xdragged*(1/xscale)*(2.0/w());
			float ymove = ydragged*(1/yscale)*(2.0/h());
			xcenter -= xmove;
			ycenter -= ymove;
			//			wmin[0] -= xmove; wmax[0] -= xmove;
			//			wmin[1] -= ymove; wmax[1] -= ymove;
			DEBUG ( cout << "xcenter, ycenter: " << xcenter << ", " << ycenter << endl);
			// redraw ();
			show_center_glyph = 1;
			needs_redraw = 1;
		}

		// scale = drag with middle-mouse (or c-left-mouse)
		else if ((Fl::event_state() == FL_BUTTON2) || (Fl::event_state() == (FL_BUTTON1 | FL_CTRL)))
		{
			if (scale_histogram)
			{
				xhscale *= 1 + xdragged*(2.0/w());
				yhscale *= 1 + ydragged*(2.0/h());
			} else {
				xscale *= 1 + xdragged*(2.0/w());
				yscale *= 1 + ydragged*(2.0/h());
				// not quite.... maybe try another approach.
				//				wmin[0] /= 1 + xdragged*(2.0/w()); wmax[0] /= 1 + xdragged*(2.0/w());
				//				wmin[1] /= 1 + ydragged*(2.0/h()); wmax[1] /= 1 + ydragged*(2.0/h());
				DEBUG ( cout << "xscale, yscale: " << xscale << ", " << yscale << endl );
			}
			// redraw();
			needs_redraw = 1;

		}

		// continue selection = drag with left mouse
		else if (Fl::event_state() & FL_BUTTON1)
		{
			// right key down = move selection
			// left shift down = extend selection (bug on OSX - no left key events)
			if (Fl::event_key(FL_Shift_L) || Fl::event_key(FL_Shift_R))
			{
				xdown += xdragged*(1/xscale)*(2.0/w());
				ydown += ydragged*(1/yscale)*(2.0/h());
				xtracked += xdragged*(1/xscale)*(2.0/w());
				ytracked += ydragged*(1/yscale)*(2.0/h());
				if (Fl::event_key(FL_Shift_R))
				{
					extend_selection = 0;
				} else {
					extend_selection = 1;
				}
			} else {
				xtracked = + (2.0*(xcur/(float)w()) -1.0) ; // window -> [-1,1]
				xtracked = xtracked / xscale;
				xtracked = xtracked + xcenter;
				
				ytracked = - (2.0*(ycur/(float)h()) -1.0) ; // window -> [-1,1]
				ytracked = ytracked/yscale;
				ytracked = ytracked + ycenter;
			}
			int isdrag = !Fl::event_is_click();
			// printf ("FL_DRAG & FL_BUTTON1, event_state: %x  isdrag = %d  xdragged=%f  ydragged=%f\n", Fl::event_state(), isdrag, xdragged, ydragged);
			if (isdrag==1 && ((abs(xdragged)+abs(ydragged))>1))
				{
					selection_changed = 1;
					redraw_all_plots (index);
				}
		}
		return 1;
    case FL_RELEASE:   
		// mouse up
		DEBUG (cout << "FL_RELEASE at " << Fl::event_x() << ", " << Fl::event_y() << endl);
		// selection_changed = 0;
		if (show_center_glyph)
		{
			show_center_glyph = 0;
			needs_redraw = 1;
		}
		return 1;
    case FL_KEYDOWN:
		// keypress, key is in Fl::event_key(), ascii in Fl::event_text()
		// Return 1 if you understand/use the keyboard event, 0 otherwise...
		DEBUG ( cout << "FL_KEYDOWN, event_key() = " << Fl::event_key() << endl);
		switch (Fl::event_key())
		{
		// XXX should figure out how to share shortcuts between plot windows and control panels... later
		case 'q': // exit
		case '\027':
			// quit
			exit (0);
		case 'x': // delete slected points from all future processing
		case FL_Delete:
			delete_selection ((Fl_Widget *)NULL);
			return 1;
		case 'i': // invert or restore (uninvert) selection
			invert_selection ();
			return 1;
		case 'c': // clear selection
			clear_selection ((Fl_Widget *)NULL);
			return 1;
		case 'd': // don't display / display deselected dots
			toggle_display_delected ((Fl_Widget *)NULL);
			return 1;
		case 'r':
			extract_data_points ();
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
		switch (Fl::event_key())
		{
		case 'h':
			scale_histogram=0;
			return 1;
		default:
			return 0;
		}
    case FL_SHORTCUT:
		// shortcut, key is in Fl::event_key(), ascii in Fl::event_text()
		// Return 1 if you understand/use the shortcut event, 0 otherwise...
		return 0;
    default:
		// pass other events to the base class...
		return Fl_Gl_Window::handle(event);}
} 


void plot_window::reset_selection_box()
{
    xdragged = ydragged = 0.0;
    xzoomcenter = yzoomcenter = zzoomcenter = 0.0;
    xdown = ydown = xtracked = ytracked = 0.0;
    xprev = yprev = xcur = ycur = 0;
}


void plot_window::reset_view()
{

    int axis2 = (int)(cp->varindex3->mvalue()->user_data());

	xscale = 2.0 / (wmax[0]-wmin[0]);
	yscale = 2.0 / (wmax[1]-wmin[1]);
	if (axis2 != nvars)
		zscale = 2.0 / (wmax[2]-wmin[2]);
	else
		zscale = 1.0;
	
	xscale *= initial_pscale; // datapoints only span 0.8 of the window dimensions, initially
	yscale *= initial_pscale; // which allows room around the edges for labels, tickmarks, histograms....
	zscale *= initial_pscale; // which allows room around the edges for labels, tickmarks, histograms....

	xcenter = (wmin[0]+wmax[0]) / 2.0;
	ycenter = (wmin[1]+wmax[1]) / 2.0;
	if (axis2 != nvars)
		zcenter = (wmin[2]+wmax[2]) / 2.0;
	else
		zcenter = 0.0;

    xhscale = 1.0;
	yhscale = 1.0;

    angle = 0.0;
    cp->spin->value(0);
    cp->rot_slider->value(0.0);
    cp->dont_clear->value(0);

    reset_selection_box ();
    if (count ==1)
		color_array_from_selection ();

    needs_redraw = 1;
}


void plot_window::draw() 
{
    DEBUG (cout << "in draw: " << xcenter << " " << ycenter << " " << xscale << " " << yscale << endl);
    // the valid() property can avoid reinitializing matrix for each redraw:
    if (!valid())
    {
		valid(1);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-1, 1, -1, 1, -MAXFLOAT, MAXFLOAT);
		glViewport(0, 0, w(), h());
		glDisable(GL_LIGHTING);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		//	glEnable(GL_POINT_SMOOTH);
		//	glHint(GL_POINT_SMOOTH_HINT,GL_NICEST);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
#ifdef FAST_APPLE_VERTEX_EXTENSIONS
		glEnableClientState(GL_VERTEX_ARRAY_RANGE_APPLE);
#endif // FAST_APPLE_VERTEX_EXTENSIONS
    }
  
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef (xzoomcenter*xscale, yzoomcenter*yscale, zzoomcenter*zscale);
    glScalef (xscale, yscale, zscale);
    if (cp->spin->value())
		angle += cp->rot_slider->value()/100.0;
    else
		angle = cp->rot_slider->value();
    glRotatef(angle, 0.0, 1.0, 0.1);
    glTranslatef (-xcenter, -ycenter, -zcenter);
    glTranslatef (-xzoomcenter, -yzoomcenter, -zzoomcenter);

    if (cp->dont_clear->value() == 0)
    {
		//glClearColor(0.0,0.0,0.0,0.0);
		glClearColor(cp->Bkg->value(), cp->Bkg->value(), cp->Bkg->value(), 0.0);
		glClearDepth(0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		draw_grid();
		draw_axes();
		draw_labels();
		draw_summary();
    }

    if (selection_changed)
    {
		handle_selection ();
    }
    draw_data_points();
	draw_center_glyph();
    draw_histograms ();
}

void 
control_panel_window::maybe_redraw() 
{
    // kludge.  Avoid double redraw when setting "don't clear".
    if (dont_clear->value())
		return;
    //pw->redraw();
    pw->needs_redraw = 1;
}

void plot_window::draw_grid()
{
    glBlendFunc(GL_ONE, GL_ZERO);
//	glBlendFunc(sfactor, dfactor);
//	glEnable(GL_LINE_SMOOTH);
//	glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
//	glLineWidth(0.5);
    glLineWidth(1.0);
    if (cp->show_grid->value())
    {
		if (cp->Bkg->value() <= 0.2)
			glColor4f(0.2,0.2,0.2,0.0);
		else
			glColor4f(0.8*cp->Bkg->value(), 0.8*cp->Bkg->value(), 0.8*cp->Bkg->value(), 0.0);
		// draw the grid here
		glBegin (GL_LINES);
		for (int k=-1; k<=1; k+=2)
		{
			for (int i=1; i<=10; i++)
			{

				// XY plane
				glVertex3f (-1.0, 0.1*i*k, 0.0); glVertex3f (+1.0, 0.1*i*k, 0.0);
				glVertex3f (0.1*i*k, -1.0, 0.0); glVertex3f (0.1*i*k, +1.0, 0.0);

				// YZ plane
				glVertex3f (0.0, -1.0, 0.1*i*k); glVertex3f (0.0, +1.0, 0.1*i*k);
				glVertex3f (0.0, 0.1*i*k, -1.0); glVertex3f (0.0, 0.1*i*k, +1.0);

				// XZ plane
				glVertex3f (-1.0, 0.0, 0.1*i*k); glVertex3f (+1.0, 0.0, 0.1*i*k);
				glVertex3f (0.1*i*k, 0.0, -1.0); glVertex3f (0.1*i*k, 0.0, +1.0);
			}
		}
		glEnd();

    }
}

void plot_window::screen_to_world (float xscreen, float yscreen, float &xworld, float &yworld)
{
	//xworld = + (2.0*(xscreen/(float)w()) -1.0) ; // window -> [-1,1]
	xworld = (xworld / xscale) + xcenter;

	//yworld = - (2.0*(yscreen/(float)h()) -1.0) ; // window -> [-1,1]
	yworld = (yworld / yscale) + ycenter;
}


void plot_window::draw_axes ()
{
    if (cp->show_axes->value())
		{
			
			glPushMatrix ();
			glLoadIdentity();

			float a = 0.1; // extra (relative) distance that axes extend past leftmost and rightmost tickmarks.
			float b = 1.5; //  scale factor for tickmark length. b<1 -> inwards, b>1 -> outwards, b==1 -> no tick.
			float c = initial_pscale;
			glScalef(c, c, c);

			glBlendFunc(GL_ONE, GL_ZERO);
			if (cp->Bkg->value() <= 0.4)
				glColor4f(0.6,0.6,0.0,0.0);
			else
				glColor4f(0.4*cp->Bkg->value(), 0.4*cp->Bkg->value(), 0.0*cp->Bkg->value(), 0.0);

			glBegin (GL_LINES);

			glVertex3f (-(1+a), -(1+a), -(1+a)); glVertex3f (+(1+a), -(1+a), -(1+a)); // X axis
			glVertex3f (-(1+a), -(1+a), -(1+a)); glVertex3f (-(1+a), +(1+a), -(1+a)); // Y axis
			glVertex3f (-(1+a), -(1+a), -(1+a)); glVertex3f (-(1+a), -(1+a), +(1+a)); // Z axis

			glVertex3f (-1, -(1+a), -(1+a)); glVertex3f (-1, -(1+b*a), -(1+a)); // lower X-axis tick
			glVertex3f (+1, -(1+a), -(1+a)); glVertex3f (+1, -(1+b*a), -(1+a)); // upper X-axis tick
			glVertex3f (-(1+a), -1, -(1+a)); glVertex3f (-(1+b*a), -1, -(1+a)); // lower Y-axis tick
			glVertex3f (-(1+a), +1, -(1+a)); glVertex3f (-(1+b*a), +1, -(1+a)); // upper Y-axis tick
			b = 1; // XXX Z-axis ticks clutter 2D plots
			glVertex3f (-(1+a), -(1+a), -1); glVertex3f (-(1+b*a), -(1+a), -1); // lower Z-axis tick
			glVertex3f (-(1+a), -(1+a), +1); glVertex3f (-(1+b*a), -(1+a), +1); // upper Z-axis tick

			glEnd();
			glPopMatrix ();
		}
}

void plot_window::draw_center_glyph ()
{
    if (!show_center_glyph)
		return;
    glDisable(GL_DEPTH_TEST);
    glPushMatrix ();
    glLoadIdentity();
    if (cp->Bkg->value() <= 0.5)
		glColor4f(0.7,0.7,0.7,0.0);
    else
		glColor4f(0.2,0.2,0.2,0.0);
	glBegin (GL_LINES);
	glVertex3f (-0.025, 0.0, 0.0); glVertex3f (0.025, 0.0, 0.0);
	glVertex3f (0.0, -0.025, 0.0); glVertex3f (0.0, 0.025, 0.0);
	glVertex3f (0.0, 0.0, -0.025); glVertex3f (0.0, 0.0, 0.025);
	glEnd ();
    glPopMatrix ();
}

void plot_window::draw_summary ()
{
    if (!cp->show_summary->value())
		return;

    int axis0 = (int)(cp->varindex1->mvalue()->user_data());
    //int axis1 = (int)(cp->varindex2->mvalue()->user_data());
    //int axis2 = (int)(cp->varindex3->mvalue()->user_data());

    // note: perhaps we should not bother with one if it is too close to the origin?
    glDisable(GL_DEPTH_TEST);
    glPushMatrix ();
    glLoadIdentity();
    gl_font (FL_HELVETICA, 10);
    if (cp->Bkg->value() <= 0.5)
		glColor4f(0.8,0.8,0.8,0.0);
    else
		glColor4f(0.2,0.2,0.2,0.0);
	
	ostringstream ss1,ss2;
	ss1 << "var\tmin\tmax";
	gl_draw((const char *)(ss1.str().c_str()), 0.0f, -.90f);
	ss2 << xlabel << "\t" << points(axis0,ranked_points(axis0,0)) << '\t' << points(axis0,ranked_points(axis0,npoints-1));
	gl_draw((const char *)(ss2.str().c_str()), 0.0f, -.95f);
		
    glPopMatrix ();
}

void plot_window::draw_labels ()
{
    if (!cp->show_labels->value())
		return;
    // note: perhaps we should not bother with one if it is too close to the origin?
    glDisable(GL_DEPTH_TEST);
    glPushMatrix ();
    glLoadIdentity();
    gl_font (FL_HELVETICA, 10);
    if (cp->Bkg->value() <= 0.5)
		glColor4f(0.8,0.8,0.8,0.0);
    else
		glColor4f(0.2,0.2,0.2,0.0);
	
    float xlabel_width = 2.0 * gl_width(xlabel.c_str())/(float)(this->w());
//	float ylabel_width = 2.0 * gl_width(ylabel.c_str())/(float)(this->w());
    float offset = 0.05;  // how far label should be from end of vector
	
    gl_draw((const char *)(xlabel.c_str()), 1.0F-(offset+xlabel_width), 0.0F-offset);
    gl_draw((const char *)(ylabel.c_str()), offset, 1.0F-2*offset);
    glPopMatrix ();
    glEnable(GL_DEPTH_TEST);
}


void plot_window::handle_selection()
{
    int draw_selection_box = 1;
    if (draw_selection_box)
    {
		glBlendFunc(GL_ONE, GL_ZERO);
		glLineWidth(1.0);
		glColor4f(0.25,0.25,0.75,0.0);
		glBegin (GL_LINE_LOOP);
		glVertex2f (xdown, 	ydown);
		glVertex2f (xtracked,   ydown);
		glVertex2f (xtracked,   ytracked);
		glVertex2f (xdown,	ytracked);
		glEnd();
    }
    blitz::Range NPTS(0,npoints-1);	
	
	newly_selected(NPTS) = where((vertices(NPTS,0)>fmaxf(xdown,xtracked) || vertices(NPTS,0)<fminf(xdown,xtracked) ||
								  vertices(NPTS,1)>fmaxf(ydown,ytracked) || vertices(NPTS,1)<fminf(ydown,ytracked)),
								 0,1);
	if (add_to_selection_button->value())
	{
		selected(NPTS) |= newly_selected(NPTS);
	} else {
		selected(NPTS) = newly_selected(NPTS);
	}			

    color_array_from_new_selection ();

    // done flagging selection for this plot
    selection_changed = 0;
	
}
	
float mincolor= 0.01, alpha1 = 1.0;

void plot_window::set_selection_colors ()
{
	color1[0] = fmax(r_deselected, mincolor);
    color1[1] = fmax(g_deselected, mincolor);
    color1[2] = fmax(b_deselected, mincolor);
    color1[3] = alpha1;
 
    color2[0] = fmax(r_selected, mincolor);
    color2[1] = fmax(g_selected, mincolor);
    color2[2] = fmax(b_selected, mincolor);
    color2[3] = alpha1;
}


void plot_window::color_array_from_selection()
{
	set_selection_colors ();

    for (int i=0; i<npoints; i++)
    {
		if (selected(i))
		{
			colors(i,0) = altcolors(i,0) = color2[0];
			colors(i,1) = altcolors(i,1) = color2[1];
			colors(i,2) = altcolors(i,2) = color2[2];
			colors(i,3) = altcolors(i,3) = color2[3];
		} else {
			colors(i,0) = color1[0];
			colors(i,1) = color1[1];
			colors(i,2) = color1[2];
			colors(i,3) = color1[3];
			// altcolors(i,0) = altcolors(i,1) = altcolors(i,2) = altcolors(i,3) = 0.0;
			// setting alpha=0 will cause these deslected points to be culled in draw_data_points
			altcolors(i,3) = 0.0;  
		}
    }
}

void plot_window::color_array_from_new_selection()
{
	if (add_to_selection_button->value())
	{
		set_selection_colors ();
		for (int i=0; i<npoints; i++)
		{
			if (newly_selected(i))
			{
				colors(i,0) = altcolors(i,0) = color2[0];
				colors(i,1) = altcolors(i,1) = color2[1];
				colors(i,2) = altcolors(i,2) = color2[2];
				colors(i,3) = altcolors(i,3) = color2[3];
			} 
		}
	} else {
		color_array_from_selection ();
	}
}

void clearAlphaPlanes()
{
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
    glClearColor (0.0, 0.0, 0.0, 0.0);
    glClear (GL_COLOR_BUFFER_BIT);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void plot_window::draw_data_points()
{
	// cout << "pw[" << index << "]: draw_data_points() " << endl;
    if (!cp->show_points->value())
		return;
//	glDisable(GL_DEPTH_TEST);

//  the following are done once if necessary in the plot_window::draw()
//	glEnable(GL_BLEND);
//	glEnable(GL_POINT_SMOOTH);
//	glHint(GL_POINT_SMOOTH_HINT,GL_NICEST);


    glPointSize(cp->pointsize_slider->value());

    float const_color[4];

    const_color[0] = const_color[1] = const_color[2] = cp->Lum->value(); 
    const_color[3] = cp->Alph->value();

    glBlendColor (const_color[0], const_color[1], const_color[2], const_color[3]);

    glBlendFunc(sfactor, dfactor);

    GLfloat *vp = (GLfloat *)vertices.data();
    GLfloat *cp = (GLfloat *)colors.data();
    GLfloat *altcp = (GLfloat *)altcolors.data();

    // tell the GPU where to find the correct colors for each vertex.
    if (display_deselected_button->value())
    {
		glColorPointer (4, GL_FLOAT, 0, cp);
    }
    else
    {
		glColorPointer (4, GL_FLOAT, 0, altcp);
		// cull any deselected points (alpha==0.0), whatever the blendfunc:
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc (GL_GEQUAL, 0.5);  
    }

    // tell the GPU where to find the vertices;
    glVertexPointer (3, GL_FLOAT, 0, vp);

#ifdef FAST_APPLE_VERTEX_EXTENSIONS
    glVertexArrayParameteriAPPLE (GL_VERTEX_ARRAY_STORAGE_HINT_APPLE, GL_STORAGE_CACHED_APPLE);  // for static data
//  glVertexArrayParameteriAPPLE (GL_VERTEX_ARRAY_STORAGE_HINT_APPLE, GL_STORAGE_SHARED_APPLE);  // for dynamic data
    glVertexArrayRangeAPPLE (3*npoints*sizeof(GLfloat),(GLvoid *)vp);
#endif // FAST_APPLE_VERTEX_EXTENSIONS

    // tell the GPU to draw the vertices.
    glDrawArrays (GL_POINTS, 0, npoints);

    if (!display_deselected_button->value())
		glDisable(GL_ALPHA_TEST);

//	glEnable(GL_DEPTH_TEST);
}

void plot_window::compute_histogram(int axis)
{
    if (!(cp->show_histogram->value()))
		return;

    nbins = (int)(cp->nbins_slider->value());
    // cout << "nbins = " << nbins << endl;
    blitz::Range BINS(0,nbins-1);
    counts(BINS,axis) = 0.0;
    counts_selected(BINS,axis) = 0.0;
    float range = amax[axis]-amin[axis];

    for (int i=0; i<npoints; i++)
    {
		float x = vertices(i,axis);
		int bin=(int)(nbins*((x-amin[axis])/range));
		if (bin < 0)
			bin = 0;
		if (bin >= nbins)
			bin=nbins-1;
		counts(bin,axis)++;
		if (selected(i))
			counts_selected(bin,axis)++;
    }
    counts(BINS,axis) = (3.0*nbins/(float)nbins_default)*counts(BINS,axis)/((float)(npoints));
    counts_selected(BINS,axis) = (3.0*nbins/(float)nbins_default)*counts_selected(BINS,axis)/((float)(npoints));
}

void plot_window::compute_histograms ()
{
    compute_histogram(0);
    compute_histogram(1);
}

void plot_window::draw_histograms()
{
    if (!(cp->show_histogram->value()))
		return;
    // draw histogram
	
    glEnable(GL_DEPTH_TEST);
    glPushMatrix();

    // x axis histograms
    glLoadIdentity();
    glTranslatef (xzoomcenter*xscale, 0.0, 0);
    glScalef (xscale, yhscale, 1.0);
    glTranslatef (-xcenter, -1.0/yhscale, 0.0);
    glTranslatef (-xzoomcenter, 0.0, 0);
    // histograms cover pointclouds
    glTranslatef (0.0, 0.0, 0.1);
    float xwidth = (amax[0]-amin[0]) / (float)(nbins);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // x axis histogram (all points)
    float x = amin[0];
    glColor4f (0.0, 1.0, 0.0, 0.5);
    glBegin(GL_LINE_STRIP);
    glVertex2f(x,0.0);					
    for (int bin=0; bin<nbins; bin++)
    {
		// left edge
		glVertex2f(x,counts(bin,0));			
		// top edge
		glVertex2f(x+xwidth,counts(bin,0));	
		// right edge 
		//glVertex2f(x+xwidth,0.0);
		x+=xwidth;
    }
    glVertex2f(x,0.0);					
    glEnd();

    // Refactor this!
    // x axis histogram (selected points)
    x = amin[0];
    glColor4f (0.25, 1.0, 0.25, 1.0);
    glBegin(GL_LINE_STRIP);
    glVertex2f(x,0.0);					
    for (int bin=0; bin<nbins; bin++)
    {
		// left edge
		glVertex2f(x,counts_selected(bin,0));			
		// top edge
		glVertex2f(x+xwidth,counts_selected(bin,0));	
		// right edge 
		//glVertex2f(x+xwidth,0.0);
		x+=xwidth;
    }
    glVertex2f(x,0.0);					
    glEnd();

    // y axis histograms
    glLoadIdentity();
    glTranslatef (0.0, yzoomcenter*yscale, 0);
    glScalef (xhscale, yscale, 1.0);
    glTranslatef (-1.0/xhscale, -ycenter, 0.0);
    glTranslatef (0.0, -yzoomcenter, 0);
    float ywidth = (amax[1]-amin[1]) / (float)(nbins);

    // y axis histogram (all points)
    float y = amin[1];
    glColor4f (0.0, 1.0, 0.0, 0.5);
    glBegin(GL_LINE_STRIP);
    glVertex2f(0.0,y);					
    for (int bin=0; bin<nbins; bin++)
    {
		// bottom
		glVertex2f(counts(bin,1),y);			
		// right edge
		glVertex2f(counts(bin,1), y+ywidth);	
		// top edge 
		// glVertex2f(0.0, y+ywidth);
		y+=ywidth;
    }
    glVertex2f(0.0,y);					
    glEnd();

    // Refactor this!
    // y axis histogram (selected points)
    y = amin[1];
    glColor4f (0.25, 1.0, 0.25, 1.0);
    glBegin(GL_LINE_STRIP);
    glVertex2f(0.0,y);					
    for (int bin=0; bin<nbins; bin++)
    {
		// bottom
		glVertex2f(counts_selected(bin,1),y);			
		// right edge
		glVertex2f(counts_selected(bin,1), y+ywidth);	
		// top edge 
		// glVertex2f(0.0, y+ywidth);
		y+=ywidth;
    }
    glVertex2f(0.0,y);					
    glEnd();

    glPopMatrix();
}

int
plot_window::transform_2d ()
{
    if (cp->no_transform->value())
		return 1;  // no transform
	
    blitz::Range NPTS(0,npoints-1);

    blitz::Array <float,1> tmp1(npoints), tmp2(npoints);
    tmp1 = vertices(NPTS,0);
    tmp2 = vertices(NPTS,1);

    if (cp->sum_vs_difference->value())
    {
		vertices(NPTS,0) = (sqrt(2.0)/2.0) * (tmp1 + tmp2);
		vertices(NPTS,1) = (sqrt(2.0)/2.0) * (tmp1 - tmp2);
    }
    else if (cp->polar->value())
    {
		vertices(NPTS,0) = atan2(tmp1, tmp2);
		vertices(NPTS,1) = sqrt(pow2(tmp1)+pow2(tmp2));
    }
    return 1;
}

int 
plot_window::normalize (blitz::Array<float,1> a, blitz::Array<int,1> a_rank, int style, int axis_index)
{

    blitz::Range NPTS(0,npoints-1);

#ifdef CHECK_FOR_NANS_IN_NORMALIZATION
    blitz::Array<int,1> inrange(npoints);
    inrange = where(((a(NPTS) < MAXFLOAT) && (a(NPTS) > -MAXFLOAT)), 1, 0);
    float tmin = min(where(inrange,a(NPTS), MAXFLOAT));
    float tmax = max(where(inrange,a(NPTS),-MAXFLOAT));
#else // CHECK_FOR_NANS_IN_NORMALIZATION
	float tmin = a(a_rank(0));
	float tmax = a(a_rank(npoints-1));
#endif // CHECK_FOR_NANS_IN_NORMALIZATION

    float mu,sigma;

    switch (style)
    {
	case NORMALIZATION_NONE:
    case NORMALIZATION_MINMAX:  // all data fits in window
		wmin[axis_index] = tmin;
		wmax[axis_index] = tmax;
		return 1;
    case NORMALIZATION_ZEROMAX: // all positive data fits in window, zero at "left" of axis.
		wmin[axis_index] = 0.0;
		wmax[axis_index] = tmax;
		return 1;
    case NORMALIZATION_MAXABS:  // all data fits in window w/zero at center of axis
		tmax = fmaxf(fabsf(tmin),fabsf(tmax));
		if (tmax != 0.0)
			{
				wmin[axis_index] = -tmax;
				wmax[axis_index] = tmax;
			}
		return 1;
    case NORMALIZATION_THREESIGMA:  // mean at center of axis, axis extends to +/- 3*sigma
		mu = mean(a(NPTS));
		sigma = sqrt((1.0/(float)npoints)*sum(pow2(a(NPTS)-mu)));
		DEBUG (cout << "mu, sigma = " << mu << ", " << sigma << endl);
		if (finite(mu) && (sigma!=0.0))
			{
				wmin[axis_index] = mu - 3*sigma;
				wmax[axis_index] = mu + 3*sigma;
			}
		return 1;
    case NORMALIZATION_RANK:	// replace each item with its rank, normalized from 0 to 1
		for(int i=0; i<npoints; i++)
		{
			a(a_rank(i)) = float(i) / ((float)npoints-1);
		}
		wmin[axis_index] = 0;
		wmax[axis_index] = 1;
		return 1;
    case NORMALIZATION_GAUSSIANIZE: // gaussianize the data, with the cnter of the gaussian at the median.
		for(int i=0; i<npoints; i++)
		{
			a(a_rank(i)) = (1.0/3.0)*(float)gsl_cdf_ugaussian_Pinv((double)(float(i+1) / (float)(npoints+2)));
		}
		wmin[axis_index] = -1.0;
		wmax[axis_index] = +1.0;
		return 1;
    default:
		return 0;
    }
}

class myCompare
{
public:
	bool operator()(const int i, const int j)
		{  
			return tmp_points(i) < tmp_points(j);
			// return (tpoints[i]<tpoints[j]);
		}
};

void
plot_window::compute_rank (blitz::Array<float,1> a, blitz::Array<int,1> a_rank, int var_index)
{
	blitz::Range NPTS(0,npoints-1);
	if (!ranked(var_index))
	{
		if (!a.isStorageContiguous())
		{
			cerr << "Warning: sorting with non-contiguous data." << endl;
		}
		if (!a_rank.isStorageContiguous())
		{
			cerr << "Warning: sorting with non-contiguous rank." << endl;
		}
		a_rank(NPTS) = identity(NPTS);
		
		tmp_points.reference(a);
		int *lo = a_rank.data(), *hi = lo + npoints;
		std::stable_sort(lo, hi, myCompare());
		ranked(var_index) = 1;  						// now we are ranked
		ranked_points(var_index,NPTS) = a_rank(NPTS);	// and our rank is cached!
		cout << "  cache STORE at index " << var_index << endl;
	} else {
		a_rank=ranked_points(var_index,NPTS);// use previously cached rank!
		cout << "  CACHE HIT   at index " << var_index << endl;
	}
}


int
plot_window::extract_data_points ()
{
    // get the labels for the plot's axes
    int axis0 = (int)(cp->varindex1->mvalue()->user_data());
    int axis1 = (int)(cp->varindex2->mvalue()->user_data());
    int axis2 = (int)(cp->varindex3->mvalue()->user_data());

    xlabel = column_labels[axis0];
    ylabel = column_labels[axis1];
	if (axis2 != nvars)
		zlabel = column_labels[axis2];
	else
		zlabel = "";
	
    blitz::Range NPTS(0,npoints-1);

	cout << "plot " << row << ", " << column << endl;

	cout << " pre-normalization: " << endl;

    compute_rank(points(axis0,NPTS),x_rank,axis0);
    cout << "  min: " << xlabel << "(" << x_rank(0) << ") = " << points(axis0,x_rank(0));
    cout << "  max: " << xlabel << "(" << x_rank(npoints-1) << ") = " << points(axis0,x_rank(npoints-1)) << endl;
    
    compute_rank(points(axis1,NPTS),y_rank,axis1);
    cout << "  min: " << ylabel << "(" << y_rank(0) << ") = " << points(axis1,y_rank(0));
    cout << "  max: " << ylabel << "(" << y_rank(npoints-1) << ") = " << points(axis1,y_rank(npoints-1)) << endl;

	if (axis2 != nvars)
	{
		compute_rank(points(axis2,NPTS),z_rank,axis2);
		cout << "  min: " << zlabel << "(" << z_rank(0) << ") = " << points(axis2,z_rank(0));
		cout << "  max: " << zlabel << "(" << z_rank(npoints-1) << ") = " << points(axis2,z_rank(npoints-1)) << endl;
	}

	cout << " post-normalization: " << endl;

    vertices(NPTS,0) = points(axis0,NPTS);
	blitz::Array<float,1> xpoints = vertices(NPTS,0);

    vertices(NPTS,1) = points(axis1,NPTS);
	blitz::Array<float,1> ypoints = vertices(NPTS,1);

	if (axis2 != nvars)
		vertices(NPTS,2) = points(axis2,NPTS);
	else
		vertices(NPTS,2) = 0;
	blitz::Array<float,1> zpoints = vertices(NPTS,2);

    (void) normalize (xpoints, x_rank, cp->x_normalization_style->value(), 0);
    amin[0] = xpoints(x_rank(0));
    amax[0] = xpoints(x_rank(npoints-1));
    cout << "  min: " << xlabel << "(" << x_rank(0) << ") = " << xpoints(x_rank(0));
    cout << "  max: " << xlabel << "(" << x_rank(npoints-1) << ") = " << xpoints(x_rank(npoints-1)) << endl;
    
    (void) normalize (ypoints, y_rank, cp->y_normalization_style->value(), 1);
    amin[1] = ypoints(y_rank(0));
    amax[1] = ypoints(y_rank(npoints-1));
    cout << "  min: " << ylabel << "(" << y_rank(0) << ") = " << ypoints(y_rank(0));
    cout << "  max: " << ylabel << "(" << y_rank(npoints-1) << ") = " << ypoints(y_rank(npoints-1)) << endl;

	if (axis2 != nvars)
	{
		(void) normalize (zpoints, z_rank, cp->z_normalization_style->value(), 2);
		amin[2] = zpoints(z_rank(0));
		amax[2] = zpoints(z_rank(npoints-1));
		cout << "  min: " << zlabel << "(" << z_rank(0) << ") = " << zpoints(z_rank(0));
		cout << "  max: " << zlabel << "(" << z_rank(npoints-1) << ") = " << zpoints(z_rank(npoints-1)) << endl;
	} else {
		amin[2] = -1.0;
		amax[2] = +1.0;
	}

	reset_view ();
    (void) transform_2d ();

    compute_histograms ();


    return 1;
}

void
control_panel_window::extract_and_redraw ()
{
    if (pw->extract_data_points())
	{
#ifdef FAST_APPLE_VERTEX_EXTENSIONS
		GLvoid *vp = (GLvoid *)pw->vertices.data();
		glFlushVertexArrayRangeAPPLE(3*npoints*sizeof(GLfloat), vp);
#endif // FAST_APPLE_VERTEX_EXTENSIONS

		//pw->redraw ();
		pw->needs_redraw = 1;
	}
}

void npoints_changed(Fl_Widget *o) 
{
    npoints = int(((Fl_Slider *)o)->value());
    redraw_all_plots (0);
}

void
write_data (Fl_Widget *o)
{
	char *output_file_name = fl_file_chooser("write binary output to file", NULL, NULL, 0);

	if (output_file_name)
	{
		blitz::Array<float,1> vars(nvars);
		blitz::Range NVARS(0,nvars-1);
		ofstream os;
		os.open (output_file_name, fstream::out | fstream::trunc | fstream::binary);
		if (os.fail())
		{
			cerr << "Error opening" << output_file_name << "for writing" << endl;
			return;
		}
		for( unsigned int i=0; i < column_labels.size()-1; i++ ) // remember, last column label is "-nothing-"
		{
			os << column_labels[i] << " ";
		}  
		os << endl;
		for (int i=0; i<npoints; i++) {
			vars = points(NVARS,i);
			os.write ((const char *)vars.data(), nvars*sizeof(float));
			if (os.fail())
			{
				cerr << "Error writing to" << output_file_name << endl;
				return;
			}
		}
	}
}

void read_ascii_file_with_headers() 
{
    // first line of file has column labels separated by whitespace
    std::string line;
    (void) getline (cin, line, '\n');
    std::stringstream ss(line); // Insert the string into a stream
    std::string buf;		   // need an intermediate buffer
    while (ss >> buf)
		column_labels.push_back(buf);
    nvars = column_labels.size();
    if (nvars > nvars_max)
    {
		cerr << "Error: too many columns, increase nvars_max and recompile" << endl;
		exit (1);
    }
	column_labels.push_back(string("-nothing-"));
    cout << "column_labels = ";
    for( unsigned int i=0; i < column_labels.size(); i++ )
    {
		cout << column_labels[i] << " ";
    }  

    cout << endl;
    cout << "there should be " << nvars << " fields (columns) per record (row)" << endl;

	// now we know the number of variables (nvars), so if we know the number of points (e.g. from the command line)
	// we can size the main points array once and for all, and not waste memory.
	if (npoints_cmd_line != 0)
	{
		npoints = npoints_cmd_line;
		points.resize(nvars,npoints);
	}

    int i=0;
    while (!cin.eof() && i<npoints)
    {
		(void) getline (cin, line, '\n');
		DEBUG (cout << "line is: " << line << endl);
		if (line.length() == 0)
			continue;
		std::stringstream ss(line); // Insert the string into a stream
		for (int j=0; j<nvars; j++)
		{
			double x;
			ss >> x;
			points(j,i) = (float)x;
// FIX THIS MISSING DATA STUFF!! IT IS BROKEN.
			if (ss.eof() && j<nvars-1)
			{
				cerr << "not enough data on line " << i+2 << ", aborting!" << endl;
				exit(1);
			}
			if (!ss.good() && j<nvars-1)
			{
				cerr << "bad data (probably non-numeric) at line " << i+1 << " column " << j+1 << ", skipping entire line." << endl;
				goto nextline;
			}
			DEBUG (cout << "points(" << j << "," << i << ") = " << points(j,i) << endl);
		}
		for (int j=0; j<nvars; j++)
			if (points(j,i) == -9999)
			{
				cerr << "bad data (-9999) at line " << i << ", column " << j << " - skipping entire line\n";
				goto nextline;  // one of the only sensible uses...
			}
		i++;
    nextline:	// got a line with bad data, do not increment i
		if ((i+1)%10000 == 0)
			cerr << "Read " << i+1 << " lines." << endl;
		continue;
    }
    cout << "Read " << i+1 << " lines total." << endl;
    npoints = i;
}

void read_ascii_file() 
{
    int i;
    char line[50*nvars+1];	// 50 characters better hold one value + delimiters
    for (i=0; i<npoints; i++)
    {
		if (!fgets ((char *)&(line[0]), sizeof(line), stdin))
		{
			if (feof(stdin))
				break;
			if (ferror(stdin))
			{
				fprintf (stderr, "error reading input occured at line %d\n", i+1);
				exit (1);
			}
		}
//		printf ("line %i = |%s|\n", i, (char *)&(line[0]));
		float val = 0.0;
		int ret, nchars = 0;
		int offset = 0;
		for (int j=0; j<nvars; j++)
		{
			ret = sscanf((char *)&(line[offset]),"%f%n",&val,&nchars);
//			printf ("i = %d, j = %d, offset = %d, ret = %d, nchars = %d, val = %f\n", i, j, offset, ret, nchars, val);
			if (ret != 1)
			{
				fprintf (stderr, "trouble reading input value %d on line %d\n", j+1, i+1);
				exit (1);
			}
			offset += nchars;
			points(j,i) = val;
		}
		if (i>0 && (i%10000 == 0))
			printf ("read %d lines\n", i);
    }
    cout << "read " << i << " lines." << endl;
    npoints = i;
}

void read_binary_file() 
{
    blitz::Array<float,1> vars(nvars);
    blitz::Range NVARS(0,nvars-1);
    int i;
    if (!points.isStorageContiguous())
    {
		cerr << "Tried to pass non contigous buffer to read.  Aborting!" << endl;
		exit (1);
    }
		
    for (i=0; i<npoints; i++)
    {
		unsigned int ret = read(0, (void *)(vars.data()), nvars*sizeof(float));
		if (ret != nvars*sizeof(float))
		{
			if (ret == 0) // EOF
				break;
			else
			{
				fprintf (stderr, "error reading input occured at line %d\n", i+1);
				exit (1);
			}
		}
		points(NVARS,i) = vars;
		if (i>0 && (i%10000 == 0))
			printf ("read %d lines\n", i);
    }
    cout << "read " << i << " lines." << endl;
    npoints = i;
}

void read_binary_file_with_headers() 
{
    // first line of file has column labels separated by whitespace
    std::string line;
    (void) getline (cin, line, '\n');
    std::stringstream ss(line); // Insert the string into a stream
    std::string buf;		   // need an intermediate buffer
    while (ss >> buf)
		column_labels.push_back(buf);
    nvars = column_labels.size();
    if (nvars > nvars_max)
    {
		cerr << "Error: too many columns, increase nvars_max and recompile" << endl;
		exit (1);
    }
	column_labels.push_back(string("-nothing-"));
    cout << "column_labels = ";
    for( unsigned int i=0; i < column_labels.size(); i++ )
    {
		cout << column_labels[i] << " ";
    }  
    cout << endl;
    cout << "there should be " << nvars << " fields (columns) per record (row)" << endl;

    blitz::Array<float,1> vars(nvars);
    blitz::Range NVARS(0,nvars-1);

	// now we know the number of variables (nvars), so if we know the number of points (e.g. from the command line)
	// we can size the main points array once and for all, and not waste memory.
	if (npoints_cmd_line != 0)
	{
		npoints = npoints_cmd_line;
		points.resize(nvars,npoints);
	}
		
    if (!points.isStorageContiguous())
    {
		cerr << "Warning: passed non contigous buffer to read." << endl;
		// exit (1);
    }
		
    int i;
    for (i=0; i<npoints; i++)
    {
		unsigned int ret = fread((void *)(vars.data()), sizeof(float), nvars, stdin);
		// cout << "read " << ret << " values " << endl;
		if (ret != (unsigned int)nvars)
		{
			if (ret == 0 || feof(stdin)) // EOF
				break;
			else
			{
				fprintf (stderr, "error reading input occured at row %d\n", i+1);
				exit (1);
			}
		}
		points(NVARS,i) = vars;
		if (i>0 && (i%10000 == 0))
			printf ("read %d rows\n", i);
    }
    cout << "read " << i << " rows." << endl;
    npoints = i;
}

void
control_panel_window::make_widgets(control_panel_window *cpw)
{
    // since these (virtual) control panels are really groups inside a tab inside a
    // window, set their child widget's coordinates relative to their enclosing
    // window's position.  (I think ;-)
    int xpos = this->x()+50;
    int ypos = this->y()+20;

    Fl_Button *b;

    pointsize_slider = new Fl_Hor_Value_Slider_Input(xpos, ypos, cpw->w()-60, 20, "size");
    pointsize_slider->align(FL_ALIGN_LEFT);
    pointsize_slider->value(pointsize);
    pointsize_slider->step(0.25);
    pointsize_slider->bounds(0.1,20.0);
    pointsize_slider->callback((Fl_Callback*)replot, this);

    Bkg = new Fl_Hor_Value_Slider_Input(xpos, ypos+=25, cpw->w()-60, 20, "Bkg");
    Bkg->align(FL_ALIGN_LEFT);
    Bkg->step(0.001);
    Bkg->bounds(0.0,1.0);
    Bkg->callback((Fl_Callback*)replot, this);
    Bkg->value(0.0);

    Lum = new Fl_Hor_Value_Slider_Input (xpos, ypos+=25, cpw->w()-60, 20, "Lum");
    Lum->align(FL_ALIGN_LEFT);
    Lum->callback((Fl_Callback*)replot, this);
    Lum->step(0.001);
    Lum->bounds(0,1.0);
    Lum->value(0.85);

    Alph = new Fl_Hor_Value_Slider_Input (xpos, ypos+=25, cpw->w()-60, 20, "Alph");
    Alph->align(FL_ALIGN_LEFT);
    Alph->callback((Fl_Callback*)replot, this);
    Alph->step(0.001);
    Alph->bounds(0,1.0);
    Alph->value(1.0);

    rot_slider = new Fl_Hor_Value_Slider_Input (xpos, ypos+=25, cpw->w()-60, 20, "rot");
    rot_slider->align(FL_ALIGN_LEFT);
    rot_slider->callback((Fl_Callback*)replot, this);
    rot_slider->value(0.0);
    rot_slider->step(0.001);
    rot_slider->bounds(-180.0, 180.0);

    nbins_slider = new Fl_Hor_Value_Slider_Input(xpos, ypos+=25, cpw->w()-60, 20, "nbins");
    nbins_slider->align(FL_ALIGN_LEFT);
    nbins_slider->callback((Fl_Callback*)redraw_one_plot, this);
    nbins_slider->value(plot_window::nbins_default);
    nbins_slider->step(1);
    nbins_slider->bounds(2,plot_window::nbins_max);

    // dynamically build the variables menu
    // cout << "starting menu build, nvars = " << nvars << endl;
    for (int i=0; i<=nvars; i++)
    {
		// cout << "label " << i << " = " << column_labels[i].c_str() << endl;
		varindex_menu_items[i].label((const char *)(column_labels[i].c_str()));
		varindex_menu_items[i].user_data((void *)i);
    }
    varindex_menu_items[nvars+1].label(0);

    xpos = 10;

    varindex1 = new Fl_Choice (xpos, ypos+=45, 100, 25, "axis 1");
    varindex1->align(FL_ALIGN_TOP);
    varindex1->textsize(12);
    varindex1->menu(varindex_menu_items);
    varindex1->callback((Fl_Callback*)static_extract_and_redraw, this);
 
    varindex2 = new Fl_Choice (xpos+100, ypos, 100, 25, "axis 2");
    varindex2->align(FL_ALIGN_TOP);
    varindex2->textsize(12);
    varindex2->menu(varindex_menu_items);
    varindex2->callback((Fl_Callback*)static_extract_and_redraw, this);

    varindex3 = new Fl_Choice (xpos+200, ypos, 100, 25, "axis 3");
    varindex3->align(FL_ALIGN_TOP);
    varindex3->textsize(12);
    varindex3->menu(varindex_menu_items);
	varindex3->value(nvars);  // initially, axis3 == "-nothing-"
    varindex3->callback((Fl_Callback*)static_extract_and_redraw, this);

    for (int i=0; i<n_normalization_styles; i++)
    {
		normalization_style_menu_items[i].label(normalization_style_labels[i]);
		normalization_style_menu_items[i].user_data((void *)normalization_styles[i]);
    }
    normalization_style_menu_items[n_normalization_styles].label(0);

    x_normalization_style = new Fl_Choice (xpos, ypos+=45, 100, 25, "normalize x");
    x_normalization_style->align(FL_ALIGN_TOP);
    x_normalization_style->textsize(12);
    x_normalization_style->menu(normalization_style_menu_items);
    x_normalization_style->value(NORMALIZATION_NONE);
    x_normalization_style->callback((Fl_Callback*)static_extract_and_redraw, this);
 
    y_normalization_style = new Fl_Choice (xpos+100, ypos, 100, 25, "normalize y");
    y_normalization_style->align(FL_ALIGN_TOP);
    y_normalization_style->textsize(12);
    y_normalization_style->menu(normalization_style_menu_items);
    y_normalization_style->value(NORMALIZATION_NONE); 
    y_normalization_style->callback((Fl_Callback*)static_extract_and_redraw, this);
 
    z_normalization_style = new Fl_Choice (xpos+200, ypos, 100, 25, "normalize z");
    z_normalization_style->align(FL_ALIGN_TOP);
    z_normalization_style->textsize(12);
    z_normalization_style->menu(normalization_style_menu_items);
    z_normalization_style->value(NORMALIZATION_NONE); 
    z_normalization_style->callback((Fl_Callback*)static_extract_and_redraw, this);
 
    int xpos2 = xpos;
    int ypos2 = ypos;

    reset_view_button = b = new Fl_Button(xpos2, ypos+=25, 20, 20, "reset view ");
    b->align(FL_ALIGN_RIGHT); b->selection_color(FL_YELLOW);
	b->callback((Fl_Callback*) reset_view, this);

    spin = b= new Fl_Button(xpos2, ypos+=25, 20, 20, "spin");
    b->align(FL_ALIGN_RIGHT); b->selection_color(FL_YELLOW);
    b->type(FL_TOGGLE_BUTTON);

    dont_clear = new Fl_Button(xpos2, ypos+=25, 20, 20, "don't clear");
    dont_clear->align(FL_ALIGN_RIGHT);
    dont_clear->type(FL_TOGGLE_BUTTON);
    dont_clear->selection_color(FL_YELLOW);
    dont_clear->callback((Fl_Callback*)static_maybe_redraw, this);

    transform_style = new Fl_Group (xpos2-1, ypos+25-1, 20+2, 4*25+2);

    no_transform = b = new Fl_Button(xpos2, ypos+=25, 20, 20, "identity");
    b->callback((Fl_Callback*)static_extract_and_redraw, this);
    b->align(FL_ALIGN_RIGHT); b->type(FL_RADIO_BUTTON); b->selection_color(FL_YELLOW);
		
    sum_vs_difference = b = new Fl_Button(xpos2, ypos+=25, 20, 20, "sum vs. diff.");
    b->callback((Fl_Callback*)static_extract_and_redraw, this);
    b->align(FL_ALIGN_RIGHT); b->type(FL_RADIO_BUTTON); b->selection_color(FL_YELLOW);
		
    polar = b = new Fl_Button(xpos2, ypos+=25, 20, 20, "polar");
    b->callback((Fl_Callback*)static_extract_and_redraw, this);
    b->align(FL_ALIGN_RIGHT); b->type(FL_RADIO_BUTTON); b->selection_color(FL_YELLOW);
		
    transform_style->end();
    no_transform->setonly();

    ypos=ypos2;
    xpos=xpos2+100;

    show_grid = b = new Fl_Button(xpos, ypos+=25, 20, 20, "grid");
    b->callback((Fl_Callback*)static_maybe_redraw, this);
    b->align(FL_ALIGN_RIGHT); b->type(FL_TOGGLE_BUTTON); b->selection_color(FL_YELLOW);	b->value(1);

    show_summary = b = new Fl_Button(xpos, ypos+=25, 20, 20, "summary");
    b->callback((Fl_Callback*)static_maybe_redraw, this);
    b->align(FL_ALIGN_RIGHT); b->type(FL_TOGGLE_BUTTON); b->selection_color(FL_YELLOW);	b->value(1);

    show_points = b = new Fl_Button(xpos, ypos+=25, 20, 20, "points");
    b->callback((Fl_Callback*)static_maybe_redraw, this);
    b->align(FL_ALIGN_RIGHT); b->type(FL_TOGGLE_BUTTON); b->selection_color(FL_YELLOW);	b->value(1);

    show_axes = b = new Fl_Button(xpos, ypos+=25, 20, 20, "axes");
    b->callback((Fl_Callback*)static_maybe_redraw, this);
    b->align(FL_ALIGN_RIGHT); b->type(FL_TOGGLE_BUTTON); b->selection_color(FL_YELLOW);	b->value(1);

    show_labels = b = new Fl_Button(xpos, ypos+=25, 20, 20, "labels");
    b->callback((Fl_Callback*)static_maybe_redraw, this);
    b->align(FL_ALIGN_RIGHT); b->type(FL_TOGGLE_BUTTON); b->selection_color(FL_YELLOW);	b->value(1);

    show_histogram = b = new Fl_Button(xpos, ypos+=25, 20, 20, "histogram");
    b->callback((Fl_Callback*)static_extract_and_redraw, this);
    b->align(FL_ALIGN_RIGHT); b->type(FL_TOGGLE_BUTTON); b->selection_color(FL_YELLOW);	b->value(0);

}

void
plot_window::redraw_one_plot ()
{
	compute_histograms();
	redraw();
	Fl::flush();
	needs_redraw = 0;
}

void
redraw_all_plots (int p)
{
	// cout << "calling redraw_all_plots(" << p << ")" << endl;

	// redraw all plots, cyclically, sarting with plot p
	// p is important, since the draw() routine for a plot handles the selection region
	// and the active plot (the one where we are making the selection) must update the selected
	// set and the color/texture arrays *before* all the other plots get redrawn.  Ugh.
    for (int i=0; i<nplots; i++)
    {
		int j=(p+i)%nplots;
		pws[j]->compute_histograms();
		pws[j]->redraw();
		Fl::flush();
		pws[j]->needs_redraw = 0;
    }
}

void 
reset_all_plots ()
{
    for (int i=0; i<nplots; i++)
    {
		pws[i]->reset_view();
    }
}

void
redraw_if_changing (void * dummy)
{
    for (int i=0; i<nplots; i++)
    {
		if (cps[i]->spin->value() || pws[i]->needs_redraw)
		{
			pws[i]->redraw();
			pws[i]->needs_redraw = 0;
		}
    }
    float fps = 100.0;
    struct timeval tp;
    static long useconds=0;
    static long seconds=0;

    // has at least 1/fps seconds elapsed? (sort of)
busy:
    (void) gettimeofday(&tp, (struct timezone *)0);
    if ((tp.tv_sec > seconds) || (((float)(tp.tv_usec - useconds)/1000000.0) > 1/fps))
    {
		seconds = tp.tv_sec;
		useconds = tp.tv_usec;
		return ;
    }
    else
    {
		usleep (1000000/(5*(int)fps));
		goto busy;
    }
}


void
resize_global_arrays ()
{
	// points.resizeAndPreserve(nvars,npoints);	

	ranked_points.resize(nvars,npoints);

	ranked.resize(nvars);
	ranked = 0;	// initially, no ranking has been done.

	tmp_points(npoints);  // for sort

    colors.resize(npoints,4);
    altcolors.resize(npoints,4);
    identity.resize(npoints);
    newly_selected.resize(npoints);
    selected.resize(npoints);

	selected=0;
}

void usage()
{
    fprintf(stderr,"Usage:\n");
    fprintf(stderr,"[--format={ascii,binary}] -f\n");
    fprintf(stderr,"[--npoints=<int>] -n\n");
    fprintf(stderr,"[--nrows=<int>] -r\n");
    fprintf(stderr,"[--ncols=<int>] -c\n");
    fprintf(stderr,"[--help] -h\n");
    exit(-1);
}

void make_global_widgets ()
{
	Fl_Button *b;

	int xpos=10, ypos=500;
    npoints_slider = new Fl_Hor_Value_Slider_Input(xpos+30, ypos+=25, 300-30, 20, "npts");
    npoints_slider->align(FL_ALIGN_LEFT);
    npoints_slider->callback(npoints_changed);
    npoints_slider->value(npoints);
    npoints_slider->step(1);
    npoints_slider->bounds(1,npoints);

	int xpos1 = xpos, ypos1 = ypos;

	display_deselected_button = b = new Fl_Button(xpos, ypos+=25, 20, 20, "display deselected");
    b->align(FL_ALIGN_RIGHT); b->selection_color(FL_YELLOW); b->type(FL_TOGGLE_BUTTON);	b->value(1);
	b->callback((Fl_Callback*)toggle_display_delected);

	add_to_selection_button = b = new Fl_Button(xpos, ypos+=25, 20, 20, "add to selection");
    b->align(FL_ALIGN_RIGHT); b->selection_color(FL_YELLOW); b->type(FL_TOGGLE_BUTTON);	b->value(0);	

	invert_selection_button = b = new Fl_Button(xpos, ypos+=25, 20, 20, "invert selection");
    b->align(FL_ALIGN_RIGHT); b->selection_color(FL_YELLOW); b->callback((Fl_Callback*)invert_selection);

	clear_selection_button = b = new Fl_Button(xpos, ypos+=25, 20, 20, "clear selection");
    b->align(FL_ALIGN_RIGHT); b->selection_color(FL_YELLOW); b->callback(clear_selection);

	delete_selection_button = b = new Fl_Button(xpos, ypos+=25, 20, 20, "delete points");
    b->align(FL_ALIGN_RIGHT); b->selection_color(FL_YELLOW); b->callback(delete_selection);

	write_data_button = b = new Fl_Button(xpos, ypos+=25, 20, 20, "write data");
    b->align(FL_ALIGN_RIGHT); b->selection_color(FL_YELLOW); b->callback(write_data);

	xpos = xpos1 + 150; ypos = ypos1;

	choose_color_selected_button = b = new Fl_Button(xpos, ypos+=25, 20, 20, "selected color");
    b->align(FL_ALIGN_RIGHT); b->selection_color(FL_YELLOW); b->callback((Fl_Callback*)choose_color_selected);

	choose_color_deselected_button = b = new Fl_Button(xpos, ypos+=25, 20, 20, "deselected color");
    b->align(FL_ALIGN_RIGHT); b->selection_color(FL_YELLOW); b->callback((Fl_Callback*)choose_color_deselected);

}


int main(int argc, char **argv)
{

    static struct option long_options[] =
		{
			{"npoints",	required_argument,	0, 'n'},
			{"nrows",	required_argument,	0, 'r'},
			{"ncols",	required_argument,	0, 'c'},
			{"format",	required_argument,	0, 'f'},
			{"help",	no_argument,		0, 'h'},
			{0, 0, 0, 0}
		};
    
    int c;
    while((c = getopt_long(argc, argv, "n:r:c:f:h", long_options, NULL)) != -1)
    {
		switch (c)
		{
		
		case 'n':		// maximum number of points (samples, rows of data) to read
			npoints_cmd_line = atoi(optarg);
			if (npoints_cmd_line < 1)
				usage();
			break;
		case 'r':		// number of rows in scatterplot matrix
			nrows=atoi(optarg);
			if (nrows < 1)
				usage();
			break;
		case 'c':		// number of columns in scatterplot matrix
			ncols=atoi(optarg);
			if (ncols < 1)
				usage();
			break;
		case 'f':		// format of input file
			if (!strncmp(optarg,"binary",1))
				format=BINARY;
			else if (!strncmp(optarg,"ascii",1))
				format=ASCII;
			else
			{
				usage();
				exit(-1);
			}
			break;
		case 'h':
		case ':':
		case '?':
			usage();
			exit(-1);
			break;
		default:
			usage();
			exit(-1);
			break;
		}
    }
    argc -= optind;
    argv += optind;
    
    srand((unsigned int)time(0));
    
    assert(format==BINARY || format==ASCII);
    if (format == BINARY)
		read_binary_file_with_headers ();
    else if (format == ASCII)
		read_ascii_file_with_headers ();
    
//  read_binary_file ();
//  read_ascii_file ();
    
	// if we read a different number of points then we anticipated, we rezise and preserve
	// note this can take lot of time and memory, temporarily.
	if (npoints != npoints_cmd_line)
		points.resizeAndPreserve(nvars,npoints);

	resize_global_arrays ();

    nplots = nrows*ncols;
    
	pointsize = max(1.0, 6.0 - (int)log10f((float)npoints));  // fewer points -> bigger starting pointsize

    cout << "making identity" << endl;
    for (int i=0; i<npoints; i++)
		identity(i)=i;
    
	// main control panel size and position
	const int main_w = 350, main_h = 700;
	const int main_x = Fl::w() - (main_w + left_frame + right_frame + right_safe), main_y = top_frame+top_safe;

    // main control panel window
    main_control_panel = new Fl_Window (main_x, main_y, main_w, main_h, "viewpoints -> creon@nas.nasa.gov");
    main_control_panel->resizable(main_control_panel);

    // add the rest of the global widgets to control panel
	make_global_widgets ();

    // inside the main control panel, there is a tab widget that contains the sub-panels (groups), one per plot.
    cpt = new Fl_Tabs(3, 10, main_w-6, 500);    
    cpt->selection_color(FL_YELLOW); cpt->labelsize(10);

	// done creating main control panel (except for tabbed sup-panels, below)
    main_control_panel->end();

    // create and add the virtul sub-panels (each a group under a tab), one per plot.
    for (int i=0; i<nplots; i++)
    {
		int row = i/ncols;
		int col = i%ncols;

		// plot window size
		int pw_w = ((Fl::w() - (main_w+left_frame+right_frame+right_safe+left_safe+20)) / ncols) - (left_frame + right_frame);
		int pw_h = ((Fl::h() - (top_safe+bottom_safe))/ nrows) - (top_frame + bottom_frame);

		int pw_x = left_safe + left_frame + col * (pw_w + left_frame + right_frame);
		int pw_y = top_safe + top_frame + row * (pw_h + top_frame + bottom_frame);
		
		// create a label
		ostringstream oss;
		oss << "" << i+1;
		string labstr = oss.str();

		// add a new virtual control panel (a group) under the tab widget
		Fl_Group::current(cpt);	
		cps[i] = new control_panel_window (3, 30, main_w-6, 480);
		cps[i]->copy_label(labstr.c_str());
		cps[i]->labelsize(10);
		cps[i]->resizable(cps[i]);
		cps[i]->make_widgets(cps[i]);

		// end the group since we want to create new plot windows at the top level.
		cps[i]->end();
		Fl_Group::current(0); 

		// create plotting window i
		pws[i] = new plot_window(pw_w, pw_h);
		pws[i]->copy_label(labstr.c_str());
		pws[i]->resizable(pws[i]);
		pws[i]->position(pw_x, pw_y);
		pws[i]->row = row; pws[i]->column = col;
		pws[i]->end();
		
		// link plot window and its associated virtual control panel
		pws[i]->index = cps[i]->index = i;
		cps[i]->pw = pws[i];
		pws[i]->cp = cps[i];

		if (i==0)
		{
			// the first plot initially shows first two attributes
			cps[i]->varindex1->value(0);  
			cps[i]->varindex2->value(1);  
		} else {
			// others plots initially show random bivariate picks
			int axis1 = rand()%nvars;  
			cps[i]->varindex1->value(axis1);
			cps[i]->varindex2->value((axis1 + ((rand()%(nvars-1)) + 1))%nvars); // avoid duplicating axis1
			// initially, only the first tab isn't hidden
			cps[i]->hide();	
		}
		pws[i]->extract_data_points();
		pws[i]->reset_view();
		pws[i]->show(argc,argv);
    }

    main_control_panel->show();     // now we can show the main control panel and all its supanels

    Fl::add_idle(redraw_if_changing);
//	Fl::add_check(redraw_if_changing);
    int result = Fl::run();  // enter main event loop
    return result;
}

