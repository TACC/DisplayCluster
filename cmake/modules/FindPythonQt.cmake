# - Try to find PythonQt
# Once done, this will define
#
#  PythonQt_FOUND - system has PythonQt
#  PythonQt_INCLUDE_DIRS - the PythonQt include directories
#  PythonQt_LIBRARIES - link these to use PythonQt
#
# this file is modeled after http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(PythonQt_PKGCONF PythonQt)

# Include dir
find_path(PythonQt_INCLUDE_DIR
  NAMES PythonQt.h
  PATHS ${PythonQt_PKGCONF_INCLUDE_DIRS}
)

# Finally the library itself
find_library(PythonQt_LIBRARY
  NAMES PythonQt
  PATHS ${PythonQt_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(PythonQt_PROCESS_INCLUDES PythonQt_INCLUDE_DIR)
set(PythonQt_PROCESS_LIBS PythonQt_LIBRARY)
libfind_process(PythonQt)