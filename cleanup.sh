#!/bin/bash

echo "Starting cleanup of temporary files in the Voicemeeter-SDK repository..."

# Define the root directory
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

echo "Removing build artifacts..."
# Remove common build directories and files
find . -type d -name "build" -exec rm -rf {} +
find . -type d -name "dist" -exec rm -rf {} +
find . -type d -name "out" -exec rm -rf {} +
find . -type d -name "__pycache__" -exec rm -rf {} +
find . -type d -name ".pytest_cache" -exec rm -rf {} +
find . -type d -name ".mypy_cache" -exec rm -rf {} +
find . -type d -name "node_modules" -exec rm -rf {} +

echo "Removing temporary files..."
# Remove temporary files
find . -type f -name "*.tmp" -delete
find . -type f -name "*.temp" -delete
find . -type f -name "*.swp" -delete
find . -type f -name "*.swo" -delete
find . -type f -name "*~" -delete
find . -type f -name "*.bak" -delete

echo "Removing log files..."
# Remove log files
find . -type f -name "*.log" -delete
find . -type f -name "npm-debug.log*" -delete
find . -type f -name "yarn-debug.log*" -delete
find . -type f -name "yarn-error.log*" -delete

echo "Removing cache files..."
# Remove cache files and directories
find . -type d -name ".cache" -exec rm -rf {} +
find . -type f -name ".DS_Store" -delete
find . -type d -name "__pycache__" -exec rm -rf {} +

# Clean up Python specific artifacts
echo "Cleaning up Python artifacts..."
find . -name "*.pyc" -delete
find . -name "*.pyo" -delete
find . -name "*.pyd" -delete
find . -name ".coverage" -delete
find . -name "coverage.xml" -delete
find . -name ".coverage.*" -delete

echo "Cleanup completed!"
echo "If you need to keep any of these file types, please restore them from version control."
