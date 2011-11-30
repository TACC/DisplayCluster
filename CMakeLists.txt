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

find_path(PYTHON_HEADERS Python.h /usr/include/python2.6)
include_directories(${PYTHON_HEADERS})

find_path(PYTHON_QT_HEADERS PythonQt.h ${CMAKE_SOURCE_DIR}/dependencies/PythonQt2.0.1/src)
include_directories(${PYTHON_QT_HEADERS})

find_library(PYTHON_QT_LIBRARIES PythonQt ${CMAKE_SOURCE_DIR}/dependencies/PythonQt2.0.1/lib)
set(LIBS ${LIBS} ${PYTHON_QT_LIBRARIES})

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
    src/DisplayGroupPython.cpp
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
    src/PixelStream.cpp
    src/PixelStreamContent.cpp
    src/PixelStreamSource.cpp
    src/PythonConsole.cpp
    src/Texture.cpp
    src/TextureContent.cpp
)

set(MOC_HEADERS
    src/Content.h
    src/ContentWindowInterface.h
    src/DisplayGroupManager.h
    src/DisplayGroupInterface.h
    src/DisplayGroupPython.h
    src/MainWindow.h
    src/NetworkListener.h
    src/NetworkListenerThread.h
    src/PythonConsole.h
)

qt4_wrap_cpp(MOC_OUTFILES ${MOC_HEADERS})

add_executable(displaycluster ${SRCS} ${MOC_OUTFILES})

target_link_libraries(displaycluster ${LIBS})

# DesktopStreamer app
set(DESKTOP_STREAMER_SRCS ${DESKTOP_STREAMER_SRCS}
    src/log.cpp
    apps/DesktopStreamer/src/main.cpp
    apps/DesktopStreamer/src/MainWindow.cpp
)

set(DESKTOP_STREAMER_MOC_HEADERS
    apps/DesktopStreamer/src/MainWindow.h
)

qt4_wrap_cpp(DESKTOP_STREAMER_MOC_OUTFILES ${DESKTOP_STREAMER_MOC_HEADERS})

add_executable(desktopstreamer ${DESKTOP_STREAMER_SRCS} ${DESKTOP_STREAMER_MOC_OUTFILES})

target_link_libraries(desktopstreamer ${QT_LIBRARIES})
