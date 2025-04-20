#include "../include/plugin_interface.h"
#include "../include/vst3_plugin.h"
#include "../include/aax_plugin.h"
#include "../include/aau_plugin.h"
#include "../include/ara_plugin.h"
#include "../include/lua_plugin.h"
#include "../include/reaper_plugin.h"

#include <memory>
#include <vector>
#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>

#ifdef _WIN32
#include <windows.h>
#include <strsafe.h>
#endif

// Global logger for plugin operations
class PluginLogger {
private:
    std::ofstream m_logFile;
    bool m_initialized = false;
    
public:
    enum class Level { Info, Warning, Error, Debug };
    
    bool initialize(const std::string& path) {
        m_logFile.open(path, std::ios::app);
        m_initialized = m_logFile.is_open();
        if (m_initialized) {
            log(Level::Info, "PluginLogger initialized");
        }
        return m_initialized;
    }
    
    void log(Level level, const std::string& message) {
        if (!m_initialized) return;
        
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        
        std::string levelStr;
        switch(level) {
            case Level::Info: levelStr = "INFO"; break;
            case Level::Warning: levelStr = "WARNING"; break;
            case Level::Error: levelStr = "ERROR"; break;
            case Level::Debug: levelStr = "DEBUG"; break;
        }
        
        m_logFile << "[" << std::ctime(&time) << "][" << levelStr << "] " << message << std::endl;
        m_logFile.flush();
        
        // Also output to console for immediate feedback
        if (level == Level::Error || level == Level::Warning) {
            std::cerr << "[" << levelStr << "] " << message << std::endl;
        } else {
            std::cout << "[" << levelStr << "] " << message << std::endl;
        }
    }
    
    ~PluginLogger() {
        if (m_initialized) {
            log(Level::Info, "PluginLogger shutting down");
            m_logFile.close();
        }
    }
};

// Global logger instance
static PluginLogger g_logger;

#ifdef _WIN32
// Windows DLL handle for Voicemeeter Remote
static HMODULE g_hVoicemeeterRemote = nullptr;

// Load the Voicemeeter Remote DLL on Windows
bool loadVoicemeeterRemoteDLL() {
    char szDllName[512];
    szDllName[0] = 0;
    
    // Get the Voicemeeter installation path from registry
    HKEY hKey;
    DWORD dwSize = 512;
    DWORD dwType = REG_SZ;
    long regResult;
    
    regResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\VB:Voicemeeter {17359A74-1236-5467}", 0, KEY_READ, &hKey);
    if (regResult != ERROR_SUCCESS) {
        // Try the 32-bit registry view for 64-bit Windows
        regResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\VB:Voicemeeter {17359A74-1236-5467}", 0, KEY_READ, &hKey);
        if (regResult != ERROR_SUCCESS) {
            g_logger.log(PluginLogger::Level::Error, "Voicemeeter is not installed (registry key not found)");
            return false;
        }
    }
    
    regResult = RegQueryValueEx(hKey, "UninstallString", 0, &dwType, (unsigned char*)szDllName, &dwSize);
    RegCloseKey(hKey);
    
    if (regResult != ERROR_SUCCESS || szDllName[0] == 0) {
        g_logger.log(PluginLogger::Level::Error, "Voicemeeter installation path not found in registry");
        return false;
    }
    
    // Remove the "uninstall.exe" part to get the installation directory
    char* p = strrchr(szDllName, '\\');
    if (p == nullptr) {
        g_logger.log(PluginLogger::Level::Error, "Invalid Voicemeeter installation path");
        return false;
    }
    
    *(p + 1) = 0; // Terminate the string after the last backslash
    
    // Use the appropriate DLL based on architecture
    if (sizeof(void*) == 8) {
        StringCbCatA(szDllName, sizeof(szDllName), "VoicemeeterRemote64.dll");
    } else {
        StringCbCatA(szDllName, sizeof(szDllName), "VoicemeeterRemote.dll");
    }
    
    // Load the DLL
    g_hVoicemeeterRemote = LoadLibrary(szDllName);
    if (g_hVoicemeeterRemote == nullptr) {
        g_logger.log(PluginLogger::Level::Error, "Failed to load VoicemeeterRemote DLL: " + std::string(szDllName));
        return false;
    }
    
    g_logger.log(PluginLogger::Level::Info, "Successfully loaded VoicemeeterRemote DLL: " + std::string(szDllName));
    return true;
}

// Unload the Voicemeeter Remote DLL on Windows
void unloadVoicemeeterRemoteDLL() {
    if (g_hVoicemeeterRemote) {
        FreeLibrary(g_hVoicemeeterRemote);
        g_hVoicemeeterRemote = nullptr;
        g_logger.log(PluginLogger::Level::Info, "Unloaded VoicemeeterRemote DLL");
    }
}

// Get a procedure address from the Voicemeeter Remote DLL
void* getVoicemeeterProcAddress(const char* procName) {
    if (!g_hVoicemeeterRemote) {
        g_logger.log(PluginLogger::Level::Error, "Cannot get Voicemeeter procedure address: DLL not loaded");
        return nullptr;
    }
    
    void* procAddress = (void*)GetProcAddress(g_hVoicemeeterRemote, procName);
    if (!procAddress) {
        g_logger.log(PluginLogger::Level::Error, "Failed to get Voicemeeter procedure address: " + std::string(procName));
    }
    
    return procAddress;
}
#endif // _WIN32

// Initialize the logger
bool initPluginLogger(const std::string& logDir) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    struct tm timeinfo;
    
#ifdef _WIN32
    localtime_s(&timeinfo, &time);
#else
    localtime_r(&time, &timeinfo);
#endif
    
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y%m%d-%H%M%S", &timeinfo);
    
    std::string logPath = logDir + "/plugin_host_" + buffer + ".log";
    return g_logger.initialize(logPath);
}

// Enhanced error handling with logging
std::vector<PluginFormat> PluginInstance::getSupportedFormats() {
    std::vector<PluginFormat> formats;
    
    // VST3 is supported on all platforms
    formats.push_back(PluginFormat::VST3);
    g_logger.log(PluginLogger::Level::Info, "VST3 support enabled");
    
    // AAX is supported on Windows and macOS
    formats.push_back(PluginFormat::AAX);
    g_logger.log(PluginLogger::Level::Info, "AAX support enabled");
    
    // AAU is macOS-specific
#ifdef __APPLE__
    formats.push_back(PluginFormat::AAU);
    g_logger.log(PluginLogger::Level::Info, "AAU support enabled (macOS only)");
#endif
    
    // ARA is cross-platform
    formats.push_back(PluginFormat::ARA);
    g_logger.log(PluginLogger::Level::Info, "ARA support enabled");
    
    // LUA scripts are cross-platform
    formats.push_back(PluginFormat::LUA);
    g_logger.log(PluginLogger::Level::Info, "LUA script plugin support enabled");
    
    // REAPER/JSFX plugins are cross-platform
    formats.push_back(PluginFormat::REAPER);
    g_logger.log(PluginLogger::Level::Info, "REAPER/JSFX plugin support enabled");
    
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
            #ifdef __APPLE__
                return std::make_unique<AAUPluginScanner>();
            #else
                g_logger.log(PluginLogger::Level::Warning, "AAU plugins are not supported on this platform (macOS only)");
                return nullptr;
            #endif
            
        case PluginFormat::ARA:
            return std::make_unique<ARAPluginScanner>();
            
        case PluginFormat::LUA:
            return std::make_unique<LuaPluginScanner>();
            
        case PluginFormat::REAPER:
            return std::make_unique<ReaperPluginScanner>();
            
        default:
            g_logger.log(PluginLogger::Level::Error, "Unknown plugin format requested");
            return nullptr;
    }
}