
set(DISPLAYCLUSTER_LIBRARY_LIBS ${QT_LIBRARIES} ${LibJpegTurbo_LIBRARIES})

set(DISPLAYCLUSTER_LIBRARY_SRCS
    ../Event.cpp
    ../log.cpp
    ../MessageHeader.cpp
    Socket.cpp
    Stream.cpp
    StreamPrivate.cpp
    ImageWrapper.cpp
    ImageSegmenter.cpp
    ImageJpegCompressor.cpp
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Version.in.h
               ${CMAKE_BINARY_DIR}/Version.h)

set(DISPLAYCLUSTER_LIBRARY_PUBLIC_HEADERS
    ImageWrapper.h
    Stream.h
    types.h
    ../Event.h
    ${CMAKE_BINARY_DIR}/Version.h
)

set(MOC_HEADERS Socket.h)
qt4_wrap_cpp(MOC_OUTFILES ${MOC_HEADERS})

add_library(dcstream SHARED ${DISPLAYCLUSTER_LIBRARY_SRCS} ${MOC_OUTFILES} ${DISPLAYCLUSTER_LIBRARY_SHARED_HEADERS})
target_link_libraries(dcstream ${DISPLAYCLUSTER_LIBRARY_LIBS})
set_target_properties(dcstream PROPERTIES
  VERSION ${VERSION} SOVERSION ${VERSION_ABI})

# install library
install(TARGETS dcstream
    ARCHIVE DESTINATION lib COMPONENT dev
    RUNTIME DESTINATION bin COMPONENT lib
    LIBRARY DESTINATION lib COMPONENT lib)

# install headers
install(FILES ${DISPLAYCLUSTER_LIBRARY_PUBLIC_HEADERS} DESTINATION include/dc COMPONENT dev)
