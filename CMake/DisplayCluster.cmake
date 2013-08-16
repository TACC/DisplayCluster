

set(DISPLAYCLUSTER_PACKAGE_VERSION 0.2.0)
set(DISPLAYCLUSTER_REPO_URL https://github.com/BlueBrain/DisplayCluster.git)

set(DISPLAYCLUSTER_DEPENDS MPI
  REQUIRED Boost LibJpegTurbo FFMPEG OpenGL Qt4 TUIO GLUT)
set(DISPLAYCLUSTER_BOOST_COMPONENTS "date_time serialization")
set(DISPLAYCLUSTER_QT4_COMPONENTS "QtNetwork QtOpenGL QtXml QtXmlPatterns")
set(DISPLAYCLUSTER_DEB_DEPENDS libavutil-dev libavformat-dev libavcodec-dev
  libopenmpi-dev libjpeg-turbo8-dev libturbojpeg libswscale-dev freeglut3-dev
  libxmu-dev libboost-date-time-dev libboost-serialization-dev)
set(DISPLAYCLUSTER_CMAKE_ARGS -DBUILD_DISPLAYCLUSTER_LIBRARY=ON -DBUILD_DESKTOPSTREAMER=ON -DENABLE_TUIO_TOUCH_LISTENER=ON)

find_package(MPI)
if(MPI_FOUND)
  if(NOT MPI_C_COMPILER)
    set(MPI_C_COMPILER mpicc)
  endif()
  if(NOT MPI_CXX_COMPILER)
    set(MPI_CXX_COMPILER mpicxx)
  endif()
  set(DISPLAYCLUSTER_EXTRA
    CMAKE_COMMAND CC=${MPI_C_COMPILER} CXX=${MPI_CXX_COMPILER} MPI_INCLUDES=${MPI_INCLUDES} cmake)
endif()

if(CI_BUILD_COMMIT)
  set(DISPLAYCLUSTER_REPO_TAG ${CI_BUILD_COMMIT})
else()
  set(DISPLAYCLUSTER_REPO_TAG master)
endif()
set(DISPLAYCLUSTER_FORCE_BUILD ON)
set(DISPLAYCLUSTER_SOURCE ${CMAKE_SOURCE_DIR})
