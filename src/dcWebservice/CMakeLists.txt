list(APPEND SOURCES
  DefaultHandler.cpp
  FastCGIWrapper.cpp
  Mapper.cpp
  RequestBuilder.cpp
  Response.cpp
  Server.cpp
)

list(APPEND HEADERS
  DefaultHandler.h
  FastCGIWrapper.h
  Handler.h
  Mapper.h
  Request.h
  RequestBuilder.h
  Response.h
  Server.h
  types.h
)

list(APPEND PUBLIC_HEADERS)

add_library(fcgiws SHARED ${SOURCES} ${HEADERS})
target_link_libraries(fcgiws ${Boost_LIBRARIES}  ${FCGI_LIBRARIES})

# install library
install(TARGETS fcgiws
    ARCHIVE DESTINATION lib COMPONENT dev
    RUNTIME DESTINATION bin COMPONENT lib
    LIBRARY DESTINATION lib COMPONENT lib)

