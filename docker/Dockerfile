FROM nvidia/opengl:1.0-glvnd-devel-ubuntu22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV NVIDIA_VISIBLE_DEVICES all
ENV NVIDIA_DRIVER_CAPABILITIES graphics,utility,compute

RUN apt-get update && apt-get install -y -qq --no-install-recommends \
  build-essential \
  cmake \
  pkg-config \
  git \
  ca-certificates \
  gnupg \
  wget \
  software-properties-common

# Qt6 - available natively on Ubuntu 22.04+, no PPA needed. Qt4 required
# the rock-core/qt4 PPA (which only targeted old Ubuntu releases) - that
# whole dance is gone now that the app is off Qt4.
RUN apt-get install -y -qq --no-install-recommends \
  qt6-base-dev \
  libqt6svg6-dev \
  libqt6opengl6-dev

RUN apt-get update \
  && apt-get install -y -qq --no-install-recommends \
    libglvnd0 \
    libgl1 \
    libglx0 \
    libegl1 \
    libglu1-mesa-dev \
    libxext6 \
    libx11-6 \
    libxcursor1 \
    libxcursor-dev \
    libxi-dev \
    libxkbcommon0 \
    libxrandr-dev \
    libxinerama-dev \
    glmark2 \
    mesa-utils \
    xterm \
    telnet \
    vim \
    bzip2 \
    cpio \
    curl \
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
    tzdata \
    libnss3 \
    libgssapi-krb5-2 \
    htop \
    libglew-dev \
    libncurses5-dev \
    gdb \
    net-tools \
    liblzma-dev \
    openssl \
    libssl-dev \
    libturbojpeg0-dev \
    zip \
    unzip \
    sudo \
    yasm \
    x11-apps

# Boost - previously hand-built from a vendored 2013-era 1.53 tarball
# (dependencies/boost_1_53_0.tar.gz - now unused, safe to remove from the
# repo separately) with only serialization/date_time enabled, which was
# actually incomplete: DisplayGroupManager.cpp also needs boost::iostreams
# for message deserialization, silently relying on it being present some
# other way. Ubuntu's packaged Boost is recent enough and covers
# everything this codebase uses in one step; libboost-dev brings in the
# header-only pieces (algorithm, tokenizer, smart_ptr) that don't have
# their own dev packages.
RUN apt-get install -y -qq --no-install-recommends \
    libboost-dev \
    libboost-serialization-dev \
    libboost-date-time-dev \
    libboost-iostreams-dev

# MPI - previously built from source (openmpi-4.1.1) because Ubuntu
# 20.04's packaged version was dated; a current Ubuntu base makes that
# unnecessary.
RUN apt-get install -y -qq --no-install-recommends \
    libopenmpi-dev \
    openmpi-bin

# FFmpeg with NVDEC hwaccel for src/Decoder.cpp's hardware video decode.
# Confirm after building with `ffmpeg -hwaccels` (want "cuda" in the
# list) - if this Ubuntu release's packaged ffmpeg doesn't have it
# compiled in, swap this block for a prebuilt NVDEC-enabled binary
# instead (see BUILD.md's Windows section, which hit exactly this gap).
RUN apt-get install -y -qq --no-install-recommends \
  ffmpeg \
  libavcodec-dev \
  libavformat-dev \
  libavutil-dev \
  libswscale-dev

# CUDA Toolkit - needed for the driver API headers (cuda.h, cudaGL.h)
# src/Decoder.cpp and src/Movie.cpp use for NVDEC decode and the
# zero-copy CUDA-GL interop path. The actual runtime driver
# (libcuda.so, matching the host's installed driver version) is
# supplied at container *run* time by the NVIDIA Container Toolkit via
# NVIDIA_VISIBLE_DEVICES above, not baked into this image - the
# cuda-toolkit package here only needs to provide a linkable stub for
# the build itself.
RUN wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2204/x86_64/cuda-keyring_1.1-1_all.deb \
  && dpkg -i cuda-keyring_1.1-1_all.deb \
  && rm cuda-keyring_1.1-1_all.deb \
  && apt-get update \
  && apt-get install -y -qq --no-install-recommends cuda-toolkit

ENV PATH=/usr/local/cuda/bin:$PATH
ENV CUDA_PATH=/usr/local/cuda

RUN mkdir -p /usr/local/src \
    && cd /usr/local/src

# Copies the repo checkout this Dockerfile lives in, rather than
# git-cloning a fresh copy from GitHub - build context must be the repo
# root for this to work (COPY can't reach outside it), so build with:
#   docker build -f docker/Dockerfile -t displaycluster .
# from the repo root, not from inside docker/.
COPY . /usr/local/src/DisplayCluster

ENV LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
ENV PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH

RUN cd /usr/local/src/DisplayCluster \
   && mkdir build && cd build \
   && sh ../docker/cfg.dc  \
   && make -j 5 install \
   && echo done
