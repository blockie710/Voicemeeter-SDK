#ifndef PLUGIN_CHAIN_LOADER_H
#define PLUGIN_CHAIN_LOADER_H

#include <string>
#include <vector>
#include <memory>
#include "plugin_interface.h"

/**
 * PluginChainLoader - Handles saving and loading plugin chains to/from files
 */
class PluginChainLoader {
public:
    struct PluginState {
        std::string path;
        std::string uniqueId;
        bool bypass;
        std::vector<std::pair<int, float>> parameters;
    };
    
    struct ChainState {
        std::string name;
        std::string description;
        std::vector<PluginState> plugins;
    };
    
    // Save the current plugin chain to a file
    static bool saveChainToFile(const std::string& filePath, const ChainState& state);
    
    // Load a plugin chain from a file
    static bool loadChainFromFile(const std::string& filePath, ChainState& state);
    
    // Get list of available chain presets
    static std::vector<std::string> getAvailableChainPresets(const std::string& directory);
    
private:
    // JSON serialization helpers
    static std::string serializeToJson(const ChainState& state);
    static bool deserializeFromJson(const std::string& jsonStr, ChainState& state);
};

#endif // PLUGIN_CHAIN_LOADER_H
