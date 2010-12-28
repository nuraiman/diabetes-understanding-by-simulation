# -------------------------------------------------
# Project created by QtCreator 2009-12-17T17:44:13
# -------------------------------------------------
TARGET = wtdubs
TEMPLATE = app
LANGUAGE = C++
QT += xml opengl svg
unix: QMAKE_RPATH =

QMAKE_MAKEFILE = Makefile.qt

CONFIG += qt \
    warn_on \
    thread \
    debug \
    console

SOURCES += main.cc \
    ArtificialPancrease.cc \
    ProgramOptions.cc \
    CgmsDataImport.cc \
    KineticModels.cc \
    ResponseModel.cc \
    ConsentrationGraph.cc \
    RungeKuttaIntegrater.cc \
    WtGui.cc \
    WtCreateNLSimple.cc \
    WtUserManagment.cc \
    WtUtils.cc
HEADERS += nlsimpleguiwindow.h \
    ArtificialPancrease.hh \
    ProgramOptions.hh \
    CgmsDataImport.hh \
    KineticModels.hh \
    ResponseModel.hh \
    ConsentrationGraph.hh \
    RungeKuttaIntegrater.hh \
    WtGui.hh \
    WtCreateNLSimple.hh \
    WtUserManagment.hh \
    WtUtils.hh

# below assumes NLSimpleGui_linkdef containes all the '#pragma link C++ class ClassName;' statments in it
CREATE_ROOT_DICT_FOR_CLASSES = ConsentrationGraphGui.hh \
    NLSimpleGui.hh \
    ProgramOptions.hh \
    NLSimpleGui_linkdef.h

FORMS += nlsimpleguiwindow.ui \
    nlsimple_create.ui \
    DataInputGui.ui \
    ProgramOptionsGui.ui \
    CustomEventDefineGui.ui




LIBS += /usr/local/lib/libboost_date_time.a \
    /usr/local/lib/libboost_serialization.a \
    /usr/local/lib/libboost_program_options.a \
    /usr/local/lib/libboost_system.a \
    /usr/local/lib/libboost_signals.a  \
    /usr/local/lib/libboost_date_time.a  \
    /usr/local/lib/libboost_thread.a  \
    /usr/local/lib/libgsl.a \
    /usr/local/lib/libgslcblas.a \
    -L$$(ROOTSYS) \
    -lMinuit \
    -lMinuit2 \
    -lTMVA \
    -lTreePlayer \
    -L/usr/local/lib/  -lwt -lwtdbo -lwthttp

INCLUDEPATH += /usr/local/include \
               $$(ROOTSYS)/include

#mac:QMAKE_INFO_PLIST=Info.plist
#unix {
#    UI_DIR = .ui
#    MOC_DIR = .moc
#    OBJECTS_DIR = .obj
#}
