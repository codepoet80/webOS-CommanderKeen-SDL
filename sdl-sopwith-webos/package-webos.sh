#!/bin/bash
# Package Bos Wars for webOS

set -e

PKGDIR=webos-pkg
APPID=com.boswars.game
APPDIR=$PKGDIR/usr/palm/applications/$APPID
OUTFILE=${APPID}_2.8.0_armv7.ipk

echo "Creating package structure..."
rm -rf $PKGDIR/usr $PKGDIR/data.tar.gz $PKGDIR/control.tar.gz
mkdir -p $APPDIR

# Copy binary
echo "Copying binary..."
cp fbuild/webos/boswars $APPDIR/

# Copy game data
echo "Copying game data..."
for dir in campaigns graphics intro languages maps scripts sounds units; do
    cp -r $dir $APPDIR/
done

# Copy appinfo.json
cp $PKGDIR/appinfo.json $APPDIR/

# Create a placeholder icon if missing
if [ ! -f "$APPDIR/icon.png" ]; then
    echo "Note: No icon.png found - creating placeholder"
    # Create a minimal 64x64 PNG placeholder (just copy any existing icon or skip)
fi

# Create data.tar.gz
echo "Creating data archive..."
cd $PKGDIR
tar czf data.tar.gz usr
cd ..

# Create control.tar.gz
echo "Creating control archive..."
cd $PKGDIR/CONTROL
tar czf ../control.tar.gz .
cd ../..

# Create debian-binary
echo "2.0" > $PKGDIR/debian-binary

# Create IPK (ar archive)
echo "Creating IPK package..."
cd $PKGDIR
ar -r ../$OUTFILE debian-binary control.tar.gz data.tar.gz
cd ..

echo ""
echo "Package created: $OUTFILE"
ls -la $OUTFILE
echo ""
echo "To install on webOS device:"
echo "  novacom put file:///media/internal/$OUTFILE < $OUTFILE"
echo "  novacom run file:///usr/bin/ipkg install /media/internal/$OUTFILE"
