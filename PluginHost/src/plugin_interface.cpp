#include "../include/plugin_interface.h"
#include "../include/vst3_plugin.h"
#include "../include/aax_plugin.h"
#include "../include/aau_plugin.h"

#include <memory>
#include <vector>
#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>

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
    
    // Check which plugin formats are supported in this build
#ifdef WITH_VST3_SUPPORT
    formats.push_back(PluginFormat::VST3);
    g_logger.log(PluginLogger::Level::Info, "VST3 support enabled");
#endif

#ifdef WITH_AAX_SUPPORT
    formats.push_back(PluginFormat::AAX);
    g_logger.log(PluginLogger::Level::Info, "AAX support enabled");
#endif

#ifdef __APPLE__
    formats.push_back(PluginFormat::AAU);
    g_logger.log(PluginLogger::Level::Info, "AAU support enabled");
#endif

    if (formats.empty()) {
        g_logger.log(PluginLogger::Level::Warning, "No plugin formats are supported in this build");
    }
    
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
            return std::make_unique<AAUPluginScanner>();
        default:
            g_logger.log(PluginLogger::Level::Error, "Unsupported plugin format requested");
            return nullptr;
    }
}