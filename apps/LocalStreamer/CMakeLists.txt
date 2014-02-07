
include_directories(${CMAKE_SOURCE_DIR}/src/core)
include_directories(${CMAKE_SOURCE_DIR}/src/dcstream)
include_directories(${CMAKE_BINARY_DIR}) ### for config.h ###

set(LOCAL_STREAMER_SRCS
  ../../src/log.cpp
  src/main.cpp
  src/Application.cpp
)

set(LOCAL_STREAMER_MOC_HEADERS
  src/Application.h
)

qt4_wrap_cpp(LOCAL_STREAMER_MOC_OUTFILES ${LOCAL_STREAMER_MOC_HEADERS})

# DisplayCluster Main Application
add_executable(localstreamer ${LOCAL_STREAMER_MOC_OUTFILES} ${LOCAL_STREAMER_SRCS})
target_link_libraries(localstreamer dccore dcstream)

# install executable
install(TARGETS localstreamer RUNTIME DESTINATION bin)
