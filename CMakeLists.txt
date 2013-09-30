project(DisplayCluster)

cmake_minimum_required(VERSION 2.4.0)

# version information
set(VERSION_MAJOR 0)
set(VERSION_MINOR 2)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})

# required for mixing libraries with full paths and not...
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

# show all warnings
set(CMAKE_CXX_FLAGS "-Wall")

# build options
option(BUILD_DISPLAYCLUSTER_LIBRARY "Build DisplayCluster library" OFF)
option(BUILD_DESKTOPSTREAMER "Build DesktopStreamer application" OFF)
option(ENABLE_TUIO_TOUCH_LISTENER "Enable TUIO touch listener for multi-touch events" OFF)
option(ENABLE_JOYSTICK_SUPPORT "Enable joystick support" OFF)
option(ENABLE_SKELETON_SUPPORT "EXPERIMENTAL: Enable skeleton tracking interface support" OFF)
option(ENABLE_PYTHON_SUPPORT "Enable Python support" OFF)
option(ENABLE_PDF_SUPPORT "Enable Pdf support using Poppler" OFF)

if(APPLE)
  # WAR needed when CMAKE_OSX_SYSROOT is set to find /usr/include/turbojpeg.h
  include_directories(SYSTEM /usr/local/../include)
  if(NOT CMAKE_OSX_ARCHITECTURES OR CMAKE_OSX_ARCHITECTURES STREQUAL "")
    if(_CMAKE_OSX_MACHINE MATCHES "ppc")
      set(CMAKE_OSX_ARCHITECTURES "ppc;ppc64" CACHE
        STRING "Build architectures for OS X" FORCE)
    else()
      set(CMAKE_OSX_ARCHITECTURES "i386;x86_64" CACHE
        STRING "Build architectures for OS X" FORCE)
    endif()
  endif()
  if (NOT CMAKE_INSTALL_NAME_DIR)
    set(CMAKE_INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib")
  endif (NOT CMAKE_INSTALL_NAME_DIR)
endif()

# path for additional modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

if(NOT BUILDYARD)
  include(Buildyard)
  return()
endif()

# find and setup Qt4
# see http://cmake.org/cmake/help/cmake2.6docs.html#module:FindQt4 for details
set(QT_USE_QTOPENGL TRUE)
set(QT_USE_QTSVG TRUE)
set(QT_USE_QTXMLPATTERNS TRUE)
set(QT_USE_QTXML TRUE)
set(QT_USE_QTNETWORK TRUE)
set(QT_USE_QTWEBKIT TRUE)

if(WIN32)
  set(QT_USE_QTMAIN TRUE)
endif()

find_package(Qt4 REQUIRED)
include(${QT_USE_FILE})
list(APPEND LIBS ${QT_LIBRARIES})

find_package(LibJpegTurbo REQUIRED)
include_directories(${LibJpegTurbo_INCLUDE_DIRS})
list(APPEND LIBS ${LibJpegTurbo_LIBRARIES})

find_package(Boost REQUIRED COMPONENTS serialization date_time)
include_directories(${Boost_INCLUDE_DIRS})
list(APPEND LIBS ${Boost_LIBRARIES})

find_package(OpenMP)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")

# DisplayCluster library
if(BUILD_DISPLAYCLUSTER_LIBRARY)
    set(DISPLAYCLUSTER_LIBRARY_LIBS ${QT_QTCORE_LIBRARY} ${QT_QTNETWORK_LIBRARY})
    set(DISPLAYCLUSTER_LIBRARY_LIBS ${DISPLAYCLUSTER_LIBRARY_LIBS} ${LibJpegTurbo_LIBRARIES})

    set(DISPLAYCLUSTER_LIBRARY_SRCS
        src/log.cpp
        src/lib/DcSocket.cpp
        src/lib/dcStream.cpp
    )

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/lib/dcVersion.in.h
      ${CMAKE_BINARY_DIR}/dcVersion.h)

    set(DISPLAYCLUSTER_LIBRARY_PUBLIC_HEADERS
        src/lib/dcStream.h
        src/InteractionState.h
        ${CMAKE_BINARY_DIR}/dcVersion.h
    )

    add_library(DisplayClusterLibrary SHARED ${DISPLAYCLUSTER_LIBRARY_SRCS})
    target_link_libraries(DisplayClusterLibrary ${DISPLAYCLUSTER_LIBRARY_LIBS})
    set_target_properties(DisplayClusterLibrary PROPERTIES
      VERSION ${VERSION} SOVERSION ${VERSION_ABI} OUTPUT_NAME DisplayCluster)

    # install library
    INSTALL(TARGETS DisplayClusterLibrary
        ARCHIVE DESTINATION lib COMPONENT dev
        RUNTIME DESTINATION bin COMPONENT lib
        LIBRARY DESTINATION lib COMPONENT lib)

    # install headers
    install(FILES ${DISPLAYCLUSTER_LIBRARY_PUBLIC_HEADERS} DESTINATION include)


    # SimpleStreamer example application
    find_package(OpenGL REQUIRED)
    include_directories(${OPENGL_INCLUDE_DIRS})
    set(SIMPLESTREAMER_LIBS ${OPENGL_LIBRARIES})

    find_package(GLUT REQUIRED)
    include_directories(${GLUT_INCLUDE_DIR})
    set(SIMPLESTREAMER_LIBS ${SIMPLESTREAMER_LIBS} ${GLUT_LIBRARIES})

    include_directories(src/lib src)
    set(SIMPLESTREAMER_LIBS ${SIMPLESTREAMER_LIBS} DisplayClusterLibrary)

    set(SIMPLESTREAMER_SRCS
        apps/SimpleStreamer/src/main.cpp
    )

    add_executable(simplestreamer ${SIMPLESTREAMER_SRCS})

    target_link_libraries(simplestreamer ${SIMPLESTREAMER_LIBS})

    # install executable
    INSTALL(TARGETS simplestreamer
        RUNTIME DESTINATION bin
    )
endif()


find_package(OpenGL)
find_package(MPI)
find_package(FFMPEG)
if(OPENGL_FOUND AND MPI_FOUND AND FFMPEG_FOUND)
  include_directories(SYSTEM ${OPENGL_INCLUDE_DIRS} ${MPI_INCLUDE_PATH}
    ${FFMPEG_INCLUDE_DIR})
  list(APPEND LIBS ${OPENGL_LIBRARIES} ${MPI_CXX_LIBRARIES}
    ${FFMPEG_LIBRARIES})

  # handle build options
  if(ENABLE_TUIO_TOUCH_LISTENER)
    find_package(TUIO REQUIRED)
    include_directories(${TUIO_INCLUDE_DIRS})
    list(APPEND LIBS ${TUIO_LIBRARIES})

    set(SRCS ${SRCS} src/Gestures.cpp)
    set(SRCS ${SRCS} src/MultiTouchListener.cpp)
    set(SRCS ${SRCS} src/TouchListener.cpp)

    find_package(X11 REQUIRED)
    include_directories(${X11_INCLUDE_DIR})
    list(APPEND LIBS ${X11_LIBRARIES})
  endif()

  if(ENABLE_JOYSTICK_SUPPORT)
    find_package(SDL REQUIRED)
    include_directories(${SDL_INCLUDE_DIR})
    list(APPEND LIBS ${SDL_LIBRARY})

    set(SRCS ${SRCS} src/JoystickThread.cpp)
    set(SRCS ${SRCS} src/DisplayGroupJoystick.cpp)

    set(MOC_HEADERS ${MOC_HEADERS} src/JoystickThread.h)
  endif()

  if(ENABLE_SKELETON_SUPPORT)
    find_package(OpenNI REQUIRED)
    include_directories(SYSTEM ${OpenNI_INCLUDE_DIRS}) # use SYSTEM to suppress OpenNI compile warnings
    list(APPEND LIBS ${OpenNI_LIBRARIES})

    list(APPEMD SRCS
      src/SkeletonThread.cpp
      src/SkeletonSensor.cpp
      src/SkeletonState.cpp)

    if(NOT ENABLE_JOYSTICK_SUPPORT)
      list(APPEMD SRCS src/DisplayGroupJoystick.cpp)
    endif()

    set(MOC_HEADERS ${MOC_HEADERS} src/SkeletonThread.h)
  endif()

    if(ENABLE_PYTHON_SUPPORT)
        find_package(PythonLibs REQUIRED)
        include_directories(${PYTHON_INCLUDE_PATH}) # deprecated, but include for old CMake versions
        include_directories(${PYTHON_INCLUDE_DIRS})
        list(APPEND LIBS ${PYTHON_LIBRARIES})

        find_package(PythonQt REQUIRED)
        include_directories(${PythonQt_INCLUDE_DIRS})
        list(APPEND LIBS ${PythonQt_LIBRARIES})

        set(SRCS ${SRCS} src/DisplayGroupPython.cpp)
        set(SRCS ${SRCS} src/PythonConsole.cpp)

        set(MOC_HEADERS ${MOC_HEADERS} src/DisplayGroupPython.h)
        set(MOC_HEADERS ${MOC_HEADERS} src/PythonConsole.h)
    endif()

    if(ENABLE_PDF_SUPPORT)
      find_package(Poppler REQUIRED)
      include_directories(${POPPLER_QT_INCLUDE_DIR})
      list(APPEND LIBS ${POPPLER_LIBRARIES})

      list(APPEND SRCS
        src/PDF.cpp
        src/PDFContent.cpp
        src/PDFInteractionDelegate.cpp
      )
      list(APPEND MOC_HEADERS
        src/PDFContent.h
        src/PDFInteractionDelegate.h
      )
    endif()

    # process config file
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
    include_directories(${CMAKE_CURRENT_BINARY_DIR})

    list(APPEND SRCS
        src/BackgroundWidget.cpp
        src/Configuration.cpp
        src/Content.cpp
        src/ContentFactory.cpp
        src/ContentInteractionDelegate.cpp
        src/ContentWindowManager.cpp
        src/ContentWindowInterface.cpp
        src/ContentWindowGraphicsItem.cpp
        src/ContentWindowListWidgetItem.cpp
        src/Marker.cpp
        src/DisplayGroupManager.cpp
        src/DisplayGroupInterface.cpp
        src/DisplayGroupGraphicsScene.cpp
        src/DisplayGroupGraphicsViewProxy.cpp
        src/DisplayGroupGraphicsView.cpp
        src/DisplayGroupListWidgetProxy.cpp
        src/DockPixelStreamer.cpp
        src/DynamicTexture.cpp
        src/DynamicTextureContent.cpp
        src/FactoryObject.cpp
        src/GLWindow.cpp
        src/log.cpp
        src/LocalPixelStreamer.cpp
        src/LocalPixelStreamerManager.cpp
        src/main.cpp
        src/MainWindow.cpp
        src/Movie.cpp
        src/MovieContent.cpp
        src/NetworkListener.cpp
        src/NetworkListenerThread.cpp
        src/Options.cpp
        src/Pictureflow.cpp
        src/PixelStream.cpp
        src/PixelStreamContent.cpp
        src/PixelStreamInteractionDelegate.cpp
        src/PixelStreamSegmentRenderer.cpp
        src/PixelStreamSegmentJpegCompressor.cpp
        src/SVG.cpp
        src/SVGContent.cpp
        src/SVGStreamSource.cpp
        src/Texture.cpp
        src/TextureContent.cpp
        src/WebkitPixelStreamer.cpp
        src/ZoomInteractionDelegate.cpp
    )

    list(APPEND MOC_HEADERS
        src/BackgroundWidget.h
        src/Content.h
        src/ContentInteractionDelegate.h
        src/ContentWindowInterface.h
        src/DisplayGroupManager.h
        src/DisplayGroupInterface.h
        src/DisplayGroupGraphicsViewProxy.h
        src/DisplayGroupListWidgetProxy.h
        src/DockPixelStreamer.h
        src/LocalPixelStreamer.h
        src/LocalPixelStreamerManager.h
        src/MainWindow.h
        src/Marker.h
        src/NetworkListener.h
        src/NetworkListenerThread.h
        src/Options.h
        src/Pictureflow.h
        src/PixelStreamInteractionDelegate.h
        src/WebkitPixelStreamer.h
        src/ZoomInteractionDelegate.h
    )

    # add OPTIONS hacks for Qt's MOC processing of Boost headers, for compiler errors with some versions of Boost
    qt4_wrap_cpp(MOC_OUTFILES ${MOC_HEADERS} OPTIONS -DPOSIX_TIME_HPP___ -DPOSIX_TIME_SERIALIZE_HPP___ -DBOOST_TT_HAS_OPERATOR_HPP_INCLUDED)

    add_executable(displaycluster ${SRCS} ${MOC_OUTFILES})

    target_link_libraries(displaycluster ${LIBS} DisplayClusterLibrary)

    # build Python module if Python support is enabled
    if(ENABLE_PYTHON_SUPPORT)
        add_custom_command(TARGET displaycluster POST_BUILD
            COMMENT "Building Python module"
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/python
            COMMAND python configure.py -I ${Boost_INCLUDE_DIRS}
            COMMAND make
            COMMAND mv pydc.so ${CMAKE_BINARY_DIR})
    endif()

    # install executable
    INSTALL(TARGETS displaycluster
        RUNTIME DESTINATION bin
    )

    # install launchers
    INSTALL(PROGRAMS examples/startdisplaycluster DESTINATION bin)
    INSTALL(PROGRAMS examples/displaycluster.py DESTINATION bin)

    # install data files
    INSTALL(DIRECTORY data DESTINATION .)

    # install example files
    INSTALL(DIRECTORY examples DESTINATION .)

    # install doc files
    INSTALL(DIRECTORY doc DESTINATION .)

    # install Python module
    if(ENABLE_PYTHON_SUPPORT)
        INSTALL(FILES ${CMAKE_BINARY_DIR}/pydc.so DESTINATION python)
    endif()
endif()

# DesktopStreamer app
if(BUILD_DESKTOPSTREAMER)
    set(DESKTOP_STREAMER_LIBS ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTNETWORK_LIBRARY})

    if(WIN32)
        set(DESKTOP_STREAMER_LIBS ${DESKTOP_STREAMER_LIBS} ${QT_QTMAIN_LIBRARY})
    endif()

    # libjpeg-turbo
    set(DESKTOP_STREAMER_LIBS ${DESKTOP_STREAMER_LIBS} ${LibJpegTurbo_LIBRARIES})

    set(DESKTOP_STREAMER_SRCS ${DESKTOP_STREAMER_SRCS}
        src/log.cpp
        apps/DesktopStreamer/src/DesktopSelectionRectangle.cpp
        apps/DesktopStreamer/src/DesktopSelectionWindow.cpp
        apps/DesktopStreamer/src/DesktopSelectionView.cpp
        apps/DesktopStreamer/src/main.cpp
        apps/DesktopStreamer/src/MainWindow.cpp
    )

    set(DESKTOP_STREAMER_MOC_HEADERS
        apps/DesktopStreamer/src/MainWindow.h
    )

    qt4_wrap_cpp(DESKTOP_STREAMER_MOC_OUTFILES ${DESKTOP_STREAMER_MOC_HEADERS})
    qt4_add_resources(DESKTOP_STREAMER_RRC apps/DesktopStreamer/resources.qrc)

    # on Apple platform set the icon
    if(APPLE)
        # set where in the bundle to put the icns file
        SET_SOURCE_FILES_PROPERTIES(apps/DesktopStreamer/desktopstreamer.icns
          PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

        # include the icns file in the target
        SET(DESKTOP_STREAMER_SRCS ${DESKTOP_STREAMER_SRCS}
          apps/DesktopStreamer/desktopstreamer.icns)
    endif(APPLE)

    add_executable(desktopstreamer MACOSX_BUNDLE WIN32
      ${DESKTOP_STREAMER_SRCS} ${DESKTOP_STREAMER_MOC_OUTFILES} ${DESKTOP_STREAMER_RRC})
    set_target_properties(desktopstreamer PROPERTIES
      MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/apps/DesktopStreamer/src/Info.plist)
    target_link_libraries(desktopstreamer ${DESKTOP_STREAMER_LIBS})

    # install executable
    install(TARGETS desktopstreamer
        RUNTIME DESTINATION bin COMPONENT Runtime
        BUNDLE DESTINATION . COMPONENT Runtime
    )
    if(APPLE)
      install(FILES ${DESKTOP_STREAMER_LIBS}
        DESTINATION ${CMAKE_BINARY_DIR}/desktopstreamer.app/Contents/MacOS)
    endif()

    # on Apple platform make the .app bundle; on Windows make the installer
    if(APPLE OR WIN32)
        set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "DesktopStreamer is an application that lets you stream your desktop to a running DisplayCluster instance.")
        set(CPACK_PACKAGE_EXECUTABLES "desktopstreamer" "DesktopStreamer")
        set(CPACK_PACKAGE_NAME "DesktopStreamer")
        set(CPACK_PACKAGE_VENDOR "The University of Texas at Austin")
        set(CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR})
        set(CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR})
        set(CPACK_PACKAGE_VERSION_PATCH ${VERSION_PATCH})

        if(APPLE)
            set(APPS "\${CMAKE_INSTALL_PREFIX}/desktopstreamer.app")

            # Qt plugins and qt.conf required so we don't accidentally dynamically link against system-installed Qt libraries
            SET(PLUGIN_DEST_DIR desktopstreamer.app/Contents/MacOS)
            SET(QTCONF_DEST_DIR desktopstreamer.app/Contents/Resources)
        elseif(WIN32)
            set(APPS "\${CMAKE_INSTALL_PREFIX}/bin/desktopstreamer.exe")

            # Qt plugins and qt.conf required so we don't accidentally dynamically link against system-installed Qt libraries
            SET(PLUGIN_DEST_DIR bin)
            SET(QTCONF_DEST_DIR bin)
        endif()

        INSTALL(DIRECTORY "${QT_PLUGINS_DIR}/imageformats" DESTINATION ${PLUGIN_DEST_DIR}/plugins COMPONENT Runtime)

        INSTALL(CODE "
            file(WRITE \"\${CMAKE_INSTALL_PREFIX}/${QTCONF_DEST_DIR}/qt.conf\" \"\")
            " COMPONENT Runtime)

        if(WIN32)
            # directories to look for dependencies
            set(DEPENDENCY_DIRS ${QT_BINARY_DIR})

            include(InstallRequiredSystemLibraries)
        endif()

        INSTALL(CODE "
            file(GLOB_RECURSE QTPLUGINS \"\${CMAKE_INSTALL_PREFIX}/${PLUGIN_DEST_DIR}/plugins/*${CMAKE_SHARED_LIBRARY_SUFFIX}\")
            include(BundleUtilities)
            fixup_bundle(\"${APPS}\" \"\${QTPLUGINS}\" \"${DEPENDENCY_DIRS}\")
            " COMPONENT Runtime)

        # to create a package, one can run "cpack -G DragNDrop CPackConfig.cmake" for example
        # where CPackConfig.cmake is created by including CPack
        if(APPLE)
            set(CPACK_BINARY_DRAGNDROP ON)
        elseif(WIN32)
            set(CPACK_BINARY_NSIS ON)
        endif()

        include(CPack)
    endif()
endif()

add_custom_target(test) # fake test target for Travis/Buildyard
