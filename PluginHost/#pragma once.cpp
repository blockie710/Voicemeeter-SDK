#pragma once

#include <string>
#include <cstdio>
#include <iostream>
#include <chrono>
#include <ctime>
#include <filesystem>

// Plugin logger class for logging operations
class PluginLogger {
public:
    enum class Level {
        Debug,
        Info,
        Warning,
        Error
    };
    
    bool initialize(const std::string& logDir, Level level = Level::Info);
    void log(Level level, const std::string& message);
    
    // Helper methods for different log levels
    void debug(const std::string& message) { log(Level::Debug, message); }
    void info(const std::string& message) { log(Level::Info, message); }
    void warning(const std::string& message) { log(Level::Warning, message); }
    void error(const std::string& message) { log(Level::Error, message); }
    
private:
    FILE* m_logFile = nullptr;
    Level m_logLevel = Level::Info;
    bool m_logToConsole = true;
    
    std::string levelToString(Level level);
};

// Global logger instance declaration
extern PluginLogger g_logger;
