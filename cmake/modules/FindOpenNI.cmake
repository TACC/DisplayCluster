# - Try to find OpenNI
# Once done, this will define
#
#  OpenNI_FOUND - system has OpenNI
#  OpenNI_INCLUDE_DIRS - the OpenNI include directories
#  OpenNI_LIBRARIES - link these to use OpenNI
#
# this file is modeled after http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(OpenNI_PKGCONF OpenNI)

# Include dir
find_path(OpenNI_INCLUDE_DIR
  NAMES XnCppWrapper.h
  PATHS ${OpenNI_PKGCONF_INCLUDE_DIRS} /usr/include/ni
)

# also add the ni/ directory to the include path
set(OpenNI_INCLUDE_DIR_2 "${OpenNI_INCLUDE_DIR}/ni")

# Finally the library itself
find_library(OpenNI_LIBRARY
  NAMES OpenNI
  PATHS ${OpenNI_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(OpenNI_PROCESS_INCLUDES OpenNI_INCLUDE_DIR OpenNI_INCLUDE_DIR_2)
set(OpenNI_PROCESS_LIBS OpenNI_LIBRARY)
libfind_process(OpenNI)