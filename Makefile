# Are we compiling on Darwin (mac OSX) or Linux ?
platform := $(shell uname)

# compiler names:
CXX		= g++
CC		= cc
MAKEDEPEND	= $(CXX) -M

ifeq ($(platform),Darwin)
	POSTBUILD = /Developer/Tools/Rez -t APPL -o
else
	POSTBUILD = echo
endif

# flags for C++ compiler:
# PROFILE		= -pg
DEBUG		= -O0 -ggdb -g3 -Wall -Wunused -DBZ_DEBUG
#DEBUG		= -g -ggdb -g3 -Wall -Wunused -fexceptions


ifeq ($(platform),Darwin)

### Uncomment ONE of the following

# debugging version
#	OPTIM = $(DEBUG)

# optimize for G5 
#	OPTIM = -O6 -ftree-vectorize -ftree-vectorizer-verbose=0 -Wall -Wno-long-double -fno-exceptions -ffast-math -pipe -fsigned-char -maltivec -mabi=altivec -faltivec -mcpu=G5 -mtune=G5 -mpowerpc-gfxopt -g

# optimize for G4
#	OPTIM = -O6 -ftree-vectorize -ftree-vectorizer-verbose=0 -Wall -Wno-long-double -fno-exceptions -ffast-math -pipe -fsigned-char -maltivec -mabi=altivec -faltivec -mcpu=G4 -mtune=G4 -mpowerpc-gfxopt -g

# optimize for intel mac
	OPTIM = -O6 -Wall -Wno-long-double -fno-exceptions -ffast-math -fsigned-char -g

else

#	OPTIM = -O0 $(DEBUG)
	OPTIM = -O6 -Wall -Wunused -ffast-math -fno-exceptions -g  -Wno-deprecated

endif

CFLAGS		= $(OPTIM) 
CXXFLAGS	= $(OPTIM) -DGL_GLEXT_PROTOTYPES


#CFLAGS		= $(DEBUG)
#CXXFLAGS	= $(DEBUG) 

# libraries to link with:
ifeq ($(platform),Darwin)
	INCPATH = -I/sw/include -I/usr/local/include
	LIBPATH	= -L/usr/local/lib -L/sw/lib
#	LDLIBS = -framework AGL -framework OpenGL -framework Carbon -framework ApplicationServices -framework vecLib -lm -lmx -lgsl
	LDLIBS = -framework AGL -framework OpenGL -framework Carbon -framework ApplicationServices -framework vecLib -lm -lmx /sw/lib/libgsl.a

else
	INCPATH = -I/u/wk/creon/include
	LIBPATH	= -L/u/wk/creon/lib 
	LDLIBS = -lGL -L/usr/X11R6/lib -lXext -lm -lgsl 
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
	$(MAKEDEPEND) $(INCBLITZ) $(INCPATH) $(INCFLEWS) -o makedepend $(SRCS)

# Automatically generated dependencies if they are there...
-include makedepend

# there has to be a better way....
tags:	TAGS
TAGS:	depend $(SRCS)
	etags -o TAGS $(SRCS) `cat makedepend | sed -e"s/.*://g; s/\\\\\//"`

stable.h.gch:	stable.h
	echo pre-compiling $<...
	$(CXX) $(INCPATH) $(INCFLEWS) -I.. $(CXXFLAGS) -c $<

