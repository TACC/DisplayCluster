FROM nvidia/opengl:1.0-glvnd-devel-ubuntu22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV NVIDIA_VISIBLE_DEVICES all
ENV NVIDIA_DRIVER_CAPABILITIES graphics,utility,compute

RUN apt-get update && apt-get install -y -qq --no-install-recommends \
  build-essential 

RUN apt-get update \
  && apt-get install -y -qq --no-install-recommends \
    libglvnd0 \
    libgl1 \
    libglx0 \
    libegl1 \
    libxext6 \
    libx11-6 \
    glmark2  \
    mesa-utils 

RUN apt-get update \
  && apt-get install -y -qq --no-install-recommends \
     xterm \
     telnet \
     bzip2 \
     cpio  \
     curl \
     g++ \
     gcc \
     git  \
     gosu \
     wget \
     zlib1g-dev \
     vim \
     tzdata \
     xterm

RUN apt-get update \
  && apt-get install -y -qq --no-install-recommends \
    freeglut3-dev \
    libxcursor1 \
    libxcursor-dev  \
    libxi-dev  \
    libxkbcommon0 \
    libxrandr-dev \
    libxinerama-dev \
    libqt5gui5 \
    htop \
    libglew-dev \
    libncurses5-dev \
    gdb \
    net-tools \
    liblzma-dev \
    xterm \
    libturbojpeg0-dev \
    zip \
    unzip \
    sudo \
    yasm \
    x11-apps \
    cmake

RUN apt-get update \
  && apt-get install -y -qq --no-install-recommends \
  ffmpeg  \
  libavcodec-dev \
  libavdevice-dev \
  libavfilter-dev \
  libavformat-dev \
  libavutil-dev \
  libpostproc-dev \
  libswresample-dev \
  libswscale-dev

RUN apt-get install -y python3 python3-dev python3-pip

COPY openmpi-4.1.1.tar.gz /usr/local/src
 
RUN cd /usr/local/src \
  && tar xzf openmpi-4.1.1.tar.gz \
  && cd openmpi-4.1.1 \
  && ./configure \
  && make -j 5 install

RUN pip3 install sip cherrypy setuptools

RUN apt-get install -y \
  qtbase5-dev \
  qtbase5-private-dev \
  qt5-qmake \
  cmake \
  cmake-curses-gui \
  libqt5svg5-dev \
  libqt5xmlpatterns5-dev

COPY pythonqt.doit /usr/local/src

RUN cd /usr/local/src \
  && git clone https://github.com/MeVisLab/pythonqt.git  \
  && sh pythonqt.doit

RUN apt-get install -y python3-pyqt5 libboost-all-dev dbus-x11 python3-sip

COPY git/DisplayCluster.tgz /usr/local/src
RUN cd /usr/local/src && tar xf DisplayCluster.tgz

RUN cd /usr/local/src/DisplayCluster \
  && rm -rf build \
  && mkdir -p build \
  && cd build  \
  && cmake .. \
   -DCMAKE_BUILD_TYPE=Debug \
   -DENABLE_PYTHON_SUPPORT=ON  \
   -DBUILD_DISPLAYCLUSTER=ON  \
   -DPYTHON_INCLUDE_DIR=/usr/include/python3.10  \
   -DPYTHON_LIBRARY=/usr/lib/x86_64-linux-gnu/libpython3.10.so \
  && make -j 6 install 


