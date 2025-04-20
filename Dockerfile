FROM ubuntu:22.04

# Avoid prompts from apt
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    pkg-config \
    git \
    libasound2-dev \
    libpulse-dev \
    libjack-jackd2-dev \
    libgl1-mesa-dev \
    libx11-dev \
    libxext-dev \
    libxrandr-dev \
    libxcursor-dev \
    libxinerama-dev \
    libxi-dev \
    libxcomposite-dev \
    libdbus-1-dev \
    libudev-dev \
    libfontconfig1-dev \
    clang \
    lld \
    llvm \
    ccache \
    nsis \
    curl \
    zip \
    unzip \
    python3 \
    python3-pip \
    libgtk-3-dev \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Install cross-compilation tools for Windows targets (MinGW)
RUN apt-get update && apt-get install -y \
    mingw-w64 \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Install nlohmann-json for modern JSON handling
RUN apt-get update && apt-get install -y \
    nlohmann-json3-dev \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Set up working directory
WORKDIR /workspace

# Entry point - default to bash
CMD ["/bin/bash"]