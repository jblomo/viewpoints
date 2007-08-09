# Are we compiling on Darwin (mac OSX) or Linux ?
platform := $(shell uname)

# Are we compiling on intel or powerPC ?
hardware := $(shell uname -m)

# compiler names:
CXX		= g++
MAKEDEPEND	= $(CXX) -E -MM

ifeq ($(platform),Darwin)
	POSTBUILD = make-OSX-application.csh vp viewpoints
else
	POSTBUILD = echo
endif

# flags for C++ compiler:
# PROFILE		= -pg
#DEBUG		= -O0 -ggdb -g3 -Wall -Wunused -DBZ_DEBUG -fexceptions
#DEBUG		= -g -ggdb -g3 -Wall -Wunused -fexceptions
DEBUG		= -gfull -ggdb -Wall -Wunused -Wconversion -fexceptions 


# compiling on Apple OSX (darwin)
ifeq ($(platform),Darwin)

  # compiling on intel mac
  ifeq ($(hardware),i386)
	OPTIM = -O6 -ftree-vectorize -ftree-vectorizer-verbose=0 -Wall -Wconversion -Wno-long-double -ffast-math -fsigned-char -gfull 

  # compiling on PowerPC mac
  else
	OPTIM = -O6 -ftree-vectorize -ftree-vectorizer-verbose=0 -Wall -Wconversion -Wno-long-double -ffast-math -fsigned-char -maltivec -mabi=altivec -faltivec -mpowerpc-gfxopt -gfull

  endif

else
# compiling on linux (assume intel HW)

#	OPTIM = -O0 $(DEBUG)
	OPTIM = -O6 -Wextra -ffast-math -fno-exceptions -g

endif

#CXXFLAGS	= $(OPTIM) -DGL_GLEXT_PROTOTYPES
CXXFLAGS	= $(DEBUG) 

# libraries to link with:
ifeq ($(platform),Darwin)

	LDLIBS = -framework AGL -framework OpenGL -framework Carbon -framework ApplicationServices -framework vecLib -lm -lmx -lgsl -lboost_serialization-d

# uncomment for OSX machines where I CAN install things as root... (don't forget to build all libraries as static only)
#	INCPATH = -I/sw/include -I/usr/local/include -I/usr/local/include/boost-1_34/
#	LIBPATH	= -L/usr/local/lib -L/sw/lib

# uncomment for OSX machines where I can NOT install things as root... (don't forget to build all libraries as static only)
	INCPATH = -I/Users/creon/include -I/Users/creon/include/boost-1_34/
	LIBPATH	= -L/Users/creon/lib 

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

SRCS =	vp.cpp global_definitions_vp.cpp control_panel_window.cpp plot_window.cpp data_file_manager.cpp New_File_Chooser.cpp \
	symbol_menu.cpp sprite_textures.cpp unescape.cpp brush.cpp

OBJS:=	$(SRCS:.cpp=.o)

TARGET = vp$(EXEEXT)

DOCUMENTATION = README vp_help_manual.htm sampledata.txt sample.desc.txt

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
	$(POSTBUILD)

clean:
	rm -f $(ALL) *.o $(TARGET) vp core* TAGS *.gch makedepend 

depend:	$(SRCS)
	$(MAKEDEPEND) $(INCBLITZ) $(INCPATH) $(INCFLEWS) $(SRCS) > makedepend

release: $(TARGET)
	if test -e /tmp/vp; then \
		/bin/rm -rf /tmp/vp ; \
	fi
	mkdir /tmp/vp
	cp -r $(DOCUMENTATION) /tmp/vp
ifeq ($(platform),Darwin)
	cp -r viewpoints.app /tmp/vp
else
	cp -r $TARGET /tmp/vp
endif
	tar -cvzf vp.tar --directory /tmp vp

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
