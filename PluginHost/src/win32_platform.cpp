#include "../include/platform_compat.h"
#include <windows.h>
#include <CommCtrl.h>
#include <ShlObj.h>
#include <iostream>
#include <string>
#include <filesystem>

#ifdef _WIN32

// Windows specific implementation of UI and platform functions

// Helper function to get special folder path
std::string getSpecialFolderPath(int csidl) {
    char path[MAX_PATH] = {0};
    if (SHGetFolderPathA(NULL, csidl, NULL, 0, path) == S_OK) {
        return path;
    }
    return "";
}

// Get application data directory
std::string getAppDataPath() {
    std::string path = getSpecialFolderPath(CSIDL_APPDATA);
    if (!path.empty()) {
        path += "\\VoicemeeterPluginHost\\";
        std::filesystem::create_directories(path);
    }
    return path;
}

// Get user documents directory
std::string getUserDocumentsPath() {
    std::string path = getSpecialFolderPath(CSIDL_PERSONAL);
    if (!path.empty()) {
        path += "\\VoicemeeterPluginHost\\";
        std::filesystem::create_directories(path);
    }
    return path;
}

// Get Windows-specific VST3 plugin directory
std::string getVST3PluginDirectory() {
    std::string programFiles = getSpecialFolderPath(CSIDL_PROGRAM_FILES);
    if (!programFiles.empty()) {
        return programFiles + "\\Common Files\\VST3\\";
    }
    return "";
}

// Get Windows-specific AAX plugin directory
std::string getAAXPluginDirectory() {
    std::string programFiles = getSpecialFolderPath(CSIDL_PROGRAM_FILES);
    if (!programFiles.empty()) {
        return programFiles + "\\Common Files\\Avid\\Audio\\Plug-Ins\\";
    }
    return "";
}

// Convert UTF-8 string to wide (UTF-16) string 
std::wstring utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return std::wstring();
    
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), NULL, 0);
    std::wstring result(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), &result[0], size_needed);
    return result;
}

// Convert wide (UTF-16) string to UTF-8 string
std::string wideToUtf8(const std::wstring& wide) {
    if (wide.empty()) return std::string();
    
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wide.data(), (int)wide.size(), NULL, 0, NULL, NULL);
    std::string result(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), (int)wide.size(), &result[0], size_needed, NULL, NULL);
    return result;
}

// Show a message dialog
int showMessageDialog(const std::string& message, const std::string& title, int flags) {
    return MessageBoxW(
        NULL, 
        utf8ToWide(message).c_str(), 
        utf8ToWide(title).c_str(), 
        flags
    );
}

// Show file open dialog
std::string showFileOpenDialog(const std::string& title, const std::string& filter, void* parentWindow) {
    OPENFILENAMEW ofn = {0};
    wchar_t szFile[MAX_PATH] = {0};
    
    ofn.lStructSize = sizeof(OPENFILENAMEW);
    ofn.hwndOwner = (HWND)parentWindow;
    ofn.lpstrFilter = utf8ToWide(filter).c_str();
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = utf8ToWide(title).c_str();
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    
    if (GetOpenFileNameW(&ofn)) {
        return wideToUtf8(szFile);
    }
    
    return "";
}

// Show file save dialog
std::string showFileSaveDialog(const std::string& title, const std::string& filter, const std::string& defaultExt, void* parentWindow) {
    OPENFILENAMEW ofn = {0};
    wchar_t szFile[MAX_PATH] = {0};
    
    ofn.lStructSize = sizeof(OPENFILENAMEW);
    ofn.hwndOwner = (HWND)parentWindow;
    ofn.lpstrFilter = utf8ToWide(filter).c_str();
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = utf8ToWide(title).c_str();
    ofn.lpstrDefExt = utf8ToWide(defaultExt).c_str();
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    
    if (GetSaveFileNameW(&ofn)) {
        return wideToUtf8(szFile);
    }
    
    return "";
}

// Set window position and size
void setWindowRect(void* window, int x, int y, int width, int height) {
    SetWindowPos(
        (HWND)window, 
        NULL, 
        x, y, width, height, 
        SWP_NOZORDER | SWP_NOACTIVATE
    );
}

// Get window position and size
void getWindowRect(void* window, int& x, int& y, int& width, int& height) {
    RECT rect;
    GetWindowRect((HWND)window, &rect);
    
    x = rect.left;
    y = rect.top;
    width = rect.right - rect.left;
    height = rect.bottom - rect.top;
}

// Window creation helper
void* createWindow(const std::string& title, int width, int height, void* parentWindow) {
    // Register the window class
    static bool windowClassRegistered = false;
    static const char* windowClassName = "VoicemeeterPluginHostWindow";
    
    if (!windowClassRegistered) {
        WNDCLASS wc = {0};
        wc.lpfnWndProc = DefWindowProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.lpszClassName = windowClassName;
        
        if (!RegisterClass(&wc)) {
            std::cerr << "Failed to register window class" << std::endl;
            return NULL;
        }
        
        windowClassRegistered = true;
    }
    
    // Create the window
    HWND hwnd = CreateWindowA(
        windowClassName,
        title.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width, height,
        (HWND)parentWindow,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );
    
    if (!hwnd) {
        std::cerr << "Failed to create window" << std::endl;
        return NULL;
    }
    
    return hwnd;
}

// Show a window
void showWindow(void* window) {
    ShowWindow((HWND)window, SW_SHOW);
    UpdateWindow((HWND)window);
}

// Hide a window
void hideWindow(void* window) {
    ShowWindow((HWND)window, SW_HIDE);
}

// Destroy a window
void destroyWindow(void* window) {
    DestroyWindow((HWND)window);
}

// Windows-specific VST3 plugin editor helper
void* createVST3PluginEditorWindow(void* parentWindow, const std::string& title) {
    // Create a child window for the VST3 plugin editor
    HWND hwnd = CreateWindowA(
        "STATIC",
        title.c_str(),
        WS_CHILD | WS_VISIBLE,
        0, 0, 600, 400,
        (HWND)parentWindow,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );
    
    return hwnd;
}

// Windows-specific AAX plugin editor helper
void* createAAXPluginEditorWindow(void* parentWindow, const std::string& title) {
    // Create a child window for the AAX plugin editor
    HWND hwnd = CreateWindowA(
        "STATIC",
        title.c_str(),
        WS_CHILD | WS_VISIBLE,
        0, 0, 600, 400,
        (HWND)parentWindow,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );
    
    return hwnd;
}

// Windows registry helper functions
bool getRegistryValueString(HKEY root, const std::string& subKey, const std::string& valueName, std::string& value) {
    HKEY hKey;
    DWORD type;
    DWORD dataSize = 0;
    char data[1024] = {0};
    
    if (RegOpenKeyExA(root, subKey.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return false;
    }
    
    dataSize = sizeof(data);
    if (RegQueryValueExA(hKey, valueName.c_str(), NULL, &type, (BYTE*)data, &dataSize) != ERROR_SUCCESS || type != REG_SZ) {
        RegCloseKey(hKey);
        return false;
    }
    
    value = std::string(data, dataSize);
    RegCloseKey(hKey);
    return true;
}

bool getRegistryValueDword(HKEY root, const std::string& subKey, const std::string& valueName, DWORD& value) {
    HKEY hKey;
    DWORD type;
    DWORD dataSize = sizeof(DWORD);
    
    if (RegOpenKeyExA(root, subKey.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return false;
    }
    
    if (RegQueryValueExA(hKey, valueName.c_str(), NULL, &type, (BYTE*)&value, &dataSize) != ERROR_SUCCESS || type != REG_DWORD) {
        RegCloseKey(hKey);
        return false;
    }
    
    RegCloseKey(hKey);
    return true;
}

bool setRegistryValueString(HKEY root, const std::string& subKey, const std::string& valueName, const std::string& value) {
    HKEY hKey;
    
    // Create the key if it doesn't exist
    if (RegCreateKeyExA(root, subKey.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS) {
        return false;
    }
    
    // Set the value
    DWORD dataSize = static_cast<DWORD>(value.size() + 1); // Include null terminator
    if (RegSetValueExA(hKey, valueName.c_str(), 0, REG_SZ, (BYTE*)value.c_str(), dataSize) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return false;
    }
    
    RegCloseKey(hKey);
    return true;
}

bool setRegistryValueDword(HKEY root, const std::string& subKey, const std::string& valueName, DWORD value) {
    HKEY hKey;
    
    // Create the key if it doesn't exist
    if (RegCreateKeyExA(root, subKey.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS) {
        return false;
    }
    
    // Set the value
    if (RegSetValueExA(hKey, valueName.c_str(), 0, REG_DWORD, (BYTE*)&value, sizeof(DWORD)) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return false;
    }
    
    RegCloseKey(hKey);
    return true;
}

// Check if Voicemeeter is installed
bool isVoicemeeterInstalled() {
    std::string uninstallString;
    
    // Try the regular path first
    if (getRegistryValueString(HKEY_LOCAL_MACHINE, 
                             "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\VB:Voicemeeter {17359A74-1236-5467}", 
                             "UninstallString", 
                             uninstallString)) {
        return true;
    }
    
    // Try the 32-bit path on 64-bit Windows
    if (getRegistryValueString(HKEY_LOCAL_MACHINE, 
                             "SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\VB:Voicemeeter {17359A74-1236-5467}", 
                             "UninstallString", 
                             uninstallString)) {
        return true;
    }
    
    return false;
}

// Get Voicemeeter installation path
std::string getVoicemeeterInstallPath() {
    std::string uninstallString;
    
    // Try the regular path first
    if (!getRegistryValueString(HKEY_LOCAL_MACHINE, 
                              "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\VB:Voicemeeter {17359A74-1236-5467}", 
                              "UninstallString", 
                              uninstallString)) {
        // Try the 32-bit path on 64-bit Windows
        if (!getRegistryValueString(HKEY_LOCAL_MACHINE, 
                                  "SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\VB:Voicemeeter {17359A74-1236-5467}", 
                                  "UninstallString", 
                                  uninstallString)) {
            return "";
        }
    }
    
    // Extract the installation path from the uninstall string
    // The uninstall string is something like: "C:\Program Files\VB\Voicemeeter\uninstall.exe"
    size_t pos = uninstallString.find_last_of("\\");
    if (pos != std::string::npos) {
        return uninstallString.substr(0, pos + 1);
    }
    
    return "";
}

#endif // _WIN32