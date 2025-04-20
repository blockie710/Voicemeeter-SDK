#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <filesystem>
#include "../PluginHost/include/plugin_interface.h"

// Test for creating plugin scanners for different formats
bool testCreatePluginScanners() {
    std::cout << "Testing plugin scanner creation..." << std::endl;
    
    // Check VST3 scanner
    auto vst3Scanner = createPluginScanner(PluginFormat::VST3);
    if (!vst3Scanner) {
        std::cerr << "Failed to create VST3 scanner" << std::endl;
        return false;
    }
    
    // Check AAX scanner
    auto aaxScanner = createPluginScanner(PluginFormat::AAX);
    if (!aaxScanner) {
        std::cerr << "Failed to create AAX scanner" << std::endl;
        return false;
    }
    
    // Check ARA scanner
    auto araScanner = createPluginScanner(PluginFormat::ARA);
    if (!araScanner) {
        std::cerr << "Failed to create ARA scanner" << std::endl;
        return false;
    }
    
    // Check REAPER scanner
    auto reaperScanner = createPluginScanner(PluginFormat::REAPER);
    if (!reaperScanner) {
        std::cerr << "Failed to create REAPER scanner" << std::endl;
        return false;
    }
    
    // Check LUA scanner
    auto luaScanner = createPluginScanner(PluginFormat::LUA);
    if (!luaScanner) {
        std::cerr << "Failed to create LUA scanner" << std::endl;
        return false;
    }
    
    // AAU scanner will be platform-dependent
    #ifdef __APPLE__
    auto aauScanner = createPluginScanner(PluginFormat::AAU);
    if (!aauScanner) {
        std::cerr << "Failed to create AAU scanner on macOS" << std::endl;
        return false;
    }
    #endif
    
    std::cout << "All available plugin scanners created successfully" << std::endl;
    return true;
}

// Test plugin scanner directory scanning
bool testScanDirectory(const std::string& directory, PluginFormat format) {
    std::cout << "Testing directory scanning for " << directory << "..." << std::endl;
    
    auto scanner = createPluginScanner(format);
    if (!scanner) {
        std::cout << "Scanner not available for this platform" << std::endl;
        return true; // Not a failure, just not supported
    }
    
    try {
        auto results = scanner->scanDirectory(directory);
        std::cout << "Found " << results.size() << " plugins" << std::endl;
        
        for (const auto& plugin : results) {
            std::cout << "  " << plugin.name << " (" << plugin.path << ")" << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error during directory scanning: " << e.what() << std::endl;
        return false;
    }
    
    return true;
}

// Main entry point
int main(int argc, char** argv) {
    std::cout << "Starting plugin scanner tests..." << std::endl;
    
    // Test scanner creation
    if (!testCreatePluginScanners()) {
        std::cerr << "Plugin scanner creation test failed" << std::endl;
        return 1;
    }
    
    // Test directory scanning with provided paths or defaults
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            std::string path = argv[i];
            std::string ext = std::filesystem::path(path).extension().string();
            
            if (ext == ".vst3" || path.find("VST3") != std::string::npos) {
                testScanDirectory(path, PluginFormat::VST3);
            }
            else if (ext == ".aaxplugin" || path.find("AAX") != std::string::npos) {
                testScanDirectory(path, PluginFormat::AAX);
            }
            else if (ext == ".component" || path.find("Components") != std::string::npos) {
                testScanDirectory(path, PluginFormat::AAU);
            }
            else {
                // Try all formats
                testScanDirectory(path, PluginFormat::VST3);
                testScanDirectory(path, PluginFormat::AAX);
                testScanDirectory(path, PluginFormat::AAU);
                testScanDirectory(path, PluginFormat::ARA);
                testScanDirectory(path, PluginFormat::REAPER);
                testScanDirectory(path, PluginFormat::LUA);
            }
        }
    }
    else {
        // Test with default paths
        #ifdef _WIN32
        testScanDirectory("C:\\Program Files\\Common Files\\VST3", PluginFormat::VST3);
        testScanDirectory("C:\\Program Files\\Common Files\\Avid\\Audio\\Plug-Ins", PluginFormat::AAX);
        #elif defined(__APPLE__)
        testScanDirectory("/Library/Audio/Plug-Ins/VST3", PluginFormat::VST3);
        testScanDirectory("/Library/Audio/Plug-Ins/Components", PluginFormat::AAU);
        #else
        testScanDirectory("/usr/lib/vst3", PluginFormat::VST3);
        testScanDirectory("/usr/local/lib/vst3", PluginFormat::VST3);
        #endif
    }
    
    std::cout << "Plugin scanner tests completed" << std::endl;
    return 0;
}
