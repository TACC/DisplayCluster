FROM nvidia/opengl:1.0-glvnd-devel-ubuntu20.04

ENV DEBIAN_FRONTEND=noninteractive
ENV NVIDIA_VISIBLE_DEVICES all
ENV NVIDIA_DRIVER_CAPABILITIES graphics,utility,compute

RUN apt-get update && apt-get install -y -qq --no-install-recommends \
  build-essential \
  software-properties-common 

RUN add-apt-repository ppa:rock-core/qt4

RUN apt-get -y install \
  xterm \
  qt4-default 

RUN apt-get update \
  && apt-get install -y -qq --no-install-recommends \
    libglvnd0 \
    libgl1 \
    libglx0 \
    libegl1 \
    libxext6 \
    libx11-6 \
    glmark2 \
    mesa-utils \
    xterm \
    telnet \
    vim \
    bzip2 \
    cpio \
    curl \
    g++-8 \
    gcc-8 \
    gfortran \
    git \
    gosu \
    libblas-dev \
    liblapack-dev \
    python3-dev \
    python3-pip \
    virtualenv \
    wget \
    zlib1g-dev \
    vim       \
    tzdata \
    libpulse-dev \
    xterm \
    freeglut3-dev \
    libnss3 \
    libxcursor1  \
    libxcursor-dev  \
    libxi-dev  \
    libxkbcommon0 \
    libxrandr-dev \
    libxinerama-dev \
    libasound2 \
    libgssapi-krb5-2 \
    libqt5gui5 \
    htop \
    libglew-dev \
    libncurses5-dev \
    gdb \
    net-tools \
    liblzma-dev \
    openssl \
    libssl-dev \
    python2 \
    python2-dev  \
    xterm \
    qt4-default \
    libturbojpeg0-dev \
    zip \
    unzip \
    sudo \
    yasm \
    x11-apps

RUN apt-get -y install \
  ffmpeg  \
  libavcodec-dev \
  libavdevice-dev \
  libavfilter-dev \
  libavformat-dev \
  libavresample-dev \
  libavutil-dev \
  libpostproc-dev \
  libswresample-dev \
  libswscale-dev

RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-8 8 && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-8 8

RUN curl https://bootstrap.pypa.io/pip/2.7/get-pip.py --output get-pip.py
RUN python2 get-pip.py

RUN pip2 install setuptools
RUN pip2 install cherrypy

RUN wget https://github.com/Kitware/CMake/releases/download/v3.24.3/cmake-3.24.3.tar.gz \
  && tar xzf cmake-3.24.3.tar.gz \
  && cd cmake-3.24.3 \
  && ./bootstrap \
  && make -j 6 \
  && make install 

RUN wget https://download.open-mpi.org/release/open-mpi/v4.1/openmpi-4.1.1.tar.gz \
  && tar xzf openmpi-4.1.1.tar.gz \
  && cd openmpi-4.1.1 \
  && ./configure \
  && make -j 5 install

RUN mkdir -p /usr/local/src \
    && cd /usr/local/src \
    && git clone https://github.com/TACC/DisplayCluster.git \
    && cd DisplayCluster \
    && git checkout docker

RUN cd /usr/local/src/DisplayCluster/dependencies \
    && tar xzf sip-4.19.25.tar.gz \
    && cd sip-4.19.25 \
    && python2 configure.py \
    && make -j 5 install 

RUN cd /usr/local/src/DisplayCluster/dependencies \
    && tar xzf PyQt4_gpl_x11-4.12.3.tar.gz \
    && cd PyQt4_gpl_x11-4.12.3 \
    && python2 configure.py --confirm-license \
    && make -j 4 \
    && make install 

RUN cd /usr/local/src/DisplayCluster/dependencies \
    && unzip PythonQt2.0.1.DC.zip \
    && cd PythonQt2.0.1 \
    && sed -i -e "s/python2.6/python2.7/" -e "s/python26/python27/" build/python.prf \
    && sed -i -e "s?ast.h?parser/ast.h?" generator/abstractmetabuilder.cpp \
    && qmake QMAKE_CXXFLAGS=-I/usr/include/python2.7 \
    && sed -i -e "s/python2.6/python2.7/" -e "s/python26/python27/" build/python.prf \
    && make -j 4 \
    && cp lib/libPythonQt.so.1.0.0 /usr/local/lib \
    && ln -s /usr/local/lib/libPythonQt.so.1.0.0 /usr/local/lib/libPythonQt.so \
    && ln -s /usr/local/lib/libPythonQt.so.1.0.0 /usr/local/lib/libPythonQt.so.1 \
    && ln -s /usr/local/lib/libPythonQt.so.1.0.0 /usr/local/lib/libPythonQt.so.1.0 

RUN cd /usr/local/src/DisplayCluster/dependencies \
    && tar xzf boost_1_53_0.tar.gz \
    && cd boost_1_53_0 \
    && ./bootstrap.sh --with-libraries=serialization,date_time \
    && ./b2 -j 4 install 

ENV LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
ENV PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH

COPY cfg.dc /usr/local/src/DisplayCluster

RUN cd /usr/local/src/DisplayCluster \
  && mkdir build && cd build \
  && sh ../cfg.dc \
    && make -j 5 install
