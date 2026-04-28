#!/bin/bash
# Package CloneKeen for HP webOS TouchPad
# Produces an IPK installable via novacom

set -e

PKGDIR="webos-pkg"
APPID="com.cmdrkeen.game"
VERSION="1.0.0"
APPDIR="$PKGDIR/usr/palm/applications/$APPID"
OUTFILE="${APPID}_${VERSION}_armv7.ipk"
BINARY="fbuild/webos/clonekeen"
GAMEDATA="../../gamedata-extracted"   # extracted from Releases/Linux/clonekeen-linux.zip

echo "=== CloneKeen webOS packaging ==="

if [ ! -f "$BINARY" ]; then
    echo "ERROR: Binary not found at $BINARY — run build-webos.sh first"
    exit 1
fi

if [ ! -d "$GAMEDATA" ]; then
    echo "ERROR: Game data not found at $GAMEDATA"
    echo "  Run: cd ../../ && unzip Releases/Linux/clonekeen-linux.zip -d gamedata-extracted/ -x keen"
    exit 1
fi

# Clean and create package structure
rm -rf "$PKGDIR/usr" "$PKGDIR/data.tar.gz" "$PKGDIR/control.tar.gz"
mkdir -p "$APPDIR"

# Copy binary
echo "Copying binary..."
cp "$BINARY" "$APPDIR/"
arm-linux-gnueabi-strip "$APPDIR/clonekeen" 2>/dev/null || true

# Copy appinfo.json
cp "$PKGDIR/appinfo.json" "$APPDIR/"

# Copy all game data (data/, gfx/, custom/, *.ini, *.dat, *.conf, demo files)
echo "Copying game data..."
cp -r "$GAMEDATA/." "$APPDIR/"

# Copy icons from repo root
echo "Copying icons..."
cp "../../../icon.png" "$APPDIR/icon.png"
[ -f "../../../icon-256.png" ] && cp "../../../icon-256.png" "$APPDIR/icon-256.png"

# Build data.tar.gz
echo "Creating data archive..."
cd "$PKGDIR"
tar czf data.tar.gz usr
cd ..

# Build control.tar.gz
echo "Creating control archive..."
cd "$PKGDIR/CONTROL"
tar czf ../control.tar.gz .
cd ../..

# debian-binary version marker
echo "2.0" > "$PKGDIR/debian-binary"

# Assemble IPK (ar archive: debian-binary + control.tar.gz + data.tar.gz)
echo "Assembling IPK..."
cd "$PKGDIR"
ar -r "../$OUTFILE" debian-binary control.tar.gz data.tar.gz
cd ..

echo ""
echo "Package ready: $OUTFILE  ($(du -sh $OUTFILE | cut -f1))"
echo ""
echo "Install on device:"
echo "  novacom put file:///media/internal/$OUTFILE < $OUTFILE"
echo "  novacom run file:///usr/bin/ipkg install /media/internal/$OUTFILE"
