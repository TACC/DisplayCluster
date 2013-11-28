
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
  set(STREAMER_APP_NAME DesktopStreamer)

  # set where in the bundle to put the icns file
  set_source_files_properties(${STREAMER_APP_NAME}.icns
    PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

  # include the icns file in the target
  set(DESKTOP_STREAMER_SRCS ${DESKTOP_STREAMER_SRCS}
    ${STREAMER_APP_NAME}.icns)
else()
  set(STREAMER_APP_NAME desktopstreamer)
endif()

add_executable(${STREAMER_APP_NAME} MACOSX_BUNDLE WIN32
  ${DESKTOP_STREAMER_SRCS} ${DESKTOP_STREAMER_MOC_OUTFILES} ${DESKTOP_STREAMER_RRC})
set_target_properties(${STREAMER_APP_NAME} PROPERTIES
  MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/src/Info.plist)
target_link_libraries(${STREAMER_APP_NAME} ${DESKTOP_STREAMER_LIBS})

# install executable
install(TARGETS ${STREAMER_APP_NAME}
    RUNTIME DESTINATION bin COMPONENT app
    BUNDLE DESTINATION . COMPONENT app
)
if(APPLE)
  install(FILES ${DESKTOP_STREAMER_LIBS}
    DESTINATION ${CMAKE_BINARY_DIR}/${STREAMER_APP_NAME}.app/Contents/MacOS)
endif()

# on Apple platform make the .app bundle; on Windows make the installer
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "DesktopStreamer is an application that lets you stream your desktop to a running DisplayCluster instance.")
set(CPACK_PACKAGE_EXECUTABLES "${STREAMER_APP_NAME}")
set(CPACK_PACKAGE_NAME "${STREAMER_APP_NAME}")
set(CPACK_PACKAGE_CONTACT "Daniel Nachbaur <daniel.nachbaur@epfl.ch>")
set(CPACK_PACKAGE_VENDOR "The University of Texas at Austin")
set(CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${VERSION_PATCH})
set(CPACK_PACKAGE_LICENSE "LGPL")

if(APPLE)
  set(APPS "\${CMAKE_INSTALL_PREFIX}/${STREAMER_APP_NAME}.app")

  # Qt plugins and qt.conf required so we don't accidentally dynamically link against system-installed Qt libraries
  set(PLUGIN_DEST_DIR ${STREAMER_APP_NAME}.app/Contents/MacOS)
  set(QTCONF_DEST_DIR ${STREAMER_APP_NAME}.app/Contents/Resources)

  # to create a package, one can run "cpack -G DragNDrop CPackConfig.cmake" for example
  # where CPackConfig.cmake is created by including CPack
  set(CPACK_BINARY_DRAGNDROP ON)

  set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${VERSION}")
elseif(WIN32)
  set(APPS "\${CMAKE_INSTALL_PREFIX}/bin/${STREAMER_APP_NAME}.exe")

  # Qt plugins and qt.conf required so we don't accidentally dynamically link against system-installed Qt libraries
  set(PLUGIN_DEST_DIR bin)
  set(QTCONF_DEST_DIR bin)

  # directories to look for dependencies
  set(DEPENDENCY_DIRS ${QT_BINARY_DIR})

  set(CPACK_BINARY_NSIS ON)
else()
  find_program(DEB_EXE debuild REQUIRED)
  find_program(LSB_RELEASE_EXECUTABLE lsb_release REQUIRED)

  execute_process(COMMAND ${LSB_RELEASE_EXECUTABLE} -sr
    OUTPUT_VARIABLE LSB_RELEASE OUTPUT_STRIP_TRAILING_WHITESPACE)

  set(CPACK_GENERATOR "DEB")
  set(CPACK_PACKAGE_VENDOR "http://www.tacc.utexas.edu/tacc-software/displaycluster")
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt4-core, libqt4-gui, libqt4-network, libturbojpeg" )
  string(TOLOWER "${CPACK_PACKAGE_NAME}" CPACK_PACKAGE_NAME)
  set(CMAKE_PACKAGE_VERSION "1")
  set(CPACK_PACKAGE_VERSION ${VERSION}-${CMAKE_PACKAGE_VERSION}~${LSB_RELEASE})
  set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}.${CMAKE_SYSTEM_PROCESSOR}")
  set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "/sbin/ldconfig")
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}")

  set(CPACK_DEB_COMPONENT_INSTALL ON)
  set(CPACK_COMPONENTS_ALL app)
endif()

if(APPLE OR WIN32)
  install(DIRECTORY "${QT_PLUGINS_DIR}/imageformats" DESTINATION ${PLUGIN_DEST_DIR}/plugins COMPONENT app)

  install(CODE "
     file(WRITE \"\${CMAKE_INSTALL_PREFIX}/${QTCONF_DEST_DIR}/qt.conf\" \"\")
     " COMPONENT app)

  install(CODE "
     file(GLOB_RECURSE QTPLUGINS \"\${CMAKE_INSTALL_PREFIX}/${PLUGIN_DEST_DIR}/plugins/*${CMAKE_SHARED_LIBRARY_SUFFIX}\")
     include(BundleUtilities)
     fixup_bundle(\"${APPS}\" \"\${QTPLUGINS}\" \"${DEPENDENCY_DIRS}\")
     " COMPONENT app)
endif()

include(InstallRequiredSystemLibraries)
include(CPack)
