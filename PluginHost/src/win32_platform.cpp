#include "../include/platform_compat.h"
#include <windows.h>
#include <CommCtrl.h>
#include <ShlObj.h>
#include <iostream>
#include <string>
#include <filesystem>

#ifdef _WIN32

// Helper function to get special folder path
std::string getSpecialFolderPath(int csidl) {
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, csidl, NULL, 0, path))) {
        return wideToUtf8(path);
    }
    return "";
}

// Get application data directory
std::string getAppDataPath() {
    return getSpecialFolderPath(CSIDL_APPDATA);
}

// Get user documents directory
std::string getUserDocumentsPath() {
    return getSpecialFolderPath(CSIDL_MYDOCUMENTS);
}

// Get Windows-specific VST3 plugin directory
std::string getVST3PluginDirectory() {
    // Common Files\VST3 is the standard location
    return getSpecialFolderPath(CSIDL_PROGRAM_FILES_COMMON) + "\\VST3";
}

// Get Windows-specific AAX plugin directory
std::string getAAXPluginDirectory() {
    // Common Files\Avid\Audio\Plug-Ins is the standard location for AAX
    return getSpecialFolderPath(CSIDL_PROGRAM_FILES_COMMON) + "\\Avid\\Audio\\Plug-Ins";
}

// Convert UTF-8 string to wide (UTF-16) string 
std::wstring utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) {
        return std::wstring();
    }
    
    // Calculate required buffer size
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, NULL, 0);
    if (size_needed <= 0) {
        std::cerr << "utf8ToWide: MultiByteToWideChar failed with error " << GetLastError() << std::endl;
        return std::wstring();
    }
    
    // Allocate buffer and convert
    std::wstring wide(size_needed, 0);
    if (MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wide[0], size_needed) <= 0) {
        std::cerr << "utf8ToWide: MultiByteToWideChar failed with error " << GetLastError() << std::endl;
        return std::wstring();
    }
    
    // Remove the null terminator from the std::wstring
    if (!wide.empty() && wide.back() == L'\0') {
        wide.pop_back();
    }
    
    return wide;
}

// Convert wide (UTF-16) string to UTF-8 string
std::string wideToUtf8(const std::wstring& wide) {
    if (wide.empty()) {
        return std::string();
    }
    
    // Calculate required buffer size
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, NULL, 0, NULL, NULL);
    if (size_needed <= 0) {
        std::cerr << "wideToUtf8: WideCharToMultiByte failed with error " << GetLastError() << std::endl;
        return std::string();
    }
    
    // Allocate buffer and convert
    std::string utf8(size_needed, 0);
    if (WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, &utf8[0], size_needed, NULL, NULL) <= 0) {
        std::cerr << "wideToUtf8: WideCharToMultiByte failed with error " << GetLastError() << std::endl;
        return std::string();
    }
    
    // Remove the null terminator from the std::string
    if (!utf8.empty() && utf8.back() == '\0') {
        utf8.pop_back();
    }
    
    return utf8;
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
            std::cerr << "Failed to register window class: " << GetLastError() << std::endl;
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
        std::cerr << "Failed to create window: " << GetLastError() << std::endl;
        return NULL;
    }
    
    return hwnd;
}

// Show a window
void showWindow(void* window) {
    if (!window) return;
    ShowWindow((HWND)window, SW_SHOW);
    UpdateWindow((HWND)window);
}

// Hide a window
void hideWindow(void* window) {
    if (!window) return;
    ShowWindow((HWND)window, SW_HIDE);
}

// Destroy a window
void destroyWindow(void* window) {
    if (!window) return;
    DestroyWindow((HWND)window);
}

// Windows-specific VST3 plugin editor helper
void* createVST3PluginEditorWindow(void* parentWindow, const std::string& title) {
    if (!parentWindow) return NULL;
    
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
    
    if (!hwnd) {
        std::cerr << "Failed to create VST3 plugin editor window: " << GetLastError() << std::endl;
    }
    
    return hwnd;
}

// Windows-specific AAX plugin editor helper
void* createAAXPluginEditorWindow(void* parentWindow, const std::string& title) {
    if (!parentWindow) return NULL;
    
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
    
    if (!hwnd) {
        std::cerr << "Failed to create AAX plugin editor window: " << GetLastError() << std::endl;
    }
    
    return hwnd;
}

// Windows registry helper functions
bool getRegistryValueString(HKEY root, const std::string& subKey, const std::string& valueName, std::string& value) {
    HKEY hKey;
    LONG result = RegOpenKeyExA(root, subKey.c_str(), 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS) {
        return false;
    }
    
    DWORD type;
    DWORD dataSize = 0;
    
    // Get the size of the value
    result = RegQueryValueExA(hKey, valueName.c_str(), NULL, &type, NULL, &dataSize);
    if (result != ERROR_SUCCESS || type != REG_SZ) {
        RegCloseKey(hKey);
        return false;
    }
    
    // Allocate buffer and read the value
    std::vector<char> data(dataSize);
    result = RegQueryValueExA(hKey, valueName.c_str(), NULL, NULL, reinterpret_cast<LPBYTE>(&data[0]), &dataSize);
    RegCloseKey(hKey);
    
    if (result != ERROR_SUCCESS) {
        return false;
    }
    
    // Convert to string
    value = std::string(data.begin(), data.end());
    
    // Remove any trailing null characters
    size_t nullPos = value.find('\0');
    if (nullPos != std::string::npos) {
        value = value.substr(0, nullPos);
    }
    
    return true;
}

bool getRegistryValueDword(HKEY root, const std::string& subKey, const std::string& valueName, DWORD& value) {
    HKEY hKey;
    LONG result = RegOpenKeyExA(root, subKey.c_str(), 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS) {
        return false;
    }
    
    DWORD type;
    DWORD dataSize = sizeof(DWORD);
    
    result = RegQueryValueExA(hKey, valueName.c_str(), NULL, &type, reinterpret_cast<LPBYTE>(&value), &dataSize);
    RegCloseKey(hKey);
    
    return (result == ERROR_SUCCESS && type == REG_DWORD);
}

#endif // _WIN32