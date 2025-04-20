#include "../include/plugin_interface.h"
#include "../include/vst3_plugin.h"
#include "../include/aax_plugin.h"
#include "../include/aau_plugin.h"

#include <memory>
#include <vector>

// Return list of supported plugin formats
std::vector<PluginFormat> PluginInstance::getSupportedFormats() {
    std::vector<PluginFormat> formats;
    formats.push_back(PluginFormat::VST3);
    formats.push_back(PluginFormat::AAX);
    
    // AAU is macOS-only but we include it for cross-platform compatibility
    formats.push_back(PluginFormat::AAU);
    
    return formats;
}

// Create a plugin scanner for the specified format
std::unique_ptr<PluginScanner> createPluginScanner(PluginFormat format) {
    switch (format) {
        case PluginFormat::VST3:
            return std::make_unique<VST3PluginScanner>();
        case PluginFormat::AAX:
            return std::make_unique<AAXPluginScanner>();
        case PluginFormat::AAU:
            return std::make_unique<AAUPluginScanner>();
        default:
            return nullptr;
    }
}