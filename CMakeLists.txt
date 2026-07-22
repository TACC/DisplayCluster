project(DisplayCluster)

cmake_minimum_required(VERSION 3.16)

# version information
set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
set(VERSION_PATCH 0)

# show all warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

# Qt6 requires C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# automatically run moc on headers with Q_OBJECT, replacing the old manual
# MOC_HEADERS list + qt4_wrap_cpp() calls
set(CMAKE_AUTOMOC ON)

# workaround for moc choking while tokenizing complex Boost template headers
# pulled in transitively by some Q_OBJECT classes (e.g. DisplayGroupManager)
set(CMAKE_AUTOMOC_MOC_OPTIONS -DPOSIX_TIME_HPP___ -DPOSIX_TIME_SERIALIZE_HPP___ -DBOOST_TT_HAS_OPERATOR_HPP_INCLUDED)

# keep the library path in installed executables
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# build options

option(INSTALL_CLIENT "Install client-side bits package" OFF)
if (INSTALL_CLIENT)
    INSTALL(PROGRAMS scripts/startdisplaycluster DESTINATION bin)
    INSTALL(PROGRAMS python/DC.py python/Bounce.ipynb DESTINATION python)
    INSTALL(PROGRAMS examples/run_script.py examples/script examples/configuration.json DESTINATION examples)
endif()

# path for additional modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

# find and setup Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Svg Xml Concurrent OpenGL)
set(LIBS ${LIBS} Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::Svg Qt6::Xml Qt6::Concurrent Qt6::OpenGL)

# libjpeg-turbo
find_package(LibJpegTurbo REQUIRED)
include_directories(${LibJpegTurbo_INCLUDE_DIRS})
set(LIBS ${LIBS} ${LibJpegTurbo_LIBRARIES})

if (INSTALL_LAUNCHER)
    INSTALL(PROGRAMS scripts/startdisplaycluster DESTINATION bin)
endif()

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})
set(LIBS ${LIBS} ${OPENGL_LIBRARIES})

find_package(Boost REQUIRED COMPONENTS serialization date_time)
include_directories(${Boost_INCLUDE_DIRS})
set(LIBS ${LIBS} ${Boost_LIBRARIES})

find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})
set(LIBS ${LIBS} ${MPI_LIBRARIES})

find_package(FFMPEG REQUIRED)
include_directories(SYSTEM ${FFMPEG_INCLUDE_DIR}) # use SYSTEM to suppress FFMPEG compile warnings
set(LIBS ${LIBS} ${FFMPEG_LIBRARIES})

# CUDA driver API (cuda.h / cudaGL.h) for NVDEC decode + zero-copy CUDA-GL
# interop in Movie.cpp/Decoder.cpp; only needs the driver, not the full
# toolkit/nvcc, since nothing here is compiled as CUDA device code
find_path(CUDA_DRIVER_INCLUDE_DIR cuda.h
    PATHS /usr/local/cuda/targets/x86_64-linux/include /usr/local/cuda/include)
find_library(CUDA_DRIVER_LIBRARY cuda
    PATHS /usr/local/cuda/targets/x86_64-linux/lib /usr/local/cuda/lib64 /usr/lib64 /usr/lib64/stubs)
if(NOT CUDA_DRIVER_INCLUDE_DIR OR NOT CUDA_DRIVER_LIBRARY)
    message(FATAL_ERROR "CUDA driver API not found (cuda.h / libcuda) - required for NVDEC decode")
endif()
include_directories(SYSTEM ${CUDA_DRIVER_INCLUDE_DIR})
set(LIBS ${LIBS} ${CUDA_DRIVER_LIBRARY})

# process config file
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(SRCS ${SRCS}
    src/Configuration.cpp
    src/Content.cpp
    src/ContentWindowManager.cpp
    src/ContentWindowInterface.cpp
    src/ContentWindowGraphicsItem.cpp
    src/ContentWindowListWidgetItem.cpp
    src/Marker.cpp
    src/DCSocketManager.cpp
    src/Decoder.cpp
    src/DisplayGroupManager.cpp
    src/DisplayGroupInterface.cpp
    src/DisplayGroupGraphicsScene.cpp
    src/DisplayGroupGraphicsViewProxy.cpp
    src/DisplayGroupGraphicsView.cpp
    src/DisplayGroupListWidgetProxy.cpp
    src/DynamicTexture.cpp
    src/DynamicTextureContent.cpp
    src/FactoryObject.cpp
    src/GLWindow.cpp
    src/log.cpp
    src/main.cpp
    src/MainWindow.cpp
    src/Movie.cpp
    src/MovieContent.cpp
    src/NetworkListener.cpp
    src/NetworkListenerThread.cpp
    src/Options.cpp
    src/ParallelPixelStream.cpp
    src/ParallelPixelStreamContent.cpp
    src/PixelStream.cpp
    src/PixelStreamContent.cpp
    src/PixelStreamSource.cpp
    src/QSSApp.cpp
    src/SocketInterface.cpp
    src/SVG.cpp
    src/SVGContent.cpp
    src/SVGStreamSource.cpp
    src/Texture.cpp
    src/TextureContent.cpp
)

add_executable(displaycluster ${SRCS})

# nothing here calls pthreads directly anymore (std::thread/mutex/
# condition_variable only), but glibc's libstdc++ still wants it linked
# explicitly on Linux; MSVC's std::thread needs no such library
if(NOT WIN32)
    set(LIBS ${LIBS} pthread)
endif()

# SocketInterface.cpp uses Winsock2 on Windows (DCSocketManager's Python/
# DC.py control socket); ws2_32 also carries its own #pragma comment(lib)
# under MSVC, but link it here too for non-MSVC Windows toolchains
if(WIN32)
    set(LIBS ${LIBS} ws2_32)
endif()

target_link_libraries(displaycluster ${LIBS})

# install executable
INSTALL(TARGETS displaycluster RUNTIME DESTINATION bin)

# install launchers
INSTALL(PROGRAMS scripts/displaycluster.py DESTINATION bin)
INSTALL(PROGRAMS scripts/displaycluster.sh DESTINATION bin)

# install remote controller
INSTALL(DIRECTORY remote DESTINATION .)

# install data files
INSTALL(DIRECTORY data DESTINATION .)

# install example files
INSTALL(DIRECTORY examples DESTINATION .)

# install doc files
INSTALL(DIRECTORY doc DESTINATION .)

# install doc files
INSTALL(DIRECTORY python DESTINATION .)
