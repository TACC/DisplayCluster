project(DisplayCluster)

cmake_minimum_required(VERSION 2.4.0)

# required for mixing libraries with full paths and not...
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

# show all warnings
set(CMAKE_CXX_FLAGS "-Wall")

# build options
option(ENABLE_TUIO_TOUCH_LISTENER "Enable TUIO touch listener for multi-touch events" OFF)
option(ENABLE_JOYSTICK_SUPPORT "Enable joystick support" OFF)
option(ENABLE_PYTHON_SUPPORT "Enable Python support" OFF)

# path for additional modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

# find and setup Qt4
# see http://cmake.org/cmake/help/cmake2.6docs.html#module:FindQt4 for details
find_package(Qt4 REQUIRED) 
set(QT_USE_QTOPENGL TRUE)
set(QT_USE_QTXMLPATTERNS TRUE)
include(${QT_USE_FILE})
set(LIBS ${LIBS} ${QT_LIBRARIES})

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})
set(LIBS ${LIBS} ${OPENGL_LIBRARIES})

find_package(GLUT REQUIRED)
include_directories(${GLUT_INCLUDE_DIRS})
set(LIBS ${LIBS} ${GLUT_LIBRARIES})

find_package(Boost REQUIRED COMPONENTS serialization date_time)
include_directories(${Boost_INCLUDE_DIRS})
set(LIBS ${LIBS} ${Boost_LIBRARIES})

find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})
set(LIBS ${LIBS} ${MPI_LIBRARIES})

find_package(FFMPEG REQUIRED)
include_directories(${FFMPEG_INCLUDE_DIR})
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

if(ENABLE_PYTHON_SUPPORT)
    find_package(PythonLibs REQUIRED)
    include_directories(${PYTHON_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${PYTHON_LIBRARIES})

    find_package(PythonQt REQUIRED)
    include_directories(${PythonQt_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${PythonQt_LIBRARIES})

    set(SRCS ${SRCS} src/DisplayGroupPython.cpp)
    set(SRCS ${SRCS} src/PythonConsole.cpp)

    set(MOC_HEADERS ${MOC_HEADERS} src/DisplayGroupPython.h)
    set(MOC_HEADERS ${MOC_HEADERS} src/PythonConsole.h)
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
    src/Marker.cpp
    src/DisplayGroupManager.cpp
    src/DisplayGroupInterface.cpp
    src/DisplayGroupGraphicsScene.cpp
    src/DisplayGroupGraphicsViewProxy.cpp
    src/DisplayGroupGraphicsView.cpp
    src/DisplayGroupListWidgetProxy.cpp
    src/DynamicTexture.cpp
    src/DynamicTextureContent.cpp
    src/GLWindow.cpp
    src/log.cpp
    src/main.cpp
    src/MainWindow.cpp
    src/Movie.cpp
    src/MovieContent.cpp
    src/NetworkListener.cpp
    src/NetworkListenerThread.cpp
    src/Options.cpp
    src/PixelStream.cpp
    src/PixelStreamContent.cpp
    src/PixelStreamSource.cpp
    src/Texture.cpp
    src/TextureContent.cpp
)

set(MOC_HEADERS ${MOC_HEADERS}
    src/Content.h
    src/ContentWindowInterface.h
    src/DisplayGroupManager.h
    src/DisplayGroupInterface.h
    src/MainWindow.h
    src/Marker.h
    src/NetworkListener.h
    src/NetworkListenerThread.h
    src/Options.h
)

qt4_wrap_cpp(MOC_OUTFILES ${MOC_HEADERS})

add_executable(displaycluster ${SRCS} ${MOC_OUTFILES})

target_link_libraries(displaycluster ${LIBS})


# DesktopStreamer app

set(DESKTOP_STREAMER_LIBS ${QT_LIBRARIES})

# libjpeg-turbo
find_package(LibJpegTurbo REQUIRED)
include_directories(${LibJpegTurbo_INCLUDE_DIRS})
set(DESKTOP_STREAMER_LIBS ${DESKTOP_STREAMER_LIBS} ${LibJpegTurbo_LIBRARIES})

set(DESKTOP_STREAMER_SRCS ${DESKTOP_STREAMER_SRCS}
    src/log.cpp
    apps/DesktopStreamer/src/DesktopSelectionRectangle.cpp
    apps/DesktopStreamer/src/DesktopSelectionWindow.cpp
    apps/DesktopStreamer/src/DesktopSelectionView.cpp
    apps/DesktopStreamer/src/main.cpp
    apps/DesktopStreamer/src/MainWindow.cpp
)

set(DESKTOP_STREAMER_MOC_HEADERS
    apps/DesktopStreamer/src/MainWindow.h
)

qt4_wrap_cpp(DESKTOP_STREAMER_MOC_OUTFILES ${DESKTOP_STREAMER_MOC_HEADERS})

add_executable(desktopstreamer ${DESKTOP_STREAMER_SRCS} ${DESKTOP_STREAMER_MOC_OUTFILES})

target_link_libraries(desktopstreamer ${DESKTOP_STREAMER_LIBS})
