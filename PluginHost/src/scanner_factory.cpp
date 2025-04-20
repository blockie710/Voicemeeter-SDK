#include "../include/plugin_interface.h"
#include "../include/vst3_plugin.h"
#include "../include/aax_plugin.h"
#include "../include/aau_plugin.h"
#include "../include/ara_plugin.h"
#include "../include/lua_plugin.h"
#include "../include/reaper_plugin.h"

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
            g_logger.log(PluginLogger::Level::Warning, "AAU plugins not supported on this platform");
            return nullptr;
            #endif
        case PluginFormat::ARA:
            return std::make_unique<ARAPluginScanner>();
        case PluginFormat::LUA:
            return std::make_unique<LuaPluginScanner>();
        case PluginFormat::REAPER:
            return std::make_unique<ReaperPluginScanner>();
        default:
            g_logger.log(PluginLogger::Level::Error, 
                "Unknown plugin format: " + std::to_string(static_cast<int>(format)));
            return nullptr;
    }
}
