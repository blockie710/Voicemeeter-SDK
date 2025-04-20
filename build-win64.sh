#!/bin/bash
#
# Cross-compilation script for building Voicemeeter Plugin Host for Windows 64-bit
#

# Exit on error
set -e

# Directory setup
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build-win64"
TOOLCHAIN_FILE="${SCRIPT_DIR}/cmake/toolchains/x86_64-w64-mingw32.cmake"
SOURCE_DIR="${SCRIPT_DIR}/PluginHost"

# Create build directory if it doesn't exist
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# Check if MinGW is installed
if ! command -v x86_64-w64-mingw32-g++ &> /dev/null; then
    echo "ERROR: MinGW cross-compiler (x86_64-w64-mingw32-g++) not found."
    echo "Please install it with: sudo apt-get install mingw-w64"
    exit 1
fi

echo "===== Configuring Voicemeeter Plugin Host for Windows 64-bit ====="
# Configure with CMake
cmake -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}" \
      -DCMAKE_BUILD_TYPE=Release \
      "${SOURCE_DIR}"

echo "===== Building Voicemeeter Plugin Host for Windows 64-bit ====="
# Build the project
cmake --build . -- -j$(nproc)

echo "===== Creating Windows package ====="
# Create the package
cpack -G "ZIP;NSIS"

echo "===== Build complete! ====="
echo "The Windows 64-bit executable can be found at:"
echo "${BUILD_DIR}/bin/VoicemeeterPluginHost.exe"

# Check if the build was successful
if [ -f "${BUILD_DIR}/bin/VoicemeeterPluginHost.exe" ]; then
    echo "Build succeeded!"
else
    echo "Build failed! Executable not found."
    exit 1
fi