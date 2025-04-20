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

// Structure to hold command line arguments
struct CommandLineArgs {
    std::vector<std::string> vst3Paths;
    std::vector<std::string> aaxPaths;
    std::vector<std::string> aauPaths;
    bool verbose = false;
    bool noScan = false;
};

// Plugin instance wrapper to manage chain state
struct PluginChainItem {
    std::shared_ptr<PluginInstance> plugin;
    bool enabled = true;
    std::string uniqueId;
    int chainPosition = 0;
};

// Platform-specific includes for window handling
#ifdef _WIN32
#include <windows.h>
#endif

// Defines for window creation
#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 768
#define WINDOW_TITLE "Voicemeeter Plugin Host"

// Custom window messages
#define WM_PLUGIN_UPDATED (WM_USER + 100)

#ifdef _WIN32
// Windows UI controls
#define ID_BTN_ADD_PLUGIN    1001
#define ID_BTN_REMOVE_PLUGIN 1002
#define ID_BTN_MOVE_UP       1003
#define ID_BTN_MOVE_DOWN     1004
#define ID_BTN_BYPASS_ALL    1005
#define ID_BTN_SAVE_CHAIN    1006
#define ID_BTN_LOAD_CHAIN    1007
#define ID_LST_PLUGINS       1008

// UI controls
HWND g_hwndPluginList = NULL;
HWND g_hwndAddBtn = NULL;
HWND g_hwndRemoveBtn = NULL;
HWND g_hwndMoveUpBtn = NULL;
HWND g_hwndMoveDownBtn = NULL;
HWND g_hwndBypassBtn = NULL;
HWND g_hwndSaveBtn = NULL;
HWND g_hwndLoadBtn = NULL;

// UI functions
void CreateControls(HWND hwndParent);
void RefreshPluginListUI();
void MovePluginUp(int index);
void MovePluginDown(int index);
void RemovePlugin(int index);
void TogglePluginEnabled(int index);
void HandlePluginListDblClick(int index);
#endif

// Global state
std::unique_ptr<VoicemeeterIntegration::VoicemeeterClient> g_voicemeeterClient;
std::vector<PluginChainItem> g_pluginChain;
std::map<std::string, std::shared_ptr<PluginInstance>> g_loadedPlugins;
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
void reorderPluginChain();
void enablePlugin(const std::string& uniqueId, bool enable);
void displayPluginList();
void displayVoicemeeterInfo();
void savePluginChainState(const std::string& filename);
bool loadPluginChainState(const std::string& filename);
CommandLineArgs parseCommandLine(int argc, char** argv);
void displayHelp();

#ifdef _WIN32
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif

// Main entry point
int main(int argc, char** argv) {
    std::cout << "Voicemeeter Plugin Host - Starting..." << std::endl;

    // Parse command line arguments
    CommandLineArgs args = parseCommandLine(argc, argv);

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

        // Default plugin directories
        #ifdef _WIN32
        scanForPlugins("C:\\Program Files\\Common Files\\VST3", PluginFormat::VST3);
        scanForPlugins("C:\\Program Files\\Common Files\\Avid\\Audio\\Plug-Ins", PluginFormat::AAX);
        #else
        // macOS paths would go here for AAU
        #endif

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
        NULL                            // Additional data
    );

    if (g_hwndMain == NULL) {
        std::cerr << "Failed to create window." << std::endl;
        return false;
    }

    ShowWindow(g_hwndMain, SW_SHOW);

    // Create UI controls
    CreateControls(g_hwndMain);
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
    g_pluginChain.clear();

    std::cout << "Voicemeeter Plugin Host - Shutdown complete." << std::endl;
}

// Audio processing callback
void processAudio(float** inputs, float** outputs, int numInputs, int numOutputs, int numSamples) {
    if (g_bypassAllPlugins) {
        // Directly pass through audio if bypass is enabled
        for (int i = 0; i < numInputs; ++i) {
            std::copy(inputs[i], inputs[i] + numSamples, outputs[i]);
        }
        return;
    }

    // Apply each plugin in the chain to the audio
    for (auto& item : g_pluginChain) {
        if (item.enabled) {
            item.plugin->processBlock(inputs, outputs, numInputs, numOutputs, numSamples);
        }
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

    // Add to plugin chain
    addPluginToChain(plugin);

    std::cout << "Loaded plugin: " << name << " (Version: " << plugin->getVersion() << 
                 ", Vendor: " << plugin->getVendor() << ")" << std::endl;

    return true;
}

// Add plugin to the processing chain
void addPluginToChain(std::shared_ptr<PluginInstance> plugin) {
    PluginChainItem item;
    item.plugin = plugin;
    item.uniqueId = plugin->getName(); // Use name as unique ID for simplicity
    item.chainPosition = g_pluginChain.size();
    g_pluginChain.push_back(item);
}

// Reorder plugin chain based on chainPosition
void reorderPluginChain() {
    std::sort(g_pluginChain.begin(), g_pluginChain.end(), [](const PluginChainItem& a, const PluginChainItem& b) {
        return a.chainPosition < b.chainPosition;
    });
}

// Enable or disable a plugin in the chain
void enablePlugin(const std::string& uniqueId, bool enable) {
    for (auto& item : g_pluginChain) {
        if (item.uniqueId == uniqueId) {
            item.enabled = enable;
            break;
        }
    }
}

// Save plugin chain state to a file
void savePluginChainState(const std::string& filename) {
    // Implementation for saving state (e.g., JSON or XML)
}

// Load plugin chain state from a file
bool loadPluginChainState(const std::string& filename) {
    // Implementation for loading state (e.g., JSON or XML)
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

// Parse command line arguments
CommandLineArgs parseCommandLine(int argc, char** argv) {
    CommandLineArgs args;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--verbose") {
            args.verbose = true;
        } else if (arg == "--no-scan") {
            args.noScan = true;
        } else if (arg == "--help") {
            displayHelp();
            exit(0);
        } else if (arg.find("--vst3=") == 0) {
            args.vst3Paths.push_back(arg.substr(7));
        } else if (arg.find("--aax=") == 0) {
            args.aaxPaths.push_back(arg.substr(6));
        } else if (arg.find("--aau=") == 0) {
            args.aauPaths.push_back(arg.substr(6));
        } else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            displayHelp();
            exit(1);
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
    std::cout << "  --help             Display this help message\n";
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

        case WM_PLUGIN_UPDATED:
            // Handle plugin updates (e.g., chain reordering)
            reorderPluginChain();
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_BTN_ADD_PLUGIN:
                    // Handle add plugin button click
                    break;
                case ID_BTN_REMOVE_PLUGIN:
                    // Handle remove plugin button click
                    break;
                case ID_BTN_MOVE_UP:
                    // Handle move up button click
                    break;
                case ID_BTN_MOVE_DOWN:
                    // Handle move down button click
                    break;
                case ID_BTN_BYPASS_ALL:
                    g_bypassAllPlugins = !g_bypassAllPlugins;
                    break;
                case ID_BTN_SAVE_CHAIN:
                    // Handle save chain button click
                    break;
                case ID_BTN_LOAD_CHAIN:
                    // Handle load chain button click
                    break;
            }
            return 0;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

// Create UI controls
void CreateControls(HWND hwndParent) {
    g_hwndPluginList = CreateWindowEx(
        WS_EX_CLIENTEDGE, "LISTBOX", NULL,
        WS_CHILD | WS_VISIBLE | LBS_NOTIFY,
        10, 10, 300, 400, hwndParent, (HMENU)ID_LST_PLUGINS, GetModuleHandle(NULL), NULL);

    g_hwndAddBtn = CreateWindow(
        "BUTTON", "Add Plugin",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        320, 10, 100, 30, hwndParent, (HMENU)ID_BTN_ADD_PLUGIN, GetModuleHandle(NULL), NULL);

    g_hwndRemoveBtn = CreateWindow(
        "BUTTON", "Remove Plugin",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        320, 50, 100, 30, hwndParent, (HMENU)ID_BTN_REMOVE_PLUGIN, GetModuleHandle(NULL), NULL);

    g_hwndMoveUpBtn = CreateWindow(
        "BUTTON", "Move Up",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        320, 90, 100, 30, hwndParent, (HMENU)ID_BTN_MOVE_UP, GetModuleHandle(NULL), NULL);

    g_hwndMoveDownBtn = CreateWindow(
        "BUTTON", "Move Down",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        320, 130, 100, 30, hwndParent, (HMENU)ID_BTN_MOVE_DOWN, GetModuleHandle(NULL), NULL);

    g_hwndBypassBtn = CreateWindow(
        "BUTTON", "Bypass All",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        320, 170, 100, 30, hwndParent, (HMENU)ID_BTN_BYPASS_ALL, GetModuleHandle(NULL), NULL);

    g_hwndSaveBtn = CreateWindow(
        "BUTTON", "Save Chain",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        320, 210, 100, 30, hwndParent, (HMENU)ID_BTN_SAVE_CHAIN, GetModuleHandle(NULL), NULL);

    g_hwndLoadBtn = CreateWindow(
        "BUTTON", "Load Chain",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        320, 250, 100, 30, hwndParent, (HMENU)ID_BTN_LOAD_CHAIN, GetModuleHandle(NULL), NULL);
}

// Refresh plugin list UI
void RefreshPluginListUI() {
    SendMessage(g_hwndPluginList, LB_RESETCONTENT, 0, 0);
    for (const auto& item : g_pluginChain) {
        SendMessage(g_hwndPluginList, LB_ADDSTRING, 0, (LPARAM)item.uniqueId.c_str());
    }
}

// Move plugin up in the chain
void MovePluginUp(int index) {
    if (index > 0) {
        std::swap(g_pluginChain[index], g_pluginChain[index - 1]);
        RefreshPluginListUI();
    }
}

// Move plugin down in the chain
void MovePluginDown(int index) {
    if (index < g_pluginChain.size() - 1) {
        std::swap(g_pluginChain[index], g_pluginChain[index + 1]);
        RefreshPluginListUI();
    }
}

// Remove plugin from the chain
void RemovePlugin(int index) {
    if (index >= 0 && index < g_pluginChain.size()) {
        g_pluginChain.erase(g_pluginChain.begin() + index);
        RefreshPluginListUI();
    }
}

// Toggle plugin enabled state
void TogglePluginEnabled(int index) {
    if (index >= 0 && index < g_pluginChain.size()) {
        g_pluginChain[index].enabled = !g_pluginChain[index].enabled;
        RefreshPluginListUI();
    }
}

// Handle double-click on plugin list
void HandlePluginListDblClick(int index) {
    TogglePluginEnabled(index);
}
#endif