
# Copyright (c) 2013, EPFL/Blue Brain Project
#                     Daniel Nachbaur <daniel.nachbaur@epfl.ch>

# Change this number when adding tests to force a CMake run: 3

enable_testing()
add_definitions(-DBOOST_TEST_DYN_LINK) # generates main() for unit tests

if(NOT WIN32) # tests want to be with DLLs on Windows - no rpath
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
endif()
configure_file(configuration.xml
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/configuration.xml)

file(GLOB_RECURSE TEST_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/dcWebservice/ *Tests.cpp)
list(SORT TEST_FILES)

find_package(X11)
if(NOT X11_FOUND)
  list(REMOVE_ITEM TEST_FILES "WebbrowserTests.cpp")
endif()

include_directories(${CMAKE_SOURCE_DIR}/src/core)
include_directories(${CMAKE_BINARY_DIR}) ### for config.h ###

#Local classes for test executables
set(MOC_HEADERS
  MockNetworkListener.h
  MockTextInputDispatcher.h
)
qt4_wrap_cpp(MOC_OUTFILES ${MOC_HEADERS})

set(LOCAL_FILES
  MockNetworkListener.cpp
  MockTextInputDispatcher.cpp
  glxDisplay.h
  GlobalQtApp.h
  MinimalGlobalQtApp.h
)
add_library(TestLibrary SHARED ${MOC_OUTFILES} ${LOCAL_FILES})

set(ALL_TESTS)
foreach(FILE ${TEST_FILES})
  string(REGEX REPLACE ".cpp" "" NAME ${FILE})
  string(REGEX REPLACE "[./]" "_" NAME ${NAME})
  source_group(\\ FILES ${FILE})
  add_executable(${NAME} ${FILE})
  set_target_properties(${NAME} PROPERTIES FOLDER "Tests")

  target_link_libraries(${NAME} TestLibrary DisplayClusterCore dcstream
    fcgiws ${Boost_LIBRARIES}
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

  get_target_property(EXECUTABLE ${NAME} LOCATION)
  STRING(REGEX REPLACE "\\$\\(.*\\)" "\${CTEST_CONFIGURATION_TYPE}"
         EXECUTABLE "${EXECUTABLE}")

  add_test(${NAME} ${EXECUTABLE})
  list(APPEND ALL_TESTS ${NAME})
endforeach()

# workaround bug that tests won't get built
add_custom_target(tests COMMAND ${CMAKE_CTEST_COMMAND} DEPENDS ${ALL_TESTS})
set_target_properties(tests PROPERTIES FOLDER "Tests")

# Copy the files needed by the tests to the build directory
set(TEST_RESOURCES webgl_interaction.html configuration.xml)

foreach(FILE ${TEST_RESOURCES})
  file(COPY ${FILE} DESTINATION ${CMAKE_BINARY_DIR}/tests)
endforeach()
