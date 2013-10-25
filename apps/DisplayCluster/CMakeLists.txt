
# DisplayCluster Main Application
add_executable(displaycluster main.cpp)
target_link_libraries(displaycluster DisplayClusterCore)

# install executable
install(TARGETS displaycluster RUNTIME DESTINATION bin)

