#include "stable.h"

#include "grid2.H"

using namespace std;

const int nvals_max = 12;  	// maximum number of columns allowed in data file
const int MAXPOINTS = 20000;	// maximum rows in data file
const int skip = 0; 			// skip this many columns at the beginning of each row

int npoints = 10000+9;			// actual number of rows in data file
int nvals = 12;				// actual number of columns in data file
int display_deselected = 1;			// display deselected objects in alternate color

blitz::Array<float,2>  points(nvals_max,MAXPOINTS);	// main data array
blitz::Array<int,1> identity(MAXPOINTS);
blitz::Array<int,1> selected(MAXPOINTS);

// shared data with mercury6_2.for
typedef struct
{
	int integrating;
	float gtime;
	float x0[MAXPOINTS], y0[MAXPOINTS], z0[MAXPOINTS], u0[MAXPOINTS], v0[MAXPOINTS], w0[MAXPOINTS];
	float q0[MAXPOINTS], e0[MAXPOINTS], i0[MAXPOINTS], p0[MAXPOINTS], n0[MAXPOINTS], l0[MAXPOINTS];
} shared;
extern shared shared_;
blitz::Array<float,2> dpoints(&(shared_.x0[0]), blitz::shape(12,MAXPOINTS), blitz::neverDeleteData);


std::vector<std::string> column_labels; // vector to hold variable names

float xmin, xmax, gmax;

float pointsize = 1.0;
int istart = 0;

int sfactor = GL_CONSTANT_COLOR;
int dfactor = GL_DST_ALPHA;

class plot_window; // love those one-pass compilers.
class control_panel_window;
void redraw_all_plots_later (void);
void reset_all_plots (void);


class plot_window : public Fl_Gl_Window {
  protected:
	void draw();
	void draw_grid();
	void draw_labels();
	void draw_data_points(); // used to be virtual when overloaded by subclass
	int handle (int event);
	void handle_selection();
	void resort();
	int xprev, yprev, xcur, ycur;
	float xdragged, ydragged;
	float xcenter, ycenter, xscale, yscale;
	float xzoomcenter, yzoomcenter;
	float xdown, ydown, xtracked, ytracked;
	int selection_changed, extend_selection;
  public:
	plot_window(int w, int h);
//	blitz::Array<float,1> xpoints, ypoints, zpoints;
	blitz::Array<float,1> *xpoints, *ypoints, *zpoints;
	blitz::Array<int,1> a_rank;
	std::string xlabel, ylabel, zlabel;
	control_panel_window *cp;	// pointer to the control panel associated with this plot window
	int extract_data_points();
	int transform_2d();
	int normalize(blitz::Array<float,1>, int);
	void reset_view();
	float angle;
	int needs_redraw;
};

plot_window::plot_window(int w,int h) : Fl_Gl_Window(w,h) 
{
//	xpoints.resize(npoints);
//	ypoints.resize(npoints);
//	zpoints.resize(npoints);

	xpoints = new blitz::Array<float,1>(MAXPOINTS);
	ypoints = new blitz::Array<float,1>(MAXPOINTS);
	zpoints = new blitz::Array<float,1>(MAXPOINTS);

	a_rank.resize(npoints);
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
	Fl_Hor_Value_Slider_Input *C0, *C1, *C2, *C3;
	Fl_Hor_Value_Slider_Input *rot_slider;
	Fl_Choice *varindex1, *varindex2, *varindex3;
	
	Fl_Button *spin, *dont_clear, *show_axes, *show_grid, *show_labels;
	Fl_Button *integrate_button;
//	Fl_Button *x_equals_delta_x, *y_equals_delta_x;
	Fl_Group *transform_style;
	Fl_Button *sum_vs_difference, *polar, *ratio, *no_transform;
//	Fl_Button *normalize_x, *normalize_y;
//	Fl_Button *rank_x, *rank_y;
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
			redraw_all_plots_later ();
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
			redraw_all_plots_later ();
			return 1;
		case 'd': // don't display / display deselected dots
			display_deselected = 1 - display_deselected;
			redraw_all_plots_later ();
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
	xcenter = ycenter = 0.0;
	xscale = yscale = 0.95;
	xzoomcenter = yzoomcenter = 0.0;
	selection_changed = extend_selection = 0;
	angle = 0.0;
	needs_redraw = 1;
	cp->spin->value(0);
	cp->rot_slider->value(0.0);
	cp->dont_clear->value(0);
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
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glEnable(GL_POINT_SMOOTH);
		glHint(GL_POINT_SMOOTH_HINT,GL_NICEST);
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
		glClearColor(cp->C0->value(), cp->C0->value(), cp->C0->value(), 0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		draw_grid();
		draw_labels();
    }

	if (selection_changed)
		handle_selection ();
	draw_data_points();
}

void 
control_panel_window::maybe_redraw() 
{
  // kludge.  Avoid float redraw when setting "don't clear".
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
		if (cp->C0->value() <= 0.2)
			glColor4f(0.2,0.2,0.2,0.0);
		else
			glColor4f(0.8*cp->C0->value(), 0.8*cp->C0->value(), 0.8*cp->C0->value(), 0.0);
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
		if (cp->C0->value() <= 0.4)
			glColor4f(0.4,0.4,0.4,0.0);
		else
			glColor4f(0.6*cp->C0->value(), 0.6*cp->C0->value(), 0.6*cp->C0->value(), 0.0);
		glColor4f(0.4,0.4,0.4,0.0);
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
		gl_font (FL_HELVETICA , 12);
		if (cp->C0->value() <= 0.8)
			glColor4f(1,1,1,0.25);
		else
			glColor4f(0,0,0,0.25);

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
		selected(NPTS) = where(((*xpoints)(NPTS)>fmaxf(xdown,xtracked) || (*xpoints)(NPTS)<fminf(xdown,xtracked) ||
								(*ypoints)(NPTS)>fmaxf(ydown,ytracked) || (*ypoints)(NPTS)<fminf(ydown,ytracked)),
							   0,1) || selected(NPTS);
	} else {
		selected(NPTS) = where(((*xpoints)(NPTS)>fmaxf(xdown,xtracked) || (*xpoints)(NPTS)<fminf(xdown,xtracked) ||
								(*ypoints)(NPTS)>fmaxf(ydown,ytracked) || (*ypoints)(NPTS)<fminf(ydown,ytracked)),
							   0,1);
	}

	// done flagging selection for this plot
	selection_changed = 0;
	
}
	
int xorder (const void *i, const void *j)
{
	plot_window *pw = current_plot_window;
	if((*pw->xpoints)(*(int *)i) < (*pw->xpoints)(*(int *)j))
		return -1;
	return ((*pw->xpoints)(*(int *)i) > (*pw->xpoints)(*(int *)j));
}

int yorder (const void *i, const void *j)
{
	plot_window *pw = current_plot_window;
	if((*pw->ypoints)(*(int *)i) < (*pw->ypoints)(*(int *)j))
		return -1;
	return ((*pw->ypoints)(*(int *)i) > (*pw->ypoints)(*(int *)j));
}

int zorder (const void *i, const void *j)
{
	plot_window *pw = current_plot_window;
	if((*pw->zpoints)(*(int *)i) < (*pw->zpoints)(*(int *)j))
		return -1;
	return ((*pw->zpoints)(*(int *)i) > (*pw->zpoints)(*(int *)j));
}

void clearAlphaPlanes()
{
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
	glClearColor (0.0, 0.0, 0.0, 0.0);
	glClear (GL_COLOR_BUFFER_BIT);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

//float r1=0.5, g1=0.1, b1=0.025; 
//float r1=0.5,  g1=0.05, b1=0.05, alpha1 = 1.0;
float r1=0.75,  g1=0.05, b1=0.05, alpha1 = 1.0;
float r2=0.05, g2=0.05, b2=0.8, alpha2 = 1.0;
//float r3=0.05,  g3=0.5, b3=0.05, alpha3 = 1.0;
float r3=0.05,  g3=0.75, b3=0.05, alpha3 = 1.0;

float const_alpha=1.0, const_color=1.0, src_alpha=1.0;

void plot_window::draw_data_points()
{
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glEnable(GL_POINT_SMOOTH);
	glHint(GL_POINT_SMOOTH_HINT,GL_NICEST);
	glPointSize(cp->pointsize_slider->value());
	glBlendColor (const_color, const_color, const_color, cp->C3->value());
	glBlendFunc(sfactor, dfactor);
	// make these multiplicative....
	r1 = cp->C1->value(); 
	g1 = cp->C1->value() * cp->C2->value();
	// b1 = cp->C1->value() * cp->C2->value() * cp->C3->value();
	b1 = cp->C1->value() * cp->C2->value();
	glBegin(GL_POINTS);
	glColor4f(r1, g1, b1, src_alpha);
	for (int i=0; i<npoints; i++) {
		if (display_deselected)
		{
			if (!selected(i))
			{
				glVertex3d((*xpoints)(i), (*ypoints)(i), (*zpoints)(i));
			}
		}
	}
	glEnd();
//	clearAlphaPlanes();
//  glBlendFunc(GL_CONSTANT_COLOR, GL_ZERO);
	glBegin(GL_POINTS);
	glColor4f(r2, g2, b2, alpha2);
//	glColor4f(r3, g3, b3, alpha3);
	for (int i=0; i<npoints; i++) {
		if (selected(i))
		{
			glVertex3d((*xpoints)(i), (*ypoints)(i), (*zpoints)(i));
		}
	}
	glEnd();
	glEnable(GL_DEPTH_TEST);
}

int
plot_window::transform_2d ()
{
	if (cp->no_transform->value())
		return 1;  // no transform
	
	blitz::Range NPTS(0,npoints-1);

	blitz::Array <float,1> tmp1(npoints), tmp2(npoints);
	tmp1 = (*xpoints)(NPTS);
	tmp2 = (*ypoints)(NPTS);

	if (cp->sum_vs_difference->value())
	{
		(*xpoints)(NPTS) = (sqrt(2.0)/2.0) * (tmp1 + tmp2);
		(*ypoints)(NPTS) = (sqrt(2.0)/2.0) * (tmp1 - tmp2);
	}
	if (cp->ratio->value())
	{
		(*xpoints)(NPTS) = tmp1 / (*zpoints)(NPTS);
	}
	else if (cp->polar->value())
	{
		(*xpoints)(NPTS) = atan2(tmp1, tmp2);
		(*ypoints)(NPTS) = sqrt(pow2(tmp1)+pow2(tmp2));
	}
	return 1;
}

int 
plot_window::normalize (blitz::Array<float,1> a, int style)
{
	float  amin, amax, mu, sigma;
	blitz::Range NPTS(0,npoints-1);
	switch (style)
	{
	case NORMALIZATION_NONE:
		return 1;
	case NORMALIZATION_MINMAX:
		amin = min(a(NPTS));
		amax = max(a(NPTS));
		a(NPTS) = -1 + 2*(a(NPTS) - amin) / (amax-amin);
		return 1;
	case NORMALIZATION_ZEROMAX:
		amax = max(a(NPTS));
		a(NPTS) = -1 + 2*a(NPTS)/amax;
		return 1;
	case NORMALIZATION_MAXABS:
		amax = fmaxf(fabsf(min(a(NPTS))),fabsf(max(a(NPTS))));
		a(NPTS) = a(NPTS) / amax;
		return 1;
	case NORMALIZATION_THREESIGMA:
		mu = mean(a(NPTS));
		sigma = sqrt((1.0/(float)npoints)*sum(pow2(a(NPTS)-mu)));
		DEBUG (cout << "mu, sigma = " << mu << ", " << sigma << endl);
		a(NPTS) = (a(NPTS) - mu) / (3*sigma);
		return 1;
	case NORMALIZATION_RANK:
		current_plot_window = this;
		if (!a_rank.isStorageContiguous() || !a.isStorageContiguous())
		{
			cerr << "Tried to pass non-contigous data to qsort.  Aborting!" << endl;
			exit (1);
		}
		a_rank(NPTS) = identity(NPTS);
		
		// this is ugly.  should replace with... C++ sort?
		if (a.data() == xpoints->data())
		{
			qsort(a_rank.data(),npoints,sizeof(int),xorder); 
		}
		else if (a.data() == ypoints->data())
		{
			qsort(a_rank.data(),npoints,sizeof(int),yorder); 
		}
		else if (a.data() == zpoints->data())
		{
			qsort(a_rank.data(),npoints,sizeof(int),zorder); 
		}
		else
		{
			cerr << "problems computing rank index order" << endl;
			return 0;
		}
		for(int i=0; i<npoints; i++)
		{
			a(a_rank(i)) = 2*float(i) / (float)npoints - 1;
		}
		return 1;
	default:
		return 0;
	}
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
	// get the labels for the x and y axes
#endif

	int t1 = (int)(cp->varindex1->mvalue()->user_data());
	int t2 = (int)(cp->varindex2->mvalue()->user_data());
	int t3 = (int)(cp->varindex3->mvalue()->user_data());

	xlabel = column_labels[t1];
	ylabel = column_labels[t2];
	zlabel = column_labels[t3];

	// this should be done with a pointer!
	blitz::Range NPTS(0,npoints-1);

#if 0
	(*xpoints)(NPTS) = dpoints(t1,NPTS);
	(*ypoints)(NPTS) = dpoints(t2,NPTS);
	(*zpoints)(NPTS) = dpoints(t3,NPTS);
#endif

	delete xpoints;
	delete ypoints;
	delete zpoints;

	xpoints = new blitz::Array<float,1>(dpoints(t1,NPTS));
	ypoints = new blitz::Array<float,1>(dpoints(t2,NPTS));
	zpoints = new blitz::Array<float,1>(dpoints(t3,NPTS));


//	(void) normalize (xpoints, cp->x_normalization_style->value());
//	(void) normalize (ypoints, cp->y_normalization_style->value());
//	(void) normalize (zpoints, cp->z_normalization_style->value());
//	(void) transform_2d ();
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
	redraw_all_plots_later ();
}

extern "C" 
{
	extern void fsleep_ (void)
	{
		usleep (100000);
	}
}

void integrating_changed(Fl_Widget *o) 
{
	shared_.integrating = int(((Fl_Button *)o)->value());
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
			ss >> points(j,i);
			if (!ss.good() && j<nvals-1)
			{
				cerr << "not enough data on line " << i+2 << ", aborting!" << endl;
				exit(1);
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
	pointsize_slider->value(1.0);
	pointsize_slider->step(0.25);
	pointsize_slider->bounds(0.1,20.0);
	pointsize_slider->callback((Fl_Callback*)replot, this);

	C0 = new Fl_Hor_Value_Slider_Input(xpos, ypos+=25, cpw->w()-60, 20, "C0");
	C0->align(FL_ALIGN_LEFT);
	C0->step(0.001);
	C0->bounds(0.0,1.0);
	C0->callback((Fl_Callback*)replot, this);
	C0->value(0.0);

	C1 = new Fl_Hor_Value_Slider_Input(xpos, ypos+=25, cpw->w()-60, 20, "C1");
	C1->align(FL_ALIGN_LEFT);
	C1->step(0.001);
	C1->bounds(0.0,1.0);
	C1->value(0.75);
	C1->callback((Fl_Callback*)replot, this);

	C2 = new Fl_Hor_Value_Slider_Input (xpos, ypos+=25, cpw->w()-60, 20, "C2");
	C2->align(FL_ALIGN_LEFT);
	C2->callback((Fl_Callback*)replot, this);
	C2->step(0.001);
	C2->bounds(0,1.0);
	C2->value(0.05);

	C3 = new Fl_Hor_Value_Slider_Input (xpos, ypos+=25, cpw->w()-60, 20, "C3");
	C3->align(FL_ALIGN_LEFT);
	C3->callback((Fl_Callback*)replot, this);
	C3->step(0.001);
	C3->bounds(0,1.0);
	C3->value(1.0);

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

	integrate_button = b = new Fl_Button(xpos, ypos+=25, 20, 20, "integrate");
	b->callback(integrating_changed);
	b->align(FL_ALIGN_RIGHT); b->type(FL_TOGGLE_BUTTON); b->selection_color(FL_YELLOW);	b->value(1);
	shared_.integrating = 1;

	show_grid = b = new Fl_Button(xpos, ypos+=25, 20, 20, "grid");
	b->callback((Fl_Callback*)static_maybe_redraw, this);
	b->align(FL_ALIGN_RIGHT); b->type(FL_TOGGLE_BUTTON); b->selection_color(FL_YELLOW);	b->value(1);

	show_axes = b = new Fl_Button(xpos, ypos+=25, 20, 20, "axes");
	b->callback((Fl_Callback*)static_maybe_redraw, this);
	b->align(FL_ALIGN_RIGHT); b->type(FL_TOGGLE_BUTTON); b->selection_color(FL_YELLOW);	b->value(1);

	show_labels = b = new Fl_Button(xpos, ypos+=25, 20, 20, "labels");
	b->callback((Fl_Callback*)static_maybe_redraw, this);
	b->align(FL_ALIGN_RIGHT); b->type(FL_TOGGLE_BUTTON); b->selection_color(FL_YELLOW);	b->value(1);
}

void
redraw_all_plots_later ()
{
	pw1->needs_redraw = 1;
	pw2->needs_redraw = 1;
	pw3->needs_redraw = 1;
	pw4->needs_redraw = 1;
}

void
redraw_all_plots ()
{
	pw1->redraw();
	pw2->redraw();
	pw3->redraw();
	pw4->redraw();

	pw1->needs_redraw = 0;
	pw2->needs_redraw = 0;
	pw3->needs_redraw = 0;
	pw4->needs_redraw = 0;
}

void
redraw_changed_plots ()
{
	if (pw1->needs_redraw)
	{
		pw1->redraw();
		pw1->needs_redraw = 0;
	}
	if (pw2->needs_redraw)
	{
		pw2->redraw();
		pw2->needs_redraw = 0;
	}
	if (pw3->needs_redraw)
	{
		pw3->redraw();
		pw3->needs_redraw = 0;
	}
	if (pw4->needs_redraw)
	{
		pw4->redraw();
		pw4->needs_redraw = 0;
	}
}

void 
reset_all_plots ()
{
	pw1->reset_view();
	pw2->reset_view();
	pw3->reset_view();
	pw4->reset_view();
}


#include <stdlib.h>
#include <pthread.h>

#define NUM_THREADS	1

extern "C" 
{
extern void mercury6_ ();
}

void *callout(void *threadid)
{
	printf("Starting mercury6 in thread %ul\n", (unsigned int)threadid);
	mercury6_ ();
	pthread_exit(NULL);
}



void
redraw_if_changing (void * dummy)
{

	float fps = 15.0;

	static float last_time = 0.0;

	if (last_time != shared_.gtime)
	{

		// if we get here, the compute thread made enough progress to redraw
		last_time = shared_.gtime;

		// extract data from fortran array

		pw1->extract_data_points();
		pw2->extract_data_points();
		pw3->extract_data_points();
		pw4->extract_data_points();

		redraw_all_plots_later ();
	}

	redraw_changed_plots ();

	// yield processor for a while
	usleep (1000000/((int)fps));

#if 0
	struct timeval tp;
	static long useconds=0;
	static long seconds=0;

	// has at least 1/fps seconds elapsed? (sort of)
busy:
	(void) gettimeofday(&tp, (struct timezone *)0);
	if ((tp.tv_sec > seconds) || (((float)(tp.tv_usec - useconds)/1000000.0) > 1.0/fps))
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
#endif 
}


void set_column_headers ()
{
	column_labels.push_back("x");
	column_labels.push_back("y");
	column_labels.push_back("z");
	column_labels.push_back("u");
	column_labels.push_back("v");
	column_labels.push_back("w");
	column_labels.push_back("q");
	column_labels.push_back("e");
	column_labels.push_back("i");
	column_labels.push_back("p");
	column_labels.push_back("n");
	column_labels.push_back("l");
}

int main(int argc, char **argv)
{

	srand((unsigned int)time(0));

//  read_binary_file ();
//	read_ascii_file ();
//	read_ascii_file_with_headers ();

	set_column_headers ();
	
//start compute thread	
	pthread_t threads[NUM_THREADS];
	int rc, t=0;
	printf("Creating compute thread\n");
	rc = pthread_create(&threads[t], NULL, callout, (void *)t);
	if (rc){
		printf("ERROR; return code from pthread_create() is %d\n", rc);
		exit(-1);
	}

	sleep(1);
	
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
	cp1->varindex1->value(0);  // galaxy x
	cp1->varindex2->value(1);  // galaxy y
	cp1->varindex3->value(2);  // galaxy z
	cp1->show(argc,argv);
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
	cp2->varindex1->value(3); // galaxy RA
	cp2->varindex2->value(4); // galaxy RS
	cp2->varindex3->value(5); // galaxy dec
	cp2->show(argc,argv);
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
	pw4->show(argc,argv);

	Fl::add_idle(redraw_if_changing);
//	Fl::add_check(redraw_if_changing);
	int result = Fl::run();  // enter main event loop

	pthread_exit(NULL);

	return result;
}
