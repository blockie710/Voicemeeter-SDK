#!/bin/bash

# Development environment setup script for Voicemeeter Plugin Host on Linux/macOS

echo -e "\e[1;36mSetting up development environment for Voicemeeter Plugin Host...\e[0m"

# Detect OS
if [[ "$OSTYPE" == "darwin"* ]]; then
    OS_TYPE="macOS"
    echo "Detected macOS"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS_TYPE="Linux"
    echo "Detected Linux"
else
    echo "Unsupported OS: $OSTYPE"
    exit 1
fi

# Check if running with sudo/root
if [[ $EUID -ne 0 && "$OS_TYPE" == "Linux" ]]; then
    echo -e "\e[1;33mWarning: This script may need administrative privileges for package installation.\e[0m"
    echo "Consider running with sudo if package installation fails."
fi

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Install packages based on OS
if [[ "$OS_TYPE" == "macOS" ]]; then
    # Check for Homebrew
    if ! command_exists brew; then
        echo -e "\e[1;33mInstalling Homebrew...\e[0m"
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    else
        echo -e "\e[1;32mHomebrew already installed\e[0m"
    fi

    # Install packages
    echo -e "\e[1;33mInstalling required packages...\e[0m"
    brew install cmake ninja git wget
    brew install --cask visual-studio-code
elif [[ "$OS_TYPE" == "Linux" ]]; then
    # Detect distribution
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        DISTRO=$ID
    else
        echo "Cannot detect Linux distribution"
        exit 1
    fi

    # Install packages based on distribution
    case $DISTRO in
        ubuntu|debian)
            echo -e "\e[1;33mInstalling packages for Ubuntu/Debian...\e[0m"
            apt-get update
            apt-get install -y build-essential cmake ninja-build git wget unzip libx11-dev libxext-dev libxinerama-dev libxrandr-dev libxcursor-dev libxi-dev libasound2-dev
            ;;
        fedora)
            echo -e "\e[1;33mInstalling packages for Fedora...\e[0m"
            dnf install -y gcc-c++ cmake ninja-build git wget unzip libX11-devel libXext-devel libXinerama-devel libXrandr-devel libXcursor-devel libXi-devel alsa-lib-devel
            ;;
        *)
            echo "Unsupported Linux distribution: $DISTRO"
            echo "Please install the following packages manually:"
            echo "- C++ compiler (gcc/g++ or clang)"
            echo "- CMake"
            echo "- Ninja build system"
            echo "- Git"
            echo "- X11 development libraries"
            echo "- ALSA development libraries (for Linux)"
            ;;
    esac
fi

# Create directories for dependencies
SDK_DIR="./external/SDKs"
if [ ! -d "$SDK_DIR" ]; then
    echo -e "\e[1;33mCreating SDKs directory...\e[0m"
    mkdir -p "$SDK_DIR"
fi

# Download and extract VST3 SDK
VST3_SDK_PATH="$SDK_DIR/VST3_SDK"
if [ ! -d "$VST3_SDK_PATH" ]; then
    echo -e "\e[1;33mDownloading VST3 SDK...\e[0m"
    TMP_FILE="/tmp/vst3sdk.zip"
    wget -O "$TMP_FILE" "https://download.steinberg.net/sdk_downloads/vst-sdk_3.7.5_build-25_2021-12-14.zip"
    
    echo -e "\e[1;33mExtracting VST3 SDK...\e[0m"
    unzip -q "$TMP_FILE" -d "$SDK_DIR"
    mv "$SDK_DIR/VST_SDK" "$VST3_SDK_PATH" 2>/dev/null || true
    rm "$TMP_FILE"
    echo -e "\e[1;32mVST3 SDK installed\e[0m"
else
    echo -e "\e[1;32mVST3 SDK already installed\e[0m"
fi

# Set environment variables
echo -e "\e[1;33mSetting up environment variables...\e[0m"
export VST3_SDK_PATH=$(realpath "$VST3_SDK_PATH")
echo "export VST3_SDK_PATH=\"$VST3_SDK_PATH\"" >> ~/.bashrc

# Generate build files with CMake
echo -e "\e[1;33mGenerating build files...\e[0m"
BUILD_DIR="./build"
if [ ! -d "$BUILD_DIR" ]; then
    mkdir -p "$BUILD_DIR"
fi

pushd "$BUILD_DIR" > /dev/null
cmake -G Ninja ..
if [ $? -ne 0 ]; then
    echo -e "\e[1;31mFailed to generate build files\e[0m"
else
    echo -e "\e[1;32mBuild files generated successfully\e[0m"
fi
popd > /dev/null

echo -e "\e[1;32mDevelopment environment setup complete!\e[0m"
echo -e "\e[1;36mYou can now build the project by running:\e[0m"
echo "cd $BUILD_DIR && cmake --build ."
