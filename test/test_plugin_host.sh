#!/bin/bash

# Test script for Voicemeeter Plugin Host
# Usage: ./test_plugin_host.sh [plugin_path] [test_type]

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/../build"
TEST_EXECUTABLE="${BUILD_DIR}/test/test_plugin_host"
DEFAULT_PLUGIN_PATH=""

# Set default plugin path based on OS
if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS
    DEFAULT_PLUGIN_PATH="/Library/Audio/Plug-Ins/Components"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    # Linux
    DEFAULT_PLUGIN_PATH="/usr/lib/vst3"
else
    # Windows
    DEFAULT_PLUGIN_PATH="C:\\Program Files\\Common Files\\VST3"
fi

# Parse arguments
PLUGIN_PATH=${1:-$DEFAULT_PLUGIN_PATH}
TEST_TYPE=${2:-"all"}

# Verify test executable exists
if [ ! -f "$TEST_EXECUTABLE" ]; then
    echo "Error: Test executable not found at $TEST_EXECUTABLE"
    echo "Please build the project first using:"
    echo "  mkdir -p build && cd build && cmake .. && make"
    exit 1
fi

# Run the test
echo "Running plugin host test with:"
echo "  Plugin path: $PLUGIN_PATH"
echo "  Test type: $TEST_TYPE"
echo ""

"$TEST_EXECUTABLE" "$PLUGIN_PATH" "$TEST_TYPE"
