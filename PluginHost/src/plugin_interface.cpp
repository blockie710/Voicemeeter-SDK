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
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#include <strsafe.h>
#endif

// Global logger for plugin operations
class PluginLogger {
public:
    enum class Level {
        Debug,
        Info,
        Warning,
        Error,
        Fatal
    };
    
    PluginLogger() : m_logFile(nullptr), m_logToConsole(true), m_logLevel(Level::Info) {}
    
    ~PluginLogger() {
        if (m_logFile) {
            fclose(m_logFile);
        }
    }
    
    bool initialize(const std::string& logDir, Level level = Level::Info) {
        m_logLevel = level;
        
        // Create log directory if it doesn't exist
        std::filesystem::path dirPath(logDir);
        try {
            if (!std::filesystem::exists(dirPath)) {
                std::filesystem::create_directories(dirPath);
            }
        } catch (const std::exception& e) {
            std::cerr << "Failed to create log directory: " << e.what() << std::endl;
            return false;
        }
        
        // Get current time for log filename
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
#ifdef _WIN32
        localtime_s(&tm, &time);
#else
        localtime_r(&time, &tm);
#endif
        
        char timeStr[100];
        std::strftime(timeStr, sizeof(timeStr), "%Y%m%d_%H%M%S", &tm);
        
        std::string logFilePath = (dirPath / ("plugin_host_" + std::string(timeStr) + ".log")).string();
        
        m_logFile = fopen(logFilePath.c_str(), "w");
        if (!m_logFile) {
            std::cerr << "Failed to open log file: " << logFilePath << std::endl;
            return false;
        }
        
        log(Level::Info, "=== Plugin Host Log Started ===");
        log(Level::Info, "Log level: " + levelToString(m_logLevel));
        return true;
    }
    
    void log(Level level, const std::string& message) {
        if (level < m_logLevel) {
            return;
        }
        
        // Get current time
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
#ifdef _WIN32
        localtime_s(&tm, &time);
#else
        localtime_r(&time, &tm);
#endif
        
        char timeStr[100];
        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &tm);
        
        // Format log message
        std::string levelStr = levelToString(level);
        std::string logMessage = std::string(timeStr) + " [" + levelStr + "] " + message;
        
        // Write to file
        if (m_logFile) {
            fprintf(m_logFile, "%s\n", logMessage.c_str());
            fflush(m_logFile);
        }
        
        // Write to console
        if (m_logToConsole) {
            std::cout << "[" + levelStr + "] " + message << std::endl;
        }
    }
    
    void setLogLevel(Level level) {
        m_logLevel = level;
        log(Level::Info, "Log level changed to: " + levelToString(level));
    }
    
    void setLogToConsole(bool logToConsole) {
        m_logToConsole = logToConsole;
    }
    
private:
    FILE* m_logFile;
    bool m_logToConsole;
    Level m_logLevel;
    
    std::string levelToString(Level level) {
        switch (level) {
            case Level::Debug: return "DEBUG";
            case Level::Info: return "INFO";
            case Level::Warning: return "WARNING";
            case Level::Error: return "ERROR";
            case Level::Fatal: return "FATAL";
            default: return "UNKNOWN";
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
    return g_logger.initialize(logDir);
}

// Set log level
void setPluginLogLevel(int level) {
    PluginLogger::Level logLevel = PluginLogger::Level::Info;
    switch (level) {
        case 0: logLevel = PluginLogger::Level::Debug; break;
        case 1: logLevel = PluginLogger::Level::Info; break;
        case 2: logLevel = PluginLogger::Level::Warning; break;
        case 3: logLevel = PluginLogger::Level::Error; break;
        case 4: logLevel = PluginLogger::Level::Fatal; break;
    }
    g_logger.setLogLevel(logLevel);
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

// Create a plugin scanner for the specified format with better error handling
std::unique_ptr<PluginScanner> createPluginScanner(PluginFormat format) {
    try {
        switch (format) {
            case PluginFormat::VST3:
                g_logger.log(PluginLogger::Level::Info, "Creating VST3 plugin scanner");
                return std::make_unique<VST3PluginScanner>();
                
            case PluginFormat::AAX:
                g_logger.log(PluginLogger::Level::Info, "Creating AAX plugin scanner");
                return std::make_unique<AAXPluginScanner>();
                
            case PluginFormat::AAU:
                #ifdef __APPLE__
                    g_logger.log(PluginLogger::Level::Info, "Creating AAU plugin scanner");
                    return std::make_unique<AAUPluginScanner>();
                #else
                    g_logger.log(PluginLogger::Level::Warning, "AAU plugins are not supported on this platform (macOS only)");
                    return nullptr;
                #endif
                
            case PluginFormat::ARA:
                g_logger.log(PluginLogger::Level::Info, "Creating ARA plugin scanner");
                return std::make_unique<ARAPluginScanner>();
                
            case PluginFormat::LUA:
                g_logger.log(PluginLogger::Level::Info, "Creating LUA plugin scanner");
                return std::make_unique<LuaPluginScanner>();
                
            case PluginFormat::REAPER:
                g_logger.log(PluginLogger::Level::Info, "Creating REAPER plugin scanner");
                return std::make_unique<ReaperPluginScanner>();
                
            default:
                g_logger.log(PluginLogger::Level::Error, "Unknown plugin format requested");
                return nullptr;
        }
    }
    catch (const std::exception& e) {
        g_logger.log(PluginLogger::Level::Error, "Failed to create plugin scanner: " + std::string(e.what()));
        return nullptr;
    }
}

// Helper function to get format name
const char* getPluginFormatName(PluginFormat format) {
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