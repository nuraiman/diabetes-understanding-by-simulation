project(dubs)


#-----------------------------------------------------------------------------
# CMake administrivia
cmake_minimum_required( VERSION 2.8 )
if ( COMMAND cmake_policy )
  cmake_policy( SET CMP0003 NEW )
endif( COMMAND cmake_policy )

#option( BUILD_SHARED_LIBS "Set to ON to build shared libraries" OFF )
#set( LIBRARY_OUTPUT_PATH    bin CACHE PATH "Path to libraries"   FORCE )

set( EXECUTABLE_OUTPUT_PATH bin CACHE PATH "Path to executables" FORCE )
set( UPDATE_TYPE "svn" )
set( CMAKE_COLOR_MAKEFILE ON CACHE BOOL "Color" FORCE )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBOOST_FILESYSTEM_VERSION=3" )


option( TRY_TO_STATIC_LINK "Try to link to static libs whenever possible" OFF )
option( BUILD_AS_DESKTOP_APP "Build as a standalone dektop type application" OFF )


IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Debug CACHE STRING
        "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)


#-----------------------------------------------------------------------------

if( BUILD_AS_DESKTOP_APP )
    SET(EXECNAME dubs)
else( BUILD_AS_DESKTOP_APP )
    SET(EXECNAME dubs.wt)
endif( BUILD_AS_DESKTOP_APP )


#try to link to static libraries whenever possible
#  doing this to boost libraries only adds about 0.6 megabytes
if(TRY_TO_STATIC_LINK)
    set(Boost_USE_STATIC_LIBS ON)
    IF(WIN32)
        SET(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
    ELSE(WIN32)
        SET(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
    ENDIF(WIN32)
else(TRY_TO_STATIC_LINK)
    set(Boost_USE_STATIC_LIBS OFF)
    SET(CMAKE_FIND_LIBRARY_SUFFIXES .so .dylib ${CMAKE_FIND_LIBRARY_SUFFIXES})
endif(TRY_TO_STATIC_LINK)


include(FindWt.cmake)
#FIND_PACKAGE( Wt REQUIRED )

SET( GSL_FIND_QUIETLY ON )
include(FindGSL.cmake)
#FIND_PACKAGE( GSL REQUIRED )


SET( Boost_ADDITIONAL_VERSIONS "1.41" "1.42" "1.43" "1.44.0" "1.44" "1.45" "1.46" "1.48" "1.49" )

#boost timer and chrono libs are only needed for development purposes
FIND_PACKAGE( Boost 1.41.0 COMPONENTS thread date_time system signals filesystem serialization timer chrono program_options regex )




set( sources
    main.cc
    src/ArtificialPancrease.cc
    src/ConsentrationGraph.cc
    src/ProgramOptions.cc
    src/RungeKuttaIntegrater.cc
    src/WtCreateNLSimple.cc
    src/WtUserManagment.cc
    src/CgmsDataImport.cc
    src/KineticModels.cc
    src/ResponseModel.cc
    src/WtChartClasses.cc
    src/WtGui.cc
    src/WtUtils.cc
    src/DubUser.cc
    src/DubsSession.cc
    src/DubsApplication.cc
    src/OverlayCanvas.cc
)


set( headers
    dubs/ArtificialPancrease.hh
    dubs/CgmsDataImport.hh
    dubs/ConsentrationGraph.hh
    dubs/KineticModels.hh
    dubs/ProgramOptions.hh
    dubs/ResponseModel.hh
    dubs/RungeKuttaIntegrater.hh
    dubs/WtChartClasses.hh
    dubs/WtCreateNLSimple.hh
    dubs/WtGui.hh
    dubs/WtUserManagment.hh
    dubs/WtUtils.hh
    dubs/DubUser.hh
    dubs/DubsSession.hh
    dubs/DubsApplication.hh
    dubs/OverlayCanvas.hh
)



#to build a dektop app we will use Qt's QWebView Widget, so we need to set up
# things for Qt and link to the appropriate Qt libraries
#if(BUILD_AS_DESKTOP_APP)
#    FIND_PACKAGE(Qt4 REQUIRED)
#    ADD_DEFINITIONS(${QT_DEFINITIONS})
#    ADD_DEFINITIONS(-DQT_NO_KEYWORDS)
#    SET(QT_USE_QTWEBKIT TRUE) #QT_USE_QTNETWORK QT_USE_QTOPENGL QT_USE_QTSQL QT_USE_QTXML QT_USE_QTSVG QT_USE_QTTEST QT_USE_QTDBUS QT_USE_QTSCRIPT QT_USE_QTWEBKIT QT_USE_QTXMLPATTERNS QT_USE_PHONON
#    set( sources src/SpectrumViewerApp.cpp ${sources} )
#    set( headers wt_histogram/SpectrumViewerApp.h ${headers} )
#    SET(QT_HEADERS html5applicationviewer/ThreadForDesktopServing.h html5applicationviewer/html5applicationviewer.h)
#    SET(QT_SOURCES html5applicationviewer/ThreadForDesktopServing.cpp html5applicationviewer/html5applicationviewer.cpp)
#    QT4_WRAP_CPP( desktop_MOC_SRCS ${QT_HEADERS} )
#    INCLUDE(${QT_USE_FILE})
#    set( sources ${sources} ${desktop_MOC_SRCS} ${QT_SOURCES} )
#    set( headers ${headers} ${QT_HEADERS} )
#    add_executable( ${EXECNAME} main.cpp ${sources} ${headers} )
#endif(BUILD_AS_DESKTOP_APP)

#add_library( ${EXECNAME} STATIC ${sources} ${headers} )
add_executable( ${EXECNAME} main.cc ${sources} ${headers} )


include(FindROOT.cmake)
#FIND_PACKAGE( ROOT REQUIRED )
INCLUDE_DIRECTORIES( ${ROOT_INCLUDE_DIR} )

EXEC_PROGRAM(${ROOT_CONFIG} ARGS "--cflags" OUTPUT_VARIABLE ROOT_CXX_FLAGS)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ROOT_CXX_FLAGS}")
target_link_libraries( ${EXECNAME} ${ROOT_LIBRARIES} TMVA Minuit Minuit2 )

if(BUILD_AS_DESKTOP_APP)
    TARGET_LINK_LIBRARIES(${EXECNAME} ${QT_LIBRARIES})
#    TARGET_LINK_LIBRARIES(${CCT_NAME} ${QT_LIBRARIES})
endif(BUILD_AS_DESKTOP_APP)


INCLUDE_DIRECTORIES(
    ${Wt_INCLUDE_DIR}
    ${Wt_INCLUDE_DIR}/..  #necassary when including Wt/Auth/Dbo/AuthInfo (I think is a Wt bug)
    ${Boost_INCLUDE_DIR}
    ${CMAKE_BINARY_DIR}
    ${GSL_INCLUDE_DIRS}
    dubs
    .
    ..
)



target_link_libraries (
    ${EXECNAME}
    ${Wt_LIBRARIES}
    ${Boost_LIBRARIES}
#    ${Boost_SYSTEM_LIBRARY}
#    ${Boost_REGEX_LIBRARY}
#    ${Boost_THREAD_LIBRARY}
#    ${Boost_FILESYSTEM_LIBRARY}
#    ${Boost_DATE_TIME_LIBRARY}
#    ${Boost_SIGNALS_LIBRARY}
#    ${Boost_SERIALIZATION_LIBRARY}
#    ${Boost_PROGRAM_OPTIONS}
    ${GSL_LIBRARIES}
    -lfftw3
)




#When I link statically to the Wt libraries on OSX, I also have to
#  link to the following libraries
#target_link_libraries (
#     ${EXECNAME}
#     /opt/local/lib/libGraphicsMagick.a
#     /opt/local/lib/libtiff.a
#     /opt/local/lib/libpng.a
#     /opt/local/lib/libjpeg.a
#     /opt/local/lib/libfreetype.a
#     /opt/local/lib/libbz2.a
#     /opt/local/lib/libxml2.a
#     /opt/local/lib/libmng.a
#     /opt/local/lib/libltdl.a
#     /opt/local/lib/liblcms2.a
#     /opt/local/lib/libiconv.a
#     /opt/local/lib/libssl.a
#     /opt/local/lib/libcrypto.a
#     /opt/local/lib/libz.a
#     /opt/local/lib/libpango-1.0.a
#     /opt/local/lib/libpangoft2-1.0.a
#     /opt/local/lib/libglib-2.0.a
#     /opt/local/lib/libgobject-2.0.a
#     /opt/local/lib/libgio-2.0.a
#     /opt/local/lib/libgthread-2.0.a
#     /opt/local/lib/libffi.a
#     /opt/local/lib/libintl.a
#     /opt/local/lib/libpoppler.a
#     /opt/local/lib/libgd.a
#     /opt/local/lib/libgconf-2.a
#     /opt/local/lib/libgmodule-2.0.a
#     /opt/local/lib/libfontconfig.a
#     /opt/local/lib/libexpat.a
#     /System/Library/Frameworks/Carbon.framework/Versions/A/Carbon
#     /System/Library/Frameworks/CoreServices.framework/Versions/A/CoreServices
#     /System/Library/Frameworks/CoreFoundation.framework/Versions/A/CoreFoundation
#)


if (${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION} VERSION_GREATER 1.46)
    target_link_libraries (${EXECNAME} ${Boost_TIMER_LIBRARY} ${Boost_CHRONO_LIBRARY} )
#    target_link_libraries ( ${CCT_NAME} ${Boost_TIMER_LIBRARY} ${Boost_CHRONO_LIBRARY} )
endif ()



#configure_file(
#    ${CMAKE_CURRENT_SOURCE_DIR}/wt_histogram/wt_histogram_config.h.in
#    ${CMAKE_BINARY_DIR}/wt_histogram_config.h
#)




#Copy files to the install directory
set(OTHER_FILES
#  wt_config.xml
  runlocal8080.sh
  )

foreach(_file ${OTHER_FILES})
  configure_file(
    ${PROJECT_SOURCE_DIR}/${_file}
    ${PROJECT_BINARY_DIR}/${_file}
    COPYONLY
    )
endforeach()



