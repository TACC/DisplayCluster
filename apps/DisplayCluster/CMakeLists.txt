
include_directories(${CMAKE_SOURCE_DIR}/src/core)
include_directories(${CMAKE_BINARY_DIR}) ### for config.h ###

# DisplayCluster Main Application
add_executable(displaycluster main.cpp)
target_link_libraries(displaycluster dccore dcwebservice)

# install executable
install(TARGETS displaycluster RUNTIME DESTINATION bin COMPONENT main)

# install launchers
install(PROGRAMS startdisplaycluster DESTINATION bin COMPONENT main)
install(PROGRAMS displaycluster.py   DESTINATION bin COMPONENT main)
