# Are we compiling on Darwin (mac OSX) or Linux ?
platform := $(shell uname)

# Are we compiling on intel or powerPC ?
hardware := $(shell uname -m)

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


CXXFLAGS_ADD	= 
LDFLAGS_ADD	=

# compiling on Apple OSX (darwin)
ifeq ($(platform),Darwin)

  # compiling on intel mac
  ifeq ($(hardware),i386)
	CXXFLAGS_ADD = -arch i386 -isysroot /Developer/SDKs/MacOSX10.6.sdk
	LDFLAGS_ADD = -arch i386 -isysroot /Developer/SDKs/MacOSX10.6.sdk
#	OPTIM = $(DEBUG) $(CXXFLAGS_ADD)
	OPTIM = -O6 -ftree-vectorize -ftree-vectorizer-verbose=0 -Wall -Wconversion -fno-strict-aliasing -ffast-math -fsigned-char -gfull $(CXXFLAGS_ADD)

  # compiling on PowerPC mac
  else
#	OPTIM = -O6 -ftree-vectorize -ftree-vectorizer-verbose=0 -Wall -Wconversion -Wno-long-double -ffast-math -fsigned-char -maltivec -mabi=altivec -faltivec -mpowerpc-gfxopt -gfull
	OPTIM = -O6 -ftree-vectorize -ftree-vectorizer-verbose=0 -Wall -Wconversion -ffast-math -fsigned-char -maltivec -mabi=altivec -faltivec -mpowerpc-gfxopt -gfull

  endif

else
# compiling on linux (assume intel HW)

#	OPTIM = -O0 $(DEBUG) -DGL_GLEXT_PROTOTYPES
	OPTIM = -O6 -ffast-math -g -DGL_GLEXT_PROTOTYPES

endif

# If svnversion causes trouble, use -D SVN_VERSION="\"local\""
CXXFLAGS	= $(OPTIM) -D SVN_VERSION="\"revision $(shell svnversion -n)\""
#CXXFLAGS	= $(DEBUG) -D SVN_VERSION="\"revision $(shell svnversion -n)\""

# libraries to link with:
ifeq ($(platform),Darwin)

	LDLIBS = -framework Foundation -framework AGL -framework OpenGL -framework Carbon -framework Cocoa -framework ApplicationServices -framework vecLib -framework AudioToolbox -lgsl -lm -lmx -lboost_serialization-xgcc40-mt -lcfitsio

# for OSX machines where I CAN install things as root... (don't forget to build all libraries as static only)
	INCPATH = -I/usr/local/include -I/sw2/include -I/usr/local/include/boost-1_38/
	LIBPATH	= -L/usr/local/lib -L/sw2/lib

else
# for NAS linux machines where I can NOT install things as root (don't forget to build all libraries as static only)
	INCPATH = -I$$HOME/include -I$$HOME/include/boost-1_34
	LIBPATH	= -L$$HOME/lib -L/usr/X11R6/lib
	LDLIBS = -lGL -lGLU -lXft -lXext -lm -lgsl -lboost_serialization-gcc34
# for debugging
#	LDLIBS = -lGLU -lGL -lXext -lm -lgsl -lefence -lpthread  
endif

INCFLEWS	= -I../flews-0.3.1

LINKFLEWS	= -L../flews-0.3.1 -lflews
LINKFLTK	= -lfltk -lfltk_gl
LINKBLITZ	= -lblitz

LDFLAGS		= $(LIBPATH) $(LINKFLEWS) $(LINKFLTK) $(LINKBLITZ) $(LDLIBS) $(LDFLAGS_ADD)

# The extension to use for executables...
EXEEXT		= 

SRCS =	vp.cpp global_definitions_vp.cpp control_panel_window.cpp plot_window.cpp data_file_manager.cpp Vp_File_Chooser.cpp \
	symbol_menu.cpp sprite_textures.cpp unescape.cpp brush.cpp Vp_Color_Chooser.cpp column_info.cpp

OBJS:=	$(SRCS:.cpp=.o)

TARGET = vp$(EXEEXT)

DOCUMENTATION = README vp_help_manual.htm sampledata.txt 

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
	$(CXX) -v $(CXXFLAGS) $(OBJS) $(LDFLAGS) -o $@
	$(POSTBUILD)

clean:
	rm -f $(ALL) *.o $(TARGET) vp core* TAGS *.gch makedepend 

depend:	$(SRCS)
	$(MAKEDEPEND) $(INCBLITZ) $(INCPATH) $(INCFLEWS) $(SRCS) > makedepend

#after "make release", don't forget to copy the tarball to the server, i.e:
#e.g.  "scp vp.tar trotsky.arc.nasa.gov:WWW/public/vp/releases/ppc_mac"
release: $(TARGET)
	if test -e /tmp/vp; then \
		/bin/rm -rf /tmp/vp ; \
	fi
	mkdir /tmp/vp
	cp -r $(DOCUMENTATION) /tmp/vp
ifeq ($(platform),Darwin)
	cp -r viewpoints.app /tmp/vp
else
	cp -r $(TARGET) /tmp/vp
endif
	tar -cvzf vp.tar.z --directory /tmp vp

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
