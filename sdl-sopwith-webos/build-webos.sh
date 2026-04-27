#!/bin/bash
# Build script for Bos Wars on webOS

set -e

# Cross-compiler - prepend to PATH so we keep system tools
export PATH="/home/stark/arm-2011.03/bin:/usr/bin:/bin:$PATH"
CC=arm-none-linux-gnueabi-gcc
CXX=arm-none-linux-gnueabi-g++
AR=arm-none-linux-gnueabi-ar
RANLIB=arm-none-linux-gnueabi-ranlib

# Paths
PDK=/home/stark/HPwebOS/PDK
LUA=/home/stark/lua-5.1.5/src
BUILDDIR=fbuild/webos

# Compiler flags
CFLAGS="-O2 -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp"
CFLAGS="$CFLAGS -D__webos__ -DUSE_OPENGL -DHAVE_STRCASESTR -DHAVE_STRNLEN"
CFLAGS="$CFLAGS -I$PDK/include -I$PDK/include/SDL"
CFLAGS="$CFLAGS -I$LUA -Iengine/include -Iengine/guichan/include"
CFLAGS="$CFLAGS -Wall -fsigned-char -D_GNU_SOURCE=1 -D_REENTRANT"

CXXFLAGS="$CFLAGS"

LDFLAGS="-L$PDK/device/lib -L$LUA"
LDFLAGS="$LDFLAGS -lSDL -lSDL_mixer -lpdl -lGLES_CM -lpng12 -lz -llua -lm"

# Create build directory
mkdir -p $BUILDDIR

# Find all source files
SOURCES=$(find engine -name "*.cpp" | grep -v apbuild)

echo "Compiling Bos Wars for webOS..."
echo "Found $(echo $SOURCES | wc -w) source files"

# Compile each source file
for src in $SOURCES; do
    obj=$BUILDDIR/$(basename ${src%.cpp}.o)
    if [ ! -f "$obj" ] || [ "$src" -nt "$obj" ]; then
        echo "Compiling $src..."
        $CXX $CXXFLAGS -c "$src" -o "$obj"
    fi
done

# Link
echo "Linking..."
OBJECTS=$(ls $BUILDDIR/*.o)
$CXX $OBJECTS $LDFLAGS -o $BUILDDIR/boswars

echo "Build complete: $BUILDDIR/boswars"
