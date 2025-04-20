#include "VMPluginHost.h"
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <stdexcept>

// External functions from plugin_interface.cpp
extern bool initPluginLogger(const std::string& logDir);
extern std::unique_ptr<PluginScanner> createPluginScanner(PluginFormat format);

VMPluginHost::VMPluginHost() : m_plugin(nullptr) {
    // Initialize logging
    initPluginLogger("logs");
}

VMPluginHost::~VMPluginHost() {
    UnloadPlugin();
    ClearPlugins();
}

bool VMPluginHost::LoadPlugin(const std::string& path) {
    try {
        // Detect plugin format
        PluginFormat format = DetectFormat(path);
        
        // Create scanner for this format
        auto scanner = createPluginScanner(format);
        if (!scanner) {
            std::cerr << "Failed to create plugin scanner for format: " << static_cast<int>(format) << std::endl;
            return false;
        }
        
        // Load the plugin
        m_plugin = scanner->loadPlugin(path, format);
        if (!m_plugin) {
            std::cerr << "Failed to load plugin: " << path << std::endl;
            return false;
        }
        
        // Initialize the plugin
        if (!m_plugin->initialize()) {
            std::cerr << "Failed to initialize plugin: " << path << std::endl;
            m_plugin = nullptr;
            return false;
        }
        
        // Prepare for audio processing
        m_plugin->prepareToPlay(48000.0, 1024);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error loading plugin: " << e.what() << std::endl;
        m_plugin = nullptr;  // Ensure plugin pointer is nullified on error
        return false;
    }
    catch (...) {
        std::cerr << "Unknown error loading plugin" << std::endl;
        m_plugin = nullptr;  // Ensure plugin pointer is nullified on error
        return false;
    }
}

void VMPluginHost::UnloadPlugin() {
    if (m_plugin) {
        try {
            // Clean up editor if open
            if (m_plugin->hasEditor()) {
                m_plugin->hideEditor();
            }
            m_plugin->suspend();
        }
        catch (...) {
            // Ignore errors during shutdown
        }
        m_plugin = nullptr;
    }
}

std::string VMPluginHost::GetPluginName() const {
    return m_plugin ? m_plugin->getName() : "No Plugin";
}

std::string VMPluginHost::GetPluginVendor() const {
    return m_plugin ? m_plugin->getVendor() : "";
}

std::string VMPluginHost::GetPluginProduct() const {
    return m_plugin ? m_plugin->getVersion() : "";
}

int VMPluginHost::GetNumParameters() const {
    return m_plugin ? m_plugin->getParameterCount() : 0;
}

std::string VMPluginHost::GetParameterName(int index) const {
    if (!m_plugin || index < 0 || index >= m_plugin->getParameterCount()) {
        return "";
    }
    
    return m_plugin->getParameter(index).name;
}

float VMPluginHost::GetParameter(int index) const {
    if (!m_plugin || index < 0 || index >= m_plugin->getParameterCount()) {
        return 0.0f;
    }
    
    return static_cast<float>(m_plugin->getParameter(index).currentValue);
}

bool VMPluginHost::SetParameter(int index, float value) {
    if (!m_plugin || index < 0 || index >= m_plugin->getParameterCount()) {
        return false;
    }
    
    return m_plugin->setParameter(index, value);
}

std::string VMPluginHost::GetParameterDisplay(int index) const {
    if (!m_plugin || index < 0 || index >= m_plugin->getParameterCount()) {
        return "";
    }
    
    // This would ideally come from the plugin's getParameterDisplay, but we're simplifying
    auto param = m_plugin->getParameter(index);
    return std::to_string(param.currentValue);
}

bool VMPluginHost::GetParameterProperties(int index, float* min, float* max, float* defaultVal) {
    if (!m_plugin || index < 0 || index >= m_plugin->getParameterCount() || !min || !max || !defaultVal) {
        return false;
    }
    
    auto param = m_plugin->getParameter(index);
    *min = static_cast<float>(param.minValue);
    *max = static_cast<float>(param.maxValue);
    *defaultVal = static_cast<float>(param.defaultValue);
    
    return true;
}

void VMPluginHost::ProcessAudio(float* inL, float* inR, float* outL, float* outR, int numSamples, float sampleRate) {
    if (!m_plugin) return;
    
    // Set up input/output buffer pointers
    float* inputs[2] = { inL, inR };
    float* outputs[2] = { outL, outR };
    
    // Process the plugin
    m_plugin->process(inputs, outputs, 2, 2, numSamples);
}

bool VMPluginHost::HasEditor() const {
    return m_plugin ? m_plugin->hasEditor() : false;
}

bool VMPluginHost::ShowEditor(void* parentWindow) {
    if (!m_plugin || !m_plugin->hasEditor()) {
        return false;
    }
    
    return m_plugin->showEditor(parentWindow);
}

void VMPluginHost::HideEditor() {
    if (m_plugin && m_plugin->hasEditor()) {
        m_plugin->hideEditor();
    }
}

bool VMPluginHost::AddPlugin(const std::string& path) {
    try {
        // Detect plugin format
        PluginFormat format = DetectFormat(path);
        
        // Create scanner for this format
        auto scanner = createPluginScanner(format);
        if (!scanner) {
            std::cerr << "Failed to create plugin scanner for format: " << static_cast<int>(format) << std::endl;
            return false;
        }
        
        // Load the plugin
        auto plugin = scanner->loadPlugin(path, format);
        if (!plugin) {
            std::cerr << "Failed to load plugin: " << path << std::endl;
            return false;
        }
        
        // Initialize the plugin
        if (!plugin->initialize()) {
            std::cerr << "Failed to initialize plugin: " << path << std::endl;
            return false;
        }
        
        // Add to the chain
        m_pluginChain.push_back(plugin);
        
        // Prepare for audio processing
        plugin->prepareToPlay(48000.0, 1024);
        
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error adding plugin: " << e.what() << std::endl;
        return false;
    }
    catch (...) {
        std::cerr << "Unknown error adding plugin" << std::endl;
        return false;
    }
}

void VMPluginHost::ClearPlugins() {
    for (auto& plugin : m_pluginChain) {
        try {
            if (plugin->hasEditor()) {
                plugin->hideEditor();
            }
            plugin->suspend();
        }
        catch (...) {
            // Ignore errors during shutdown
        }
    }
    m_pluginChain.clear();
}

int VMPluginHost::GetPluginCount() const {
    return static_cast<int>(m_pluginChain.size());
}

PluginFormat VMPluginHost::DetectFormat(const std::string& path) {
    std::string extension = std::filesystem::path(path).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), 
                   [](unsigned char c) { return std::tolower(c); });
    
    if (extension == ".vst3") {
        return PluginFormat::VST3;
    }
    else if (extension == ".aaxplugin") {
        return PluginFormat::AAX;
    }
    else if (extension == ".component" || extension == ".vst" || extension == ".au") {
        return PluginFormat::AAU;
    }
    else if (extension == ".dll") {
        // Need more sophisticated detection for DLLs
        // For now, default to VST
        return PluginFormat::VST;
    }
    else if (extension == ".lua") {
        return PluginFormat::LUA;
    }
    else if (extension == ".jsfx") {
        return PluginFormat::REAPER;
    }
    
    // Default to VST3
    return PluginFormat::VST3;
}
