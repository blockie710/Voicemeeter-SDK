#!/bin/bash
set -e

# Build script for Voicemeeter-SDK
# Usage: ./build.sh [linux|windows|plugin-host|package|clean]

# Default to linux build
BUILD_TYPE=${1:-linux}

# Detect number of CPU cores for parallel builds
if [ -f /proc/cpuinfo ]; then
    NUM_CORES=$(grep -c ^processor /proc/cpuinfo)
else
    NUM_CORES=4  # Default if we can't detect
fi

echo "Building Voicemeeter-SDK with target: $BUILD_TYPE using $NUM_CORES cores"

case $BUILD_TYPE in
    linux)
        echo "Building for Linux..."
        mkdir -p build-linux
        cd build-linux
        cmake .. -DCMAKE_BUILD_TYPE=Release
        cmake --build . --parallel $NUM_CORES
        ;;
    windows)
        echo "Building for Windows..."
        mkdir -p build-win64
        cd build-win64
        cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/x86_64-w64-mingw32.cmake
        cmake --build . --parallel $NUM_CORES
        ;;
    plugin-host)
        echo "Building Plugin Host only..."
        mkdir -p build-plugin
        cd build-plugin
        cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_PLUGIN_HOST=ON -DBUILD_EXAMPLE0=OFF -DBUILD_MATRIX8X8=OFF -DBUILD_VMR_OSD=OFF -DBUILD_VMR_PLAY=OFF -DBUILD_VMR_STREAMER=OFF
        cmake --build . --parallel $NUM_CORES
        ;;
    package)
        echo "Creating packages..."
        mkdir -p build-package
        cd build-package
        cmake .. -DCMAKE_BUILD_TYPE=Release
        cmake --build . --parallel $NUM_CORES
        cpack -G "TGZ;ZIP"
        echo "Packages created in build-package directory"
        ;;
    clean)
        echo "Cleaning build directories..."
        rm -rf build-linux build-win64 build-plugin build-package
        echo "Clean complete"
        ;;
    *)
        echo "Unknown build target: $BUILD_TYPE"
        echo "Available targets: linux, windows, plugin-host, package, clean"
        exit 1
        ;;
esac

echo "Build completed successfully!"
exit 0