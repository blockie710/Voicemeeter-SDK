#include "VMPluginHost.h"
#include <filesystem>
#include <iostream>
#include <algorithm>

VMPluginHost::VMPluginHost() : m_plugin(nullptr) {
    // Initialize logging
    initPluginLogger("logs");
}

VMPluginHost::~VMPluginHost() {
    UnloadPlugin();
}

bool VMPluginHost::LoadPlugin(const std::string& path) {
    try {
        // Detect plugin format from file extension
        PluginFormat format = detectFormat(path);
        if (format == PluginFormat::UNKNOWN) {
            std::cerr << "Unknown plugin format: " << path << std::endl;
            return false;
        }
        
        // Create scanner for the format
        auto scanner = createPluginScanner(format);
        if (!scanner) {
            std::cerr << "Plugin format not supported on this platform" << std::endl;
            return false;
        }
        
        // Load the plugin
        m_plugin = scanner->loadPlugin(path, format);
        if (!m_plugin) {
            std::cerr << "Failed to load plugin: " << path << std::endl;
            return false;
        }
        
        // Initialize plugin
        if (!m_plugin->initialize()) {
            std::cerr << "Failed to initialize plugin: " << path << std::endl;
            m_plugin = nullptr;
            return false;
        }
        
        // Prepare for audio processing with default values
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
        auto param = m_plugin->getParameter(index);
        // Use string representation if available, otherwise just use the value
        return param.label.empty() ? std::to_string(param.currentValue) : param.label;
    }
    return "";
}

bool VMPluginHost::GetParameterProperties(int index, float* min, float* max, float* defaultVal) {
    if (m_plugin && index >= 0 && index < m_plugin->getParameterCount()) {
        auto param = m_plugin->getParameter(index);
        *min = param.minValue;
        *max = param.maxValue;
        *defaultVal = param.defaultValue;
        return true;
    }
    return false;
}

void VMPluginHost::ProcessAudio(float* inL, float* inR, float* outL, float* outR, int numSamples, float sampleRate) {
    if (!m_plugin) {
        // Pass through if no plugin loaded
        if (inL && outL) std::copy(inL, inL + numSamples, outL);
        if (inR && outR) std::copy(inR, inR + numSamples, outR);
        return;
    }
    
    try {
        // Set up input and output arrays
        float* inputs[2] = { inL, inR };
        float* outputs[2] = { outL, outR };
        
        // Update sample rate if different
        static double currentSampleRate = 48000.0;
        if (std::abs(currentSampleRate - sampleRate) > 0.01) {
            m_plugin->prepareToPlay(sampleRate, numSamples);
            currentSampleRate = sampleRate;
        }
        
        // Process audio
        m_plugin->process(inputs, outputs, 2, 2, numSamples);
    }
    catch (const std::exception& e) {
        std::cerr << "Error processing audio: " << e.what() << std::endl;
        // On error, pass through
        if (inL && outL) std::copy(inL, inL + numSamples, outL);
        if (inR && outR) std::copy(inR, inR + numSamples, outR);
    }
}

PluginFormat VMPluginHost::detectFormat(const std::string& path) {
    std::filesystem::path filePath(path);
    std::string extension = filePath.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    if (extension == ".vst3") return PluginFormat::VST3;
    if (extension == ".aaxplugin" || extension == ".aax") return PluginFormat::AAX;
    if (extension == ".component") return PluginFormat::AAU;
    if (extension == ".lua") return PluginFormat::LUA;
    if (extension == ".jsfx") return PluginFormat::REAPER;
    
    // Special case for ARA - may have various extensions
    if (extension == ".dll" || extension == ".so" || extension == ".dylib") {
        // Check for ARA signature if possible
        // Basic assumption based on filename patterns
        std::string filename = filePath.filename().string();
        if (filename.find("ARA") != std::string::npos || 
            filename.find("ara") != std::string::npos) {
            return PluginFormat::ARA;
        }
        
        // Could be any format, try VST3 as default for DLLs
        return PluginFormat::VST3;
    }
    
    return PluginFormat::UNKNOWN;
}
