
# Copyright (c) 2013, EPFL/Blue Brain Project
#                     Daniel Nachbaur <daniel.nachbaur@epfl.ch>

# Change this number when adding tests to force a CMake run: 0

enable_testing()
add_definitions(-DBOOST_TEST_DYN_LINK) # generates main() for unit tests

if(NOT WIN32) # tests want to be with DLLs on Windows - no rpath
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
endif()

file(GLOB_RECURSE TEST_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)
list(SORT TEST_FILES)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)

set(ALL_TESTS)
foreach(FILE ${TEST_FILES})
  string(REGEX REPLACE ".cpp" "" NAME ${FILE})
  string(REGEX REPLACE "[./]" "_" NAME ${NAME})
  source_group(\\ FILES ${FILE})
  add_executable(${NAME} ${FILE})
  set_target_properties(${NAME} PROPERTIES FOLDER "Tests")

  target_link_libraries(${NAME} DisplayClusterCore ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

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
set(TEST_RESOURCES webgl_interaction.html)

foreach(FILE ${TEST_RESOURCES})
  file(COPY ${FILE} DESTINATION ${CMAKE_BINARY_DIR}/tests)
endforeach()
