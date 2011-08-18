project(DisplayCluster)

cmake_minimum_required(VERSION 2.4.0)

# enable debugging, and show all warnings
set(CMAKE_CXX_FLAGS "-g -Wall")

# find and setup Qt4
# see http://cmake.org/cmake/help/cmake2.6docs.html#module:FindQt4 for details
find_package(Qt4 REQUIRED) 
set(QT_USE_QTOPENGL TRUE)
include(${QT_USE_FILE})
set(LIBS ${LIBS} ${QT_LIBRARIES})

# path for additional modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package(GLUT REQUIRED)
include_directories(${GLUT_INCLUDE_DIRS})
set(LIBS ${LIBS} ${GLUT_LIBRARIES})

set(SRCS
    src/main.cpp
)

set(MOC_HEADERS
)

qt4_wrap_cpp(MOC_OUTFILES ${MOC_HEADERS})

add_executable(displaycluster ${SRCS} ${MOC_OUTFILES})

target_link_libraries(displaycluster ${LIBS})
