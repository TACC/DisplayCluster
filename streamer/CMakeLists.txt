# Standalone build for the client-side streaming tools (dcStream library,
# DesktopStreamer). Deliberately NOT integrated into the root project via
# add_subdirectory(): these are meant to run on a presenter's own machine
# (most realistically Windows or Mac, not a cluster render node), and have
# none of the root project's MPI/CUDA/FFmpeg requirements - bundling them
# together would mean anyone wanting to build just this small client tool
# would first need to satisfy the full cluster-side dependency list, none
# of which this code actually uses.
#
# SimpleStreamer (streamer/apps/SimpleStreamer) isn't included here - it's
# a GLUT-based teapot demo, not a real tool, and GLUT isn't otherwise a
# project dependency anywhere; add it later if actually wanted.

cmake_minimum_required(VERSION 3.16)

project(DisplayClusterStreamer)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

if(NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)

# same moc-choking-on-Boost workaround as the root CMakeLists.txt - needed
# here too since MainWindow.h (a Q_OBJECT class) pulls in
# ../src/ParallelPixelStream.h, which pulls in Boost's serialization/
# archive headers
set(CMAKE_AUTOMOC_MOC_OPTIONS -DPOSIX_TIME_HPP___ -DPOSIX_TIME_SERIALIZE_HPP___ -DBOOST_TT_HAS_OPERATOR_HPP_INCLUDED)

if(WIN32)
    add_definitions(-DNOMINMAX -DWIN32_LEAN_AND_MEAN)
endif()

# reuse the root project's cmake/ modules (FindLibJpegTurbo.cmake) and
# config.h.in, rather than duplicating them
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules/")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Concurrent)

find_package(libjpeg-turbo CONFIG QUIET)
if(libjpeg-turbo_FOUND)
    set(JPEG_LIBS libjpeg-turbo::turbojpeg)
else()
    find_package(LibJpegTurbo REQUIRED)
    include_directories(${LibJpegTurbo_INCLUDE_DIRS})
    set(JPEG_LIBS ${LibJpegTurbo_LIBRARIES})
endif()

find_package(Boost REQUIRED COMPONENTS serialization)
include_directories(${Boost_INCLUDE_DIRS})

# a couple of the root project's headers pulled in transitively (PixelStream.h)
# include "config.h" - generate a minimal one rather than duplicating the
# root build's git-version-stamping (not very useful for a tool run locally
# on a presenter's own machine, unlike the cluster-side render processes
# this was actually added for)
set(ENABLE_PYTHON_SUPPORT OFF)
set(DISPLAYCLUSTER_GIT_VERSION "n/a")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)

# dcStream: the client library (connection/protocol/JPEG-encode helpers),
# used directly by third-party streaming clients (see SimpleStreamer, once
# it's ported) - DesktopStreamer below implements the wire protocol itself
# rather than linking this, so it's built here mainly so it stays compiled
# and ready for whoever wants to link against it
add_library(dcStream STATIC
    lib/dcStream.cpp
    ../src/log.cpp
)
target_link_libraries(dcStream Qt6::Core Qt6::Network Qt6::Concurrent ${JPEG_LIBS})

add_executable(DesktopStreamer
    apps/DesktopStreamer/src/main.cpp
    apps/DesktopStreamer/src/MainWindow.cpp
    apps/DesktopStreamer/src/DesktopSelectionWindow.cpp
    apps/DesktopStreamer/src/DesktopSelectionView.cpp
    apps/DesktopStreamer/src/DesktopSelectionRectangle.cpp
    ../src/log.cpp
)
target_include_directories(DesktopStreamer PRIVATE apps/DesktopStreamer/src)
target_link_libraries(DesktopStreamer Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::Concurrent ${JPEG_LIBS} ${Boost_LIBRARIES})
