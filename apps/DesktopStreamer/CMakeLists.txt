
set(DESKTOP_STREAMER_LIBS ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTNETWORK_LIBRARY})

if(WIN32)
    set(DESKTOP_STREAMER_LIBS ${DESKTOP_STREAMER_LIBS} ${QT_QTMAIN_LIBRARY})
endif()

# libjpeg-turbo
set(DESKTOP_STREAMER_LIBS ${DESKTOP_STREAMER_LIBS} ${LibJpegTurbo_LIBRARIES})

set(DESKTOP_STREAMER_SRCS ${DESKTOP_STREAMER_SRCS}
    ../../src/log.cpp
    src/DesktopSelectionRectangle.cpp
    src/DesktopSelectionWindow.cpp
    src/DesktopSelectionView.cpp
    src/main.cpp
    src/MainWindow.cpp
)

set(DESKTOP_STREAMER_MOC_HEADERS
    src/MainWindow.h
)

qt4_wrap_cpp(DESKTOP_STREAMER_MOC_OUTFILES ${DESKTOP_STREAMER_MOC_HEADERS})
qt4_add_resources(DESKTOP_STREAMER_RRC resources.qrc)

# on Apple platform set the icon
if(APPLE)
    # set where in the bundle to put the icns file
    SET_SOURCE_FILES_PROPERTIES(desktopstreamer.icns
      PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

    # include the icns file in the target
    SET(DESKTOP_STREAMER_SRCS ${DESKTOP_STREAMER_SRCS}
      desktopstreamer.icns)
endif(APPLE)

add_executable(desktopstreamer MACOSX_BUNDLE WIN32
  ${DESKTOP_STREAMER_SRCS} ${DESKTOP_STREAMER_MOC_OUTFILES} ${DESKTOP_STREAMER_RRC})
set_target_properties(desktopstreamer PROPERTIES
  MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/src/Info.plist)
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

    install(DIRECTORY "${QT_PLUGINS_DIR}/imageformats" DESTINATION ${PLUGIN_DEST_DIR}/plugins COMPONENT Runtime)

    install(CODE "
        file(WRITE \"\${CMAKE_INSTALL_PREFIX}/${QTCONF_DEST_DIR}/qt.conf\" \"\")
        " COMPONENT Runtime)

    if(WIN32)
        # directories to look for dependencies
        set(DEPENDENCY_DIRS ${QT_BINARY_DIR})

        include(InstallRequiredSystemLibraries)
    endif()

    install(CODE "
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
