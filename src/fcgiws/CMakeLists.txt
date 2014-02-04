list(APPEND SOURCES
  DefaultHandler.cpp
  FastCGIWrapper.cpp
  Handler.cpp
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
)

list(APPEND PUBLIC_HEADERS)

add_library(fcgiws ${SOURCES} ${HEADERS})
target_link_libraries(fcgiws ${Boost_LIBRARIES}  ${FCGI_LIBRARIES})
