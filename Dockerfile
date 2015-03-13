FROM debian:stable

#
#	En cas d'erreur suivante : error: 'UINT64_C' was not declared in this scope
# 	Compiler  g++ -D__STDC_CONSTANT_MACROS subshot-detector.cc -o subshot-detector `pkg-config --cflags --libs opencv`  -I../../include -lswscale -lavdevice -lavformat -lavcodec -lavutil -lswresample -lz
#
#
#

RUN apt-get update && apt-get -y upgrade
RUN apt-get install -y git wget build-essential cmake zlib1g-dev libx11-dev pkg-config unzip  libconfig++-dev

RUN cd /opt/ && wget https://www.ffmpeg.org/releases/ffmpeg-1.2.12.tar.gz && tar -xvf ffmpeg-1.2.12.tar.gz

RUN cd /opt/ffmpeg-1.2.12/ && ./configure --disable-yasm --enable-shared && make && make install

RUN cd /opt/ && wget http://sourceforge.net/projects/opencvlibrary/files/opencv-unix/2.4.10/opencv-2.4.10.zip/download

RUN cd /opt/ && ldconfig && unzip download && cd /opt/opencv-2.4.10 && cmake . && make && make install && ldconfig

RUN cd /opt/ && git clone https://github.com/meriembendris/opencv-utils.git

# Compilation 
RUN cd /opt/opencv-utils/src/shots &&  g++ -D__STDC_CONSTANT_MACROS subshot-detector.cc -o subshot-detector `pkg-config --cflags --libs opencv`  -I../../include -lswscale -lavdevice -lavformat -lavcodec -lavutil -lswresample -lz

RUN cd /opt/opencv-utils/src/shots &&  g++ -D__STDC_CONSTANT_MACROS shot-type.cc -o shot-type `pkg-config --cflags --libs opencv`  -I../../include -lswscale -lavdevice -lavformat -lavcodec -lavutil -lswresample -lz

RUN cd /opt/opencv-utils/src/shots &&  g++ -D__STDC_CONSTANT_MACROS subshot-from-template.cc -o subshot-from-template `pkg-config --cflags --libs opencv`  -I../../include -lswscale -lavdevice -lavformat -lavcodec -lavutil -lswresample -lz

RUN cd /opt/opencv-utils/src/shots &&  g++ -D__STDC_CONSTANT_MACROS  view-shot-boundaries.cc -o view-shot-boundaries `pkg-config --cflags --libs opencv`  -I../../include -lswscale -lavdevice -lavformat -lavcodec -lavutil -lswresample -lz

RUN cd /opt/opencv-utils/src/shots &&  g++ -D__STDC_CONSTANT_MACROS view-shot-clustering.cc -o view-shot-clustering `pkg-config --cflags --libs opencv`  -I../../include -lswscale -lavdevice -lavformat -lavcodec -lavutil -lswresample -lz

RUN cd /opt/opencv-utils/src/utils &&  g++ -D__STDC_CONSTANT_MACROS  play-video.cc -o play-video `pkg-config --cflags --libs opencv`  -I../../include -lswscale -lavdevice -lavformat -lavcodec -lavutil -lswresample -lz