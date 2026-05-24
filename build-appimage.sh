#!/bin/bash
set -e

cd ~/Dev/LMAO

echo "Building LMAO AppImage in Docker (Ubuntu 24.04)..."

docker run --rm \
  -v "$PWD":/src:z \
  -w /src \
  -e NO_STRIP=true \
  ubuntu:24.04 bash -c '
set -e

echo "=== Installing dependencies ==="
apt-get update -qq
apt-get install -y -qq cmake g++ \
  qt6-base-dev qt6-base-dev-tools libgl-dev \
  libmpv-dev libdbus-1-dev \
  libfuse2 file wget patchelf > /dev/null

echo "=== Building LMAO ==="
rm -rf /tmp/build
cmake -B /tmp/build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build /tmp/build -j$(nproc)

echo "=== Creating AppDir ==="
rm -rf /tmp/AppDir
DESTDIR=/tmp/AppDir cmake --install /tmp/build

echo "=== Downloading linuxdeploy ==="
cd /tmp
wget -q https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
wget -q https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
chmod +x linuxdeploy*.AppImage

# Extract both since FUSE is unavailable in Docker
./linuxdeploy-x86_64.AppImage --appimage-extract
mv squashfs-root linuxdeploy
./linuxdeploy-plugin-qt-x86_64.AppImage --appimage-extract
mv squashfs-root linuxdeploy-plugin-qt

# Put the plugin where linuxdeploy can find it
cp linuxdeploy-plugin-qt/AppRun /tmp/linuxdeploy-plugin-qt-x86_64.AppImage

echo "=== Building AppImage ==="
export QMAKE=$(which qmake6)
export APPIMAGE_EXTRACT_AND_RUN=1
./linuxdeploy/AppRun \
  --appdir /tmp/AppDir \
  --desktop-file /src/dev.elek.LMAO.desktop \
  --icon-file /src/icons/hicolor/256x256/apps/dev.elek.LMAO.png \
  --plugin qt \
  --output appimage

echo "=== Copying output ==="
mv -f /tmp/LMAO*.AppImage /src/LMAO-x86_64.AppImage 2>/dev/null || true
'

echo ""
echo "Done: ./LMAO-x86_64.AppImage"
ls -lh LMAO-x86_64.AppImage