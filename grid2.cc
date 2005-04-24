/* Add C includes here */

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

#ifdef __APPLE__
#include <vecLib/vBLAS.h>
#elif linux
//extern "C" {
//# include <cblas.h>
//}
#endif
        
/* Add C++ includes here */
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <sstream>
 
// FLTK 
#include <FL/math.h>
#include <FL/gl.h>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Hor_Value_Slider.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Gl_Window.H>

// flews (FLTK extenstion) extras
#include <FL/Fl_flews.h>
#include <FL/Fl_Value_Slider_Input.H>
#include "Fl_Hor_Value_Slider_Input.H"

// Blitz++
#include <blitz/array.h>

#  ifdef __APPLE__
#    include <OpenGL/glext.h>
#  else
#    include <GL/glext.h>
#  endif

#include "grid2.H"

using namespace std;

const int nvals_max = 256;  	// maximum number of columns allowed in data file
const int MAXPOINTS = 200000;	// maximum rows in data file
const int skip = 0; 			// skip this many columns at the beginning of each row

int npoints = MAXPOINTS;			// actual number of rows in data file
int nvals = nvals_max;				// actual number of columns in data file
int display_deselected = 1;			// display deselected objects in alternate color

blitz::Array<float,2> points(nvals_max,MAXPOINTS);	// main data array
blitz::Array<int,1> identity;
blitz::Array<int,1> selected;
blitz::Array<float,2> colors, altcolors;

// for interface to normalization routines.  eliminate by using interlaced arrays?
blitz::Array<float,1> xpoints,ypoints,zpoints;  

std::vector<std::string> column_labels; // vector to hold variable names

float xmin, xmax, gmax;

float pointsize = 1.0;
int istart = 0;

int sfactor = GL_CONSTANT_COLOR;
int dfactor = GL_DST_ALPHA;

class plot_window; // love those one-pass compilers.
class control_panel_window;
void redraw_all_plots (void);
void reset_all_plots (void);


class plot_window : public Fl_Gl_Window {
  protected:
	void draw();
	void draw_grid();
	void draw_labels();
	void draw_data_points();
	void draw_histograms();
	int handle (int event);
	void handle_selection();
	void color_array_from_selection();
	void resort();
	int xprev, yprev, xcur, ycur;
	float xdragged, ydragged;
	float xcenter, ycenter, xscale, yscale;
	float xzoomcenter, yzoomcenter;
	float xdown, ydown, xtracked, ytracked;
	int selection_changed, extend_selection;
	static int count;
  public:
	plot_window(int w, int h);
	blitz::Array<float,2> vertices;
	blitz::Array<int,1> x_rank, y_rank, z_rank;
	std::string xlabel, ylabel, zlabel;
	control_panel_window *cp;	// pointer to the control panel associated with this plot window
	int extract_data_points();
	int transform_2d();
	void compute_rank (blitz::Array<float,1>, blitz::Array<int,1>);
	int normalize(blitz::Array<float,1>, blitz::Array<int,1>, int);
	void reset_view();
	float angle;
	int needs_redraw;
};

int plot_window::count = 0;

plot_window::plot_window(int w,int h) : Fl_Gl_Window(w,h) 
{
	count++;
	vertices.resize(npoints,3);
	x_rank.resize(npoints);
	y_rank.resize(npoints);
	z_rank.resize(npoints);
#if 0
	if (can_do(FL_RGB8|FL_DOUBLE|FL_ALPHA|FL_DEPTH))
		mode(FL_RGB8|FL_DOUBLE|FL_ALPHA|FL_DEPTH);  // Can't seem to make this work on PBG4 OSX 
	else
		mode(FL_RGB|FL_DOUBLE|FL_ALPHA);
#endif
		mode(FL_RGB8|FL_DOUBLE|FL_ALPHA);
}


class control_panel_window : public Fl_Window {
  protected:
	void maybe_redraw ();
  public:
	control_panel_window(int w, int h);
	void make_widgets(control_panel_window *cpw);
	void extract_and_redraw ();
	static void static_extract_and_redraw (Fl_Widget *w, control_panel_window *cpw)
		{ cpw->extract_and_redraw(); }
	static void static_maybe_redraw(Fl_Widget *w, control_panel_window *cpw)
		{ cpw->maybe_redraw() ;}
	static void replot (Fl_Widget *w, control_panel_window *cpw)
		{ /* cpw->pw->redraw(); */ cpw->pw->needs_redraw=1;}
	Fl_Hor_Value_Slider_Input *npts_slider;
	Fl_Hor_Value_Slider_Input *pointsize_slider;
	Fl_Hor_Value_Slider_Input *Bkg, *Lum, *Alph;
	Fl_Hor_Value_Slider_Input *rot_slider;
	Fl_Choice *varindex1, *varindex2, *varindex3;
	
	Fl_Button *spin, *dont_clear, *show_axes, *show_grid, *show_labels, *show_histogram;
//	Fl_Button *x_equals_delta_x, *y_equals_delta_x;
	Fl_Group *transform_style;
	Fl_Button *sum_vs_difference, *polar, *ratio, *no_transform;
	Fl_Choice *x_normalization_style, *y_normalization_style, *z_normalization_style;

	plot_window *pw;  // pointer to the plot window associated with this control panel
};

control_panel_window::control_panel_window(int w, int h) : Fl_Window(w, h) {
}

// these menu related lists should really be class variables in class control_panel_window
Fl_Menu_Item varindex_menu_items[nvals_max]; 

const int NORMALIZATION_NONE 	= 0;
const int NORMALIZATION_MINMAX 	= 1;
const int NORMALIZATION_ZEROMAX = 2;
const int NORMALIZATION_MAXABS	= 3;
const int NORMALIZATION_THREESIGMA = 4;
const int NORMALIZATION_RANK = 5;

const char *normalization_style_labels[] = { "none","minmax","zeromax","maxabs","threesigma","rank"};

int normalization_styles[] = 
{NORMALIZATION_NONE, NORMALIZATION_MINMAX, NORMALIZATION_ZEROMAX, NORMALIZATION_MAXABS, NORMALIZATION_THREESIGMA, NORMALIZATION_RANK};

const int n_normalization_styles = sizeof(normalization_styles)/sizeof(normalization_styles[0]);

Fl_Menu_Item normalization_style_menu_items[n_normalization_styles+1];
  
plot_window *current_plot_window;
plot_window *pw1, *pw2, *pw3, *pw4;
control_panel_window *cp1, *cp2, *cp3, *cp4;

int plot_window::handle(int event) {
	switch(event) {
	case FL_PUSH:
		DEBUG(cout << "FL_PUSH at " << xprev << ", " << yprev << endl);
		// cp->show();	// show (raise) the control panel associated with this plot window.
		xprev = Fl::event_x();
		yprev = Fl::event_y();

		if ((Fl::event_state() == FL_BUTTON2) || (Fl::event_state() == (FL_BUTTON1 | FL_CTRL)))
		{
			#if 0
			// wish this worked
			xzoomcenter = (float)xprev;
			xzoomcenter = + (2.0*(xzoomcenter/(float)w()) -1.0) ; // window -> [-1,1]
			
			yzoomcenter = (float)yprev;
			yzoomcenter = - (2.0*(yzoomcenter/(float)h()) -1.0) ; // window -> [-1,1]
			#endif
		}

		if (Fl::event_state() & FL_BUTTON1) // left button down = start new selection
		{
			if (! (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))) // move or extend old selection
			{
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
			xcenter -= xdragged*(1/xscale)*(2.0/w());
			ycenter -= ydragged*(1/yscale)*(2.0/h());
			DEBUG ( cout << "xcenter, ycenter: " << xcenter << ", " << ycenter << endl);
			// redraw ();
			needs_redraw = 1;
		}

		// scale = drag with middle-mouse (or c-left-mouse)
		else if ((Fl::event_state() == FL_BUTTON2) || (Fl::event_state() == (FL_BUTTON1 | FL_CTRL)))
		{
			xscale *= 1 + xdragged*(2.0/w());
			yscale *= 1 + ydragged*(2.0/h());
			DEBUG ( cout << "xscale, yscale: " << xscale << ", " << yscale << endl );
			// redraw();
			needs_redraw = 1;
		}

		// continue selection = drag with left mouse
		else if (Fl::event_state() & FL_BUTTON1)
		{
			// right key down = move selection
			// left shift down = extend selection (bug on OSX - no left key events)
			if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
			{
				xdown += xdragged*(1/xscale)*(2.0/w());
				ydown += ydragged*(1/yscale)*(2.0/h());
				xtracked += xdragged*(1/xscale)*(2.0/w());
				ytracked += ydragged*(1/yscale)*(2.0/h());
				if (Fl::get_key(FL_Shift_R))
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
			selection_changed = 1;
			redraw_all_plots ();
		}
		return 1;
	case FL_RELEASE:   
		// mouse up
		DEBUG (cout << "FL_RELEASE at " << Fl::event_x() << ", " << Fl::event_y() << endl);
		// selection_changed = 0;
		return 1;
	case FL_KEYDOWN:
		// keypress, key is in Fl::event_key(), ascii in Fl::event_text()
		// Return 1 if you understand/use the keyboard event, 0 otherwise...
		DEBUG ( cout << "FL_KEYDOWN, event_key() = " << Fl::event_key() << endl);
		switch (Fl::event_key())
		{
		case 'i': // invert or restore (uninvert) selection
			selected(blitz::Range(0,npoints-1)) = 1-selected(blitz::Range(0,npoints-1));
			color_array_from_selection ();
			redraw_all_plots ();
			return 1;
		case 'd': // don't display / display deselected dots
			display_deselected = 1 - display_deselected;
			redraw_all_plots ();
			return 1;
		case 'r':
			reset_view ();
			//redraw();
			return 1;
		case 'q':
		case '\027':
			// quit
			exit (0);
		default: return 0;
		}
	case FL_KEYUP:
		DEBUG ( cout << "FL_KEYUP" << endl);
		return 0;
	case FL_SHORTCUT:
		// shortcut, key is in Fl::event_key(), ascii in Fl::event_text()
		// Return 1 if you understand/use the shortcut event, 0 otherwise...
		return 0;
	default:
		// pass other events to the base class...
		return Fl_Gl_Window::handle(event);}
} 


void plot_window::reset_view()
{
	xscale = yscale = 0.95;
	xcenter = ycenter = 0.0;

	xdragged = ydragged = 0.0;
	xzoomcenter = yzoomcenter = 0.0;
	xdown = ydown = xtracked = ytracked = 0.0;
	xprev = yprev = xcur = ycur = 0;

	angle = 0.0;
	cp->spin->value(0);
	cp->rot_slider->value(0.0);
	cp->dont_clear->value(0);

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
		glOrtho(-1, 1, -1, 1, -1000, +1000);
		glViewport(0, 0, w(), h());
		glDisable(GL_LIGHTING);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glEnable(GL_POINT_SMOOTH);
		glHint(GL_POINT_SMOOTH_HINT,GL_NICEST);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
#ifdef __APPLE__
#ifdef FAST_APPLE_VERTEX_EXTENSIONS
		glEnableClientState(GL_VERTEX_ARRAY_RANGE_APPLE);
#endif // FAST_APPLE_VERTEX_EXTENSIONS
#endif // __APPLE__		
    }
  
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef (xzoomcenter*xscale, yzoomcenter*yscale, 0);
	glScalef (xscale, yscale, 1.0);
	if (cp->spin->value())
		angle += cp->rot_slider->value()/100.0;
	else
		angle = cp->rot_slider->value();
	glRotatef(angle, 0.0, 1.0, 0.1);
	glTranslatef (-xcenter, -ycenter, 0.0);
	glTranslatef (-xzoomcenter, -yzoomcenter, 0);

	if (cp->dont_clear->value() == 0)
    {
		//glClearColor(0.0,0.0,0.0,0.0);
		glClearColor(cp->Bkg->value(), cp->Bkg->value(), cp->Bkg->value(), 0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		draw_grid();
		draw_labels();
    }

	if (selection_changed)
		handle_selection ();
	draw_data_points();
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
	// axes
	if (cp->show_axes->value())
	{
		if (cp->Bkg->value() <= 0.4)
			glColor4f(0.4,0.4,0.4,0.0);
		else
			glColor4f(0.6*cp->Bkg->value(), 0.6*cp->Bkg->value(), 0.6*cp->Bkg->value(), 0.0);
		glBegin (GL_LINES);
		glVertex3f (-1.0, 0.0,  0.0); glVertex3f (+1.0, 0.0,  0.0);
		glVertex3f (0.0, -1.0,  0.0); glVertex3f (0.0, +1.0,  0.0);
		glVertex3f (0.0,  0.0, -1.0); glVertex3f (0.0,  0.0, +1.0);
		glEnd();
	}
}

void plot_window::draw_labels ()
{
	if (!cp->show_labels->value())
		return;
	// note: perhaps we should not bother with one if it is too close to the origin?
	// if (show_axis_labels->value())
    {
		glPushMatrix ();
		glLoadIdentity();
		gl_font (FL_HELVETICA, 14);
		if (cp->Bkg->value() <= 0.5)
			glColor4f(0.8,0.8,0.8,0.0);
		else
			glColor4f(0.2,0.2,0.2,0.0);

		float xlabel_width = 2.0 * gl_width(xlabel.c_str())/(float)(this->w());
//		float ylabel_width = 2.0 * gl_width(ylabel.c_str())/(float)(this->w());
		float offset = 0.05;  // how far label should be from end of vector

		gl_draw((const char *)(xlabel.c_str()), 1.0F-(offset+xlabel_width), 0.0F-offset);
		gl_draw((const char *)(ylabel.c_str()), offset, 1.0F-2*offset);
		glPopMatrix ();
	}
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
		glVertex2f (xdown, 		ydown);
		glVertex2f (xtracked,   ydown);
		glVertex2f (xtracked,   ytracked);
		glVertex2f (xdown,		ytracked);
		glEnd();
	}
	blitz::Range NPTS(0,npoints-1);	
	if (extend_selection)
	{
		selected(NPTS) = where((vertices(NPTS,0)>fmaxf(xdown,xtracked) || vertices(NPTS,0)<fminf(xdown,xtracked) ||
								vertices(NPTS,1)>fmaxf(ydown,ytracked) || vertices(NPTS,1)<fminf(ydown,ytracked)),
							   0,1) || selected(NPTS);
	} else {
		selected(NPTS) = where((vertices(NPTS,0)>fmaxf(xdown,xtracked) || vertices(NPTS,0)<fminf(xdown,xtracked) ||
								vertices(NPTS,1)>fmaxf(ydown,ytracked) || vertices(NPTS,1)<fminf(ydown,ytracked)),
							   0,1);
	}

	color_array_from_selection ();

	// done flagging selection for this plot
	selection_changed = 0;
	
}
	
float r1=1.0, g1=0.05, b1=0.05, alpha1 = 1.0;

void plot_window::color_array_from_selection()
{
	GLfloat color1[4], color2[4];
	color1[0] = color2[2] = r1;
	color1[1] = color2[1] = g1;
	color1[2] = color2[0] = b1;
	color1[3] = color2[3] = alpha1;
	
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
			altcolors(i,0) = altcolors(i,1) = altcolors(i,2) = altcolors(i,3) = 0.0;
		}
	}
}

int xorder (const void *i, const void *j)
{
	if(xpoints(*(int *)i) < xpoints(*(int *)j))
		return -1;
	return (xpoints(*(int *)i) > xpoints(*(int *)j));
}

int yorder (const void *i, const void *j)
{
	if(ypoints(*(int *)i) < ypoints(*(int *)j))
		return -1;
	return (ypoints(*(int *)i) > ypoints(*(int *)j));
}

int zorder (const void *i, const void *j)
{
	if(zpoints(*(int *)i) < zpoints(*(int *)j))
		return -1;
	return (zpoints(*(int *)i) > zpoints(*(int *)j));
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
	glDisable(GL_DEPTH_TEST);

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

	// draw all the points
	if (display_deselected)
		glColorPointer (4, GL_FLOAT, 0, cp);
	else
		glColorPointer (4, GL_FLOAT, 0, altcp);
		
	glVertexPointer (3, GL_FLOAT, 0, vp);

#ifdef __APPLE__
#ifdef FAST_APPLE_VERTEX_EXTENSIONS
	glVertexArrayRangeAPPLE (3*npoints*sizeof(GLfloat),(GLvoid *)vp);
	glVertexArrayParameteriAPPLE (GL_VERTEX_ARRAY_STORAGE_HINT_APPLE, GL_STORAGE_CACHED_APPLE);  // for static data
	glVertexArrayParameteriAPPLE (GL_VERTEX_ARRAY_STORAGE_HINT_APPLE, GL_STORAGE_SHARED_APPLE);  // for dynamic data
#endif // FAST_APPLE_VERTEX_EXTENSIONS
#endif __APPLE__

	glDrawArrays (GL_POINTS, 0, npoints);

	glEnable(GL_DEPTH_TEST);
}

void plot_window::draw_histograms()
{
	if (!(cp->show_histogram->value()))
		return;

	// compute histogram
	const int nbins=128;
	blitz::Array<float,1> counts(nbins);
	counts = 0.0;
	float xmin=vertices(x_rank(0),0);
	float xmax=vertices(x_rank(npoints-1), 0);
	float xrange= xmax-xmin;
	for (int i=0; i<npoints; i++)
	{
		float x = vertices(i,0);
		int bin=(int)(nbins*((x-xmin)/xrange));
		if (bin == nbins)
			bin--;
		counts(bin)++;
	}
	counts = counts/sqrt((float)npoints);

	// draw histogram
	glDisable(GL_DEPTH_TEST);
	glPushMatrix();
	glLoadIdentity();

	glTranslatef (xzoomcenter*xscale, 0.0, 0);
	glScalef (xscale, 1.0/(float)nbins, 1.0);
	glTranslatef (-xcenter, 0.0, 0.0);
	glTranslatef (-xzoomcenter, 0.0, 0);
	glColor4f (0.0, 1.0, 0.0, 1.0);
	float width = (xmax-xmin) / (float)(nbins-1);
	float x = xmin;
	glBegin(GL_LINE_STRIP);
	glVertex2f(x,0.0);					
	for (int bin=0; bin<nbins; bin++)
	{
		// left edge
		glVertex2f(x,counts(bin));			
		// top edge
		glVertex2f(x+width,counts(bin));	
		// right edge 
		glVertex2f(x+width,0.0);
		x+=width;
	}
	glEnd();
	glPopMatrix();
	glEnable(GL_DEPTH_TEST);
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
	if (cp->ratio->value())
	{
		vertices(NPTS,0) = tmp1 / vertices(NPTS,2);
	}
	else if (cp->polar->value())
	{
		vertices(NPTS,0) = atan2(tmp1, tmp2);
		vertices(NPTS,1) = sqrt(pow2(tmp1)+pow2(tmp2));
	}
	return 1;
}

int 
plot_window::normalize (blitz::Array<float,1> a, blitz::Array<int,1> a_rank, int style)
{
	if (style == NORMALIZATION_NONE)
		return 1;
	blitz::Range NPTS(0,npoints-1);
	blitz::Array<int,1> inrange(npoints);
	inrange = where(((a(NPTS) < MAXFLOAT) && (a(NPTS) > -MAXFLOAT)), 1, 0);
	float amin = min(where(inrange,a(NPTS), MAXFLOAT));
	float amax = max(where(inrange,a(NPTS),-MAXFLOAT));
	float mu,sigma;

	switch (style)
	{
	case NORMALIZATION_MINMAX:
		if ((amax != amin) && finite(amin) && finite(amax))
			a(NPTS) = -1 + 2*(a(NPTS) - amin) / (amax-amin);
		return 1;
	case NORMALIZATION_ZEROMAX:
		if (finite(amax) && amax != 0.0)
			a(NPTS) = -1 + 2*a(NPTS)/amax;
		return 1;
	case NORMALIZATION_MAXABS:
		amax = fmaxf(fabsf(amin),fabs(amax));
		if (finite(amax) && amax != 0.0)
			a(NPTS) = -1 + 2*a(NPTS)/amax;
		a(NPTS) = a(NPTS) / amax;
		return 1;
	case NORMALIZATION_THREESIGMA:
		mu = mean(a(NPTS));
		sigma = sqrt((1.0/(float)npoints)*sum(pow2(a(NPTS)-mu)));
		DEBUG (cout << "mu, sigma = " << mu << ", " << sigma << endl);
		if (finite(mu) && (sigma!=0.0))
			a(NPTS) = (a(NPTS) - mu) / (3*sigma);
		return 1;
	case NORMALIZATION_RANK:
		for(int i=0; i<npoints; i++)
		{
			a(a_rank(i)) = 2*float(i) / (float)npoints - 1;
		}
		return 1;
	default:
		return 0;
	}
}

void
plot_window::compute_rank (blitz::Array<float,1> a, blitz::Array<int,1> a_rank)
{
	current_plot_window = this;
	blitz::Range NPTS(0,npoints-1);
	if (!a_rank.isStorageContiguous() || !a.isStorageContiguous())
	{
		cerr << "Tried to pass non-contigous data to qsort.  Aborting!" << endl;
		exit (1);
	}
	a_rank(NPTS) = identity(NPTS);
		
	// this is ugly.  should replace with... C++ sort?
	if (a.data() == xpoints.data())
	{
		qsort(a_rank.data(),npoints,sizeof(int),xorder); 
	}
	else if (a.data() == ypoints.data())
	{
		qsort(a_rank.data(),npoints,sizeof(int),yorder); 
	}
	else if (a.data() == zpoints.data())
	{
		qsort(a_rank.data(),npoints,sizeof(int),zorder); 
	}
	else
	{
		cerr << "problems computing rank index order" << endl;
		return;
	}
	cout << "a_rank(0)=" << a_rank(0) << ", a(a_rank(0))=" << a(a_rank(0)) << endl;
	cout << "a_rank(npoints-1)=" << a_rank(npoints-1) << ", a(a_rank(npoints-1))=" << a(a_rank(npoints-1)) << endl;
}

int
plot_window::extract_data_points ()
{
#if 0
	int t1 = (int)(cp->t1_slider->value());
	int dt = (int)(cp->dt_slider->value());
	int t2 = t1 + dt;
	if (t1 < 0)
		return 0;
	if (t2 > nvals)
		return 0;
#endif
	// get the labels for the plot's axes
	int t1 = (int)(cp->varindex1->mvalue()->user_data());
	int t2 = (int)(cp->varindex2->mvalue()->user_data());
	int t3 = (int)(cp->varindex3->mvalue()->user_data());

	xlabel = column_labels[t1];
	ylabel = column_labels[t2];
	zlabel = column_labels[t3];

	blitz::Range NPTS(0,npoints-1);

	xpoints(NPTS) = points(t1,NPTS);
	ypoints(NPTS) = points(t2,NPTS);
	zpoints(NPTS) = points(t3,NPTS);

	cout << "computing rank of " << xlabel << endl;
	compute_rank(xpoints,x_rank);
	cout << "computing rank of " << ylabel << endl;
	compute_rank(ypoints,y_rank);
	cout << "computing rank of " << zlabel << endl;
	compute_rank(zpoints,z_rank);

	(void) normalize (xpoints, x_rank, cp->x_normalization_style->value());
	(void) normalize (ypoints, y_rank, cp->y_normalization_style->value());
	(void) normalize (zpoints, z_rank, cp->z_normalization_style->value());
	(void) transform_2d ();

	vertices(NPTS,0) = xpoints(NPTS);
	vertices(NPTS,1) = ypoints(NPTS);
	vertices(NPTS,2) = zpoints(NPTS);

#ifdef __APPLE___
#ifdef FAST_APPLE_VERTEX_EXTENSIONS
	GLvoid *vp = (GLvoid *)vertices.data();
	glFlushVertexArrayRangeAPPLE(3*npoints*sizeof(GLfloat), vp);
#endif FAST_APPLE_VERTEX_EXTENSIONS
#endif // __APPLE__
	return 1;
}

void
control_panel_window::extract_and_redraw ()
{
	if (pw->extract_data_points())
		//pw->redraw ();
		pw->needs_redraw = 1;
}

// normalize data array using global max magnitude
void normalize_minmax ()
{
	float limit = 1.0e20;  // anything with higher magnitude is assumed to be bogus.
	blitz::Range NPTS(0,npoints-1);
	blitz::Range NVALS(skip,nvals-1);
	xmin = min(where(points(NVALS,NPTS)<-limit, 0.0, points(NVALS,NPTS)));
	xmax = max(where(points(NVALS,NPTS)>+limit, 0.0, points(NVALS,NPTS)));
	gmax = (fabs(xmin)>fabs(xmax)) ? fabs(xmin) : fabs(xmax);
	cout << "global min = " << xmin << ", global max = " << xmax << ", scaling all data by " << 1.0/gmax << endl;
	points(NVALS,NPTS) = points(NVALS,NPTS) / gmax;
}
  
void npts_changed(Fl_Widget *o) 
{
	npoints = int(((Fl_Slider *)o)->value());
	redraw_all_plots ();
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
	nvals = column_labels.size();
	if (nvals > nvals_max)
	{
		cerr << "Error: too many columns, increase nvals_max and recompile" << endl;
		exit (1);
	}
	cout << "column_labels = ";
	for( int i=0; i < nvals; i++ )
	{
		cout << column_labels[i] << " ";
	}  
	cout << endl;
	cout << "there should be " << nvals << " fields (columns) per record (row)" << endl;

	int i=0;
	while (!cin.eof() && i<npoints)
    {
		(void) getline (cin, line, '\n');
		DEBUG (cout << "line is: " << line << endl);
		if (line.length() == 0)
			continue;
		std::stringstream ss(line); // Insert the string into a stream
		for (int j=0; j<nvals; j++)
		{
			double x;
			ss >> x;
			points(j,i) = (float)x;
// FIX THIS MISSING DATA STUFF!! IT IS BROKEN.
			if (ss.eof() && j<nvals-1)
			{
				cerr << "not enough data on line " << i+2 << ", aborting!" << endl;
				exit(1);
			}
			if (!ss.good() && j<nvals-1)
			{
				cerr << "bad data at line " << i+1 << " column " << j+1 << ", skipping entire line." << endl;
				goto nextline;
			}
			DEBUG (cout << "points(" << j << "," << i << ") = " << points(j,i) << endl);
		}
		for (int j=0; j<nvals; j++)
			if (points(j,i) == -9999)
			{
				cerr << "bad data at line " << i << ", column " << j << " - skipping entire line\n";
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
	char line[50*nvals+1];	// 50 characters better hold one value + delimiters
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
		for (int j=0; j<nvals; j++)
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
	blitz::Array<float,1> vals(nvals);
	blitz::Range NVALS(0,nvals-1);
	int i;
	if (!points.isStorageContiguous())
	{
		cerr << "Tried to pass non contigous buffer to read.  Aborting!" << endl;
		exit (1);
	}
		
	for (i=0; i<npoints; i++)
	{
		unsigned int ret = read(0, (void *)(vals.data()), nvals*sizeof(float));
		if (ret != nvals*sizeof(float))
		{
			if (ret == 0) // EOF
				break;
			else
			{
				fprintf (stderr, "error reading input occured at line %d\n", i+1);
				exit (1);
			}
		}
		points(NVALS,i) = vals;
		if (i>0 && (i%10000 == 0))
			printf ("read %d lines\n", i);
	}
	cout << "read " << i << " lines." << endl;
	npoints = i;
}

void
control_panel_window::make_widgets(control_panel_window *cpw)
{

	int xpos = 50;
	int ypos = 0;

	Fl_Button *b;

	npts_slider = new Fl_Hor_Value_Slider_Input(xpos, ypos, cpw->w()-60, 20, "npts");
	npts_slider->align(FL_ALIGN_LEFT);
	npts_slider->callback(npts_changed);
	npts_slider->value(npoints);
	npts_slider->step(1);
	npts_slider->bounds(1,npoints);

	pointsize_slider = new Fl_Hor_Value_Slider_Input(xpos, ypos+=25, cpw->w()-60, 20, "size");
	pointsize_slider->align(FL_ALIGN_LEFT);
	pointsize_slider->value(2.0);
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
	rot_slider->bounds(-90.0, 90.0);

	// dynamically build the variables menu
	// cout << "starting menu build, nvals = " << nvals << endl;
	for (int i=0; i<nvals; i++)
    {
		// cout << "label " << i << " = " << column_labels[i].c_str() << endl;
		varindex_menu_items[i].label((const char *)(column_labels[i].c_str()));
		varindex_menu_items[i].user_data((void *)i);
    }
	varindex_menu_items[nvals].label(0);

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
	x_normalization_style->value(NORMALIZATION_MINMAX);
	x_normalization_style->callback((Fl_Callback*)static_extract_and_redraw, this);
 
	y_normalization_style = new Fl_Choice (xpos+100, ypos, 100, 25, "normalize y");
	y_normalization_style->align(FL_ALIGN_TOP);
	y_normalization_style->textsize(12);
	y_normalization_style->menu(normalization_style_menu_items);
	y_normalization_style->value(NORMALIZATION_MINMAX); 
	y_normalization_style->callback((Fl_Callback*)static_extract_and_redraw, this);
 
	z_normalization_style = new Fl_Choice (xpos+200, ypos, 100, 25, "normalize z");
	z_normalization_style->align(FL_ALIGN_TOP);
	z_normalization_style->textsize(12);
	z_normalization_style->menu(normalization_style_menu_items);
	z_normalization_style->value(NORMALIZATION_MINMAX); 
	z_normalization_style->callback((Fl_Callback*)static_extract_and_redraw, this);
 
	int xpos2 = xpos;
	int ypos2 = ypos;

	spin = new Fl_Button(xpos2, ypos+=25, 20, 20, "spin");
	spin->align(FL_ALIGN_RIGHT);
	spin->type(FL_TOGGLE_BUTTON);
	spin->selection_color(FL_YELLOW);

	dont_clear = new Fl_Button(xpos2, ypos+=25, 20, 20, "don't clear");
	dont_clear->align(FL_ALIGN_RIGHT);
	dont_clear->type(FL_TOGGLE_BUTTON);
	dont_clear->selection_color(FL_YELLOW);
	dont_clear->callback((Fl_Callback*)static_maybe_redraw, this);

	transform_style = new Fl_Group (xpos2-1, ypos+25-1, 20+2, 3*20+4);

		no_transform = b = new Fl_Button(xpos2, ypos+=25, 20, 20, "identity");
		b->callback((Fl_Callback*)static_extract_and_redraw, this);
		b->align(FL_ALIGN_RIGHT); b->type(FL_RADIO_BUTTON); b->selection_color(FL_YELLOW);
		
		sum_vs_difference = b = new Fl_Button(xpos2, ypos+=25, 20, 20, "sum vs. diff.");
		b->callback((Fl_Callback*)static_extract_and_redraw, this);
		b->align(FL_ALIGN_RIGHT); b->type(FL_RADIO_BUTTON); b->selection_color(FL_YELLOW);
		
		ratio = b = new Fl_Button(xpos2, ypos+=25, 20, 20, "x/z vs y");
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

	show_axes = b = new Fl_Button(xpos, ypos+=25, 20, 20, "axes");
	b->callback((Fl_Callback*)static_maybe_redraw, this);
	b->align(FL_ALIGN_RIGHT); b->type(FL_TOGGLE_BUTTON); b->selection_color(FL_YELLOW);	b->value(1);

	show_labels = b = new Fl_Button(xpos, ypos+=25, 20, 20, "labels");
	b->callback((Fl_Callback*)static_maybe_redraw, this);
	b->align(FL_ALIGN_RIGHT); b->type(FL_TOGGLE_BUTTON); b->selection_color(FL_YELLOW);	b->value(1);

	show_histogram = b = new Fl_Button(xpos, ypos+=25, 20, 20, "histogram");
	b->callback((Fl_Callback*)static_maybe_redraw, this);
	b->align(FL_ALIGN_RIGHT); b->type(FL_TOGGLE_BUTTON); b->selection_color(FL_YELLOW);	b->value(0);
}

void
redraw_all_plots ()
{
	#if 0
	spin = new Fl_Button(xpos2, ypos+=25, 20, 20, "spin");
	spin->align(FL_ALIGN_RIGHT);
	spin->type(FL_TOGGLE_BUTTON);
	spin->selection_color(FL_YELLOW);
	//spin->callback((Fl_Callback*)static_maybe_redraw, this);

	pw1->redraw();
	pw2->redraw();
	pw3->redraw();
	pw4->redraw();
	#endif
	pw1->needs_redraw = 1;
	pw2->needs_redraw = 1;
	pw3->needs_redraw = 1;
	pw4->needs_redraw = 1;
}

void 
reset_all_plots ()
{
	pw1->reset_view();
	pw2->reset_view();
	pw3->reset_view();
	pw4->reset_view();
}

void
redraw_if_changing (void * dummy)
{
	if (cp1->spin->value() || pw1->needs_redraw)
	{
		pw1->redraw();
		pw1->needs_redraw = 0;
	}
	if (cp2->spin->value() || pw2->needs_redraw)
	{
		pw2->redraw();
		pw2->needs_redraw = 0;
	}
	if (cp3->spin->value() || pw3->needs_redraw)
	{
		pw3->redraw();
		pw3->needs_redraw = 0;
	}
	if (cp4->spin->value() || pw4->needs_redraw)
	{
		pw4->redraw();
		pw4->needs_redraw = 0;
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
	xpoints.resize(npoints);
	ypoints.resize(npoints);
	zpoints.resize(npoints);

	colors.resize(npoints,4);
	altcolors.resize(npoints,4);
	identity.resize(npoints);
	selected.resize(npoints);

	for (int i=0; i<npoints; i++)
		selected(i)=0;
}

int main(int argc, char **argv)
{

	srand((unsigned int)time(0));

//  read_binary_file ();
//	read_ascii_file ();
	read_ascii_file_with_headers ();
	
	resize_global_arrays ();

	cout << "making identity" << endl;
	for (int i=0; i<npoints; i++)
		identity(i)=i;

//	normalize_minmax();

	// create control panel window
	cp1 = new control_panel_window (300, (Fl::h()-50)/2);
	cp1->label("1");
	cp1->resizable(cp1);
	cp1->position(Fl::h(),0);
	cp1->make_widgets (cp1);
	cp1->end();
    
	// create plotting window
	pw1 = new plot_window(Fl::h()/2, (Fl::h()-50)/2);
	pw1->label("1");
	pw1->resizable(pw1);
	pw1->position(1,0);
	pw1->end();

	// link plot window and its associated control panel window
	cp1->pw = pw1;
	pw1->cp = cp1;
	pw1->reset_view();

	// first display of data and control panel windows
	cp1->varindex1->value(0);  
	cp1->varindex2->value(1);  
	cp1->varindex3->value(2);  
	cp1->show(argc,argv);
	pw1->extract_data_points();
	pw1->show(argc,argv);


	// create control panel window
	cp2 = new control_panel_window (300, (Fl::h()-50)/2);
	cp2->label("2");
	cp2->resizable(cp2);
	cp2->position((Fl::h()+300+75),0);
	cp2->make_widgets (cp2);
	cp2->end();
    
	// create plotting window
	pw2 = new plot_window((Fl::h())/2, (Fl::h()-50)/2);
	pw2->label("2");
	pw2->resizable(pw2);
	pw2->position((Fl::h())/2,0);
	pw2->end();

	// link plot window and its associated control panel window
	cp2->pw = pw2;
	pw2->cp = cp2;
	pw2->reset_view();

	// first display of data and control panel windows
	cp2->varindex1->value(0); 
	cp2->varindex2->value(2); 
	cp2->varindex3->value(1); 
	cp2->show(argc,argv);
	pw2->extract_data_points();
	pw2->show(argc,argv);

	// create control panel window
	cp3 = new control_panel_window (300, (Fl::h()-50)/2);
	cp3->label("3");
	cp3->resizable(cp3);
	cp3->position(Fl::h(),((Fl::h()-50)/2)+50);
	cp3->make_widgets (cp3);
	cp3->end();
    
	// create plotting window
	pw3 = new plot_window(Fl::h()/2, (Fl::h()-50)/2);
	pw3->label("3");
	pw3->resizable(pw3);
	pw3->position(0,((Fl::h()-50)/2)+50);
	pw3->end();

	// link plot window and its associated control panel window
	cp3->pw = pw3;
	pw3->cp = cp3;
	pw3->reset_view();

	// first display of data and control panel windows
	cp3->varindex1->value(rand()%nvals);
	cp3->varindex2->value(rand()%nvals);
	cp3->varindex3->value(rand()%nvals);
	cp3->show(argc,argv);
	pw3->extract_data_points();
	pw3->show(argc,argv);

	// create control panel window
	cp4 = new control_panel_window (300, (Fl::h()-50)/2);
	cp4->label("4");
	cp4->resizable(cp4);
	cp4->position((Fl::h()+300+75),((Fl::h()-50)/2)+50);
	cp4->make_widgets (cp4);
	cp4->end();
    
	// create plotting window
	pw4 = new plot_window((Fl::h())/2, (Fl::h()-50)/2);
	pw4->label("4");
	pw4->resizable(pw4);
	pw4->position((Fl::h())/2,((Fl::h()-50)/2)+50);
	pw4->end();

	// link plot window and its associated control panel window
	cp4->pw = pw4;
	pw4->cp = cp4;
	pw4->reset_view();

	// first display of data and control panel windows
	cp4->varindex1->value(rand()%nvals);
	cp4->varindex2->value(rand()%nvals);
	cp4->varindex3->value(rand()%nvals);
	cp4->show(argc,argv);
	pw4->extract_data_points();
	pw4->show(argc,argv);

	Fl::add_idle(redraw_if_changing);
//	Fl::add_check(redraw_if_changing);
	int result = Fl::run();  // enter main event loop
	return result;
}

