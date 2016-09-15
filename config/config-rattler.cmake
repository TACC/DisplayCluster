#! /bin/bash

CC=mpicc CXX=mpicxx cmake ../DisplayCluster  \
	-DCMAKE_INSTALL_PREFIX=/share/apps/displaycluster/displayClusterRattler \
	-DBUILD_DESKTOPSTREAMER=ON  \
	-DBUILD_DISPLAYCLUSTER=ON  \
	-DBUILD_DISPLAYCLUSTER_LIBRARY=ON  \
	-DFFMPEG_INCLUDE_DIR=/share/apps/ffmpeg/1.1.2/include  \
	-DENABLE_PYTHON_SUPPORT=ON \
	-DPythonQt_INCLUDE_DIR=/home/vislab/Projects/DisplayClusterAndrew/PythonQt2.0.1/src \
	-DPythonQt_LIBRARY=/home/vislab/Projects/DisplayClusterAndrew/PythonQt2.0.1/lib/libPythonQt.so \
	-DFFMPEG_INCLUDE_DIR=/share/apps/ffmpeg/1.1.2/include	\
	-DFFMPEG_avcodec_LIBRARY=/share/apps/ffmpeg/1.1.2/lib/libavcodec.a	\
	-DFFMPEG_avformat_LIBRARY=/share/apps/ffmpeg/1.1.2/lib/libavformat.a	\
	-DFFMPEG_avutil_LIBRARY=/share/apps/ffmpeg/1.1.2/lib/libavutil.a	\
	-DFFMPEG_swscale_LIBRARY=/share/apps/ffmpeg/1.1.2/lib/libswscale.a

