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
GAMEDATA="../../bin"        # relative to build/webos/
GAMEDATA2="../../data"      # original episode data (KEEN1/)

echo "=== CloneKeen webOS packaging ==="

if [ ! -f "$BINARY" ]; then
    echo "ERROR: Binary not found at $BINARY — run build-webos.sh first"
    exit 1
fi

# Clean and create package structure
rm -rf "$PKGDIR/usr" "$PKGDIR/data.tar.gz" "$PKGDIR/control.tar.gz"
mkdir -p "$APPDIR"

# Copy binary
echo "Copying binary..."
cp "$BINARY" "$APPDIR/"
arm-none-linux-gnueabi-strip "$APPDIR/clonekeen" 2>/dev/null || true

# Copy appinfo.json
cp "$PKGDIR/appinfo.json" "$APPDIR/"

# Copy game data files (bin/ directory: gfx, sound, config, demo files)
echo "Copying game data..."
cp -r "$GAMEDATA/." "$APPDIR/"

# Copy original episode data (EGAHEAD.CK1, etc.)
if [ -d "$GAMEDATA2/GAMEDATA/KEEN1" ]; then
    echo "Copying episode 1 data..."
    mkdir -p "$APPDIR/GAMEDATA/KEEN1"
    cp "$GAMEDATA2/GAMEDATA/KEEN1/"* "$APPDIR/GAMEDATA/KEEN1/"
fi

# Placeholder icon if none exists
if [ ! -f "$APPDIR/icon.png" ]; then
    echo "WARNING: No icon.png found — add one at build/webos/webos-pkg/icon.png"
    # copy a small placeholder if available
    [ -f "$PKGDIR/icon.png" ] && cp "$PKGDIR/icon.png" "$APPDIR/"
fi

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
