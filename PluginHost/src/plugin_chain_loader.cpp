#include "../include/plugin_chain_loader.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <nlohmann/json.hpp>

// External logger function
extern void log(int level, const std::string& message);

using json = nlohmann::json;
namespace fs = std::filesystem;

bool PluginChainLoader::saveChainToFile(const std::string& filePath, const ChainState& state) {
    try {
        // Serialize to JSON
        std::string jsonStr = serializeToJson(state);
        
        // Write to file
        std::ofstream file(filePath);
        if (!file.is_open()) {
            log(3, "Failed to open file for writing: " + filePath);
            return false;
        }
        
        file << jsonStr;
        file.close();
        
        log(1, "Chain saved successfully to: " + filePath);
        return true;
    }
    catch (const std::exception& e) {
        log(3, "Error saving chain: " + std::string(e.what()));
        return false;
    }
}

bool PluginChainLoader::loadChainFromFile(const std::string& filePath, ChainState& state) {
    try {
        // Read from file
        std::ifstream file(filePath);
        if (!file.is_open()) {
            log(3, "Failed to open file for reading: " + filePath);
            return false;
        }
        
        std::string jsonStr((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        
        // Deserialize from JSON
        if (!deserializeFromJson(jsonStr, state)) {
            log(3, "Failed to deserialize chain from JSON");
            return false;
        }
        
        log(1, "Chain loaded successfully from: " + filePath);
        return true;
    }
    catch (const std::exception& e) {
        log(3, "Error loading chain: " + std::string(e.what()));
        return false;
    }
}

std::vector<std::string> PluginChainLoader::getAvailableChainPresets(const std::string& directory) {
    std::vector<std::string> presets;
    
    try {
        // Check if directory exists
        if (!fs::exists(directory) || !fs::is_directory(directory)) {
            return presets;
        }
        
        // Find all .vmpchain files
        for (const auto& entry : fs::directory_iterator(directory)) {
            if (entry.is_regular_file() && entry.path().extension() == ".vmpchain") {
                presets.push_back(entry.path().filename().string());
            }
        }
    }
    catch (const std::exception& e) {
        log(3, "Error getting available presets: " + std::string(e.what()));
    }
    
    return presets;
}

std::string PluginChainLoader::serializeToJson(const ChainState& state) {
    json j;
    
    j["name"] = state.name;
    j["description"] = state.description;
    
    json plugins = json::array();
    for (const auto& plugin : state.plugins) {
        json p;
        p["path"] = plugin.path;
        p["uniqueId"] = plugin.uniqueId;
        p["bypass"] = plugin.bypass;
        
        json params = json::array();
        for (const auto& param : plugin.parameters) {
            json pp;
            pp["index"] = param.first;
            pp["value"] = param.second;
            params.push_back(pp);
        }
        
        p["parameters"] = params;
        plugins.push_back(p);
    }
    
    j["plugins"] = plugins;
    
    return j.dump(4); // Pretty formatting with 4-space indent
}

bool PluginChainLoader::deserializeFromJson(const std::string& jsonStr, ChainState& state) {
    try {
        json j = json::parse(jsonStr);
        
        state.name = j.value("name", "");
        state.description = j.value("description", "");
        
        state.plugins.clear();
        
        for (const auto& pluginJson : j["plugins"]) {
            PluginState plugin;
            plugin.path = pluginJson.value("path", "");
            plugin.uniqueId = pluginJson.value("uniqueId", "");
            plugin.bypass = pluginJson.value("bypass", false);
            
            for (const auto& paramJson : pluginJson["parameters"]) {
                int index = paramJson.value("index", 0);
                float value = paramJson.value("value", 0.0f);
                plugin.parameters.push_back({index, value});
            }
            
            state.plugins.push_back(plugin);
        }
        
        return true;
    }
    catch (const std::exception& e) {
        log(3, "JSON parsing error: " + std::string(e.what()));
        return false;
    }
}
