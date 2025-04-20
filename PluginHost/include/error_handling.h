/**
 * Error handling utilities for Voicemeeter Plugin Host
 * 
 * Provides consistent error handling, logging, and reporting across the application.
 */
#pragma once

#include <string>
#include <stdexcept>
#include <functional>
#include "plugin_logger.h"

namespace ErrorHandling {

/**
 * Exception class for plugin-related errors
 */
class PluginException : public std::runtime_error {
public:
    enum class ErrorCode {
        LOAD_FAILED,
        INITIALIZATION_FAILED,
        PROCESS_FAILED,
        PARAMETER_ERROR,
        EDITOR_ERROR,
        UNKNOWN_ERROR
    };
    
    PluginException(const std::string& message, ErrorCode code = ErrorCode::UNKNOWN_ERROR)
        : std::runtime_error(message), errorCode(code) {}
        
    ErrorCode getErrorCode() const { return errorCode; }
    
private:
    ErrorCode errorCode;
};

/**
 * Execute a function with proper exception handling and logging
 * 
 * @param func Function to execute
 * @param errorMessage Message to log on error
 * @param defaultValue Value to return on error
 * @return Result of the function or defaultValue on error
 */
template<typename Func, typename ReturnType>
ReturnType safeExecute(Func func, const std::string& errorMessage, ReturnType defaultValue) {
    try {
        return func();
    }
    catch (const PluginException& e) {
        g_logger.log(PluginLogger::Level::Error, errorMessage + ": " + e.what());
    }
    catch (const std::exception& e) {
        g_logger.log(PluginLogger::Level::Error, errorMessage + ": " + e.what());
    }
    catch (...) {
        g_logger.log(PluginLogger::Level::Error, errorMessage + ": Unknown error");
    }
    return defaultValue;
}

/**
 * Execute a void function with proper exception handling and logging
 * 
 * @param func Function to execute
 * @param errorMessage Message to log on error
 */
template<typename Func>
void safeExecuteVoid(Func func, const std::string& errorMessage) {
    try {
        func();
    }
    catch (const PluginException& e) {
        g_logger.log(PluginLogger::Level::Error, errorMessage + ": " + e.what());
    }
    catch (const std::exception& e) {
        g_logger.log(PluginLogger::Level::Error, errorMessage + ": " + e.what());
    }
    catch (...) {
        g_logger.log(PluginLogger::Level::Error, errorMessage + ": Unknown error");
    }
}

} // namespace ErrorHandling
