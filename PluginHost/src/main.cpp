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
#include <commctrl.h>
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
#define ID_GRP_PLUGINS       1009
#define ID_GRP_CONTROLS      1010
#define ID_GRP_PARAMETERS    1011
#define ID_BTN_EDIT_PLUGIN   1012
#define ID_SLD_PARAMETER     1013
#define ID_LBL_PARAMETER     1014
#define ID_CMB_INPUT_CHANNEL 1015
#define ID_CMB_OUTPUT_CHANNEL 1016
#define ID_CHK_ENABLE_PLUGIN 1017
#define ID_BTN_REFRESH_SCAN  1018
#define ID_STATUS_BAR        1019
#define ID_TAB_CONTROL       1020
#define ID_BTN_SHOW_EDITOR   1021

#define ID_GRP_PLUGINS       1100
#define ID_GRP_CONTROLS      1101
#define ID_GRP_PARAMETERS    1102
#define ID_LST_PARAMETERS    1103
#define ID_SLIDER_PARAMETER  1104
#define ID_LBL_PARAMETER     1105
#define ID_BTN_REFRESH_SCAN  1106
#define ID_CHK_ENABLE_PLUGIN 1107
#define ID_BTN_SHOW_EDITOR   1108
#define ID_CMB_INPUT_CHANNEL 1109
#define ID_CMB_OUTPUT_CHANNEL 1110
#define ID_STATUS_BAR        1111

// UI controls
HWND g_hwndPluginList = NULL;
HWND g_hwndAddBtn = NULL;
HWND g_hwndRemoveBtn = NULL;
HWND g_hwndMoveUpBtn = NULL;
HWND g_hwndMoveDownBtn = NULL;
HWND g_hwndBypassBtn = NULL;
HWND g_hwndSaveBtn = NULL;
HWND g_hwndLoadBtn = NULL;
HWND g_hwndRefreshScanBtn = NULL;
HWND g_hwndEnablePluginCheck = NULL;
HWND g_hwndShowEditorBtn = NULL;

// Advanced UI elements
HWND g_hwndPluginsGroup = NULL;
HWND g_hwndControlsGroup = NULL;
HWND g_hwndParametersGroup = NULL;
HWND g_hwndParameterList = NULL;
HWND g_hwndParameterSlider = NULL;
HWND g_hwndParameterLabel = NULL;
HWND g_hwndInputChannelCombo = NULL;
HWND g_hwndOutputChannelCombo = NULL;
HWND g_hwndStatusBar = NULL;

// UI fonts
HFONT g_hFont = NULL;
HFONT g_hBoldFont = NULL;

// Current selected plugin
int g_selectedPluginIndex = -1;
int g_selectedParameterIndex = -1;
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
void CreateControls(HWND hwndParent);
HFONT CreateStyledFont(bool bold, int height);
void UpdateParameterControls();
void UpdateParameterSlider(int parameterIndex);
void UpdatePluginControls();
void RefreshPluginListUI();
void MovePluginUp();
void MovePluginDown();
void RemoveSelectedPlugin();
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
// Create UI controls with an advanced layout
void CreateControls(HWND hwndParent) {
    // Create fonts for UI elements
    g_hFont = CreateStyledFont(false, 16);
    g_hBoldFont = CreateStyledFont(true, 16);

    // Get client area dimensions
    RECT rcClient;
    GetClientRect(hwndParent, &rcClient);
    int width = rcClient.right - rcClient.left;
    int height = rcClient.bottom - rcClient.top;

    // Create status bar at the bottom
    g_hwndStatusBar = CreateWindowEx(
        0, STATUSCLASSNAME, NULL,
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0, 0, 0, 0, // Size and position will be set by system
        hwndParent, (HMENU)ID_STATUS_BAR, GetModuleHandle(NULL), NULL);
    
    // Update the status bar immediately
    SendMessage(g_hwndStatusBar, SB_SETTEXT, 0, (LPARAM)"Ready");
    
    // Create group boxes to organize controls better
    g_hwndPluginsGroup = CreateWindowEx(
        0, "BUTTON", "Plugins",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        10, 10, 350, height - 80, 
        hwndParent, (HMENU)ID_GRP_PLUGINS, GetModuleHandle(NULL), NULL);
    SendMessage(g_hwndPluginsGroup, WM_SETFONT, (WPARAM)g_hBoldFont, TRUE);

    g_hwndControlsGroup = CreateWindowEx(
        0, "BUTTON", "Controls",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        370, 10, 200, height - 80, 
        hwndParent, (HMENU)ID_GRP_CONTROLS, GetModuleHandle(NULL), NULL);
    SendMessage(g_hwndControlsGroup, WM_SETFONT, (WPARAM)g_hBoldFont, TRUE);

    g_hwndParametersGroup = CreateWindowEx(
        0, "BUTTON", "Parameters",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        580, 10, width - 590, height - 80, 
        hwndParent, (HMENU)ID_GRP_PARAMETERS, GetModuleHandle(NULL), NULL);
    SendMessage(g_hwndParametersGroup, WM_SETFONT, (WPARAM)g_hBoldFont, TRUE);

    // Create plugins list with custom drawing for status indicators
    g_hwndPluginList = CreateWindowEx(
        WS_EX_CLIENTEDGE, "LISTBOX", NULL,
        WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL | LBS_OWNERDRAWFIXED,
        20, 30, 330, height - 120, 
        g_hwndPluginsGroup, (HMENU)ID_LST_PLUGINS, GetModuleHandle(NULL), NULL);
    SendMessage(g_hwndPluginList, WM_SETFONT, (WPARAM)g_hFont, TRUE);

    // Create parameter list
    g_hwndParameterList = CreateWindowEx(
        WS_EX_CLIENTEDGE, "LISTBOX", NULL,
        WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL,
        590, 30, width - 610, 150, 
        g_hwndParametersGroup, (HMENU)ID_LST_PARAMETERS, GetModuleHandle(NULL), NULL);
    SendMessage(g_hwndParameterList, WM_SETFONT, (WPARAM)g_hFont, TRUE);

    // Create parameter control elements
    g_hwndParameterLabel = CreateWindowEx(
        0, "STATIC", "No Parameter Selected",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        590, 190, width - 610, 20, 
        g_hwndParametersGroup, (HMENU)ID_LBL_PARAMETER, GetModuleHandle(NULL), NULL);
    SendMessage(g_hwndParameterLabel, WM_SETFONT, (WPARAM)g_hFont, TRUE);

    g_hwndParameterSlider = CreateWindowEx(
        0, TRACKBAR_CLASS, NULL,
        WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_AUTOTICKS,
        590, 220, width - 610, 30, 
        g_hwndParametersGroup, (HMENU)ID_SLIDER_PARAMETER, GetModuleHandle(NULL), NULL);
    SendMessage(g_hwndParameterSlider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));

    // Input/Output Channel selection
    CreateWindowEx(
        0, "STATIC", "Input Channel:",
        WS_CHILD | WS_VISIBLE,
        590, 270, 100, 20, 
        g_hwndParametersGroup, (HMENU)-1, GetModuleHandle(NULL), NULL);
    SendMessage(GetDlgItem(g_hwndParametersGroup, -1), WM_SETFONT, (WPARAM)g_hFont, TRUE);

    g_hwndInputChannelCombo = CreateWindowEx(
        0, "COMBOBOX", NULL,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
        700, 270, 150, 200, 
        g_hwndParametersGroup, (HMENU)ID_CMB_INPUT_CHANNEL, GetModuleHandle(NULL), NULL);
    SendMessage(g_hwndInputChannelCombo, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    SendMessage(g_hwndInputChannelCombo, CB_ADDSTRING, 0, (LPARAM)"All Channels");
    for (int i = 1; i <= 8; i++) {
        char buffer[20];
        sprintf(buffer, "Channel %d", i);
        SendMessage(g_hwndInputChannelCombo, CB_ADDSTRING, 0, (LPARAM)buffer);
    }
    SendMessage(g_hwndInputChannelCombo, CB_SETCURSEL, 0, 0);

    CreateWindowEx(
        0, "STATIC", "Output Channel:",
        WS_CHILD | WS_VISIBLE,
        590, 310, 100, 20, 
        g_hwndParametersGroup, (HMENU)-1, GetModuleHandle(NULL), NULL);
    SendMessage(GetDlgItem(g_hwndParametersGroup, -1), WM_SETFONT, (WPARAM)g_hFont, TRUE);

    g_hwndOutputChannelCombo = CreateWindowEx(
        0, "COMBOBOX", NULL,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
        700, 310, 150, 200, 
        g_hwndParametersGroup, (HMENU)ID_CMB_OUTPUT_CHANNEL, GetModuleHandle(NULL), NULL);
    SendMessage(g_hwndOutputChannelCombo, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    SendMessage(g_hwndOutputChannelCombo, CB_ADDSTRING, 0, (LPARAM)"All Channels");
    for (int i = 1; i <= 8; i++) {
        char buffer[20];
        sprintf(buffer, "Channel %d", i);
        SendMessage(g_hwndOutputChannelCombo, CB_ADDSTRING, 0, (LPARAM)buffer);
    }
    SendMessage(g_hwndOutputChannelCombo, CB_SETCURSEL, 0, 0);

    // Control buttons in the controls group
    int btnWidth = 180;
    int btnHeight = 30;
    int btnX = 380;
    int btnY = 40;
    int btnSpacing = 40;

    g_hwndAddBtn = CreateWindow(
        "BUTTON", "Add Plugin...",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        btnX, btnY, btnWidth, btnHeight, 
        g_hwndControlsGroup, (HMENU)ID_BTN_ADD_PLUGIN, GetModuleHandle(NULL), NULL);
    SendMessage(g_hwndAddBtn, WM_SETFONT, (WPARAM)g_hFont, TRUE);

    g_hwndRemoveBtn = CreateWindow(
        "BUTTON", "Remove Plugin",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        btnX, btnY += btnSpacing, btnWidth, btnHeight, 
        g_hwndControlsGroup, (HMENU)ID_BTN_REMOVE_PLUGIN, GetModuleHandle(NULL), NULL);
    SendMessage(g_hwndRemoveBtn, WM_SETFONT, (WPARAM)g_hFont, TRUE);

    g_hwndMoveUpBtn = CreateWindow(
        "BUTTON", "Move Up",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        btnX, btnY += btnSpacing, btnWidth, btnHeight, 
        g_hwndControlsGroup, (HMENU)ID_BTN_MOVE_UP, GetModuleHandle(NULL), NULL);
    SendMessage(g_hwndMoveUpBtn, WM_SETFONT, (WPARAM)g_hFont, TRUE);

    g_hwndMoveDownBtn = CreateWindow(
        "BUTTON", "Move Down",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        btnX, btnY += btnSpacing, btnWidth, btnHeight, 
        g_hwndControlsGroup, (HMENU)ID_BTN_MOVE_DOWN, GetModuleHandle(NULL), NULL);
    SendMessage(g_hwndMoveDownBtn, WM_SETFONT, (WPARAM)g_hFont, TRUE);

    g_hwndBypassBtn = CreateWindow(
        "BUTTON", "Bypass All Effects",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
        btnX, btnY += btnSpacing, btnWidth, btnHeight, 
        g_hwndControlsGroup, (HMENU)ID_BTN_BYPASS_ALL, GetModuleHandle(NULL), NULL);
    SendMessage(g_hwndBypassBtn, WM_SETFONT, (WPARAM)g_hFont, TRUE);

    g_hwndEnablePluginCheck = CreateWindow(
        "BUTTON", "Enable Selected Plugin",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
        btnX, btnY += btnSpacing, btnWidth, btnHeight, 
        g_hwndControlsGroup, (HMENU)ID_CHK_ENABLE_PLUGIN, GetModuleHandle(NULL), NULL);
    SendMessage(g_hwndEnablePluginCheck, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    EnableWindow(g_hwndEnablePluginCheck, FALSE); // Disabled until a plugin is selected

    g_hwndShowEditorBtn = CreateWindow(
        "BUTTON", "Show Plugin Editor",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        btnX, btnY += btnSpacing, btnWidth, btnHeight, 
        g_hwndControlsGroup, (HMENU)ID_BTN_SHOW_EDITOR, GetModuleHandle(NULL), NULL);
    SendMessage(g_hwndShowEditorBtn, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    EnableWindow(g_hwndShowEditorBtn, FALSE); // Disabled until a plugin is selected

    g_hwndSaveBtn = CreateWindow(
        "BUTTON", "Save Chain...",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        btnX, btnY += btnSpacing, btnWidth, btnHeight, 
        g_hwndControlsGroup, (HMENU)ID_BTN_SAVE_CHAIN, GetModuleHandle(NULL), NULL);
    SendMessage(g_hwndSaveBtn, WM_SETFONT, (WPARAM)g_hFont, TRUE);

    g_hwndLoadBtn = CreateWindow(
        "BUTTON", "Load Chain...",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        btnX, btnY += btnSpacing, btnWidth, btnHeight, 
        g_hwndControlsGroup, (HMENU)ID_BTN_LOAD_CHAIN, GetModuleHandle(NULL), NULL);
    SendMessage(g_hwndLoadBtn, WM_SETFONT, (WPARAM)g_hFont, TRUE);

    g_hwndRefreshScanBtn = CreateWindow(
        "BUTTON", "Rescan Plugins",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        btnX, btnY += btnSpacing, btnWidth, btnHeight, 
        g_hwndControlsGroup, (HMENU)ID_BTN_REFRESH_SCAN, GetModuleHandle(NULL), NULL);
    SendMessage(g_hwndRefreshScanBtn, WM_SETFONT, (WPARAM)g_hFont, TRUE);

    // Initial state - disable parameter controls since no plugin is selected yet
    EnableWindow(g_hwndParameterList, FALSE);
    EnableWindow(g_hwndParameterSlider, FALSE);

    // Initialize common controls (for trackbar)
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);
    
    // Initial UI refresh
    UpdateParameterControls();
    UpdatePluginControls();
    RefreshPluginListUI();
}

// Create a styled font with specified properties
HFONT CreateStyledFont(bool bold, int height) {
    LOGFONT lf = {0};
    lf.lfHeight = height;
    strcpy(lf.lfFaceName, "Segoe UI");
    lf.lfWeight = bold ? FW_BOLD : FW_NORMAL;
    return CreateFontIndirect(&lf);
}

// Update controls related to parameter editing
void UpdateParameterControls() {
    if (g_selectedPluginIndex < 0 || g_selectedPluginIndex >= g_pluginChain.size()) {
        // No plugin selected, disable parameter controls
        EnableWindow(g_hwndParameterList, FALSE);
        EnableWindow(g_hwndParameterSlider, FALSE);
        SetWindowText(g_hwndParameterLabel, "No plugin selected");
        SendMessage(g_hwndParameterList, LB_RESETCONTENT, 0, 0);
        return;
    }

    // Enable parameter controls
    EnableWindow(g_hwndParameterList, TRUE);
    
    // Get selected plugin
    auto& item = g_pluginChain[g_selectedPluginIndex];
    auto plugin = item.plugin;
    
    // Populate parameter list
    SendMessage(g_hwndParameterList, LB_RESETCONTENT, 0, 0);
    int paramCount = plugin->getNumParameters();
    
    if (paramCount == 0) {
        SendMessage(g_hwndParameterList, LB_ADDSTRING, 0, (LPARAM)"No parameters available");
        EnableWindow(g_hwndParameterSlider, FALSE);
        return;
    }
    
    // Add all parameters to the list
    for (int i = 0; i < paramCount; i++) {
        auto param = plugin->getParameter(i);
        char buffer[256];
        sprintf(buffer, "%s: %.2f", param.name.c_str(), param.currentValue);
        SendMessage(g_hwndParameterList, LB_ADDSTRING, 0, (LPARAM)buffer);
    }
    
    // Select the first parameter
    SendMessage(g_hwndParameterList, LB_SETCURSEL, 0, 0);
    g_selectedParameterIndex = 0;
    
    // Update parameter slider
    if (paramCount > 0) {
        EnableWindow(g_hwndParameterSlider, TRUE);
        UpdateParameterSlider(g_selectedParameterIndex);
    }
}

// Update the parameter slider for the selected parameter
void UpdateParameterSlider(int parameterIndex) {
    if (g_selectedPluginIndex < 0 || g_selectedPluginIndex >= g_pluginChain.size()) {
        return;
    }
    
    auto& item = g_pluginChain[g_selectedPluginIndex];
    auto plugin = item.plugin;
    
    if (parameterIndex < 0 || parameterIndex >= plugin->getNumParameters()) {
        return;
    }
    
    auto param = plugin->getParameter(parameterIndex);
    
    // Update parameter label
    char buffer[256];
    sprintf(buffer, "%s: %.2f", param.name.c_str(), param.currentValue);
    SetWindowText(g_hwndParameterLabel, buffer);
    
    // Update slider position (scale to 0-100 range)
    double normalizedValue = (param.currentValue - param.minValue) / (param.maxValue - param.minValue);
    int sliderPos = static_cast<int>(normalizedValue * 100);
    SendMessage(g_hwndParameterSlider, TBM_SETPOS, TRUE, sliderPos);
}

// Update enable/disable state of plugin control buttons
void UpdatePluginControls() {
    bool hasSelection = (g_selectedPluginIndex >= 0 && g_selectedPluginIndex < g_pluginChain.size());
    
    EnableWindow(g_hwndRemoveBtn, hasSelection);
    EnableWindow(g_hwndMoveUpBtn, hasSelection && g_selectedPluginIndex > 0);
    EnableWindow(g_hwndMoveDownBtn, hasSelection && g_selectedPluginIndex < g_pluginChain.size() - 1);
    EnableWindow(g_hwndEnablePluginCheck, hasSelection);
    
    if (hasSelection) {
        auto& item = g_pluginChain[g_selectedPluginIndex];
        Button_SetCheck(g_hwndEnablePluginCheck, item.enabled ? BST_CHECKED : BST_UNCHECKED);
        EnableWindow(g_hwndShowEditorBtn, item.plugin->hasEditor());
    } else {
        EnableWindow(g_hwndShowEditorBtn, FALSE);
    }
}

// Refresh the plugin list UI
void RefreshPluginListUI() {
    SendMessage(g_hwndPluginList, LB_RESETCONTENT, 0, 0);
    
    for (const auto& item : g_pluginChain) {
        SendMessage(g_hwndPluginList, LB_ADDSTRING, 0, (LPARAM)item.uniqueId.c_str());
    }
    
    if (g_selectedPluginIndex >= 0 && g_selectedPluginIndex < g_pluginChain.size()) {
        SendMessage(g_hwndPluginList, LB_SETCURSEL, g_selectedPluginIndex, 0);
    }
    
    char statusMsg[256];
    sprintf(statusMsg, "Plugin chain: %zu plugin(s)", g_pluginChain.size());
    SendMessage(g_hwndStatusBar, SB_SETTEXT, 0, (LPARAM)statusMsg);
    
    UpdatePluginControls();
}

// Move selected plugin up in the chain
void MovePluginUp() {
    if (g_selectedPluginIndex > 0 && g_selectedPluginIndex < g_pluginChain.size()) {
        std::swap(g_pluginChain[g_selectedPluginIndex], g_pluginChain[g_selectedPluginIndex - 1]);
        g_selectedPluginIndex--;
        
        // Update chain positions
        for (size_t i = 0; i < g_pluginChain.size(); ++i) {
            g_pluginChain[i].chainPosition = i;
        }
        
        RefreshPluginListUI();
        UpdateParameterControls();
    }
}

// Move selected plugin down in the chain
void MovePluginDown() {
    if (g_selectedPluginIndex >= 0 && g_selectedPluginIndex < g_pluginChain.size() - 1) {
        std::swap(g_pluginChain[g_selectedPluginIndex], g_pluginChain[g_selectedPluginIndex + 1]);
        g_selectedPluginIndex++;
        
        // Update chain positions
        for (size_t i = 0; i < g_pluginChain.size(); ++i) {
            g_pluginChain[i].chainPosition = i;
        }
        
        RefreshPluginListUI();
        UpdateParameterControls();
    }
}

// Remove the selected plugin from the chain
void RemoveSelectedPlugin() {
    if (g_selectedPluginIndex >= 0 && g_selectedPluginIndex < g_pluginChain.size()) {
        std::string pluginName = g_pluginChain[g_selectedPluginIndex].uniqueId;
        g_pluginChain.erase(g_pluginChain.begin() + g_selectedPluginIndex);
        
        // Update chain positions
        for (size_t i = 0; i < g_pluginChain.size(); ++i) {
            g_pluginChain[i].chainPosition = i;
        }
        
        // Update selection
        if (g_pluginChain.empty()) {
            g_selectedPluginIndex = -1;
        } else if (g_selectedPluginIndex >= g_pluginChain.size()) {
            g_selectedPluginIndex = g_pluginChain.size() - 1;
        }
        
        RefreshPluginListUI();
        UpdateParameterControls();
        
        char statusMsg[256];
        sprintf(statusMsg, "Removed plugin: %s", pluginName.c_str());
        SendMessage(g_hwndStatusBar, SB_SETTEXT, 0, (LPARAM)statusMsg);
    }
}
#endif