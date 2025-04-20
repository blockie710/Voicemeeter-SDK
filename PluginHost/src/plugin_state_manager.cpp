#include "../include/plugin_state_manager.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <nlohmann/json.hpp>

// For convenience
using json = nlohmann::json;
namespace fs = std::filesystem;

// Save the current plugin chain state to a file
bool PluginStateManager::saveState(const std::string& filePath, const ChainState& state) {
    try {
        std::string jsonStr = serializeToJson(state);
        std::ofstream outFile(filePath);
        if (!outFile.is_open()) {
            std::cerr << "Failed to open file for writing: " << filePath << std::endl;
            return false;
        }
        outFile << jsonStr;
        outFile.close();
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error saving state: " << e.what() << std::endl;
        return false;
    }
}

// Load a plugin chain state from a file
bool PluginStateManager::loadState(const std::string& filePath, ChainState& state) {
    try {
        std::ifstream inFile(filePath);
        if (!inFile.is_open()) {
            std::cerr << "Failed to open file for reading: " << filePath << std::endl;
            return false;
        }
        
        std::string jsonStr((std::istreambuf_iterator<char>(inFile)),
                            std::istreambuf_iterator<char>());
        inFile.close();
        
        return deserializeFromJson(jsonStr, state);
    }
    catch (const std::exception& e) {
        std::cerr << "Error loading state: " << e.what() << std::endl;
        return false;
    }
}

// Create a chain state from the current plugin chain
ChainState PluginStateManager::createStateFromPlugins(
    const std::vector<std::shared_ptr<PluginInstance>>& plugins, 
    double sampleRate, int blockSize, bool bypassAll) {
    
    ChainState state;
    state.name = "Preset"; // Default name
    state.sampleRate = sampleRate;
    state.blockSize = blockSize;
    state.bypassAll = bypassAll;
    
    for (const auto& plugin : plugins) {
        if (!plugin) continue;
        
        PluginState pluginState;
        // We'll need to store the plugin path in a real implementation
        pluginState.pluginPath = ""; // This should be stored in the plugin instance
        pluginState.format = plugin->getFormat();
        pluginState.enabled = true; // This should come from the plugin instance
        pluginState.name = plugin->getName();
        pluginState.vendor = plugin->getVendor();
        pluginState.version = plugin->getVersion();
        
        // Store all parameters
        int paramCount = plugin->getNumParameters();
        for (int i = 0; i < paramCount; i++) {
            auto param = plugin->getParameter(i);
            
            PluginState::ParameterState paramState;
            paramState.id = param.id;
            paramState.name = param.name;
            paramState.value = plugin->getParameterValue(i);
            
            pluginState.parameters.push_back(paramState);
        }
        
        state.plugins.push_back(pluginState);
    }
    
    return state;
}

// Create plugins from a chain state
std::vector<std::shared_ptr<PluginInstance>> PluginStateManager::createPluginsFromState(
    const ChainState& state, 
    std::function<std::shared_ptr<PluginInstance>(const std::string&, PluginFormat)> loadPluginFunc) {
    
    std::vector<std::shared_ptr<PluginInstance>> plugins;
    
    for (const auto& pluginState : state.plugins) {
        // Load the plugin
        auto plugin = loadPluginFunc(pluginState.pluginPath, pluginState.format);
        if (!plugin) {
            std::cerr << "Failed to load plugin: " << pluginState.name << std::endl;
            continue;
        }
        
        // Configure it with sample rate and block size
        plugin->prepareToPlay(state.sampleRate, state.blockSize);
        
        // Set parameters
        for (const auto& paramState : pluginState.parameters) {
            // Find parameter by name or ID
            bool paramFound = false;
            for (int i = 0; i < plugin->getNumParameters(); i++) {
                auto param = plugin->getParameter(i);
                if (param.id == paramState.id || param.name == paramState.name) {
                    plugin->setParameterValue(i, paramState.value);
                    paramFound = true;
                    break;
                }
            }
            
            if (!paramFound) {
                std::cerr << "Parameter not found: " << paramState.name << " in plugin " << pluginState.name << std::endl;
            }
        }
        
        plugins.push_back(plugin);
    }
    
    return plugins;
}

// Get the list of available presets in a directory
std::vector<std::string> PluginStateManager::getAvailablePresets(const std::string& directory) {
    std::vector<std::string> presets;
    
    try {
        if (!fs::exists(directory) || !fs::is_directory(directory)) {
            return presets;
        }
        
        for (const auto& entry : fs::directory_iterator(directory)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                presets.push_back(entry.path().string());
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error scanning preset directory: " << e.what() << std::endl;
    }
    
    return presets;
}

// JSON serialization helpers
std::string PluginStateManager::serializeToJson(const ChainState& state) {
    json j;
    
    // Serialize main chain properties
    j["name"] = state.name;
    j["sampleRate"] = state.sampleRate;
    j["blockSize"] = state.blockSize;
    j["bypassAll"] = state.bypassAll;
    
    // Serialize plugins
    json plugins = json::array();
    for (const auto& plugin : state.plugins) {
        json p;
        p["path"] = plugin.pluginPath;
        p["format"] = static_cast<int>(plugin.format);
        p["enabled"] = plugin.enabled;
        p["name"] = plugin.name;
        p["vendor"] = plugin.vendor;
        p["version"] = plugin.version;
        
        // Serialize parameters
        json params = json::array();
        for (const auto& param : plugin.parameters) {
            json pp;
            pp["id"] = param.id;
            pp["name"] = param.name;
            pp["value"] = param.value;
            params.push_back(pp);
        }
        
        p["parameters"] = params;
        plugins.push_back(p);
    }
    
    j["plugins"] = plugins;
    
    return j.dump(4); // Pretty formatting with 4-space indent
}

bool PluginStateManager::deserializeFromJson(const std::string& jsonStr, ChainState& state) {
    try {
        json j = json::parse(jsonStr);
        
        // Parse main chain properties
        state.name = j["name"].get<std::string>();
        state.sampleRate = j["sampleRate"].get<double>();
        state.blockSize = j["blockSize"].get<int>();
        state.bypassAll = j["bypassAll"].get<bool>();
        
        // Parse plugins
        state.plugins.clear();
        for (const auto& pluginJson : j["plugins"]) {
            PluginState plugin;
            plugin.pluginPath = pluginJson["path"].get<std::string>();
            plugin.format = static_cast<PluginFormat>(pluginJson["format"].get<int>());
            plugin.enabled = pluginJson["enabled"].get<bool>();
            plugin.name = pluginJson["name"].get<std::string>();
            plugin.vendor = pluginJson["vendor"].get<std::string>();
            plugin.version = pluginJson["version"].get<std::string>();
            
            // Parse parameters
            plugin.parameters.clear();
            for (const auto& paramJson : pluginJson["parameters"]) {
                PluginState::ParameterState param;
                param.id = paramJson["id"].get<std::string>();
                param.name = paramJson["name"].get<std::string>();
                param.value = paramJson["value"].get<double>();
                plugin.parameters.push_back(param);
            }
            
            state.plugins.push_back(plugin);
        }
        
        return true;
    }
    catch (const json::exception& e) {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
        return false;
    }
}