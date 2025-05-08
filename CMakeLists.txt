project(DisplayCluster)

cmake_minimum_required(VERSION 2.4.0)

# version information
set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
set(VERSION_PATCH 0)

# required for mixing libraries with full paths and not...
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

# show all warnings
# set(CMAKE_CXX_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

# keep the library path in installed executables
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# build options

option(INSTALL_CLIENT "Install client-side bits package" OFF)
if (INSTALL_CLIENT)
    INSTALL(PROGRAMS scripts/startdisplaycluster DESTINATION bin)
    INSTALL(PROGRAMS python/DC.py python/Bounce.ipynb DESTINATION python)
    INSTALL(PROGRAMS examples/run_script.py examples/script examples/configuration.xml DESTINATION examples)
endif()

option(ENABLE_TUIO_TOUCH_LISTENER "Enable TUIO touch listener for multi-touch events" OFF)
option(ENABLE_JOYSTICK_SUPPORT "Enable joystick support" OFF)
option(ENABLE_SKELETON_SUPPORT "EXPERIMENTAL: Enable skeleton tracking interface support" OFF)

# path for additional modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

# find and setup Qt4
# see http://cmake.org/cmake/help/cmake2.6docs.html#module:FindQt4 for details
set(QT5 0)
set(QT_USE_QTOPENGL TRUE)
set(QT_USE_QTSVG TRUE)
set(QT_USE_QTXMLPATTERNS TRUE)
set(QT_USE_QTXML TRUE)
set(QT_USE_QTNETWORK TRUE)

if(WIN32)
    set(QT_USE_QTMAIN TRUE)
endif()

find_package(Qt4 REQUIRED)

include(${QT_USE_FILE})
set(LIBS ${LIBS} ${QT_LIBRARIES})

# libjpeg-turbo
find_package(LibJpegTurbo REQUIRED)
include_directories(${LibJpegTurbo_INCLUDE_DIRS})
set(LIBS ${LIBS} ${LibJpegTurbo_LIBRARIES})

if (INSTALL_LAUNCHER)
    INSTALL(PROGRAMS scripts/startdisplaycluster DESTINATION bin)
endif()

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})
set(LIBS ${LIBS} ${OPENGL_LIBRARIES})

find_package(Boost REQUIRED COMPONENTS serialization date_time)
include_directories(${Boost_INCLUDE_DIRS})
set(LIBS ${LIBS} ${Boost_LIBRARIES})

find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})
set(LIBS ${LIBS} ${MPI_LIBRARIES})

find_package(FFMPEG REQUIRED)
include_directories(SYSTEM ${FFMPEG_INCLUDE_DIR}) # use SYSTEM to suppress FFMPEG compile warnings
set(LIBS ${LIBS} ${FFMPEG_LIBRARIES})

# handle build options
if(ENABLE_TUIO_TOUCH_LISTENER)
find_package(TUIO REQUIRED)
include_directories(${TUIO_INCLUDE_DIRS})
set(LIBS ${LIBS} ${TUIO_LIBRARIES})

set(SRCS ${SRCS} src/TouchListener.cpp)

find_package(X11 REQUIRED)
include_directories(${X11_INCLUDE_DIR})
set(LIBS ${LIBS} ${X11_LIBRARIES})
endif()

if(ENABLE_JOYSTICK_SUPPORT)
find_package(SDL REQUIRED)
include_directories(${SDL_INCLUDE_DIR})
set(LIBS ${LIBS} ${SDL_LIBRARY})

set(SRCS ${SRCS} src/JoystickThread.cpp)
set(SRCS ${SRCS} src/DisplayGroupJoystick.cpp)

set(MOC_HEADERS ${MOC_HEADERS} src/JoystickThread.h)
endif()

if(ENABLE_SKELETON_SUPPORT)
    find_package(OpenNI REQUIRED)
    include_directories(SYSTEM ${OpenNI_INCLUDE_DIRS}) # use SYSTEM to suppress OpenNI compile warnings
    set(LIBS ${LIBS} ${OpenNI_LIBRARIES})

    set(SRCS ${SRCS}
        src/SkeletonThread.cpp
        src/SkeletonSensor.cpp
        src/SkeletonState.cpp)

    if(NOT ENABLE_JOYSTICK_SUPPORT)
        set(SRCS ${SRCS} src/DisplayGroupJoystick.cpp)
    endif()

    set(MOC_HEADERS ${MOC_HEADERS} src/SkeletonThread.h)
endif()

# process config file
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(SRCS ${SRCS}
    src/Configuration.cpp
    src/Content.cpp
    src/ContentWindowManager.cpp
    src/ContentWindowInterface.cpp
    src/ContentWindowGraphicsItem.cpp
    src/ContentWindowListWidgetItem.cpp
    src/Marker.cpp
    src/DCSocketManager.cpp
    src/Decoder.cpp
    src/DisplayGroupManager.cpp
    src/DisplayGroupInterface.cpp
    src/DisplayGroupGraphicsScene.cpp
    src/DisplayGroupGraphicsViewProxy.cpp
    src/DisplayGroupGraphicsView.cpp
    src/DisplayGroupListWidgetProxy.cpp
    src/DynamicTexture.cpp
    src/DynamicTextureContent.cpp
    src/FactoryObject.cpp
    src/GLWindow.cpp
    src/log.cpp
    src/main.cpp
    src/MainWindow.cpp
    src/Movie.cpp
    src/MovieContent.cpp
    src/NetworkListener.cpp
    src/NetworkListenerThread.cpp
    src/Options.cpp
    src/ParallelPixelStream.cpp
    src/ParallelPixelStreamContent.cpp
    src/PixelStream.cpp
    src/PixelStreamContent.cpp
    src/PixelStreamSource.cpp
    src/QSSApp.cpp
    src/SocketInterface.cpp
    src/SVG.cpp
    src/SVGContent.cpp
    src/SVGStreamSource.cpp
    src/Texture.cpp
    src/TextureContent.cpp
)

set(MOC_HEADERS ${MOC_HEADERS}
    src/Content.h
    src/ContentWindowInterface.h
    src/DCSocketManager.h
    src/DisplayGroupManager.h
    src/DisplayGroupInterface.h
    src/DisplayGroupGraphicsViewProxy.h
    src/DisplayGroupListWidgetProxy.h
    src/MainWindow.h
    src/Marker.h
    src/NetworkListener.h
    src/NetworkListenerThread.h
    src/Options.h
    src/QSSApp.h
)


# add OPTIONS hacks for Qt's MOC processing of Boost headers, for compiler errors with some versions of Boost
qt4_wrap_cpp(MOC_OUTFILES ${MOC_HEADERS} OPTIONS -DPOSIX_TIME_HPP___ -DPOSIX_TIME_SERIALIZE_HPP___ -DBOOST_TT_HAS_OPERATOR_HPP_INCLUDED)

add_executable(displaycluster ${SRCS} ${MOC_OUTFILES})

target_link_libraries(displaycluster ${LIBS} pthread)

# install executable
INSTALL(TARGETS displaycluster RUNTIME DESTINATION bin)

# install launchers
INSTALL(PROGRAMS scripts/displaycluster.py DESTINATION bin)
INSTALL(PROGRAMS scripts/displaycluster.sh DESTINATION bin)

# install remote controller
INSTALL(DIRECTORY remote DESTINATION .)

# install data files
INSTALL(DIRECTORY data DESTINATION .)

# install example files
INSTALL(DIRECTORY examples DESTINATION .)

# install doc files
INSTALL(DIRECTORY doc DESTINATION .)

# install doc files
INSTALL(DIRECTORY python DESTINATION .)
