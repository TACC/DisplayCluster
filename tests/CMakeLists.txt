
# Copyright (c) 2013, EPFL/Blue Brain Project
#                     Daniel Nachbaur <daniel.nachbaur@epfl.ch>
#
# Generates executables for the unit tests and defines a "tests" make target
# which runs all of them.

# Change this number when adding tests to force a CMake run: 3

enable_testing()
add_definitions(-DBOOST_TEST_DYN_LINK) # generates main() for unit tests

if(NOT WIN32) # tests want to be with DLLs on Windows - no rpath
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
endif()
configure_file(configuration.xml
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/configuration.xml)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}) ### for tests headers ###
include_directories(${CMAKE_SOURCE_DIR}/src/core)
include_directories(${CMAKE_BINARY_DIR}) ### for config.h ###

# Libraries to link the tests executables with
set(TESTS_LIBRARIES
  ${Boost_LIBRARIES}
  ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
)

# Shared tests headers, added to the TestLibrary so that they are visible in editor
set(TEST_LIBRARY_FILES
  glxDisplay.h
  GlobalQtApp.h
  MinimalGlobalQtApp.h
)

# DcStream Library Tests
list(APPEND MOC_HEADERS dcstream/MockNetworkListener.h)
list(APPEND TEST_LIBRARY_FILES dcstream/MockNetworkListener.cpp)
list(APPEND TEST_FILES
  dcstream/ImageSegmenterTests.cpp
  dcstream/ImageWrapperTests.cpp
  dcstream/SocketTests.cpp
)
list(APPEND TESTS_LIBRARIES
  dcstream
)

# Core Library Tests
if(BUILD_CORE_LIBRARY)
  list(APPEND TESTS_LIBRARIES
    dccore
    dcwebservice
  )

  # Common Tests (core + dcstream)
  list(APPEND TEST_FILES
    common/NetworkSerializationTests.cpp
    common/PixelStreamSegmentDecoderTests.cpp
    common/PixelStreamSegmentParametersTests.cpp
  )

  # Core Tests
  list(APPEND MOC_HEADERS core/MockTextInputDispatcher.h)
  list(APPEND TEST_LIBRARY_FILES core/MockTextInputDispatcher.cpp)
  list(APPEND TEST_FILES
    core/AsciiToQtKeyCodeMapperTests.cpp
    core/CommandLineOptionsTests.cpp
    core/CommandTests.cpp
    core/ConfigurationTests.cpp
    core/DockToolbarTests.cpp
    core/LocalPixelStreamerTests.cpp
    core/PixelStreamBufferTests.cpp
    core/TextInputHandlerTests.cpp
  )
  find_package(X11)
  if(X11_FOUND)
    list(APPEND TEST_FILES
      core/WebbrowserTests.cpp
    )
  endif()

  # Webservice Tests
  list(APPEND TEST_FILES
    dcWebservice/BoostRegexTests.cpp
    dcWebservice/HandlerTests.cpp
    dcWebservice/MapperTests.cpp
    dcWebservice/RequestBuilderTests.cpp
    dcWebservice/ResponseTests.cpp
    dcWebservice/ServerTests.cpp
  )

  # Performance tests
  list(APPEND PERF_TEST_FILES
    perf/dcStreamTests.cpp
  )
endif()
list(SORT TEST_FILES)

# Create the TestLibrary
qt4_wrap_cpp(MOC_OUTFILES ${MOC_HEADERS})
add_library(TestLibrary SHARED ${MOC_OUTFILES} ${TEST_LIBRARY_FILES})
target_link_libraries(TestLibrary ${TESTS_LIBRARIES})
list(APPEND TESTS_LIBRARIES TestLibrary)

# Create all the tests
set(ALL_TESTS)
foreach(FILE ${TEST_FILES})
  string(REGEX REPLACE ".cpp" "" NAME ${FILE})
  string(REGEX REPLACE "[./]" "_" NAME ${NAME})
  source_group(\\ FILES ${FILE})
  add_executable(${NAME} ${FILE})
  set_target_properties(${NAME} PROPERTIES FOLDER "Tests")

  target_link_libraries(${NAME} ${TESTS_LIBRARIES})

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

# Create executables but do not add them to the tests target
foreach(FILE ${PERF_TEST_FILES})
  string(REGEX REPLACE ".cpp" "" NAME ${FILE})
  string(REGEX REPLACE "[./]" "_" NAME ${NAME})
  source_group(\\ FILES ${FILE})
  add_executable(${NAME} ${FILE})
  set_target_properties(${NAME} PROPERTIES FOLDER "Tests")
  target_link_libraries(${NAME} ${TESTS_LIBRARIES})
endforeach()
