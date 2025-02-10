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

RUN echo 5

RUN mkdir -p /usr/local/src \
    && cd /usr/local/src \
    && git clone https://github.com/TACC/DisplayCluster.git \
    && cd DisplayCluster \
    && git checkout docker

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
   && sh ../cfg.dc  \
   && make -j 5 install \
   && echo done
