FROM ubuntu:18.04

ENV HOME /root
SHELL ["/bin/bash", "-c"]

RUN apt-get update
RUN apt-get update
RUN apt-get update

RUN apt-get -y --no-install-recommends install \
  build-essential \
  clang \
  gdb \
  wget \
  ssh \
  git \
  python \
  python-pip \
  vim \
  ffmpeg

RUN apt-get -y install \
  xterm \
  cmake \
  cmake-curses-gui \
  qt4-default \
  mpi-default-bin \
  mpi-default-dev \
  libboost-all-dev \
  libturbojpeg0-dev \
  zip \
  sudo \
  yasm \
  x11-apps 

RUN pip2 install cherrypy

COPY displaycluster-recipe displaycluster-recipe
RUN sh displaycluster-recipe
