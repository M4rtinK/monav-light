#!/bin/sh

rm -fr al
mkdir al
cp -a lib*.so* al/

/usr/local/android-ndk-r5/toolchains/arm-linux-androideabi-4.4.3/prebuilt/linux-x86/bin/arm-linux-androideabi-strip --strip-unneeded al/* || echo "Can't find strip for android - pushing unstripped libs to device"

adb push al /data/local/qt/lib
