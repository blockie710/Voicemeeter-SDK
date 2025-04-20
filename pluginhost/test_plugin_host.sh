#!/bin/bash

echo "Starting Plugin Host test suite..."
cd "$(dirname "$0")"

# Check if Node.js is installed
if ! command -v node &> /dev/null; then
    echo "Error: Node.js is not installed. Please install Node.js to run the tests."
    exit 1
fi

# Run the test script
echo "Running plugin host tests..."
node test_plugin_host.js

# Check the exit code
if [ $? -eq 0 ]; then
    echo "Plugin Host test completed successfully."
else
    echo "Plugin Host test failed!"
fi
