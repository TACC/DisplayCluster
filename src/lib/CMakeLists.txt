
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

set(DISPLAYCLUSTER_LIBRARY_LIBS ${QT_QTCORE_LIBRARY} ${QT_QTNETWORK_LIBRARY})
set(DISPLAYCLUSTER_LIBRARY_LIBS ${DISPLAYCLUSTER_LIBRARY_LIBS} ${LibJpegTurbo_LIBRARIES})

set(DISPLAYCLUSTER_LIBRARY_SRCS
    ../log.cpp
    DcSocket.cpp
    dcStream.cpp
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/dcVersion.in.h
  ${CMAKE_BINARY_DIR}/dcVersion.h)

set(DISPLAYCLUSTER_LIBRARY_PUBLIC_HEADERS
    dcStream.h
    ../InteractionState.h
    ${CMAKE_BINARY_DIR}/dcVersion.h
)

add_library(DisplayClusterLibrary SHARED ${DISPLAYCLUSTER_LIBRARY_SRCS})
target_link_libraries(DisplayClusterLibrary ${DISPLAYCLUSTER_LIBRARY_LIBS})
set_target_properties(DisplayClusterLibrary PROPERTIES
  VERSION ${VERSION} SOVERSION ${VERSION_ABI} OUTPUT_NAME DisplayCluster)

# install library
install(TARGETS DisplayClusterLibrary
    ARCHIVE DESTINATION lib COMPONENT dev
    RUNTIME DESTINATION bin COMPONENT lib
    LIBRARY DESTINATION lib COMPONENT lib)

# install headers
install(FILES ${DISPLAYCLUSTER_LIBRARY_PUBLIC_HEADERS} DESTINATION include)
