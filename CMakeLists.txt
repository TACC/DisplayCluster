project(DisplayCluster)

cmake_minimum_required(VERSION 2.4.0)

# version information
set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
set(VERSION_PATCH 0)

# required for mixing libraries with full paths and not...
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

# show all warnings
set(CMAKE_CXX_FLAGS "-Wall")

# keep the library path in installed executables
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# build options
option(BUILD_DISPLAYCLUSTER "Build main DisplayCluster application" OFF)
option(BUILD_DISPLAYCLUSTER_LIBRARY "Build DisplayCluster library" OFF)
option(BUILD_DESKTOPSTREAMER "Build DesktopStreamer application" OFF)

if(BUILD_DISPLAYCLUSTER)
    option(ENABLE_TUIO_TOUCH_LISTENER "Enable TUIO touch listener for multi-touch events" OFF)
    option(ENABLE_JOYSTICK_SUPPORT "Enable joystick support" OFF)
    option(ENABLE_SKELETON_SUPPORT "EXPERIMENTAL: Enable skeleton tracking interface support" OFF)
    option(ENABLE_PYTHON_SUPPORT "Enable Python support" OFF)
endif()

# path for additional modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

if(BUILD_DISPLAYCLUSTER OR BUILD_DISPLAYCLUSTER_LIBRARY OR BUILD_DESKTOPSTREAMER)
    # find and setup Qt4
    # see http://cmake.org/cmake/help/cmake2.6docs.html#module:FindQt4 for details
    set(QT_USE_QTOPENGL TRUE)
    set(QT_USE_QTXMLPATTERNS TRUE)
    set(QT_USE_QTXML TRUE)
    set(QT_USE_QTNETWORK TRUE)

    if(WIN32)
        set(QT_USE_QTMAIN TRUE)
    endif()

    find_package(Qt4 REQUIRED)

    include(${QT_USE_FILE})
    set(LIBS ${LIBS} ${QT_LIBRARIES})

    # libjpeg-turbo
    find_package(LibJpegTurbo REQUIRED)
    include_directories(${LibJpegTurbo_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${LibJpegTurbo_LIBRARIES})
endif()

if(BUILD_DISPLAYCLUSTER)
    find_package(OpenGL REQUIRED)
    include_directories(${OPENGL_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${OPENGL_LIBRARIES})

    find_package(Boost REQUIRED COMPONENTS serialization date_time)
    include_directories(${Boost_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${Boost_LIBRARIES})

    find_package(MPI REQUIRED)
    include_directories(${MPI_INCLUDE_PATH})
    set(LIBS ${LIBS} ${MPI_LIBRARIES})

    find_package(FFMPEG REQUIRED)
    include_directories(SYSTEM ${FFMPEG_INCLUDE_DIR}) # use SYSTEM to suppress FFMPEG compile warnings
    set(LIBS ${LIBS} ${FFMPEG_LIBRARIES})

    # handle build options
    if(ENABLE_TUIO_TOUCH_LISTENER)
        find_package(TUIO REQUIRED)
        include_directories(${TUIO_INCLUDE_DIRS})
        set(LIBS ${LIBS} ${TUIO_LIBRARIES})

        set(SRCS ${SRCS} src/TouchListener.cpp)

        find_package(X11 REQUIRED)
        include_directories(${X11_INCLUDE_DIR})
        set(LIBS ${LIBS} ${X11_LIBRARIES})
    endif()

    if(ENABLE_JOYSTICK_SUPPORT)
        find_package(SDL REQUIRED)
        include_directories(${SDL_INCLUDE_DIR})
        set(LIBS ${LIBS} ${SDL_LIBRARY})

        set(SRCS ${SRCS} src/JoystickThread.cpp)
        set(SRCS ${SRCS} src/DisplayGroupJoystick.cpp)

        set(MOC_HEADERS ${MOC_HEADERS} src/JoystickThread.h)
    endif()

    if(ENABLE_SKELETON_SUPPORT)
        find_package(OpenNI REQUIRED)
        include_directories(SYSTEM ${OpenNI_INCLUDE_DIRS}) # use SYSTEM to suppress OpenNI compile warnings
        set(LIBS ${LIBS} ${OpenNI_LIBRARIES})

        set(SRCS ${SRCS}
            src/SkeletonThread.cpp
            src/SkeletonSensor.cpp
            src/SkeletonState.cpp)

        if(NOT ENABLE_JOYSTICK_SUPPORT)
            set(SRCS ${SRCS} src/DisplayGroupJoystick.cpp)
        endif()

        set(MOC_HEADERS ${MOC_HEADERS} src/SkeletonThread.h)
    endif()

    if(ENABLE_PYTHON_SUPPORT)
        find_package(PythonLibs REQUIRED)
        include_directories(${PYTHON_INCLUDE_PATH}) # deprecated, but include for old CMake versions
        include_directories(${PYTHON_INCLUDE_DIRS})
        set(LIBS ${LIBS} ${PYTHON_LIBRARIES})

        find_package(PythonQt REQUIRED)
        include_directories(${PythonQt_INCLUDE_DIRS})
        set(LIBS ${LIBS} ${PythonQt_LIBRARIES})

        set(SRCS ${SRCS} src/DisplayGroupPython.cpp)
        set(SRCS ${SRCS} src/PythonConsole.cpp)

        set(MOC_HEADERS ${MOC_HEADERS} src/DisplayGroupPython.h)
        set(MOC_HEADERS ${MOC_HEADERS} src/PythonConsole.h)
    endif()

    # process config file
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
    include_directories(${CMAKE_CURRENT_BINARY_DIR})

    set(SRCS ${SRCS}
        src/Configuration.cpp
        src/Content.cpp
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
        src/DynamicTexture.cpp
        src/DynamicTextureContent.cpp
        src/FactoryObject.cpp
        src/GLWindow.cpp
        src/log.cpp
        src/main.cpp
        src/MainWindow.cpp
        src/Movie.cpp
        src/MovieContent.cpp
        src/NetworkListener.cpp
        src/NetworkListenerThread.cpp
        src/Options.cpp
        src/ParallelPixelStream.cpp
        src/ParallelPixelStreamContent.cpp
        src/PixelStream.cpp
        src/PixelStreamContent.cpp
        src/PixelStreamSource.cpp
        src/Texture.cpp
        src/TextureContent.cpp
    )

    set(MOC_HEADERS ${MOC_HEADERS}
        src/Content.h
        src/ContentWindowInterface.h
        src/DisplayGroupManager.h
        src/DisplayGroupInterface.h
        src/DisplayGroupGraphicsViewProxy.h
        src/DisplayGroupListWidgetProxy.h
        src/MainWindow.h
        src/Marker.h
        src/NetworkListener.h
        src/NetworkListenerThread.h
        src/Options.h
    )

    qt4_wrap_cpp(MOC_OUTFILES ${MOC_HEADERS})

    add_executable(displaycluster ${SRCS} ${MOC_OUTFILES})

    target_link_libraries(displaycluster ${LIBS})

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


# DisplayCluster library
if(BUILD_DISPLAYCLUSTER_LIBRARY)
    set(DISPLAYCLUSTER_LIBRARY_LIBS ${QT_QTCORE_LIBRARY} ${QT_QTNETWORK_LIBRARY})
    set(DISPLAYCLUSTER_LIBRARY_LIBS ${DISPLAYCLUSTER_LIBRARY_LIBS} ${LibJpegTurbo_LIBRARIES})

    set(DISPLAYCLUSTER_LIBRARY_SRCS
        src/log.cpp
        src/lib/dcStream.cpp
    )

    set(DISPLAYCLUSTER_LIBRARY_PUBLIC_HEADERS
        src/lib/dcStream.h
    )

    # build as a static library on Apple platforms, shared on others
    # RPATH seems to have trouble on Apple...
    if(APPLE)
        add_library(DisplayClusterLibrary ${DISPLAYCLUSTER_LIBRARY_SRCS})
    else(APPLE)
        add_library(DisplayClusterLibrary SHARED ${DISPLAYCLUSTER_LIBRARY_SRCS})
    endif(APPLE)

    set_target_properties(DisplayClusterLibrary PROPERTIES OUTPUT_NAME DisplayCluster)

    target_link_libraries(DisplayClusterLibrary ${DISPLAYCLUSTER_LIBRARY_LIBS})

    # install library
    INSTALL(TARGETS DisplayClusterLibrary
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )

    # install headers
    install(FILES ${DISPLAYCLUSTER_LIBRARY_PUBLIC_HEADERS} DESTINATION include)


    # SimpleStreamer example application
    find_package(OpenGL REQUIRED)
    include_directories(${OPENGL_INCLUDE_DIRS})
    set(SIMPLESTREAMER_LIBS ${OPENGL_LIBRARIES})

    find_package(GLUT REQUIRED)
    include_directories(${GLUT_INCLUDE_DIR})
    set(SIMPLESTREAMER_LIBS ${SIMPLESTREAMER_LIBS} ${GLUT_LIBRARIES})

    include_directories(src/lib)
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

    # on Apple platform set the icon
    if(APPLE)
        # set how it shows up in the Info.plist file
        SET(MACOSX_BUNDLE_ICON_FILE desktopstreamer.icns)

        # set where in the bundle to put the icns file
        SET_SOURCE_FILES_PROPERTIES(apps/DesktopStreamer/desktopstreamer.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

        # include the icns file in the target
        SET(DESKTOP_STREAMER_SRCS ${DESKTOP_STREAMER_SRCS} apps/DesktopStreamer/desktopstreamer.icns)
    endif(APPLE)

    add_executable(desktopstreamer MACOSX_BUNDLE WIN32
        ${DESKTOP_STREAMER_SRCS} ${DESKTOP_STREAMER_MOC_OUTFILES})

    target_link_libraries(desktopstreamer ${DESKTOP_STREAMER_LIBS})

    # install executable
    INSTALL(TARGETS desktopstreamer
        RUNTIME DESTINATION bin COMPONENT Runtime
        BUNDLE DESTINATION . COMPONENT Runtime
    )

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
