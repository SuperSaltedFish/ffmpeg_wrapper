#!/bin/bash

#git clone https://git.ffmpeg.org/ffmpeg.git
#cd ffmpeg

./configure \
--enable-small \
--enable-rpath \
--enable-shared \
--enable-pic \
--disable-static \
--disable-symver \
--disable-version-tracking \
--disable-debug \
--disable-programs \
--disable-doc \
--install-name-dir=@rpath

make -j8

