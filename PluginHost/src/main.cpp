#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <thread>
#include <chrono>
#include <map>
#include <mutex>              // Explicitly include mutex
#include <condition_variable> // Explicitly include condition_variable

// Include our header files
#include "../include/plugin_interface.h"
#include "../include/voicemeeter_integration.h"
#include "../include/vst3_plugin.h"
#include "../include/aax_plugin.h"
#include "../include/aau_plugin.h"
#include "../include/platform_utils.h"
#include "../include/error_handling.h"

// External functions from plugin_interface.cpp
extern bool initPluginLogger(const std::string& logDir);

// Voicemeeter theme colors
#define VM_COLOR_BACKGROUND         RGB(18, 30, 40)
#define VM_COLOR_BUTTON_BG          RGB(44, 61, 77)
#define VM_COLOR_BUTTON_HOVER       RGB(55, 75, 95)
#define VM_COLOR_TEXT               RGB(200, 200, 200)
#define VM_COLOR_TEXT_HIGHLIGHT     RGB(255, 255, 255)
#define VM_COLOR_SLIDER_BG          RGB(30, 45, 60)
#define VM_COLOR_SLIDER_ACTIVE      RGB(110, 190, 150)
#define VM_COLOR_SLIDER_RED         RGB(190, 80, 80)
#define VM_COLOR_LISTBOX_BG         RGB(25, 40, 55)
#define VM_COLOR_STATUS_BG          RGB(12, 20, 30)
#define VM_COLOR_GROUP_BORDER       RGB(65, 85, 105)

// Structure to hold command line arguments
struct CommandLineArgs {
    bool verbose = false;
    bool noScan = false;
    std::vector<std::string> vst3Paths;
    std::vector<std::string> aaxPaths;
    std::vector<std::string> aauPaths;
    std::vector<std::string> araPaths;
    std::vector<std::string> luaPaths;
    std::vector<std::string> reaperPaths;
    bool showHelp = false;
};

// Plugin instance wrapper to manage chain state
struct PluginChainItem {
    std::shared_ptr<PluginInstance> plugin;
    bool bypass = false;
    std::string name;
    std::string uniqueId;

    PluginChainItem(std::shared_ptr<PluginInstance> p) 
        : plugin(p), uniqueId(p->getUniqueId()) {
        name = p->getName();
    }
};

// Platform-specific includes for window handling
#ifdef _WIN32
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#endif

// Utility function to check if a string ends with a given suffix
bool string_ends_with(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() && 
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// Helper to get user's home directory
std::string getHomeDirectory() {
    return PlatformUtils::getHomeDirectory();
}

// Defines for window creation
#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 768
#define WINDOW_TITLE "Voicemeeter Plugin Host"

// Custom window messages
#define WM_PLUGIN_UPDATED (WM_USER + 100)
#define WM_AUDIO_LEVELS_UPDATED (WM_USER + 101)

#ifdef _WIN32
// Windows UI controls
#define ID_BTN_ADD_PLUGIN    1001
#define ID_BTN_REMOVE_PLUGIN 1002
#define ID_BTN_MOVE_UP       1003
#define ID_BTN_MOVE_DOWN     1004
#define ID_BTN_BYPASS_ALL    1005
#define ID_BTN_SAVE_CHAIN    1006
#define ID_BTN_LOAD_CHAIN    1007
#define ID_LISTBOX_PLUGINS   1008
#define ID_GROUPBOX_CHAIN    1009
#define ID_GROUPBOX_PARAMS   1010
#define ID_STATUS_BAR        1011

// UI State
HWND g_hwndPluginList = NULL;
HWND g_hwndStatusBar = NULL;
HWND g_hwndParamEditor = NULL;
int g_selectedPluginIndex = -1;
int g_selectedParameterIndex = -1;
#endif

// Global state
std::unique_ptr<VoicemeeterIntegration::VoicemeeterClient> g_voicemeeterClient;
std::vector<PluginChainItem> g_pluginChain;
std::map<std::string, std::shared_ptr<PluginInstance>> g_loadedPlugins;
std::vector<PluginDescription> g_pluginDescriptions; // Store plugin descriptions globally
std::mutex g_audioMutex; // Mutex for thread safety
bool g_running = true;
bool g_bypassAllPlugins = false;

#ifdef _WIN32
HWND g_hwndMain = NULL;
#endif

// Forward declarations
bool initializeApplication();
void shutdownApplication();
void processAudio(float** inputs, float** outputs, int numInputs, int numOutputs, int numSamples);
bool scanForPlugins(const std::string& directory, PluginFormat format);
bool loadPlugin(const std::string& path, PluginFormat format);
void addPluginToChain(std::shared_ptr<PluginInstance> plugin);
bool removePluginFromChain(int index);
bool movePluginInChain(int fromIndex, int toIndex);
void displayPluginList();
void displayVoicemeeterInfo();
CommandLineArgs parseCommandLine(int argc, char** argv);
void displayHelp();

#ifdef _WIN32
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif

// Parse command line arguments
CommandLineArgs parseCommandLine(int argc, char** argv) {
    CommandLineArgs args;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--verbose") {
            args.verbose = true;
        }
        else if (arg == "--no-scan") {
            args.noScan = true;
        }
        else if (arg.rfind("--vst3=", 0) == 0) {
            args.vst3Paths.push_back(arg.substr(7));
        }
        else if (arg.rfind("--aax=", 0) == 0) {
            args.aaxPaths.push_back(arg.substr(6));
        }
        else if (arg.rfind("--aau=", 0) == 0) {
            args.aauPaths.push_back(arg.substr(6));
        }
        else if (arg.rfind("--ara=", 0) == 0) {
            args.araPaths.push_back(arg.substr(6));
        }
        else if (arg.rfind("--lua=", 0) == 0) {
            args.luaPaths.push_back(arg.substr(6));
        }
        else if (arg.rfind("--reaper=", 0) == 0) {
            args.reaperPaths.push_back(arg.substr(9));
        }
        else if (arg == "--help" || arg == "-h") {
            args.showHelp = true;
        }
    }

    return args;
}

// Display help message
void displayHelp() {
    std::cout << "Usage: voicemeeter_plugin_host [options]\n";
    std::cout << "Options:\n";
    std::cout << "  --verbose          Enable verbose mode\n";
    std::cout << "  --no-scan          Disable plugin scanning\n";
    std::cout << "  --vst3=<path>      Add VST3 plugin path\n";
    std::cout << "  --aax=<path>       Add AAX plugin path\n";
    std::cout << "  --aau=<path>       Add AAU plugin path\n";
    std::cout << "  --ara=<path>       Add ARA plugin path\n";
    std::cout << "  --lua=<path>       Add LUA script path\n";
    std::cout << "  --reaper=<path>    Add REAPER plugin path\n";
    std::cout << "  --help             Display this help message\n";
}

// Helper to convert PluginFormat to string
std::string getFormatName(PluginFormat format) {
    switch (format) {
        case PluginFormat::VST3: return "VST3";
        case PluginFormat::AAX: return "AAX";
        case PluginFormat::AAU: return "Audio Unit";
        case PluginFormat::ARA: return "ARA";
        case PluginFormat::LUA: return "Lua Script";
        case PluginFormat::REAPER: return "REAPER/JSFX";
        default: return "Unknown";
    }
}

// Improved plugin scanning with progress tracking
bool scanForPlugins(const std::string& directory, PluginFormat format) {
    std::cout << "Scanning for " << getFormatName(format) << " plugins in: " << directory << std::endl;
    
    // Check if directory exists
    if (!std::filesystem::exists(directory)) {
        std::cout << "  Directory does not exist or is not accessible" << std::endl;
        return false;
    }

    return ErrorHandling::safeExecute<std::function<bool()>, bool>(
        [&]() {
            // Create appropriate scanner
            auto scanner = createPluginScanner(format);
            if (!scanner) {
                std::cout << "  Plugin format not supported on this platform" << std::endl;
                return false;
            }
            
            // Scan for plugins
            std::vector<PluginDescription> plugins = scanner->scanDirectory(directory);
            std::cout << "  Found " << plugins.size() << " plugins" << std::endl;
            
            // Save plugin info for later use
            for (const auto& desc : plugins) {
                std::cout << "  - " << desc.name << " (" << desc.path << ")" << std::endl;
                // Store plugin descriptions in a global cache for later use
                g_pluginDescriptions.push_back(desc);
            }
            
            return !plugins.empty();
        },
        "Error scanning directory " + directory,
        false
    );
}

// Improved plugin loading function with format auto-detection
bool loadPlugin(const std::string& path, PluginFormat format) {
    std::cout << "Loading " << getFormatName(format) << " plugin: " << path << std::endl;
    
    try {
        // Create scanner for the format
        auto scanner = createPluginScanner(format);
        if (!scanner) {
            std::cerr << "Plugin format not supported on this platform" << std::endl;
            return false;
        }
        
        // Load the plugin
        auto plugin = scanner->loadPlugin(path, format);
        if (!plugin) {
            std::cerr << "Failed to load plugin" << std::endl;
            return false;
        }
        
        // Prepare plugin for audio processing
        plugin->initialize();
        plugin->prepareToPlay(48000.0, 1024); // Default to standard values
        
        // Success, add to the chain
        addPluginToChain(plugin);
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading plugin: " << e.what() << std::endl;
        return false;
    }
}

// Add a plugin to the processing chain
void addPluginToChain(std::shared_ptr<PluginInstance> plugin) {
    if (!plugin) {
        std::cerr << "Attempted to add null plugin to chain" << std::endl;
        return;
    }
    
    // Create a chain item for the plugin
    PluginChainItem item(plugin);

    // Add to the chain
    g_pluginChain.push_back(std::move(item));

    // Store in loaded plugins map
    g_loadedPlugins[plugin->getUniqueId()] = plugin;

    // Update UI
    #ifdef _WIN32
    if (g_hwndPluginList) {
        displayPluginList();
    }
    #endif
}

// Enhanced plugin chain management
bool removePluginFromChain(int index) {
    if (index < 0 || index >= g_pluginChain.size()) {
        std::cerr << "Invalid plugin index for removal: " << index << std::endl;
        return false;
    }
    
    std::string pluginName = g_pluginChain[index].name;
    
    // If the plugin has an open editor, close it
    if (g_pluginChain[index].plugin->hasEditor()) {
        g_pluginChain[index].plugin->hideEditor();
    }
    
    // Remove from chain
    g_pluginChain.erase(g_pluginChain.begin() + index);
    
    std::cout << "Removed plugin from chain: " << pluginName << std::endl;
    
    return true;
}

bool movePluginInChain(int fromIndex, int toIndex) {
    if (fromIndex < 0 || fromIndex >= g_pluginChain.size() ||
        toIndex < 0 || toIndex >= g_pluginChain.size()) {
        std::cerr << "Invalid plugin indices for move: " << fromIndex << " -> " << toIndex << std::endl;
        return false;
    }
    
    if (fromIndex == toIndex) {
        return true; // No change needed
    }
    
    // Store the plugin to move
    auto pluginToMove = std::move(g_pluginChain[fromIndex]);
    
    // Remove from original position
    g_pluginChain.erase(g_pluginChain.begin() + fromIndex);
    
    // Insert at new position
    g_pluginChain.insert(g_pluginChain.begin() + toIndex, std::move(pluginToMove));
    
    std::cout << "Moved plugin in chain: " << fromIndex << " -> " << toIndex << std::endl;
    
    return true;
}

// Display the list of plugins in the UI
void displayPluginList() {
    #ifdef _WIN32
    if (g_hwndPluginList) {
        // Clear the list
        SendMessage(g_hwndPluginList, LB_RESETCONTENT, 0, 0);

        // Add each plugin to the list
        for (const auto& item : g_pluginChain) {
            std::string displayName = item.name;
            if (item.bypass) {
                displayName += " (Bypassed)";
            }
            SendMessage(g_hwndPluginList, LB_ADDSTRING, 0, (LPARAM)displayName.c_str());
        }
    }
    #else
    // Console-based list for non-Windows platforms
    std::cout << "Plugin Chain:" << std::endl;
    int index = 0;
    for (const auto& item : g_pluginChain) {
        std::cout << index << ": " << item.name;
        if (item.bypass) {
            std::cout << " (Bypassed)";
        }
        std::cout << std::endl;
        index++;
    }
    std::cout << std::endl;
    #endif
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
            break;
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

// Enhanced audio processing function
void processAudio(float** inputs, float** outputs, int numInputs, int numOutputs, int numSamples) {
    // Lock audio mutex for thread safety
    std::lock_guard<std::mutex> lock(g_audioMutex);
    
    if (g_pluginChain.empty() || g_bypassAllPlugins) {
        // No plugins or all bypassed - pass through
        for (int ch = 0; ch < std::min(numInputs, numOutputs); ch++) {
            if (inputs[ch] && outputs[ch]) {
                std::memcpy(outputs[ch], inputs[ch], numSamples * sizeof(float));
            }
        }
        return;
    }

    // Allocate temporary buffers for plugin chain processing
    std::vector<float*> tempInputs(numInputs, nullptr);
    std::vector<float*> tempOutputs(numOutputs, nullptr);
    
    for (int ch = 0; ch < numOutputs; ch++) {
        tempOutputs[ch] = new float[numSamples];
    }
    
    for (int ch = 0; ch < numInputs; ch++) {
        tempInputs[ch] = new float[numSamples];
        std::memcpy(tempInputs[ch], inputs[ch], numSamples * sizeof(float));
    }
    
    try {
        // Process each plugin in the chain
        for (auto& item : g_pluginChain) {
            if (item.bypass) {
                // Plugin is bypassed, just pass through
                continue;
            }
            
            // Process with this plugin
            try {
                item.plugin->process(tempInputs.data(), tempOutputs.data(), numInputs, numOutputs, numSamples);
                
                // Swap buffers - output becomes input for next plugin
                for (int ch = 0; ch < numInputs && ch < numOutputs; ch++) {
                    std::swap(tempInputs[ch], tempOutputs[ch]);
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Error processing plugin " << item.name << ": " << e.what() << std::endl;
                // On error, just continue with unmodified audio
            }
        }
        
        // Copy final result to output - it will be in tempInputs after the last swap
        for (int ch = 0; ch < numOutputs; ch++) {
            if (outputs[ch] && tempInputs[ch]) {
                std::memcpy(outputs[ch], tempInputs[ch], numSamples * sizeof(float));
            }
        }
    }
    catch (const std::exception& e) {
        // Handle any unexpected errors by passing through original audio
        std::cerr << "Unexpected error in audio processing chain: " << e.what() << std::endl;
        
        for (int ch = 0; ch < std::min(numInputs, numOutputs); ch++) {
            if (inputs[ch] && outputs[ch]) {
                std::memcpy(outputs[ch], inputs[ch], numSamples * sizeof(float));
            }
        }
    }
    
    // Clean up temporary buffers
    for (int ch = 0; ch < numOutputs; ch++) {
        delete[] tempOutputs[ch];
    }
    
    for (int ch = 0; ch < numInputs; ch++) {
        delete[] tempInputs[ch];
    }
}

// Initialize application components with better error handling
bool initializeApplication() {
    std::cout << "Initializing application" << std::endl;
    
    try {
        // Initialize Voicemeeter integration
        g_voicemeeterClient = std::make_unique<VoicemeeterIntegration::VoicemeeterClient>();

        if (!g_voicemeeterClient->isVoicemeeterInstalled()) {
            std::cerr << "Voicemeeter is not installed. Please install Voicemeeter first." << std::endl;
            return false;
        }

        // Try to connect
        if (!g_voicemeeterClient->initialize()) {
            // Voicemeeter might not be running, try to launch it
            std::cout << "Voicemeeter not running, attempting to launch..." << std::endl;
            if (!g_voicemeeterClient->launchVoicemeeter()) {
                std::cerr << "Failed to launch Voicemeeter." << std::endl;
                return false;
            }
            
            // Give some time for Voicemeeter to start up
            std::cout << "Waiting for Voicemeeter to start..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2));
            
            // Try to connect again
            if (!g_voicemeeterClient->initialize()) {
                std::cerr << "Failed to connect to Voicemeeter after launching." << std::endl;
                return false;
            }
        }

        std::cout << "Successfully connected to Voicemeeter" << std::endl;
        
        #ifdef _WIN32
        // Register window class
        WNDCLASS wc = {0};
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = "VoicemeeterPluginHostClass";
        wc.hbrBackground = CreateSolidBrush(VM_COLOR_BACKGROUND);
        wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);

        if (!RegisterClass(&wc)) {
            std::cerr << "Failed to register window class" << std::endl;
            return false;
        }

        // Create main window
        g_hwndMain = CreateWindowEx(
            0,                              // Optional window styles
            "VoicemeeterPluginHostClass",   // Window class
            WINDOW_TITLE,                   // Window text
            WS_OVERLAPPEDWINDOW,            // Window style
            CW_USEDEFAULT, CW_USEDEFAULT,   // Position
            WINDOW_WIDTH, WINDOW_HEIGHT,    // Size
            NULL,                           // Parent window
            NULL,                           // Menu
            GetModuleHandle(NULL),          // Instance handle
            NULL                            // Additional application data
        );

        if (g_hwndMain == NULL) {
            std::cerr << "Failed to create window" << std::endl;
            return false;
        }

        // Set window title with Voicemeeter info
        auto type = g_voicemeeterClient->getVoicemeeterType();
        std::string title = WINDOW_TITLE;

        switch (type) {
            case VoicemeeterIntegration::VoicemeeterType::STANDARD:
                title += " - Voicemeeter Standard";
                break;
            case VoicemeeterIntegration::VoicemeeterType::BANANA:
                title += " - Voicemeeter Banana";
                break;
            case VoicemeeterIntegration::VoicemeeterType::POTATO:
                title += " - Voicemeeter Potato";
                break;
            case VoicemeeterIntegration::VoicemeeterType::POTATO_X64:
                title += " - Voicemeeter Potato x64";
                break;
        }

        SetWindowText(g_hwndMain, title.c_str());

        // Show window
        ShowWindow(g_hwndMain, SW_SHOW);
        UpdateWindow(g_hwndMain);
        #endif

        std::cout << "Application initialized successfully" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error initializing application: " << e.what() << std::endl;
        return false;
    }
}

// Improved shutdown method
void shutdownApplication() {
    std::cout << "Shutting down application" << std::endl;
    
    // Stop audio processing
    if (g_voicemeeterClient) {
        g_voicemeeterClient->stopAudioProcessing();
        g_voicemeeterClient->shutdown();
        std::cout << "Voicemeeter client shutdown complete" << std::endl;
    }

    // Clean up plugin chain
    for (auto& item : g_pluginChain) {
        try {
            if (item.plugin->hasEditor()) {
                item.plugin->hideEditor();
            }
            item.plugin->suspend();
        }
        catch (const std::exception& e) {
            std::cerr << "Error during plugin cleanup: " << e.what() << std::endl;
        }
    }
    
    g_pluginChain.clear();
    g_loadedPlugins.clear();
    g_pluginDescriptions.clear();

    std::cout << "Application shutdown complete" << std::endl;
}

// Main entry point
int main(int argc, char** argv) {
    std::cout << "Voicemeeter Plugin Host - Starting..." << std::endl;

    // Initialize logging
    initPluginLogger("logs");

    // Parse command line arguments
    CommandLineArgs args = parseCommandLine(argc, argv);

    if (args.showHelp) {
        displayHelp();
        return 0;
    }

    if (args.verbose) {
        std::cout << "Verbose mode enabled." << std::endl;
    }

    if (!initializeApplication()) {
        std::cerr << "Failed to initialize application. Exiting." << std::endl;
        return 1;
    }

    // Display Voicemeeter information
    displayVoicemeeterInfo();

    // Scan for plugins in default directories
    if (!args.noScan) {
        std::cout << "Scanning for plugins..." << std::endl;

        // Use platform-independent paths for scanning plugins
        #ifdef _WIN32
        for (const auto& path : PlatformUtils::getStandardPluginDirectories()) {
            if (path.find("VST3") != std::string::npos) {
                scanForPlugins(path, PluginFormat::VST3);
            } else if (path.find("Avid") != std::string::npos) {
                scanForPlugins(path, PluginFormat::AAX);
            }
        }
        #elif defined(__APPLE__)
        for (const auto& path : PlatformUtils::getStandardPluginDirectories()) {
            if (path.find("VST3") != std::string::npos) {
                scanForPlugins(path, PluginFormat::VST3);
            } else if (path.find("Components") != std::string::npos) {
                scanForPlugins(path, PluginFormat::AAU);
            } else if (path.find("Avid") != std::string::npos) {
                scanForPlugins(path, PluginFormat::AAX);
            }
        }
        #else
        for (const auto& path : PlatformUtils::getStandardPluginDirectories()) {
            scanForPlugins(path, PluginFormat::VST3);
        }
        #endif

        // Additional plugin directories from command line
        for (const auto& path : args.vst3Paths) {
            scanForPlugins(path, PluginFormat::VST3);
        }

        for (const auto& path : args.aaxPaths) {
            scanForPlugins(path, PluginFormat::AAX);
        }

        for (const auto& path : args.aauPaths) {
            #ifdef __APPLE__
            scanForPlugins(path, PluginFormat::AAU);
            #else
            std::cout << "AAU plugins are only supported on macOS." << std::endl;
            #endif
        }

        for (const auto& path : args.araPaths) {
            scanForPlugins(path, PluginFormat::ARA);
        }

        for (const auto& path : args.luaPaths) {
            scanForPlugins(path, PluginFormat::LUA);
        }

        for (const auto& path : args.reaperPaths) {
            scanForPlugins(path, PluginFormat::REAPER);
        }

        // Display found plugins
        displayPluginList();
    }

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