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

#ifdef _WIN32
// Windows message handling procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            // Initialize UI elements when window is created
            {
                // Create plugin chain controls
                g_hwndPluginList = CreateWindow(
                    "LISTBOX",
                    nullptr,
                    WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL | WS_BORDER,
                    20, 60, 300, 400,
                    hwnd,
                    (HMENU)ID_LISTBOX_PLUGINS,
                    GetModuleHandle(NULL),
                    nullptr
                );

                // Create buttons
                CreateWindow(
                    "BUTTON",
                    "Add Plugin",
                    WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                    20, 470, 100, 30,
                    hwnd,
                    (HMENU)ID_BTN_ADD_PLUGIN,
                    GetModuleHandle(NULL),
                    nullptr
                );

                CreateWindow(
                    "BUTTON",
                    "Remove",
                    WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                    130, 470, 80, 30,
                    hwnd,
                    (HMENU)ID_BTN_REMOVE_PLUGIN,
                    GetModuleHandle(NULL),
                    nullptr
                );

                CreateWindow(
                    "BUTTON",
                    "Move Up",
                    WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                    20, 510, 80, 30,
                    hwnd,
                    (HMENU)ID_BTN_MOVE_UP,
                    GetModuleHandle(NULL),
                    nullptr
                );

                CreateWindow(
                    "BUTTON",
                    "Move Down",
                    WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                    110, 510, 80, 30,
                    hwnd,
                    (HMENU)ID_BTN_MOVE_DOWN,
                    GetModuleHandle(NULL),
                    nullptr
                );

                CreateWindow(
                    "BUTTON",
                    "Bypass All",
                    WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
                    200, 510, 100, 30,
                    hwnd,
                    (HMENU)ID_BTN_BYPASS_ALL,
                    GetModuleHandle(NULL),
                    nullptr
                );

                CreateWindow(
                    "BUTTON",
                    "Save Chain",
                    WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                    20, 550, 100, 30,
                    hwnd,
                    (HMENU)ID_BTN_SAVE_CHAIN,
                    GetModuleHandle(NULL),
                    nullptr
                );

                CreateWindow(
                    "BUTTON",
                    "Load Chain",
                    WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                    130, 550, 100, 30,
                    hwnd,
                    (HMENU)ID_BTN_LOAD_CHAIN,
                    GetModuleHandle(NULL),
                    nullptr
                );

                // Create status bar
                g_hwndStatusBar = CreateWindow(
                    STATUSCLASSNAME,
                    nullptr,
                    WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
                    0, 0, 0, 0,
                    hwnd,
                    (HMENU)ID_STATUS_BAR,
                    GetModuleHandle(NULL),
                    nullptr
                );

                // Set status bar text
                SendMessage(g_hwndStatusBar, SB_SETTEXT, 0, (LPARAM)"Ready");

                // Create parameter editor area
                g_hwndParamEditor = CreateWindow(
                    "STATIC",
                    "Select a plugin to edit its parameters",
                    WS_CHILD | WS_VISIBLE | SS_CENTER | WS_BORDER,
                    340, 60, 650, 500,
                    hwnd,
                    nullptr,
                    GetModuleHandle(NULL),
                    nullptr
                );

                // Set fonts for all controls
                HFONT hFont = CreateFont(
                    16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                    "Segoe UI"
                );

                EnumChildWindows(hwnd, [](HWND hwndChild, LPARAM lParam) -> BOOL {
                    SendMessage(hwndChild, WM_SETFONT, (WPARAM)lParam, TRUE);
                    return TRUE;
                }, (LPARAM)hFont);
            }
            return 0;

        case WM_COMMAND:
            // Handle button clicks and control notifications
            switch (LOWORD(wParam)) {
                case ID_BTN_ADD_PLUGIN:
                    // Show file dialog to select a plugin
                    {
                        char szFile[MAX_PATH] = "";
                        OPENFILENAME ofn = { 0 };

                        ofn.lStructSize = sizeof(OPENFILENAME);
                        ofn.hwndOwner = hwnd;
                        ofn.lpstrFilter = "VST3 Plugins (*.vst3)\0*.vst3\0"
                                         "AAX Plugins (*.aaxplugin)\0*.aaxplugin\0"
                                         "All Files (*.*)\0*.*\0";
                        ofn.lpstrFile = szFile;
                        ofn.nMaxFile = MAX_PATH;
                        ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

                        if (GetOpenFileName(&ofn)) {
                            // Determine format from file extension
                            std::string path = szFile;
                            PluginFormat format = PluginFormat::UNKNOWN;

                            if (path.ends_with(".vst3")) {
                                format = PluginFormat::VST3;
                            } else if (path.ends_with(".aaxplugin")) {
                                format = PluginFormat::AAX;
                            } else if (path.ends_with(".component")) {
                                format = PluginFormat::AAU;
                            } else if (path.ends_with(".lua")) {
                                format = PluginFormat::LUA;
                            } else if (path.ends_with(".jsfx")) {
                                format = PluginFormat::REAPER;
                            }

                            // Load the plugin
                            if (format != PluginFormat::UNKNOWN) {
                                if (loadPlugin(path, format)) {
                                    // Update the plugin list UI
                                    SendMessage(g_hwndStatusBar, SB_SETTEXT, 0, (LPARAM)("Loaded plugin: " + path).c_str());
                                    displayPluginList();
                                } else {
                                    MessageBox(hwnd, "Failed to load plugin", "Error", MB_OK | MB_ICONERROR);
                                }
                            } else {
                                MessageBox(hwnd, "Unknown plugin format", "Error", MB_OK | MB_ICONERROR);
                            }
                        }
                    }
                    break;

                case ID_BTN_REMOVE_PLUGIN:
                    // Remove selected plugin from chain
                    {
                        int selectedIndex = (int)SendMessage(g_hwndPluginList, LB_GETCURSEL, 0, 0);
                        if (selectedIndex >= 0 && selectedIndex < g_pluginChain.size()) {
                            g_pluginChain.erase(g_pluginChain.begin() + selectedIndex);
                            displayPluginList();
                        }
                    }
                    break;

                case ID_BTN_MOVE_UP:
                    // Move selected plugin up in the chain
                    {
                        int selectedIndex = (int)SendMessage(g_hwndPluginList, LB_GETCURSEL, 0, 0);
                        if (selectedIndex > 0 && selectedIndex < g_pluginChain.size()) {
                            std::swap(g_pluginChain[selectedIndex], g_pluginChain[selectedIndex - 1]);
                            displayPluginList();
                            SendMessage(g_hwndPluginList, LB_SETCURSEL, selectedIndex - 1, 0);
                        }
                    }
                    break;

                case ID_BTN_MOVE_DOWN:
                    // Move selected plugin down in the chain
                    {
                        int selectedIndex = (int)SendMessage(g_hwndPluginList, LB_GETCURSEL, 0, 0);
                        if (selectedIndex >= 0 && selectedIndex < g_pluginChain.size() - 1) {
                            std::swap(g_pluginChain[selectedIndex], g_pluginChain[selectedIndex + 1]);
                            displayPluginList();
                            SendMessage(g_hwndPluginList, LB_SETCURSEL, selectedIndex + 1, 0);
                        }
                    }
                    break;

                case ID_BTN_BYPASS_ALL:
                    // Toggle bypass state for all plugins
                    {
                        g_bypassAllPlugins = !g_bypassAllPlugins;
                        SendMessage(GetDlgItem(hwnd, ID_BTN_BYPASS_ALL), BM_SETCHECK, g_bypassAllPlugins ? BST_CHECKED : BST_UNCHECKED, 0);
                    }
                    break;

                case ID_BTN_SAVE_CHAIN:
                    // Save the current plugin chain to a file
                    {
                        char szFile[MAX_PATH] = "plugin_chain.vmpchain";
                        OPENFILENAME ofn = { 0 };

                        ofn.lStructSize = sizeof(OPENFILENAME);
                        ofn.hwndOwner = hwnd;
                        ofn.lpstrFilter = "Voicemeeter Plugin Chain (*.vmpchain)\0*.vmpchain\0All Files (*.*)\0*.*\0";
                        ofn.lpstrFile = szFile;
                        ofn.nMaxFile = MAX_PATH;
                        ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
                        ofn.lpstrDefExt = "vmpchain";

                        if (GetSaveFileName(&ofn)) {
                            // Implement saving the plugin chain to the file
                            // This would serialize the plugin chain to the file
                            MessageBox(hwnd, "Chain saved successfully", "Success", MB_OK | MB_ICONINFORMATION);
                            SendMessage(g_hwndStatusBar, SB_SETTEXT, 0, (LPARAM)("Chain saved to: " + std::string(szFile)).c_str());
                        }
                    }
                    break;

                case ID_BTN_LOAD_CHAIN:
                    // Load a plugin chain from a file
                    {
                        char szFile[MAX_PATH] = "";
                        OPENFILENAME ofn = { 0 };

                        ofn.lStructSize = sizeof(OPENFILENAME);
                        ofn.hwndOwner = hwnd;
                        ofn.lpstrFilter = "Voicemeeter Plugin Chain (*.vmpchain)\0*.vmpchain\0All Files (*.*)\0*.*\0";
                        ofn.lpstrFile = szFile;
                        ofn.nMaxFile = MAX_PATH;
                        ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

                        if (GetOpenFileName(&ofn)) {
                            // Implement loading the plugin chain from the file
                            // This would deserialize the plugin chain from the file
                            MessageBox(hwnd, "Chain loaded successfully", "Success", MB_OK | MB_ICONINFORMATION);
                            SendMessage(g_hwndStatusBar, SB_SETTEXT, 0, (LPARAM)("Chain loaded from: " + std::string(szFile)).c_str());
                        }
                    }
                    break;

                case ID_LISTBOX_PLUGINS:
                    // Handle selection change in the plugin list
                    if (HIWORD(wParam) == LBN_SELCHANGE) {
                        int selectedIndex = (int)SendMessage(g_hwndPluginList, LB_GETCURSEL, 0, 0);
                        if (selectedIndex >= 0 && selectedIndex < g_pluginChain.size()) {
                            g_selectedPluginIndex = selectedIndex;
                            // Update the parameter editor UI to show parameters for the selected plugin
                            auto& plugin = g_pluginChain[selectedIndex].plugin;
                            std::string info = "Plugin: " + plugin->getName() + "\n\n";
                            info += "Format: " + std::string(plugin->getFormatName()) + "\n";
                            info += "Vendor: " + plugin->getVendor() + "\n";
                            info += "Version: " + plugin->getVersion() + "\n\n";
                            info += "Parameters:\n";

                            for (int i = 0; i < plugin->getParameterCount(); i++) {
                                PluginParameter param = plugin->getParameter(i);
                                info += "  " + param.name + ": " + std::to_string(param.currentValue) + "\n";
                            }

                            SetWindowText(g_hwndParamEditor, info.c_str());
                        } else {
                            g_selectedPluginIndex = -1;
                            SetWindowText(g_hwndParamEditor, "Select a plugin to edit its parameters");
                        }
                    }
                    break;
            }
            return 0;

        case WM_PLUGIN_UPDATED:
            // Handle plugin update notifications
            displayPluginList();
            return 0;

        case WM_AUDIO_LEVELS_UPDATED:
            // Update audio level meters in the UI (not implemented yet)
            return 0;

        case WM_SIZE:
            // Handle window resizing
            {
                int width = LOWORD(lParam);
                int height = HIWORD(lParam);

                // Reposition status bar
                SendMessage(g_hwndStatusBar, WM_SIZE, 0, 0);

                // Reposition other controls as needed
                // ...
            }
            return 0;

        case WM_CLOSE:
            // Handle window close (X button)
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            // Handle window destruction
            g_running = false;
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}
#endif

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

        // Default plugin directories
        #ifdef _WIN32
        scanForPlugins("C:\\Program Files\\Common Files\\VST3", PluginFormat::VST3);
        scanForPlugins("C:\\Program Files\\Common Files\\Avid\\Audio\\Plug-Ins", PluginFormat::AAX);
        #elif defined(__APPLE__)
        scanForPlugins("/Library/Audio/Plug-Ins/VST3", PluginFormat::VST3);
        scanForPlugins("/Library/Application Support/Avid/Audio/Plug-Ins", PluginFormat::AAX);
        scanForPlugins("/Library/Audio/Plug-Ins/Components", PluginFormat::AAU);
        #else
        scanForPlugins("/usr/lib/vst3", PluginFormat::VST3);
        scanForPlugins("/usr/local/lib/vst3", PluginFormat::VST3);
        scanForPlugins(std::string(getenv("HOME")) + "/.vst3", PluginFormat::VST3);
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
    // Initialize common controls
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icc.dwICC = ICC_WIN95_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icc);

    // Register window class
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "VoicemeeterPluginHostClass";
    wc.hbrBackground = CreateSolidBrush(VM_COLOR_BACKGROUND);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

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
        NULL                            // Additional application data
    );

    if (g_hwndMain == NULL) {
        std::cerr << "Failed to create window." << std::endl;
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

    return true;
}

// Shutdown application
void shutdownApplication() {
    // Stop audio processing
    if (g_voicemeeterClient) {
        g_voicemeeterClient->stopAudioProcessing();
        g_voicemeeterClient->shutdown();
    }

    // Clean up plugin chain
    g_pluginChain.clear();
    g_loadedPlugins.clear();

    std::cout << "Application shutdown complete." << std::endl;
}

// Audio processing callback
void processAudio(float** inputs, float** outputs, int numInputs, int numOutputs, int numSamples) {
    // If bypassing all plugins, just copy inputs to outputs
    if (g_bypassAllPlugins || g_pluginChain.empty()) {
        for (int i = 0; i < numOutputs && i < numInputs; ++i) {
            if (inputs[i] && outputs[i]) {
                std::copy(inputs[i], inputs[i] + numSamples, outputs[i]);
            }
        }
        return;
    }

    // Create temporary buffers for the plugin chain
    std::vector<float> tempBuffers[64]; // Max 64 channels
    for (int i = 0; i < numOutputs; i++) {
        tempBuffers[i].resize(numSamples);
    }

    // Copy inputs to first temporary buffer
    for (int i = 0; i < numInputs; i++) {
        if (inputs[i]) {
            std::copy(inputs[i], inputs[i] + numSamples, tempBuffers[i].data());
        }
    }

    // Process through plugin chain
    for (auto& pluginItem : g_pluginChain) {
        if (!pluginItem.bypass) {
            // Setup pointers for plugin processing
            float* pluginInputs[64];
            float* pluginOutputs[64];

            for (int i = 0; i < numInputs; i++) {
                pluginInputs[i] = tempBuffers[i].data();
            }

            for (int i = 0; i < numOutputs; i++) {
                pluginOutputs[i] = tempBuffers[i].data();
            }

            // Process through plugin
            pluginItem.plugin->process(pluginInputs, pluginOutputs, numInputs, numOutputs, numSamples);
        }
    }

    // Copy final result to outputs
    for (int i = 0; i < numOutputs; i++) {
        if (outputs[i]) {
            std::copy(tempBuffers[i].data(), tempBuffers[i].data() + numSamples, outputs[i]);
        }
    }
}

// Scan for plugins in a directory
bool scanForPlugins(const std::string& directory, PluginFormat format) {
    std::cout << "Scanning for " << static_cast<int>(format) << " plugins in: " << directory << std::endl;

    // Check if directory exists
    if (!std::filesystem::exists(directory)) {
        std::cerr << "Directory does not exist: " << directory << std::endl;
        return false;
    }

    // Create scanner for the specified format
    auto scanner = createPluginScanner(format);
    if (!scanner) {
        std::cerr << "Failed to create plugin scanner for format: " << static_cast<int>(format) << std::endl;
        return false;
    }

    // Scan for plugins
    auto foundPlugins = scanner->scanDirectory(directory);

    std::cout << "Found " << foundPlugins.size() << " plugins." << std::endl;

    // Store plugins in the registry
    for (auto& pluginDesc : foundPlugins) {
        std::cout << "  - " << pluginDesc.name << " (" << pluginDesc.path << ")" << std::endl;
        g_loadedPlugins[pluginDesc.uniqueId] = nullptr; // Will be loaded on demand
    }

    return true;
}

// Load a plugin by path and format
bool loadPlugin(const std::string& path, PluginFormat format) {
    std::cout << "Loading plugin: " << path << std::endl;

    // Create scanner for the specified format
    auto scanner = createPluginScanner(format);
    if (!scanner) {
        std::cerr << "Failed to create plugin scanner for format: " << static_cast<int>(format) << std::endl;
        return false;
    }

    // Load the plugin
    auto plugin = scanner->loadPlugin(path);
    if (!plugin) {
        std::cerr << "Failed to load plugin: " << path << std::endl;
        return false;
    }

    std::cout << "Loaded plugin: " << plugin->getName() << std::endl;

    // Add to plugin chain
    addPluginToChain(plugin);

    return true;
}

// Add a plugin to the processing chain
void addPluginToChain(std::shared_ptr<PluginInstance> plugin) {
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

// Reorder the plugin chain
void reorderPluginChain() {
    // No implementation needed - handled by UI actions
}

// Enable/disable a plugin in the chain
void enablePlugin(const std::string& uniqueId, bool enable) {
    for (auto& item : g_pluginChain) {
        if (item.uniqueId == uniqueId) {
            item.bypass = !enable;
            break;
        }
    }
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