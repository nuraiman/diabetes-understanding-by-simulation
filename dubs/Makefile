WORKDIR=./
EXECNAME=$(shell basename `pwd`).wt

ROOTCONFIG   := root-config
ObjDir        = obj
DictDir       = obj
RootVersion = RootV$(subst .,,$(subst /,,$(shell $(ROOTCONFIG) --version)))

BOOSTLIB=/usr/lib/ 
WTLIB=-L/usr/local/lib/  -lwt -lwtdbo -lwtfcgi -lwtdbosqlite3
#WTLIB=-L/usr/local/lib/  -lwt -lwtdbo -lwthttp -lwtdbosqlite3
WTINC=/usr/local/include/Wt/
SQLITE3LIB=/usr/lib/libsqlite.so

ARCH         := $(shell $(ROOTCONFIG) --arch)
PLATFORM     := $(shell $(ROOTCONFIG) --platform)
ALTCC        := $(shell $(ROOTCONFIG) --cc)
ALTCXX       := $(shell $(ROOTCONFIG) --cxx)
ALTF77       := $(shell $(ROOTCONFIG) --f77)
ALTLD        := $(shell $(ROOTCONFIG) --ld)


CXX           =
ObjSuf        = o
SrcSuf        = cxx
ExeSuf        =
DllSuf        = so
OutPutOpt     = -o # keep whitespace after "-o"

ifeq (debug,$(findstring debug,$(ROOTBUILD)))
OPT           = -g
OPT2          = -g
else
ifneq ($(findstring debug, $(strip $(shell $(ROOTCONFIG) --config))),)
OPT           = -g
OPT2          = -g
else
OPT           = -O
OPT2          = -O2
endif
endif

ROOTCFLAGS   := $(shell $(ROOTCONFIG) --cflags)
ROOTLDFLAGS  := $(shell $(ROOTCONFIG) --ldflags)
ROOTLIBS     := $(shell $(ROOTCONFIG) --libs) -lMinuit -lMinuit2 -lTreePlayer -lTMVA
ROOTGLIBS    := $(shell $(ROOTCONFIG) --glibs)  -lMinuit -lMinuit2 -lTreePlayer -lTMVA
HASTHREAD    := $(shell $(ROOTCONFIG) --has-thread)
ROOTDICTTYPE := $(shell $(ROOTCONFIG) --dicttype)
#NOSTUBS      := $(shell $(ROOTCONFIG) --nostubs)
ROOTCINT     := rootcint

# Stub Functions Generation
#ifeq ($(NOSTUBS),yes)
#   ROOTCINT = export CXXFLAGS="$(CXXFLAGS)"; $(ROOTSYS)/core/utils/src/rootcint_nostubs.sh -$(ROOTDICTTYPE)
#endif



ifeq ($(ARCH),linux)
# Linux with egcs, gcc 2.9x, gcc 3.x
CXX           = g++
CXXFLAGS      = $(OPT2) -Wall -fPIC
LD            = g++
LDFLAGS       = $(OPT2)
SOFLAGS       = -shared
BOOSTLIB      = /usr/lib/
SQLITE3LIB    = /usr/lib/libsqlite3.so
MYSQLLIB      =-L/usr/lib/mysql -lmysqlclient
WTLIB=       -L/usr/lib/  -lwt -lwtdbo -lwtfcgi
CXXFLAGS     += -DNDEBUG #if you use  DNDEBUG then you need to add this flag to all other libraries and recompile them
LDFLAGS      += -DNDEBUG
endif


ifeq ($(ARCH),linuxx8664gcc)
# AMD Opteron and Intel EM64T (64 bit mode) Linux with gcc 3.x
CXX           = g++
CXXFLAGS      = $(OPT2) -Wall -fPIC
LD            = g++
LDFLAGS       = $(OPT2)
SOFLAGS       = -shared
endif


ifeq ($(ARCH),macosx)
# MacOS X with cc (GNU cc 2.95.2 and gcc 3.3)
MACOSX_MINOR := $(shell sw_vers | sed -n 's/ProductVersion://p' | cut -d . -f 2)
MACOSXTARGET := MACOSX_DEPLOYMENT_TARGET=10.$(MACOSX_MINOR)
#CXX           = g++
CXX           = /usr/local/bin/clang++
CXXFLAGS      = $(OPT2) -pipe -Wall -W -Woverloaded-virtual
LD            = $(MACOSXTARGET) g++
LDFLAGS       = $(OPT2)
BOOSTLIB      = /usr/local/lib
SQLITE3LIB    = /usr/lib/libsqlite3.dylib
# The SOFLAGS will be used to create the .dylib,
# the .so will be created separately
ifeq ($(subst $(MACOSX_MINOR),,1234),1234)
DllSuf        = so
else
DllSuf        = dylib
endif
UNDEFOPT      = dynamic_lookup
ifneq ($(subst $(MACOSX_MINOR),,12),12)
UNDEFOPT      = suppress
LD            = g++
endif
SOFLAGS       = -dynamiclib -single_module -undefined $(UNDEFOPT) -install_name $(CURDIR)/
endif



ifeq ($(ARCH),macosx64)
# MacOS X >= 10.4 with gcc 64 bit mode (GNU gcc 4.*)
# Only specific option (-m64) comes from root-config
MACOSX_MINOR := $(shell sw_vers | sed -n 's/ProductVersion://p' | cut -d . -f 2)
MACOSXTARGET := MACOSX_DEPLOYMENT_TARGET=10.$(MACOSX_MINOR)
#CXX           = g++
CXX           = /usr/local/bin/clang++
CXXFLAGS      = $(OPT2) -pipe -Wall -W -Woverloaded-virtual
LD            = $(MACOSXTARGET) g++
LDFLAGS       = $(OPT2)
BOOSTLIB      = /usr/local/lib
SQLITE3LIB    = /usr/lib/libsqlite3.dylib
# The SOFLAGS will be used to create the .dylib,
# the .so will be created separately
ifeq ($(subst $(MACOSX_MINOR),,1234),1234)
DllSuf        = so
else
DllSuf        = dylib
endif
SOFLAGS       = -dynamiclib -single_module -undefined dynamic_lookup -install_name $(CURDIR)/
endif



ifeq ($(ARCH),win32)
# Windows with the VC++ compiler
ObjDir        = .
DictDir       = .
LD=/cygdrive/c/Program\ Files/Microsoft\ Visual\ Studio\ 9.0/VC/bin/link.exe
ALTLD=/cygdrive/c/Program\ Files/Microsoft\ Visual\ Studio\ 9.0/VC/bin/link.exe
VC_MAJOR     := $(shell unset VS_UNICODE_OUTPUT; cl.exe 2>&1 | awk '{ if (NR==1) print $$8 }' | \
                cut -d'.' -f1)
ObjSuf        = obj
DllSuf        = dll
OutPutOpt     = -out:
CXX           = cl
ifeq (debug,$(findstring debug,$(ROOTBUILD)))
CXXOPT        = -Z7
LDOPT         = -debug
else
ifneq ($(findstring debug, $(strip $(shell $(ROOTCONFIG) --config))),)
CXXOPT        = -Z7
LDOPT         = -debug
else
CXXOPT        = -O2
LDOPT         = -opt:ref
endif
endif
ROOTINCDIR   := -I$(shell cygpath -m `$(ROOTCONFIG) --incdir`)
CXXFLAGS      = $(CXXOPT) -nologo $(ROOTINCDIR) -FIw32pragma.h
#LD            = link
LDFLAGS       = $(LDOPT) -nologo
SOFLAGS       = -DLL

EXPLLINKLIBS  = $(ROOTLIBS) $(ROOTGLIBS)
ifneq (,$(findstring $(VC_MAJOR),14 15))
MT_EXE        = mt -nologo -manifest $@.manifest -outputresource:$@\;1; rm -f $@.manifest
MT_DLL        = mt -nologo -manifest $@.manifest -outputresource:$@\;2; rm -f $@.manifest
else
MT_EXE        =
MT_DLL        =
endif
endif

ifeq ($(ARCH),win32gcc)
# Windows with gcc
DllSuf        = dll
CXX           = g++
CXXFLAGS      = $(OPT) -pipe -Wall -Woverloaded-virtual -I/usr/X11R6/include
LD            = g++
LDFLAGS       = $(OPT) -Wl,--enable-auto-import \
                -Wl,--enable-runtime-pseudo-reloc \
		-L/usr/X11R6/lib
SOFLAGS       = -shared -Wl,--enable-auto-image-base \
                -Wl,--export-all-symbols
EXPLLINKLIBS  = $(ROOTLIBS) $(ROOTGLIBS)
endif


ifeq ($(CXX),)
$(error $(ARCH) invalid architecture - consult $(ROOTSYS)/test/Makefile.arch )
endif

ADDLIBS=$(BOOSTLIB)/libboost_system.a \
        $(BOOSTLIB)/libboost_signals.a \
        $(BOOSTLIB)/libboost_date_time.a \
        $(BOOSTLIB)/libboost_thread.a \
        $(WTLIB) \
        $(BOOSTLIB)/libboost_date_time.a \
        $(BOOSTLIB)/libboost_serialization.a \
        $(BOOSTLIB)/libboost_program_options.a \
        /usr/local/lib/libgsl.a \
        /usr/local/lib/libgslcblas.a \
        $(SQLITE3LIB) \
	/opt/local/lib/libfftw3.a

ADDINCS=-I$(WTINC) 



CXXFLAGS     += $(ROOTCFLAGS) 
LDFLAGS      += $(ROOTLDFLAGS)
LIBS          = $(ROOTGLIBS) $(SYSLIBS)


#ifneq ($(ALTCXX),)
#   CXX = $(ALTCXX)
#endif
#ifneq ($(ALTLD),)
#   LD  = $(ALTLD)
#endif



CXXFLAGS += $(INCS) $(ADDINCS)

LIBS =  $(ADDLIBS) $(ROOTLIBS)

SRCS =  $(wildcard *.cc)

DEPS =  $(patsubst %.cc, $(ObjDir)/%.d, $(wildcard *.cc))

OBJS =  $(patsubst %.cc,$(ObjDir)/%.$(ObjSuf), $(wildcard *.cc))

$(EXECNAME): $(OBJS) $(SrbReportsStaticLib) $(SrbReportsStaticLib) Makefile
	@echo "making stuff with $(OBJS) : $(SRCS)!"
	$(LD) $(LDFLAGS) $(OBJS) $(LIBS) -o $(EXECNAME)

$(ObjDir)/%.$(ObjSuf): %.cc  Makefile
	@echo "Generating object file for $< $@"
	$(CXX) $(CXXFLAGS) -c $(OutPutOpt)$@ $<

ifneq ($(ARCH),win32)
#Generate dependancies
$(ObjDir)/%.d:	%.cc
	@echo "Generating dependencies for $<"
	@set -e; $(CXX) -M $(CXXFLAGS) $< \
	| sed 's%\($*\)\.$(ObjSuf)[ :]*%\1.$(ObjSuf) $@ : %g' > $@; \
	[ -s $@ ] || rm -f $@
endif #ifneq ($(ARCH),win32)

#Make ROOT dictionsary code, and associated object files
#$(DictDir)/dict_%.$(ObjSuf): %.h
#	@echo .
#	@echo "Generating dictionary for $< $@"
#	$(ROOTCINT) -f $(patsubst %.$(ObjSuf), %.cc, $(DictDir)/$(notdir $@)) -c -I$(DictDir) $(INCFLAGS) $(notdir $^)+
#	$(CXX) -c $(CXXFLAGS) $(OutPutOpt)$(ObjDir)/$(notdir $@) $(patsubst %.$(ObjSuf), %.cc, $(DictDir)/$(notdir $@))


echo:
	@echo For Debugging:
	@echo .
	@echo SRCS: $(SRCS)
	@echo .
	@echo INCLUDES: $(INCS)
	@echo .
	@echo OBJECTS: $(OBJS)
	@echo .
	@echo DEPS: $(DEPS)
	@echo .
	@echo CXXFLAGS: $(CFLAGS)

clean:
	@rm -f $(ObjDir)/*.d $(ObjDir)/*.o core* $(EXECNAME)

ifneq ($(ARCH),win32)
include $(DEPS)
endif







