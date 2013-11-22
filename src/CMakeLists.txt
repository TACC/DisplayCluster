
# process config file
configure_file(config.h.in ${CMAKE_BINARY_DIR}/config.h)
include_directories(${CMAKE_BINARY_DIR})

include_directories(SYSTEM ${MPI_INCLUDE_PATH})
list(APPEND LIBS ${MPI_CXX_LIBRARIES})

set(DISPLAYCLUSTER_LIBRARY_SHARED_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/InteractionState.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MessageHeader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/NetworkProtocol.h
    ${CMAKE_CURRENT_SOURCE_DIR}/NonCopyable.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PixelStreamSegment.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PixelStreamSegmentParameters.h
)

if(BUILD_CORE_LIBRARY)
  add_subdirectory(core)
endif()

if(BUILD_DISPLAYCLUSTER_LIBRARY)
  add_subdirectory(dcstream)
endif()
