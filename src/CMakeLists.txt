# process config file
configure_file(config.h.in ${CMAKE_BINARY_DIR}/config.h)
include_directories(${CMAKE_BINARY_DIR})

set(DISPLAYCLUSTER_LIBRARY_SHARED_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Event.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MessageHeader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/NetworkProtocol.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PixelStreamSegment.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PixelStreamSegmentParameters.h
)

if(BUILD_CORE_LIBRARY)
  add_subdirectory(core)
  # Build FastCGI web services library
  add_subdirectory(dcWebservice)
endif()

add_subdirectory(dcstream)
