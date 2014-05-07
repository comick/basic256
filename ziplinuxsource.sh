#!/bin/bash
# ziplinuxsource.sh
# create a zip file of the trunk source for uploading to sourceforge
# rewrote to better align with the creation of debian build folders 2014-01-23 j.m.reneau

make clean

VERSION=`cat Version.h | grep "#define VERSION " | sed -e 's/#define VERSION "\(.*\)/\1/;s/ .*//'`

rm -Rf ../basic256-$VERSION
mkdir ../basic256-$VERSION

cp -r * ../basic256-$VERSION

rm ../basic256-$VERSION/*.sqlite3
rm ../basic256-$VERSION/*.kbs
rm ../basic256-$VERSION/*.bin
rm ../basic256-$VERSION/*.pdf

rm -R ../basic256-$VERSION/.*
rm -R ../basic256-$VERSION/*~
rm -R ../basic256-$VERSION/BASIC256
rm -R ../basic256-$VERSION/BASIC256Portable/App/
rm -R ../basic256-$VERSION/BASIC256PortableDebug/App/
rm -R ../basic256-$VERSION/basic256
rm -R ../basic256-$VERSION/debug
rm -R ../basic256-$VERSION/tmp
rm -R ../basic256-$VERSION/tmp_portable_debug
rm -R ../basic256-$VERSION/tmp_portable_release
rm -R ../basic256-$VERSION/release

tar -cvf ../basic256_$VERSION.orig.tar ../basic256-$VERSION
gzip ../basic256_$VERSION.orig.tar
