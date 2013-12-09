project(DisplayCluster)

cmake_minimum_required(VERSION 2.4.0)

# version information
set(VERSION_MAJOR 0)
set(VERSION_MINOR 2)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})

# required for mixing libraries with full paths and not...
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

# show all warnings
set(CMAKE_CXX_FLAGS "-Wall -ftemplate-depth-1024")

# build options
option(BUILD_DISPLAYCLUSTER_LIBRARY "Build DisplayCluster library" OFF)
option(BUILD_DESKTOPSTREAMER "Build DesktopStreamer application" OFF)
option(ENABLE_TUIO_TOUCH_LISTENER "Enable TUIO touch listener for multi-touch events" OFF)
option(ENABLE_JOYSTICK_SUPPORT "Enable joystick support" OFF)
option(ENABLE_SKELETON_SUPPORT "EXPERIMENTAL: Enable skeleton tracking interface support" OFF)
option(ENABLE_PYTHON_SUPPORT "Enable Python support" OFF)
option(ENABLE_PDF_SUPPORT "Enable Pdf support using Poppler" OFF)

if(APPLE)
  # WAR needed when CMAKE_OSX_SYSROOT is set to find /usr/include/turbojpeg.h
  include_directories(SYSTEM /usr/local/../include)
  if(NOT CMAKE_OSX_ARCHITECTURES OR CMAKE_OSX_ARCHITECTURES STREQUAL "")
    if(_CMAKE_OSX_MACHINE MATCHES "ppc")
      set(CMAKE_OSX_ARCHITECTURES "ppc;ppc64" CACHE
        STRING "Build architectures for OS X" FORCE)
    else()
      set(CMAKE_OSX_ARCHITECTURES "i386;x86_64" CACHE
        STRING "Build architectures for OS X" FORCE)
    endif()
  endif()
  if(NOT CMAKE_INSTALL_NAME_DIR)
    set(CMAKE_INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib")
  endif()
endif()

# Used by cmake/modules/DoxygenRule.cmake to install documentation
if(NOT DOC_DIR)
  set(DOC_DIR share/${CMAKE_PROJECT_NAME}/doc)
endif()

# path for additional modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

if(NOT BUILDYARD)
  include(Buildyard)
  return()
endif()

# find and setup Qt4
# see http://cmake.org/cmake/help/cmake2.6docs.html#module:FindQt4 for details
set(QT_USE_QTOPENGL TRUE)
set(QT_USE_QTSVG TRUE)
set(QT_USE_QTXMLPATTERNS TRUE)
set(QT_USE_QTXML TRUE)
set(QT_USE_QTNETWORK TRUE)
set(QT_USE_QTWEBKIT TRUE)

if(WIN32)
  set(QT_USE_QTMAIN TRUE)
endif()

find_package(Qt4 REQUIRED)
include(${QT_USE_FILE})
list(APPEND LIBS ${QT_LIBRARIES})

find_package(LibJpegTurbo REQUIRED)
include_directories(${LibJpegTurbo_INCLUDE_DIRS})
list(APPEND LIBS ${LibJpegTurbo_LIBRARIES})

find_package(Boost REQUIRED COMPONENTS serialization date_time unit_test_framework program_options)
include_directories(${Boost_INCLUDE_DIRS})
list(APPEND LIBS ${Boost_LIBRARIES})

find_package(OpenMP)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")

find_package(OpenGL)
find_package(MPI)
find_package(FFMPEG)
if(OPENGL_FOUND AND MPI_FOUND AND FFMPEG_FOUND)
  option(BUILD_CORE_LIBRARY "Build DisplayCluster Core library" ON)
endif()

# Libraries
include_directories(src)
add_subdirectory(src)

# Applications
add_subdirectory(apps)

# Tests
if(BUILD_CORE_LIBRARY)
  add_subdirectory(tests)
  set(CTEST_CUSTOM_WARNING_EXCEPTION ${CTEST_CUSTOM_WARNING_EXCEPTION}
    # http://public.kitware.com/Bug/view.php?id=10179
    "ld: warning: duplicate dylib")
  include(CTest)
endif()

# install doc files
include(DoxygenRule)
add_subdirectory(doc)

# install example files
install(DIRECTORY examples DESTINATION . COMPONENT examples)
