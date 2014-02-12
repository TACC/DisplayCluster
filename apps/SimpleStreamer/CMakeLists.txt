
# SimpleStreamer example application
set(SIMPLESTREAMER_LIBS ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES} dcstream)

set(SIMPLESTREAMER_SRCS
  src/main.cpp
)

add_executable(simplestreamer ${SIMPLESTREAMER_SRCS})

target_link_libraries(simplestreamer ${SIMPLESTREAMER_LIBS})

# install executable
INSTALL(TARGETS simplestreamer
    RUNTIME DESTINATION bin COMPONENT simplestreamer
)
