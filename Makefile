# Are we compiling on Darwin (mac OSX) or Linux ?
platform := $(shell uname)

# compiler names:
CXX		= g++
CC		= cc
#CXX = gxlc++
#CC = gxlc++
MAKEDEPEND	= $(CXX) -M

ifeq ($(platform),Darwin)
	POSTBUILD = /Developer/Tools/Rez -t APPL -o
else
	POSTBUILD = echo
endif

# flags for C++ compiler:
# DEBUG		= -ggdb -g3 -Wall -Wunused -DBZ_DEBUG
DEBUG		= -g -ggdb -g3 -Wall -Wunused -fexceptions

ifeq ($(platform),Darwin)
	OPTIM = -O0 $(DEBUG) -pipe
#	OPTIM = -O3 -ftree-vectorize -ftree-vectorizer-verbose=0 -Wall -Wno-unused -Wno-long-double -Wno-deprecated -fno-exceptions -ffast-math -pipe -fsigned-char -maltivec -mabi=altivec -faltivec -mcpu=G4 -mtune=G4 -mpowerpc-gfxopt -g
else
#	OPTIM = -O4 -Wall -Wunused -ffast-math -fno-exceptions -g  -Wno-deprecated
	OPTIM = -O0 $(DEBUG) -pipe
endif

CFLAGS		= $(OPTIM) 
CXXFLAGS	= $(OPTIM) -DGL_GLEXT_PROTOTYPES


#CFLAGS		= $(DEBUG)
#CXXFLAGS	= $(DEBUG) 

# libraries to link with:
ifeq ($(platform),Darwin)
	INCPATH = -I/sw/include
	LIBPATH	= -L/usr/local/lib -L/sw/lib
	LDLIBS = -framework AGL -framework OpenGL -framework Carbon -framework ApplicationServices -framework veclib -lm -lmx -lgsl
else
	INCPATH = -I/u/wk/creon/include
	LIBPATH	= -L/u/wk/creon/lib 
	LDLIBS = -L/usr/X11R6/lib -lGL -lXext -lm -lgsl 
endif
INCFLEWS	= -I../flews-0.3
LINKFLEWS	= -L../flews-0.3 -lflews
LINKFLTK	= -lfltk
LINKFLTKGL	= -lfltk_gl
LINKBLITZ	= -lblitz

# The extension to use for executables...
EXEEXT		= 

# Build commands and filename extensions...
.SUFFIXES:	.c .cc .H .h .o $(EXEEXT)

.o$(EXEEXT):
	echo Linking $@...
	$(CXX) $(CXXFLAGS) $< $(LIBPATH) $(LINKFLEWS) $(LINKFLTK) $(LINKBLITZ) $(LDLIBS) -o $@
	$(POSTBUILD) $@ ../FL/mac.r

.c.o:
	echo Compiling $<...
	$(CC) -I.. $(CFLAGS) -c $<

.cc.o:
	echo Compiling $<...
	$(CXX) $(INCPATH) $(INCFLEWS) -I.. $(CXXFLAGS) -c $<

TARGET =	grid2$(EXEEXT)
#TARGET =	mgrid$(EXEEXT)

#SRCS =	grid.cc histo.cc
SRCS =	grid2.cc 
#SRCS =	mgrid.cc grid2.cc

OBJS:=	$(SRCS:.cc=.o)

default:	$(TARGET)

clean:
	rm -f $(ALL) *.o $(TARGET) grid2 core TAGS *.gch makedepend

depend:	$(SRCS)
	touch makedepend
	$(MAKEDEPEND) $(INCBLITZ) $(INCPATH) $(INCFLEWS) -o makedepend $(SRCS)

# there has to be a better way....
tags:	$(SRCS) depend
	etags --declarations --defines --members -o TAGS $(SRCS) `cat makedepend`

stable.h.gch:	stable.h
	echo pre-compiling $<...
	$(CXX) $(INCPATH) $(INCFLEWS) -I.. $(CXXFLAGS) -c $<

grid2$(EXEEXT): $(OBJS)
	echo Linking $@...
	$(CXX) -I.. $(CXXFLAGS) -o $@ $(OBJS) $(LIBPATH) $(LINKFLEWS) $(LINKFLTKGL) $(LINKBLITZ) $(LINKFLTK) $(LDLIBS) 
	$(POSTBUILD) $@ mac.r


# for g77
#F77LIBS = /sw/lib/libiberty-g77.a /sw/lib/libg2c.a

#for gfortan
F77LIBS = /usr/local/lib/libgfortran.a

#	g77 -g -c mercury6_2.for
#	g77 -g -c mercury6_2.for -O5 -funroll-loops
mercury6_2.o:	mercury6_2.for mercury.inc swift.inc
		gfortran mercury6_2.for -ffixed-line-length-none -ftree-vectorize -maltivec -ftree-vectorizer-verbose=2 -c -O3 -force_cpusubtype_ALL -g


mgrid:	mercury6_2.o mgrid.o
		echo Linking $@...
		$(CXX) -I.. $(CXXFLAGS) -o $@ $(OBJS) mercury6_2.o $(LIBPATH) $(LINKFLEWS) $(LINKFLTKGL) $(LINKBLITZ) $(LINKFLTK) $(LDLIBS) $(F77LIBS)
		$(POSTBUILD) $@ mac.r


