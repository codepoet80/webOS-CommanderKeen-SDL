#!/bin/bash
# Build CloneKeen for HP webOS TouchPad (ARM Cortex-A8)
# Requires: Linaro arm-none-linux-gnueabi toolchain and HP webOS PDK

set -e

# --- Paths (edit these for your environment) ---
TOOLCHAIN_BIN="/home/stark/arm-2011.03/bin"
PDK="/home/stark/HPwebOS/PDK"

export PATH="$TOOLCHAIN_BIN:/usr/bin:/bin:$PATH"
CC=arm-none-linux-gnueabi-gcc
CXX=arm-none-linux-gnueabi-g++

SRC="../../src"
BUILDDIR="fbuild/webos"

# --- Compiler flags ---
CFLAGS="-O2 -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp"
CFLAGS="$CFLAGS -D__webos__ -DLINUX"
CFLAGS="$CFLAGS -I$PDK/include -I$PDK/include/SDL"
CFLAGS="$CFLAGS -I$SRC -Wall -fsigned-char -D_GNU_SOURCE=1 -D_REENTRANT"

CXXFLAGS="$CFLAGS"

LDFLAGS="-L$PDK/device/lib -lSDL -lSDL_mixer -lpdl -lz -lstdc++ -lm"

# --- Source file lists (mirrors the Linux Makefile) ---
C_SRCS="
  $SRC/main.c
  $SRC/sanity.c
  $SRC/game.c
  $SRC/gamedo.c
  $SRC/gamepdo.c
  $SRC/gamepdo_wm.c
  $SRC/editor/editor.c
  $SRC/editor/autolight.c
  $SRC/console.c
  $SRC/fileio.c
  $SRC/maploader.c
  $SRC/map.c
  $SRC/graphics.c
  $SRC/palette.c
  $SRC/fonts.c
  $SRC/misc.c
  $SRC/misc_ui.c
  $SRC/graphicmaker.c
  $SRC/ini.c
  $SRC/intro.c
  $SRC/menumanager.c
  $SRC/menu_options.c
  $SRC/menu_keysetup.c
  $SRC/menu_savegames.c
  $SRC/menu_custommap.c
  $SRC/editor/menu_editor.c
  $SRC/customepisode.c
  $SRC/savegame.c
  $SRC/twirly.c
  $SRC/sgrle.c
  $SRC/lprintf.c
  $SRC/vgatiles.c
  $SRC/latch.c
  $SRC/lz.c
  $SRC/message.c
  $SRC/cinematics/seqcommon.c
  $SRC/cinematics/e1ending.c
  $SRC/cinematics/e3ending.c
  $SRC/cinematics/blowupworld.c
  $SRC/cinematics/mortimer.c
  $SRC/cinematics/TBC.c
  $SRC/FinaleScreenLoader.c
  $SRC/globals.c
  $SRC/ai/yorp.c
  $SRC/ai/garg.c
  $SRC/ai/vort.c
  $SRC/ai/butler.c
  $SRC/ai/tank.c
  $SRC/ai/door.c
  $SRC/ai/ray.c
  $SRC/ai/icecannon.c
  $SRC/ai/teleport.c
  $SRC/ai/rope.c
  $SRC/ai/walker.c
  $SRC/ai/tankep2.c
  $SRC/ai/platform.c
  $SRC/ai/platvert.c
  $SRC/ai/vortelite.c
  $SRC/ai/se.c
  $SRC/ai/baby.c
  $SRC/ai/earth.c
  $SRC/ai/foob.c
  $SRC/ai/ninja.c
  $SRC/ai/meep.c
  $SRC/ai/sndwave.c
  $SRC/ai/mother.c
  $SRC/ai/fireball.c
  $SRC/ai/balljack.c
  $SRC/ai/nessie.c
  $SRC/ai/autoray.c
  $SRC/ai/gotpoints.c
  $SRC/sdl/keydrv.c
  $SRC/sdl/snddrv.c
  $SRC/sdl/timedrv.c
  $SRC/sdl/viddrv.c
  $SRC/scale2x/scalebit.c
  $SRC/scale2x/scale2x.c
  $SRC/scale2x/scale3x.c
  $SRC/scale2x/pixel.c
"

CPP_SRCS="$SRC/bootstrap.cpp"

mkdir -p "$BUILDDIR"

echo "=== CloneKeen webOS build ==="
echo "Toolchain : $TOOLCHAIN_BIN"
echo "PDK       : $PDK"
echo ""

OBJECTS=""

for src in $C_SRCS; do
    obj="$BUILDDIR/$(basename ${src%.c}.o)"
    if [ ! -f "$obj" ] || [ "$src" -nt "$obj" ]; then
        echo "CC  $src"
        $CC $CFLAGS -c "$src" -o "$obj"
    fi
    OBJECTS="$OBJECTS $obj"
done

for src in $CPP_SRCS; do
    obj="$BUILDDIR/$(basename ${src%.cpp}.o)"
    if [ ! -f "$obj" ] || [ "$src" -nt "$obj" ]; then
        echo "CXX $src"
        $CXX $CXXFLAGS -c "$src" -o "$obj"
    fi
    OBJECTS="$OBJECTS $obj"
done

echo ""
echo "Linking..."
$CXX $OBJECTS $LDFLAGS -o "$BUILDDIR/clonekeen"

echo ""
echo "Build complete: $BUILDDIR/clonekeen"
ls -lh "$BUILDDIR/clonekeen"
