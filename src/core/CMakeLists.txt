
include_directories(SYSTEM ${OPENGL_INCLUDE_DIRS} ${FFMPEG_INCLUDE_DIR})
list(APPEND CORE_LIBRARY_LIBS ${OPENGL_LIBRARIES} ${FFMPEG_LIBRARIES})

include_directories(SYSTEM ${MPI_INCLUDE_PATH})
list(APPEND CORE_LIBRARY_LIBS ${MPI_CXX_LIBRARIES})

list(APPEND CORE_LIBRARY_LIBS ${QT_LIBRARIES})
list(APPEND CORE_LIBRARY_LIBS ${LibJpegTurbo_LIBRARIES})
list(APPEND CORE_LIBRARY_LIBS ${Boost_LIBRARIES})

#OpenMP
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")

# FastCGI WebService
list(APPEND CORE_LIBRARY_LIBS fcgiws)

# handle build options
if(ENABLE_TUIO_TOUCH_LISTENER)
  list(APPEND CORE_LIBRARY_LIBS ${TUIO_LIBRARIES})

  list(APPEND SRCS MultiTouchListener.cpp
                   TouchListener.cpp)

  find_package(X11 REQUIRED)
  include_directories(${X11_INCLUDE_DIR})
  list(APPEND CORE_LIBRARY_LIBS ${X11_LIBRARIES})
endif()

if(ENABLE_JOYSTICK_SUPPORT)
  find_package(SDL REQUIRED)
  include_directories(${SDL_INCLUDE_DIR})
  list(APPEND CORE_LIBRARY_LIBS ${SDL_LIBRARY})

  set(SRCS ${SRCS} JoystickThread.cpp)
  set(SRCS ${SRCS} DisplayGroupJoystick.cpp)

  set(MOC_HEADERS ${MOC_HEADERS} JoystickThread.h)
endif()

if(ENABLE_SKELETON_SUPPORT)
  find_package(OpenNI REQUIRED)
  include_directories(SYSTEM ${OpenNI_INCLUDE_DIRS}) # use SYSTEM to suppress OpenNI compile warnings
  list(APPEND CORE_LIBRARY_LIBS ${OpenNI_LIBRARIES})

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
    list(APPEND CORE_LIBRARY_LIBS ${PYTHON_LIBRARIES})

    find_package(PythonQt REQUIRED)
    include_directories(${PythonQt_INCLUDE_DIRS})
    list(APPEND CORE_LIBRARY_LIBS ${PythonQt_LIBRARIES})

    set(SRCS ${SRCS} DisplayGroupPython.cpp)
    set(SRCS ${SRCS} PythonConsole.cpp)

    set(MOC_HEADERS ${MOC_HEADERS} DisplayGroupPython.h)
    set(MOC_HEADERS ${MOC_HEADERS} PythonConsole.h)
endif()

if(ENABLE_PDF_SUPPORT)
  list(APPEND CORE_LIBRARY_LIBS ${POPPLER_LIBRARIES})

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

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

list(APPEND SRCS
    BackgroundWidget.cpp
    Command.cpp
    CommandHandler.cpp
    CommandType.cpp
    Content.cpp
    ContentFactory.cpp
    ContentLoader.cpp
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
    DynamicTexture.cpp
    DynamicTextureContent.cpp
    FactoryObject.cpp
    FileCommandHandler.cpp
    FpsCounter.cpp
    GLWindow.cpp
    globals.cpp
    gestures/DoubleTapGestureRecognizer.cpp
    gestures/PanGesture.cpp
    gestures/PanGestureRecognizer.cpp
    gestures/PinchGesture.cpp
    gestures/PinchGestureRecognizer.cpp
    ../Event.cpp
    ../log.cpp
    ../MessageHeader.cpp
    ImageJpegDecompressor.cpp
    MainWindow.cpp
    Movie.cpp
    MovieContent.cpp
    NetworkListener.cpp
    NetworkListenerThread.cpp
    Options.cpp
    PixelStream.cpp
    PixelStreamBuffer.cpp
    PixelStreamContent.cpp
    PixelStreamDispatcher.cpp
    PixelStreamInteractionDelegate.cpp
    PixelStreamSegmentDecoder.cpp
    PixelStreamSegmentRenderer.cpp
    SessionCommandHandler.cpp
    State.cpp
    StatePreview.cpp
    StateSerializationHelper.cpp
    SVG.cpp
    SVGContent.cpp
    Texture.cpp
    TextureContent.cpp
    WebbrowserCommandHandler.cpp
    ZoomInteractionDelegate.cpp
    configuration/Configuration.cpp
    configuration/MasterConfiguration.cpp
    configuration/WallConfiguration.cpp
    localstreamer/AsyncImageLoader.cpp
    localstreamer/CommandLineOptions.cpp
    localstreamer/DockPixelStreamer.cpp
    localstreamer/DockToolbar.cpp
    localstreamer/PixelStreamer.cpp
    localstreamer/PixelStreamerFactory.cpp
    localstreamer/PixelStreamerLauncher.cpp
    localstreamer/PixelStreamerType.cpp
    localstreamer/Pictureflow.cpp
    localstreamer/WebkitAuthenticationHelper.cpp
    localstreamer/WebkitPixelStreamer.cpp
    thumbnail/DefaultThumbnailGenerator.cpp
    thumbnail/FolderThumbnailGenerator.cpp
    thumbnail/ImageThumbnailGenerator.cpp
    thumbnail/MovieThumbnailGenerator.cpp
    thumbnail/PyramidThumbnailGenerator.cpp
    thumbnail/StateThumbnailGenerator.cpp
    thumbnail/ThumbnailGenerator.cpp
    thumbnail/ThumbnailGeneratorFactory.cpp
    ws/AsciiToQtKeyCodeMapper.cpp
    ws/DisplayGroupManagerAdapter.cpp
    ws/TextInputDispatcher.cpp
    ws/TextInputHandler.cpp
    ws/WebServiceServer.cpp
)

list(APPEND MOC_HEADERS
    BackgroundWidget.h
    CommandHandler.h
    Content.h
    ContentWindowInterface.h
    DisplayGroupManager.h
    DisplayGroupInterface.h
    DisplayGroupGraphicsView.h
    DisplayGroupGraphicsViewProxy.h
    DisplayGroupListWidgetProxy.h
    EventReceiver.h
    MainWindow.h
    Marker.h
    NetworkListener.h
    NetworkListenerThread.h
    Options.h
    PixelStreamDispatcher.h
    WebbrowserCommandHandler.h
    localstreamer/AsyncImageLoader.h
    localstreamer/DockPixelStreamer.h
    localstreamer/PixelStreamer.h
    localstreamer/PixelStreamerLauncher.h
    localstreamer/Pictureflow.h
    localstreamer/WebkitAuthenticationHelper.h
    localstreamer/WebkitPixelStreamer.h
    ws/TextInputDispatcher.h
    ws/TextInputHandler.h
    ws/WebServiceServer.h
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
target_link_libraries(DisplayClusterCore ${CORE_LIBRARY_LIBS})
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
  install(FILES ${CMAKE_BINARY_DIR}/pydc.so DESTINATION python COMPONENT lib)
endif()
