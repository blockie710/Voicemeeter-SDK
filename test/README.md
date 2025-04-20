# Voicemeeter Plugin Host Testing

This directory contains tools to test the Voicemeeter Plugin Host functionality.

## Available Test Tools

1. **test_existing_plugin_host.cpp**: Comprehensive test suite for VMPluginHost class
2. **plugin_host_test.cpp**: Simple test for TestPluginHost class
3. **simple_plugin_test.cpp**: Easy-to-use command line tool for plugin testing
4. **test_plugin_host.sh**: Shell script to simplify running tests

## Building the Tests

```bash
# From the project root directory:
mkdir -p build && cd build
cmake ..
make

# The test executables will be in build/test/
```

## Using the Test Tools

### Running the Test Script

```bash
# Make the script executable
chmod +x test/test_plugin_host.sh

# Run the test script with a specific plugin
./test/test_plugin_host.sh /path/to/plugin.vst3 all

# Available test types: all, load, params, audio, perf
```

### Using the Simple Plugin Test Tool

The `simple_plugin_test` tool provides a convenient command-line interface for testing plugins:

```bash
# Scan a directory for plugins
./build/test/simple_plugin_test scan /path/to/plugins

# Show information about a specific plugin
./build/test/simple_plugin_test info /path/to/plugin.vst3

# List plugin parameters
./build/test/simple_plugin_test params /path/to/plugin.vst3

# Run audio processing test
./build/test/simple_plugin_test test /path/to/plugin.vst3

# Run all tests
./build/test/simple_plugin_test full /path/to/plugin.vst3
```

## Common Plugin Directories

- **Windows**:
  - VST3: `C:\Program Files\Common Files\VST3`
  - AAX: `C:\Program Files\Common Files\Avid\Audio\Plug-Ins`

- **macOS**:
  - VST3: `/Library/Audio/Plug-Ins/VST3`
  - AU: `/Library/Audio/Plug-Ins/Components`
  - AAX: `/Library/Application Support/Avid/Audio/Plug-Ins`

- **Linux**:
  - VST3: `/usr/lib/vst3`, `/usr/local/lib/vst3`, or `~/.vst3`
