# DisplayCluster main app
if(BUILD_CORE_LIBRARY)
  add_subdirectory(DisplayCluster)
  add_subdirectory(LocalStreamer)
endif()

# DesktopStreamer app
if(BUILD_DESKTOPSTREAMER)
  add_subdirectory(DesktopStreamer)
endif()

# SimpleStreamer
if(BUILD_DISPLAYCLUSTER_LIBRARY)
  add_subdirectory(SimpleStreamer)
endif()
