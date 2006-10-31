# Are we compiling on Darwin (mac OSX) or Linux ?
platform := $(shell uname)

# compiler names:
CXX		= g++
CC		= cc
MAKEDEPEND	= $(CXX) -E -M

ifeq ($(platform),Darwin)
	POSTBUILD = /Developer/Tools/Rez -t APPL -o
else
	POSTBUILD = echo
endif

# flags for C++ compiler:
# PROFILE		= -pg
#DEBUG		= -O0 -ggdb -g3 -Wall -Wunused -DBZ_DEBUG -fexceptions
#DEBUG		= -g -ggdb -g3 -Wall -Wunused -fexceptions
DEBUG		= -gfull -ggdb -Wall -Wunused -fexceptions


ifeq ($(platform),Darwin)

### Uncomment ONE of the following

# uncomment for debugging version
#	OPTIM = $(DEBUG)

# uncomment to optimize for PowerPC (G4 and G5) 
#	OPTIM = -O6 -ftree-vectorize -ftree-vectorizer-verbose=0 -Wall -Wno-long-double -fno-exceptions -ffast-math -fsigned-char -maltivec -mabi=altivec -faltivec -mpowerpc-gfxopt -g

# uncomment optimize for intel mac
	OPTIM = -O6 -ftree-vectorize -ftree-vectorizer-verbose=0 -Wall -Wno-long-double -fno-exceptions -ffast-math -fsigned-char -gfull

else
# platform assumed to be linux

#	OPTIM = -O0 $(DEBUG)
	OPTIM = -O6 -Wextra -ffast-math -fno-exceptions -g

endif

CFLAGS		= $(OPTIM) 
CXXFLAGS	= $(OPTIM) -DGL_GLEXT_PROTOTYPES


#CFLAGS		= $(DEBUG)
#CXXFLAGS	= $(DEBUG) 

# libraries to link with:
ifeq ($(platform),Darwin)

# uncomment for OSX machines where I CAN install things as root... (don't forget to build all libraries as static only)
	INCPATH = -I/sw/include -I/usr/local/include
	LIBPATH	= -L/usr/local/lib -L/sw/lib
	LDLIBS = -framework AGL -framework OpenGL -framework Carbon -framework ApplicationServices -framework vecLib -lm -lmx -lgsl

# uncomment for OSX machines where I can NOT install things as root... (don't forget to build all libraries as static only)
#	INCPATH = -I/Users/creon/include
#	LIBPATH	= -L/Users/creon/lib 
#	LDLIBS = -framework AGL -framework OpenGL -framework Carbon -framework ApplicationServices -framework vecLib -lm -lmx -lgsl

else
# for NAS linux machines where I can NOT install things as root (don't forget to build all libraries as static only)
	INCPATH = -I/u/wk/creon/include
	LIBPATH	= -L/u/wk/creon/lib -L/usr/X11R6/lib 
	LDLIBS = -lGLU -lGL -lXext -lm -lgsl
# for debugging
#	LDLIBS = -lGLU -lGL -lXext -lm -lgsl -lefence -lpthread  
endif

INCFLEWS	= -I../flews-0.3

LINKFLEWS	= -L../flews-0.3 -lflews
LINKFLTK	= -lfltk -lfltk_gl
LINKBLITZ	= -lblitz

# The extension to use for executables...
EXEEXT		= 

SRCS =	vp.cpp control_panel_window.cpp plot_window.cpp data_file_manager.cpp
#SRCS =	vp.cpp 

OBJS:=	$(SRCS:.cpp=.o)

TARGET = vp$(EXEEXT)

default: $(TARGET)

all: depend tags $(TARGET)

%.o : %.c	
	echo Compiling $<...
	$(CC) -I.. $(CFLAGS) -c $<

%.o : %.cpp
	echo Compiling $<...
	$(CXX) $(INCPATH) $(INCFLEWS) -I.. $(CXXFLAGS) -c $<

$(TARGET):	$(OBJS)
	echo Linking $@...
	$(CXX) $(CXXFLAGS) $(OBJS) $(LIBPATH) $(LINKFLEWS) $(LINKFLTK) $(LINKBLITZ) $(LDLIBS) -o $@
	$(POSTBUILD) $@ ./mac.r

clean:
	rm -f $(ALL) *.o $(TARGET) vp core* TAGS *.gch makedepend 

depend:	$(SRCS)
	$(MAKEDEPEND) $(INCBLITZ) $(INCPATH) $(INCFLEWS) $(SRCS) > makedepend

# Automatically generated dependencies if they are there...
-include makedepend

# there has to be a better way....
tags:	TAGS
TAGS:	depend $(SRCS)
	etags -o TAGS $(SRCS) `cat makedepend | sed -e"s/.*://g; s/\\\\\//"`

stable.h.gch:	stable.h
	echo pre-compiling $<...
	$(CXX) $(INCPATH) $(INCFLEWS) -I.. $(CXXFLAGS) -c $<

# DO NOT DELETE
