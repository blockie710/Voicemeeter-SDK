#include "VMPluginHost.h"
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <stdexcept>

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
        UnloadPlugin(); // Unload any existing plugin
        
        // Detect plugin format
        PluginFormat format = DetectFormat(path);
        if (format == PluginFormat::UNKNOWN) {
            std::cerr << "Unsupported plugin format: " << path << std::endl;
            return false;
        }

        // Create scanner for this format
        auto scanner = createPluginScanner(format);
        if (!scanner) {
            std::cerr << "Failed to create plugin scanner for format" << std::endl;
            return false;
        }

        // Load plugin
        m_plugin = scanner->loadPlugin(path, format);
        if (!m_plugin) {
            std::cerr << "Failed to load plugin: " << path << std::endl;
            return false;
        }

        // Prepare for audio processing with default values
        if (!m_plugin->initialize()) {
            std::cerr << "Failed to initialize plugin: " << path << std::endl;
            m_plugin = nullptr;
            return false;
        }
        
        m_plugin->prepareToPlay(48000.0, 1024);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error loading plugin: " << e.what() << std::endl;
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
    if (m_plugin && index >= 0 && index < m_plugin->getParameterCount()) {
        return m_plugin->getParameter(index).name;
    }
    return "";
}

float VMPluginHost::GetParameter(int index) const {
    if (m_plugin && index >= 0 && index < m_plugin->getParameterCount()) {
        return m_plugin->getParameter(index).currentValue;
    }
    return 0.0f;
}

bool VMPluginHost::SetParameter(int index, float value) {
    if (m_plugin && index >= 0 && index < m_plugin->getParameterCount()) {
        return m_plugin->setParameter(index, value);
    }
    return false;
}

std::string VMPluginHost::GetParameterDisplay(int index) const {
    if (m_plugin && index >= 0 && index < m_plugin->getParameterCount()) {
        return m_plugin->getParameter(index).displayText;
    }
    return "";
}

bool VMPluginHost::GetParameterProperties(int index, float* min, float* max, float* defaultVal) {
    if (!m_plugin || index < 0 || index >= m_plugin->getParameterCount()) {
        return false;
    }
    
    auto param = m_plugin->getParameter(index);
    if (min) *min = param.minValue;
    if (max) *max = param.maxValue;
    if (defaultVal) *defaultVal = param.defaultValue;
    return true;
}

void VMPluginHost::ProcessAudio(float* inL, float* inR, float* outL, float* outR, int numSamples, float sampleRate) {
    if (!m_plugin) {
        // No plugin loaded, copy input to output
        std::memcpy(outL, inL, numSamples * sizeof(float));
        std::memcpy(outR, inR, numSamples * sizeof(float));
        return;
    }

    // Process with single plugin
    float* inputs[2] = { inL, inR };
    float* outputs[2] = { outL, outR };
    try {
        m_plugin->process(inputs, outputs, 2, 2, numSamples);
    }
    catch (const std::exception& e) {
        std::cerr << "Error during audio processing: " << e.what() << std::endl;
        // On error, pass through
        std::memcpy(outL, inL, numSamples * sizeof(float));
        std::memcpy(outR, inR, numSamples * sizeof(float));
    }
}

bool VMPluginHost::HasEditor() const {
    return m_plugin ? m_plugin->hasEditor() : false;
}

bool VMPluginHost::ShowEditor(void* parentWindow) {
    return m_plugin ? m_plugin->showEditor(parentWindow) : false;
}

void VMPluginHost::HideEditor() {
    if (m_plugin) {
        m_plugin->hideEditor();
    }
}

bool VMPluginHost::AddPlugin(const std::string& path) {
    try {
        // Detect plugin format
        PluginFormat format = DetectFormat(path);
        if (format == PluginFormat::UNKNOWN) {
            std::cerr << "Unsupported plugin format: " << path << std::endl;
            return false;
        }

        // Create scanner for this format
        auto scanner = createPluginScanner(format);
        if (!scanner) {
            std::cerr << "Failed to create plugin scanner for format" << std::endl;
            return false;
        }

        // Load plugin
        auto plugin = scanner->loadPlugin(path);
        if (!plugin) {
            std::cerr << "Failed to load plugin: " << path << std::endl;
            return false;
        }

        // Prepare for audio processing with default values
        plugin->initialize();
        plugin->prepareToPlay(48000.0, 1024);
        
        // Add to chain
        m_pluginChain.push_back(plugin);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error adding plugin: " << e.what() << std::endl;
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
    auto extension = std::filesystem::path(path).extension().string();
    
    if (extension == ".vst3") return PluginFormat::VST3;
    if (extension == ".aaxplugin") return PluginFormat::AAX;
    if (extension == ".component") return PluginFormat::AAU;
    if (extension == ".jsfx") return PluginFormat::REAPER;
    if (extension == ".lua") return PluginFormat::LUA;
    
    // For directories, try to determine from structure
    if (std::filesystem::is_directory(path)) {
        if (path.find(".vst3") != std::string::npos) return PluginFormat::VST3;
        if (path.find(".aaxplugin") != std::string::npos) return PluginFormat::AAX;
        if (path.find(".component") != std::string::npos) return PluginFormat::AAU;
    }
    
    return PluginFormat::UNKNOWN;
}
