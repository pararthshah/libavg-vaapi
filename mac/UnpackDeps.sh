#!/bin/bash
if [[ x"${AVG_PATH}" == "x" ]]
then
    echo Please set AVG_PATH before calling this script.
    exit -1 
fi

cd $AVG_PATH/deps

echo "Unpacking libavg dependencies."
for file in $(ls tarballs/*.bz2); do
    echo "  Unpacking $file."
    tar xjf $file
done

for file in $(ls tarballs/*.gz); do
    echo "  Unpacking $file."
    tar xzf $file
done

echo "  Copying ffmpeg."
rm -rf ffmpeg
cp -pR tarballs/ffmpeg ffmpeg

echo "  Applying patches."
cd gettext-0.18.1.1
patch -p0 <  ../../libavg/mac/stpncpy.patch
cd ..
cd fontconfig-2.7.0
patch -R Makefile.am <../../libavg/mac/fontconfig-disablecache.patch
patch fontconfig.pc.in < ../../libavg/mac/fontconfig.pc.in.patch
cd ..
cd glib-2.29.2/glib
patch -R gconvert.c < ../../../libavg/mac/glib.patch
cd ../..
cd ffmpeg/libswscale
patch -p0 <../../../libavg/mac/ffmpeg.broken-yuv.patch
cd ../..
cd boost_1_41_0/tools/build/v2/tools
patch -R darwin.jam < ../../../../../../libavg/mac/boost-lion.patch
cd ../../../../..
#cd freetype-2.4.4/
#patch -p1 < ../../libavg/mac/freetype_linespacing.patch
#cd ..
echo "Done"
