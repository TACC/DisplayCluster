# DisplayCluster main app
if(BUILD_CORE_LIBRARY)
  add_subdirectory(DisplayCluster)
  add_subdirectory(LocalStreamer)
endif()

# DesktopStreamer app
add_subdirectory(DesktopStreamer)

# SimpleStreamer
if(GLUT_FOUND)
  add_subdirectory(SimpleStreamer)
endif()
