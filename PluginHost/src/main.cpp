#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <thread>
#include <chrono>
#include <map>

// Include our header files
#include "../include/plugin_interface.h"
#include "../include/voicemeeter_integration.h"
#include "../include/vst3_plugin.h"
#include "../include/aax_plugin.h"
#include "../include/aau_plugin.h"

// Platform-specific includes for window handling
#ifdef _WIN32
#include <windows.h>
#else
// macOS or Linux includes would go here
#endif

// Defines for window creation
#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 768
#define WINDOW_TITLE "Voicemeeter Plugin Host"

// Global state
std::unique_ptr<VoicemeeterIntegration::VoicemeeterClient> g_voicemeeterClient;
std::map<std::string, std::shared_ptr<PluginInstance>> g_loadedPlugins;
bool g_running = true;

// Forward declarations
bool initializeApplication();
void shutdownApplication();
void processAudio(float** inputs, float** outputs, int numInputs, int numOutputs, int numSamples);
bool scanForPlugins(const std::string& directory, PluginFormat format);
bool loadPlugin(const std::string& path, PluginFormat format);
void displayPluginList();
void displayVoicemeeterInfo();

#ifdef _WIN32
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif

// Main entry point
int main(int argc, char** argv) {
    std::cout << "Voicemeeter Plugin Host - Starting..." << std::endl;
    
    if (!initializeApplication()) {
        std::cerr << "Failed to initialize application. Exiting." << std::endl;
        return 1;
    }
    
    // Display Voicemeeter information
    displayVoicemeeterInfo();
    
    // Scan for plugins in default directories
    std::cout << "Scanning for plugins..." << std::endl;
    
    // Default plugin directories
    #ifdef _WIN32
    scanForPlugins("C:\\Program Files\\Common Files\\VST3", PluginFormat::VST3);
    scanForPlugins("C:\\Program Files\\Common Files\\Avid\\Audio\\Plug-Ins", PluginFormat::AAX);
    #else
    // macOS paths would go here for AAU
    #endif
    
    // Display found plugins
    displayPluginList();
    
    // Main message loop
    std::cout << "Starting audio processing..." << std::endl;
    
    // Start Voicemeeter audio processing
    g_voicemeeterClient->startAudioProcessing();
    
    // Main application loop
    #ifdef _WIN32
    MSG msg = {};
    while (g_running && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    #else
    // Simple console loop for non-Windows platforms
    std::string input;
    while (g_running) {
        std::cout << "Enter 'q' to quit: ";
        std::getline(std::cin, input);
        if (input == "q") g_running = false;
    }
    #endif
    
    // Clean up
    shutdownApplication();
    
    return 0;
}

// Initialize application components
bool initializeApplication() {
    // Create Voicemeeter client
    g_voicemeeterClient = std::make_unique<VoicemeeterIntegration::VoicemeeterClient>();
    
    // Initialize Voicemeeter connection
    if (!g_voicemeeterClient->initialize()) {
        std::cerr << "Failed to connect to Voicemeeter. Is it running?" << std::endl;
        
        // Try to launch Voicemeeter if not running
        std::cout << "Attempting to launch Voicemeeter..." << std::endl;
        if (!g_voicemeeterClient->launchVoicemeeter(VoicemeeterIntegration::VoicemeeterType::POTATO_X64)) {
            std::cerr << "Failed to launch Voicemeeter." << std::endl;
            return false;
        }
        
        // Wait for Voicemeeter to start
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        // Try to initialize again
        if (!g_voicemeeterClient->initialize()) {
            std::cerr << "Failed to connect to Voicemeeter after launching." << std::endl;
            return false;
        }
    }
    
    // Register audio callback
    g_voicemeeterClient->registerAudioCallback(processAudio);
    
    // Initialize UI (minimal implementation for now)
    #ifdef _WIN32
    // Register window class
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "VoicemeeterPluginHostClass";
    
    RegisterClass(&wc);
    
    // Create main window
    HWND hwnd = CreateWindowEx(
        0,                              // Optional window styles
        "VoicemeeterPluginHostClass",   // Window class
        WINDOW_TITLE,                   // Window text
        WS_OVERLAPPEDWINDOW,            // Window style
        CW_USEDEFAULT, CW_USEDEFAULT,   // Position
        WINDOW_WIDTH, WINDOW_HEIGHT,    // Size
        NULL,                           // Parent window    
        NULL,                           // Menu
        GetModuleHandle(NULL),          // Instance handle
        NULL                            // Additional data
    );
    
    if (hwnd == NULL) {
        std::cerr << "Failed to create window." << std::endl;
        return false;
    }
    
    ShowWindow(hwnd, SW_SHOW);
    #endif
    
    return true;
}

// Clean up and shut down
void shutdownApplication() {
    // Stop Voicemeeter audio processing
    if (g_voicemeeterClient) {
        g_voicemeeterClient->stopAudioProcessing();
        g_voicemeeterClient->shutdown();
    }
    
    // Unload all plugins
    g_loadedPlugins.clear();
    
    std::cout << "Voicemeeter Plugin Host - Shutdown complete." << std::endl;
}

// Audio processing callback
void processAudio(float** inputs, float** outputs, int numInputs, int numOutputs, int numSamples) {
    // Apply each loaded plugin to the audio
    for (auto& pair : g_loadedPlugins) {
        auto& plugin = pair.second;
        plugin->processBlock(inputs, outputs, numInputs, numOutputs, numSamples);
    }
}

// Scan for plugins in a directory
bool scanForPlugins(const std::string& directory, PluginFormat format) {
    // Create scanner for the specified format
    auto scanner = createPluginScanner(format);
    if (!scanner) {
        std::cerr << "Failed to create plugin scanner for format: " << static_cast<int>(format) << std::endl;
        return false;
    }
    
    // Scan for plugins
    auto pluginPaths = scanner->scanDirectory(directory, format);
    std::cout << "Found " << pluginPaths.size() << " plugins in " << directory << std::endl;
    
    // Try to load the first few plugins as an example
    size_t loadCount = std::min(pluginPaths.size(), size_t(5));
    for (size_t i = 0; i < loadCount; i++) {
        loadPlugin(pluginPaths[i], format);
    }
    
    return true;
}

// Load a specific plugin
bool loadPlugin(const std::string& path, PluginFormat format) {
    // Create scanner for the specified format
    auto scanner = createPluginScanner(format);
    if (!scanner) {
        std::cerr << "Failed to create plugin scanner for format: " << static_cast<int>(format) << std::endl;
        return false;
    }
    
    // Load the plugin
    auto plugin = scanner->loadPlugin(path, format);
    if (!plugin) {
        std::cerr << "Failed to load plugin: " << path << std::endl;
        return false;
    }
    
    // Initialize plugin with current sample rate
    plugin->prepareToPlay(48000.0, 1024);  // Default values, should get from Voicemeeter
    
    // Add to loaded plugins map
    std::string name = plugin->getName();
    g_loadedPlugins[name] = plugin;
    
    std::cout << "Loaded plugin: " << name << " (Version: " << plugin->getVersion() << 
                 ", Vendor: " << plugin->getVendor() << ")" << std::endl;
    
    return true;
}

// Display list of loaded plugins
void displayPluginList() {
    if (g_loadedPlugins.empty()) {
        std::cout << "No plugins loaded." << std::endl;
        return;
    }
    
    std::cout << "\nLoaded Plugins:\n";
    std::cout << "---------------------\n";
    
    for (const auto& pair : g_loadedPlugins) {
        auto& plugin = pair.second;
        std::string formatName;
        
        switch (plugin->getFormat()) {
            case PluginFormat::VST3: formatName = "VST3"; break;
            case PluginFormat::AAX: formatName = "AAX"; break;
            case PluginFormat::AAU: formatName = "AAU"; break;
            default: formatName = "Unknown"; break;
        }
        
        std::cout << plugin->getName() << " (" << formatName << ")\n";
        std::cout << "  Vendor: " << plugin->getVendor() << "\n";
        std::cout << "  Version: " << plugin->getVersion() << "\n";
        std::cout << "  Channels: " << plugin->getNumInputChannels() << " in, " 
                  << plugin->getNumOutputChannels() << " out\n";
        std::cout << "---------------------\n";
    }
}

// Display Voicemeeter information
void displayVoicemeeterInfo() {
    if (!g_voicemeeterClient) {
        std::cout << "Voicemeeter client not initialized." << std::endl;
        return;
    }
    
    auto type = g_voicemeeterClient->getVoicemeeterType();
    std::string typeName;
    
    switch (type) {
        case VoicemeeterIntegration::VoicemeeterType::STANDARD:
            typeName = "Standard";
            break;
        case VoicemeeterIntegration::VoicemeeterType::BANANA:
            typeName = "Banana";
            break;
        case VoicemeeterIntegration::VoicemeeterType::POTATO:
            typeName = "Potato";
            break;
        case VoicemeeterIntegration::VoicemeeterType::POTATO_X64:
            typeName = "Potato x64";
            break;
        default:
            typeName = "Unknown";
    }
    
    long version = g_voicemeeterClient->getVoicemeeterVersion();
    int v1 = (version & 0xFF000000) >> 24;
    int v2 = (version & 0x00FF0000) >> 16;
    int v3 = (version & 0x0000FF00) >> 8;
    int v4 = version & 0x000000FF;
    
    std::cout << "\nVoicemeeter Information:\n";
    std::cout << "----------------------\n";
    std::cout << "Type: " << typeName << "\n";
    std::cout << "Version: " << v1 << "." << v2 << "." << v3 << "." << v4 << "\n";
    std::cout << "Strips: " << g_voicemeeterClient->getNumStrips() << "\n";
    std::cout << "Buses: " << g_voicemeeterClient->getNumBuses() << "\n";
    std::cout << "----------------------\n\n";
}

#ifdef _WIN32
// Windows message procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
            
        case WM_DESTROY:
            g_running = false;
            PostQuitMessage(0);
            return 0;
            
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}
#endif