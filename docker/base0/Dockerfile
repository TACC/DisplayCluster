FROM gregabram/ubuntu-20

ENV HOME /root
SHELL ["/bin/bash", "-c"]

RUN apt-get update

RUN apt-get -y --no-install-recommends install \
  build-essential \
  software-properties-common \
  python2 \
  python2-dev \
  curl

RUN curl https://bootstrap.pypa.io/pip/2.7/get-pip.py --output get-pip.py
RUN python2 get-pip.py

RUN add-apt-repository ppa:rock-core/qt4

RUN apt-get -y install \
  ffmpeg  \
  libavcodec-dev \
  libavcodec58 \
  libavdevice-dev \
  libavdevice58 \
  libavfilter-dev \
  libavfilter7 \
  libavformat-dev \
  libavformat58 \
  libavresample-dev \
  libavresample4 \
  libavutil-dev \
  libavutil56 \
  libpostproc-dev \
  libpostproc55 \
  libswresample-dev \
  libswresample3 \
  libswscale-dev \
  libswscale5

RUN apt-get -y install \
  xterm \
  qt4-default \
  libturbojpeg0-dev \
  zip \
  sudo \
  yasm \
  x11-apps 

RUN pip2 install setuptools
RUN pip2 install cherrypy

COPY base0-recipe base0-recipe
RUN sh base0-recipe
