
include_directories(SYSTEM ${OPENGL_INCLUDE_DIRS} ${FFMPEG_INCLUDE_DIR})
list(APPEND LIBS ${OPENGL_LIBRARIES} ${FFMPEG_LIBRARIES})

# handle build options
if(ENABLE_TUIO_TOUCH_LISTENER)
  find_package(TUIO REQUIRED)
  include_directories(${TUIO_INCLUDE_DIRS})
  list(APPEND LIBS ${TUIO_LIBRARIES})

  list(APPEND SRCS gestures/DoubleTapGestureRecognizer.cpp
                   gestures/PanGesture.cpp
                   gestures/PanGestureRecognizer.cpp
                   gestures/PinchGesture.cpp
                   gestures/PinchGestureRecognizer.cpp
                   MultiTouchListener.cpp
                   TouchListener.cpp)

  find_package(X11 REQUIRED)
  include_directories(${X11_INCLUDE_DIR})
  list(APPEND LIBS ${X11_LIBRARIES})
endif()

if(ENABLE_JOYSTICK_SUPPORT)
  find_package(SDL REQUIRED)
  include_directories(${SDL_INCLUDE_DIR})
  list(APPEND LIBS ${SDL_LIBRARY})

  set(SRCS ${SRCS} JoystickThread.cpp)
  set(SRCS ${SRCS} DisplayGroupJoystick.cpp)

  set(MOC_HEADERS ${MOC_HEADERS} JoystickThread.h)
endif()

if(ENABLE_SKELETON_SUPPORT)
  find_package(OpenNI REQUIRED)
  include_directories(SYSTEM ${OpenNI_INCLUDE_DIRS}) # use SYSTEM to suppress OpenNI compile warnings
  list(APPEND LIBS ${OpenNI_LIBRARIES})

  list(APPEMD SRCS
    SkeletonThread.cpp
    SkeletonSensor.cpp
    SkeletonState.cpp)

  if(NOT ENABLE_JOYSTICK_SUPPORT)
    list(APPEMD SRCS DisplayGroupJoystick.cpp)
  endif()

  set(MOC_HEADERS ${MOC_HEADERS} SkeletonThread.h)
endif()

if(ENABLE_PYTHON_SUPPORT)
    find_package(PythonLibs REQUIRED)
    include_directories(${PYTHON_INCLUDE_PATH}) # deprecated, but include for old CMake versions
    include_directories(${PYTHON_INCLUDE_DIRS})
    list(APPEND LIBS ${PYTHON_LIBRARIES})

    find_package(PythonQt REQUIRED)
    include_directories(${PythonQt_INCLUDE_DIRS})
    list(APPEND LIBS ${PythonQt_LIBRARIES})

    set(SRCS ${SRCS} DisplayGroupPython.cpp)
    set(SRCS ${SRCS} PythonConsole.cpp)

    set(MOC_HEADERS ${MOC_HEADERS} DisplayGroupPython.h)
    set(MOC_HEADERS ${MOC_HEADERS} PythonConsole.h)
endif()

if(ENABLE_PDF_SUPPORT)
  find_package(Poppler REQUIRED)
  include_directories(${POPPLER_QT_INCLUDE_DIR})
  list(APPEND LIBS ${POPPLER_LIBRARIES})

  list(APPEND SRCS
    PDF.cpp
    PDFContent.cpp
    PDFInteractionDelegate.cpp
    thumbnail/PDFThumbnailGenerator.cpp
  )
  list(APPEND MOC_HEADERS
    PDFContent.h
  )
endif()

list(APPEND SRCS
    AsyncImageLoader.cpp
    BackgroundWidget.cpp
    Content.cpp
    ContentFactory.cpp
    ContentType.cpp
    ContentInteractionDelegate.cpp
    ContentWindowManager.cpp
    ContentWindowInterface.cpp
    ContentWindowGraphicsItem.cpp
    ContentWindowListWidgetItem.cpp
    Marker.cpp
    DisplayGroupManager.cpp
    DisplayGroupInterface.cpp
    DisplayGroupGraphicsScene.cpp
    DisplayGroupGraphicsViewProxy.cpp
    DisplayGroupGraphicsView.cpp
    DisplayGroupListWidgetProxy.cpp
    DockPixelStreamer.cpp
    DynamicTexture.cpp
    DynamicTextureContent.cpp
    FactoryObject.cpp
    GLWindow.cpp
    globals.cpp
    log.cpp
    LocalPixelStreamer.cpp
    LocalPixelStreamerManager.cpp
    MainWindow.cpp
    Movie.cpp
    MovieContent.cpp
    NetworkListener.cpp
    NetworkListenerThread.cpp
    Options.cpp
    Pictureflow.cpp
    PixelStream.cpp
    PixelStreamContent.cpp
    PixelStreamInteractionDelegate.cpp
    PixelStreamSegmentRenderer.cpp
    PixelStreamSegmentJpegCompressor.cpp
    State.cpp
    StatePreview.cpp
    SVG.cpp
    SVGContent.cpp
    SVGStreamSource.cpp
    Texture.cpp
    TextureContent.cpp
    WebkitAuthenticationHelper.cpp
    WebkitPixelStreamer.cpp
    ZoomInteractionDelegate.cpp
    configuration/Configuration.cpp
    configuration/MasterConfiguration.cpp
    configuration/WallConfiguration.cpp
    thumbnail/DefaultThumbnailGenerator.cpp
    thumbnail/FolderThumbnailGenerator.cpp
    thumbnail/ImageThumbnailGenerator.cpp
    thumbnail/MovieThumbnailGenerator.cpp
    thumbnail/PyramidThumbnailGenerator.cpp
    thumbnail/StateThumbnailGenerator.cpp
    thumbnail/ThumbnailGenerator.cpp
    thumbnail/ThumbnailGeneratorFactory.cpp
)

list(APPEND MOC_HEADERS
    AsyncImageLoader.h
    BackgroundWidget.h
    Content.h
    ContentWindowInterface.h
    DisplayGroupManager.h
    DisplayGroupInterface.h
    DisplayGroupGraphicsViewProxy.h
    DisplayGroupListWidgetProxy.h
    DockPixelStreamer.h
    LocalPixelStreamer.h
    LocalPixelStreamerManager.h
    MainWindow.h
    Marker.h
    NetworkListener.h
    NetworkListenerThread.h
    Options.h
    Pictureflow.h
    WebkitAuthenticationHelper.h
    WebkitPixelStreamer.h
)

# Qt resource file
SET( CORE_RC
   resources/core.qrc
)
# this command will generate rules that will run rcc on all files from SAMPLE_RCS
# in result SAMPLE_RC_SRCS variable will contain paths to files produced by rcc
QT4_ADD_RESOURCES( CORE_RC_SRCS ${CORE_RC} )

# add OPTIONS hacks for Qt's MOC processing of Boost headers, for compiler errors with some versions of Boost
qt4_wrap_cpp(MOC_OUTFILES ${MOC_HEADERS} OPTIONS -DPOSIX_TIME_HPP___ -DPOSIX_TIME_SERIALIZE_HPP___ -DBOOST_TT_HAS_OPERATOR_HPP_INCLUDED)

add_library(DisplayClusterCore SHARED ${SRCS} ${MOC_OUTFILES} ${CORE_RC_SRCS} ${DISPLAYCLUSTER_LIBRARY_SHARED_HEADERS})
target_link_libraries(DisplayClusterCore ${LIBS})
set_target_properties(DisplayClusterCore PROPERTIES
  VERSION ${VERSION} SOVERSION ${VERSION_ABI} OUTPUT_NAME DisplayClusterCore)
install(TARGETS DisplayClusterCore
    ARCHIVE DESTINATION lib COMPONENT dev
    RUNTIME DESTINATION bin COMPONENT lib
    LIBRARY DESTINATION lib COMPONENT lib)

# build Python module if Python support is enabled
if(ENABLE_PYTHON_SUPPORT)
    add_custom_command(TARGET displaycluster POST_BUILD
        COMMENT "Building Python module"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/python
        COMMAND python configure.py -I ${Boost_INCLUDE_DIRS}
        COMMAND make
        COMMAND mv pydc.so ${CMAKE_BINARY_DIR})
endif()

# install Python module
if(ENABLE_PYTHON_SUPPORT)
  install(FILES ${CMAKE_BINARY_DIR}/pydc.so DESTINATION python)
endif()
