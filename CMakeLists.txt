project(DisplayCluster)

cmake_minimum_required(VERSION 2.4.0)

# required for mixing libraries with full paths and not...
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

# enable debugging, and show all warnings
set(CMAKE_CXX_FLAGS "-g -Wall")

# build options
option(ENABLE_TUIO_TOUCH_LISTENER "Enable TUIO touch listener for multi-touch events" OFF)

# find and setup Qt4
# see http://cmake.org/cmake/help/cmake2.6docs.html#module:FindQt4 for details
find_package(Qt4 REQUIRED) 
set(QT_USE_QTOPENGL TRUE)
set(QT_USE_QTXMLPATTERNS TRUE)
include(${QT_USE_FILE})
set(LIBS ${LIBS} ${QT_LIBRARIES})

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})
set(LIBS ${LIBS} ${OPENGL_LIBRARIES})

find_package(GLUT REQUIRED)
include_directories(${GLUT_INCLUDE_DIRS})
set(LIBS ${LIBS} ${GLUT_LIBRARIES})

find_package(Boost REQUIRED COMPONENTS serialization)
include_directories(${Boost_INCLUDE_DIRS})
set(LIBS ${LIBS} ${Boost_LIBRARIES})

include_directories("/usr/include/mpi")
set(LIBS ${LIBS} mpi mpi_cxx)

# handle build options
if(ENABLE_TUIO_TOUCH_LISTENER)
    include_directories("/usr/local/include/TUIO")
    set(LIBS ${LIBS} TUIO)
    set(SRCS ${SRCS} src/TouchListener.cpp)
endif()

# process config file
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(SRCS ${SRCS}
    src/Configuration.cpp
    src/Content.cpp
    src/ContentGraphicsItem.cpp
    src/Cursor.cpp
    src/DisplayGroup.cpp
    src/DisplayGroupGraphicsScene.cpp
    src/DisplayGroupGraphicsView.cpp
    src/DynamicTexture.cpp
    src/GLWindow.cpp
    src/log.cpp
    src/main.cpp
    src/MainWindow.cpp
    src/TextureFactory.cpp
)

set(MOC_HEADERS
    src/MainWindow.h
)

qt4_wrap_cpp(MOC_OUTFILES ${MOC_HEADERS})

add_executable(displaycluster ${SRCS} ${MOC_OUTFILES})

target_link_libraries(displaycluster ${LIBS})
