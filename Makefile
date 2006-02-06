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
# PROFILE		= -pg
# DEBUG		= -ggdb -g3 -Wall -Wunused -DBZ_DEBUG
DEBUG		= -g -ggdb -g3 -Wall -Wunused -fexceptions

ifeq ($(platform),Darwin)
#	OPTIM = $(DEBUG) -pipe
	OPTIM = -O6 -ftree-vectorize -ftree-vectorizer-verbose=0 -Wall -Wno-unused -Wno-long-double -Wno-deprecated -fno-exceptions -ffast-math -pipe -fsigned-char -maltivec -mabi=altivec -faltivec -mcpu=G4 -mtune=G4 -mpowerpc-gfxopt -g
else
#	OPTIM = -O0 $(DEBUG) -pipe
	OPTIM = -O6 -Wall -Wunused -ffast-math -fno-exceptions -g  -Wno-deprecated
endif

CFLAGS		= $(OPTIM) 
CXXFLAGS	= $(OPTIM) -DGL_GLEXT_PROTOTYPES


#CFLAGS		= $(DEBUG)
#CXXFLAGS	= $(DEBUG) 

# libraries to link with:
ifeq ($(platform),Darwin)
	INCPATH = -I/sw/include
	LIBPATH	= -L/usr/local/lib -L/sw/lib
	LDLIBS = -framework AGL -framework OpenGL -framework Carbon -framework ApplicationServices -framework vecLib -lm -lmx -lgsl 
else
	INCPATH = -I/u/wk/creon/include
	LIBPATH	= -L/u/wk/creon/lib 
	LDLIBS = -lGL -L/usr/X11R6/lib -lXext -lm -lgsl 
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

SRCS =	grid2.cc 
#SRCS =	mgrid.cc grid2.cc

OBJS:=	$(SRCS:.cc=.o)

default:	$(TARGET)

clean:
	rm -f $(ALL) *.o $(TARGET) grid2 core* TAGS *.gch makedepend 

depend:	$(SRCS)
	$(MAKEDEPEND) $(INCBLITZ) $(INCPATH) $(INCFLEWS) -o makedepend $(SRCS)

# Automatically generated dependencies if they are there...
-include makedepend

# there has to be a better way....
tags:	TAGS
TAGS:	depend $(SRCS)
	etags --defines --members -o TAGS $(SRCS) `cat makedepend | sed -e"s/.*://g; s/\\\\\//"`

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

