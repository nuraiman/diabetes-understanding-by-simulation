# -------------------------------------------------
# Project created by QtCreator 2009-12-17T17:44:13
# -------------------------------------------------
TARGET = dubs
TEMPLATE = app
LANGUAGE = C++
QMAKE_RPATH = 
!exists ($$(ROOTSYS)/lib/libQtRootGui.$$QMAKE_EXTENSION_SHLIB):message ("No ROOT Qt Extension was found. Use Qt-layer instead")
CONFIG += qt \
    warn_on \
    thread \
    debug
QT += qt3support
SOURCES += main.cpp \
    nlsimpleguiwindow.cpp \
    ArtificialPancrease.cc \
    ConsentrationGraphGui.cc \
    ProgramOptions.cc \
    CgmsDataImport.cc \
    KineticModels.cc \
    ResponseModel.cc \
    ConsentrationGraph.cc \
    NLSimpleGui.cc \
    RungeKuttaIntegrater.cc \
    MiscGuiUtils.cc \
    nlsimple_create.cpp
HEADERS += nlsimpleguiwindow.h \
    ArtificialPancrease.hh \
    ConsentrationGraphGui.hh \
    ProgramOptions.hh \
    CgmsDataImport.hh \
    KineticModels.hh \
    ResponseModel.hh \
    ConsentrationGraph.hh \
    NLSimpleGui.hh \
    RungeKuttaIntegrater.hh \
    MiscGuiUtils.hh \
    nlsimple_create.h

# below assumes NLSimpleGui_linkdef containes all the '#pragma link C++ class ClassName;' statments in it
CREATE_ROOT_DICT_FOR_CLASSES = ConsentrationGraphGui.hh \
    NLSimpleGui.hh \
    ProgramOptions.hh \
    NLSimpleGui_linkdef.h
FORMS = nlsimpleguiwindow.ui \
    nlsimple_create.ui
includeDir = $$(QTROOTSYSDIR)/include
incFile = $$includeDir/rootcint.pri
LIBS += /usr/local/lib/libboost_date_time.a \
    /usr/local/lib/libboost_serialization.a \
    /usr/local/lib/libboost_program_options.a \
    /usr/local/lib/libgsl.a \
    /usr/local/lib/libgslcblas.a \
    -L$$(ROOTSYS) \
    -lGQt \
    -lMinuit \
    -lMLP \
    -lTreePlayer \
    -lMinuit2 \
    -lTMVA
exists ($$includeDir):exists ($$incFile):include ($$incFile)# Win32 wants us to check the directory existence separately
!exists ($$includeDir) { 
    incFile = $$(ROOTSYS)/include/rootcint.pri
    exists ($$incFile):include ($$incFile)
    !exists ($$incFile) { 
        message (" ")
        message ("WARNING: The $$inlcudeFile was not found !!!")
        message ("Please update your Qt layer version from http://root.bnl.gov ")
        message (" ")
        LIBS += $$system(root-config --glibs) \
            -lGQt \
            -lMinuit \
            -lMLP \
            -lTreePlayer \
            -lMinuit2 \
            -lTMVA
        INCLUDEPATH += $(ROOTSYS)/include
    }
}

# mac:QMAKE_INFO_PLIST=Info.plist
unix { 
    UI_DIR = .ui
    MOC_DIR = .moc
    OBJECTS_DIR = .obj
}
