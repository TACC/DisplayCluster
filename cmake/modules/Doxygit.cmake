# used by DoxygenRule.cmake, don't use directly

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake/oss)
find_package(Git)

if(NOT GIT_EXECUTABLE)
  return()
endif()
include(UpdateFile)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/index.html"
"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd\">\n"
"<html>\n"
"  <head>\n"
"    <title>${CMAKE_PROJECT_NAME} API Documentation</title>\n"
"    <link rel=\"stylesheet\" href=\"CMake/github.css\" type=\"text/css\">"
"  </head>\n"
"  <body>\n"
"  <h1>API Documentation</h1>"
"  <table>\n"
)

file(GLOB ENTRIES RELATIVE ${CMAKE_SOURCE_DIR} *-*)
list(SORT ENTRIES)
set(MAX_VERSIONS 0)
set(LAST_PROJECT)

# Create project and version lists
foreach(ENTRY ${ENTRIES})
  string(REGEX REPLACE "^(.+)-.+$" "\\1" PROJECT ${ENTRY})
  if(PROJECT STREQUAL LAST_PROJECT)
    math(EXPR VERSIONS "${VERSIONS} + 1")
  else()
    if(VERSIONS GREATER MAX_VERSIONS)
      set(MAX_VERSIONS ${VERSIONS})
    endif()
    set(VERSIONS 1)
    set(LAST_PROJECT ${PROJECT})
  endif()
endforeach()

# generate version table
set(GIT_DOCUMENTATION_INSTALL)
set(LAST_PROJECT)
set(VERSIONS 0)

file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/index.html"
  "    <tr><th>Project</th><th colspan=\"${MAX_VERSIONS}\">Versions</th>")
if(DOXYGIT_PROJECT_EXTRA)
  file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/index.html" "<th>Status</th></tr>\n")
endif()

macro(DOXYGIT_WRITE_ROW)
  # start row
  file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/index.html"
    "    <tr><th><a href=\"https://github.com/${CMAKE_PROJECT_NAME}/${LAST_PROJECT}\">${LAST_PROJECT}</a></th>${CONTENT}")

  # fill unused versions
  math(EXPR FILL_VERSIONS "${MAX_VERSIONS} - ${VERSIONS}")
  if(FILL_VERSIONS)
    file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/index.html"
      "<th colspan=\"${FILL_VERSIONS}\"></th>")
  endif()

  # CI status
  if(DOXYGIT_PROJECT_EXTRA AND LAST_PROJECT)
    string(REPLACE "[PROJECT]" "${LAST_PROJECT}" DOXYGIT_PROJECT_EXTRA_
      ${DOXYGIT_PROJECT_EXTRA})
    file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/index.html"
      "<th>${DOXYGIT_PROJECT_EXTRA_}</th>")
  endif()

  # finish row
  file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/index.html" "</tr>\n")
endmacro()

foreach(ENTRY ${ENTRIES})
  string(REGEX REPLACE "^(.+)-.+$" "\\1" PROJECT ${ENTRY})
  string(REGEX REPLACE "^.+-(.+)$" "\\1" VERSION ${ENTRY})

  if(NOT PROJECT STREQUAL LAST_PROJECT) # finish and start new table row
    if(LAST_PROJECT)
      doxygit_write_row()
    endif()
    # reset
    set(VERSIONS 0)
    set(LAST_PROJECT ${PROJECT})
    set(CONTENT)
  endif()

  math(EXPR VERSIONS "${VERSIONS} + 1")
  set(CONTENT
    "<td><a href=\"${ENTRY}/index.html\">${VERSION}</a></td>${CONTENT}")
  list(APPEND GIT_DOCUMENTATION_INSTALL ${ENTRY})
endforeach()

doxygit_write_row()

file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/index.html"
"  </table>\n"
"  <h1>Project Dependencies</h1>"
"  <a href=\"images/all.png\"><img src=\"images/all.png\" width=100%></a>"
"  </body>\n"
"</html>\n"
)

update_file("${CMAKE_CURRENT_BINARY_DIR}/index.html"
  "${CMAKE_SOURCE_DIR}/index.html")

execute_process(COMMAND "${GIT_EXECUTABLE}" add images ${ENTRIES}
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

 # hack to detect that not invoked as script and not under CI
if(VERSION_MAJOR)
  if(NOT "$ENV{TRAVIS}")
    foreach(FOLDER ${GIT_DOCUMENTATION_INSTALL})
      install(DIRECTORY ${FOLDER} DESTINATION share/${CMAKE_PROJECT_NAME}
        CONFIGURATIONS Release)
    endforeach()
  endif()
  install(FILES index.html DESTINATION share/${CMAKE_PROJECT_NAME}
    CONFIGURATIONS Release)
endif()
