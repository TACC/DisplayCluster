project(DisplayCluster)

cmake_minimum_required(VERSION 2.4.0)

# path for additional modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules/")

if(NOT BUILDYARD)
  include(Buildyard)
  return()
endif()

include(FindPackages)

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

# Qt
if(WIN32)
  set(QT_USE_QTMAIN TRUE)
endif()

include(${QT_USE_FILE})

# build options
if(OPENGL_FOUND AND MPI_FOUND AND FFMPEG_FOUND AND FCGI_FOUND)
  set(BUILD_CORE_LIBRARY ON)
endif()

if(TUIO_FOUND)
  option(ENABLE_TUIO_TOUCH_LISTENER "Enable TUIO touch listener for multi-touch events" ON)
endif()

option(ENABLE_JOYSTICK_SUPPORT "Enable joystick support" OFF)
option(ENABLE_SKELETON_SUPPORT "EXPERIMENTAL: Enable skeleton tracking interface support" OFF)
option(ENABLE_PYTHON_SUPPORT "Enable Python support" OFF)

if(POPPLER_FOUND)
  option(ENABLE_PDF_SUPPORT "Enable Pdf support using Poppler" ON)
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
