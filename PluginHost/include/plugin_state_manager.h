#ifndef PLUGIN_STATE_MANAGER_H
#define PLUGIN_STATE_MANAGER_H

#include "plugin_interface.h"
#include <string>
#include <vector>
#include <memory>

/**
 * PluginState - Represents the serialized state of a single plugin
 */
struct PluginState {
    std::string pluginPath;  // Path to the plugin file
    PluginFormat format;     // Plugin format (VST3, AAX, AAU)
    bool enabled;            // Whether the plugin is enabled
    std::string name;        // Plugin name
    std::string vendor;      // Plugin vendor
    std::string version;     // Plugin version
    
    // Parameter states
    struct ParameterState {
        std::string id;
        std::string name;
        double value;
    };
    std::vector<ParameterState> parameters;
};

/**
 * ChainState - Represents the serialized state of the entire plugin chain
 */
struct ChainState {
    std::string name;                   // Name of this chain preset
    double sampleRate;                  // Sample rate used
    int blockSize;                      // Block size used
    bool bypassAll;                     // Whether all plugins are bypassed
    std::vector<PluginState> plugins;   // Plugins in the chain
};

/**
 * PluginStateManager - Manages saving and loading plugin chain states
 */
class PluginStateManager {
public:
    // Save the current plugin chain state to a file
    static bool saveState(const std::string& filePath, const ChainState& state);
    
    // Load a plugin chain state from a file
    static bool loadState(const std::string& filePath, ChainState& state);
    
    // Create a chain state from the current plugin chain
    static ChainState createStateFromPlugins(const std::vector<std::shared_ptr<PluginInstance>>& plugins, 
                                           double sampleRate, int blockSize, bool bypassAll);
    
    // Create plugins from a chain state
    static std::vector<std::shared_ptr<PluginInstance>> createPluginsFromState(
        const ChainState& state, 
        std::function<std::shared_ptr<PluginInstance>(const std::string&, PluginFormat)> loadPluginFunc);
        
    // Get the list of available presets in a directory
    static std::vector<std::string> getAvailablePresets(const std::string& directory);
    
private:
    // JSON serialization helpers
    static std::string serializeToJson(const ChainState& state);
    static bool deserializeFromJson(const std::string& json, ChainState& state);
};

#endif // PLUGIN_STATE_MANAGER_H