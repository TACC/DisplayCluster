# - Try to find OpenNI
# Once done, this will define
#
#  OPENNI_FOUND - system has OpenNI
#  OPENNI_INCLUDE_DIRS - the OpenNI include directories
#  OPENNI_LIBRARIES - link these to use OpenNI
#
# this file is modeled after http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(OPENNI_PKGCONF OPENNI)

# Include dir
find_path(OPENNI_INCLUDE_DIR
  NAMES XnCppWrapper.h
  PATHS ${OPENNI_PKGCONF_INCLUDE_DIRS} /usr/include/ni
)

# also add the ni/ directory to the include path
SET(OPENNI_INCLUDE_DIR_2 "${OPENNI_INCLUDE_DIR}/ni")

# Finally the library itself
find_library(OPENNI_LIBRARY
  NAMES OpenNI
  PATHS ${OPENNI_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(OPENNI_PROCESS_INCLUDES OPENNI_INCLUDE_DIR OPENNI_INCLUDE_DIR_2)
set(OPENNI_PROCESS_LIBS OPENNI_LIBRARY)
libfind_process(OPENNI)