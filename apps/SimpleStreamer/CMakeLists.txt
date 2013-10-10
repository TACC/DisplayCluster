
# DisplayCluster library
if(BUILD_DISPLAYCLUSTER_LIBRARY)
    # SimpleStreamer example application
    find_package(OpenGL REQUIRED)
    include_directories(${OPENGL_INCLUDE_DIRS})
    set(SIMPLESTREAMER_LIBS ${OPENGL_LIBRARIES})

    find_package(GLUT REQUIRED)
    include_directories(${GLUT_INCLUDE_DIR})
    set(SIMPLESTREAMER_LIBS ${SIMPLESTREAMER_LIBS} ${GLUT_LIBRARIES})

    include_directories(../../src/lib src)
    set(SIMPLESTREAMER_LIBS ${SIMPLESTREAMER_LIBS} DisplayClusterLibrary)

    set(SIMPLESTREAMER_SRCS
      src/main.cpp
    )

    add_executable(simplestreamer ${SIMPLESTREAMER_SRCS})

    target_link_libraries(simplestreamer ${SIMPLESTREAMER_LIBS})

    # install executable
    INSTALL(TARGETS simplestreamer
        RUNTIME DESTINATION bin
    )
endif()
