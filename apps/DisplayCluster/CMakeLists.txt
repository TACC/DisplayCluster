
include_directories(${CMAKE_SOURCE_DIR}/src/core)
include_directories(${CMAKE_BINARY_DIR}) ### for config.h ###

# DisplayCluster Main Application
add_executable(displaycluster main.cpp)
target_link_libraries(displaycluster DisplayClusterCore)

# install executable
install(TARGETS displaycluster RUNTIME DESTINATION bin)

# install launchers
install(PROGRAMS startdisplaycluster DESTINATION bin)
install(PROGRAMS displaycluster.py   DESTINATION bin)
