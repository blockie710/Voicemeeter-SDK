#include "../include/aax_plugin.h"
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <memory>
#include <string>

#ifdef _WIN32
#include <windows.h>
#define DLL_HANDLE HMODULE
#define LOAD_LIBRARY(path) LoadLibraryA(path)
#define GET_PROC_ADDRESS(handle, proc) GetProcAddress(handle, proc)
#define FREE_LIBRARY(handle) FreeLibrary(handle)
#else
#include <dlfcn.h>
#define DLL_HANDLE void*
#define LOAD_LIBRARY(path) dlopen(path, RTLD_LAZY)
#define GET_PROC_ADDRESS(handle, proc) dlsym(handle, proc)
#define FREE_LIBRARY(handle) dlclose(handle)
#endif

// AAXPlugin implementation
AAXPlugin::AAXPlugin(const std::string& path)
    : m_path(path), 
      m_moduleHandle(nullptr),
      m_pluginDefinition(nullptr),
      m_controller(nullptr),
      m_parameters(nullptr),
      m_sampleRate(0.0),
      m_blockSize(0),
      m_isActive(false),
      m_name("Unknown AAX Plugin"),
      m_vendor("Unknown"),
      m_version("1.0.0"),
      m_numInputChannels(0),
      m_numOutputChannels(0),
      m_editorHandle(nullptr),
      m_hasEditor(false) {
    // Try to load the plugin
    loadPlugin();
}

AAXPlugin::~AAXPlugin() {
    // Clean up AAX resources
    releaseResources();
    
    // Clean up interfaces
    if (m_parameters) {
        // In real code, would release the parameters interface
        m_parameters = nullptr;
    }
    
    if (m_controller) {
        // In real code, would release the controller
        m_controller = nullptr;
    }
    
    if (m_pluginDefinition) {
        // In real code, would release the plugin definition
        m_pluginDefinition = nullptr;
    }
    
    if (m_moduleHandle) {
        FREE_LIBRARY((DLL_HANDLE)m_moduleHandle);
        m_moduleHandle = nullptr;
    }
}

bool AAXPlugin::loadPlugin() {
    std::cout << "Loading AAX plugin: " << m_path << std::endl;
    
    // Load the module (DLL)
    m_moduleHandle = LOAD_LIBRARY(m_path.c_str());
    
    if (!m_moduleHandle) {
        std::cerr << "Failed to load AAX module: " << m_path << std::endl;
        return false;
    }
    
    // In a real implementation, would call AAX-specific entry points
    // to get plugin definition, controller, etc.
    
    // Initialize plugin components
    bool success = initializePlugin();
    if (!success) {
        if (m_moduleHandle) {
            FREE_LIBRARY((DLL_HANDLE)m_moduleHandle);
            m_moduleHandle = nullptr;
        }
        return false;
    }
    
    // Cache parameter information
    cacheParameters();
    
    return true;
}

bool AAXPlugin::initializePlugin() {
    std::cout << "Initializing AAX plugin..." << std::endl;
    
    // In a full implementation, this would:
    // 1. Get the plugin description
    // 2. Create the effect
    // 3. Set up the controller
    // 4. Get plugin information (name, vendor, etc.)
    
    // Simplified placeholder since we don't have the full AAX SDK
    m_name = "AAX Plugin";
    m_vendor = "Plugin Developer";
    m_version = "1.0.0";
    m_numInputChannels = 2;
    m_numOutputChannels = 2;
    m_hasEditor = true;
    
    return true;
}

void AAXPlugin::cacheParameters() {
    // In a full implementation, this would:
    // 1. Enumerate parameters from the controller
    // 2. Store parameter info in the m_parameterCache map
    // 3. Create name-to-index mapping for fast lookups
    
    // Add a placeholder parameter
    PluginParameter param;
    param.id = "gain";
    param.name = "Gain";
    param.type = ParameterType::FLOAT;
    param.minValue = 0.0;
    param.maxValue = 1.0;
    param.defaultValue = 0.5;
    param.currentValue = 0.5;
    param.automatable = true;
    
    m_parameterCache[0] = param;
    m_parameterNameToIndex["Gain"] = 0;
}

// PluginInstance interface implementation
std::string AAXPlugin::getName() const {
    return m_name;
}

std::string AAXPlugin::getVendor() const {
    return m_vendor;
}

std::string AAXPlugin::getVersion() const {
    return m_version;
}

// Audio processing
void AAXPlugin::prepareToPlay(double sampleRate, int maxSamplesPerBlock) {
    m_sampleRate = sampleRate;
    m_blockSize = maxSamplesPerBlock;
    
    // Resize internal buffers for processing
    m_inputInterleavedBuffer.resize(maxSamplesPerBlock * m_numInputChannels);
    m_outputInterleavedBuffer.resize(maxSamplesPerBlock * m_numOutputChannels);
    
    std::cout << "Preparing AAX plugin " << m_name << " for playback: " 
              << sampleRate << " Hz, " << maxSamplesPerBlock << " samples" << std::endl;
}

void AAXPlugin::processBlock(float** inputBuffers, float** outputBuffers, int numInputs, int numOutputs, int numSamples) {
    // In a real implementation with AAX, we would:
    // 1. Convert from separate channel buffers to interleaved format
    // 2. Process through the AAX plugin
    // 3. Convert from interleaved back to separate channels
    
    // Simplified version that just copies inputs to outputs
    for (int o = 0; o < numOutputs && o < m_numOutputChannels; ++o) {
        if (o < numInputs && o < m_numInputChannels && inputBuffers[o] && outputBuffers[o]) {
            // Copy input to output
            std::copy(inputBuffers[o], inputBuffers[o] + numSamples, outputBuffers[o]);
        } else if (outputBuffers[o]) {
            // Clear output if no input
            std::fill(outputBuffers[o], outputBuffers[o] + numSamples, 0.0f);
        }
    }
}

void AAXPlugin::releaseResources() {
    // Free any allocated resources
    m_inputInterleavedBuffer.clear();
    m_outputInterleavedBuffer.clear();
    m_isActive = false;
}

// Parameter handling
int AAXPlugin::getNumParameters() const {
    return static_cast<int>(m_parameterCache.size());
}

PluginParameter AAXPlugin::getParameter(int index) const {
    auto it = m_parameterCache.find(index);
    if (it != m_parameterCache.end()) {
        return it->second;
    }
    
    // Return empty parameter if not found
    PluginParameter emptyParam;
    emptyParam.id = "invalid";
    emptyParam.name = "Invalid Parameter";
    emptyParam.type = ParameterType::FLOAT;
    emptyParam.minValue = 0.0;
    emptyParam.maxValue = 1.0;
    emptyParam.defaultValue = 0.0;
    emptyParam.currentValue = 0.0;
    return emptyParam;
}

void AAXPlugin::setParameterValue(int index, double value) {
    auto it = m_parameterCache.find(index);
    if (it != m_parameterCache.end()) {
        it->second.currentValue = value;
        
        // In a full implementation, this would also update the AAX parameter
        std::cout << "Setting parameter " << it->second.name << " to " << value << std::endl;
    }
}

double AAXPlugin::getParameterValue(int index) const {
    auto it = m_parameterCache.find(index);
    if (it != m_parameterCache.end()) {
        return it->second.currentValue;
    }
    return 0.0;
}

void AAXPlugin::setParameterValueByName(const std::string& name, double value) {
    auto it = m_parameterNameToIndex.find(name);
    if (it != m_parameterNameToIndex.end()) {
        setParameterValue(it->second, value);
    }
}

// Plugin I/O configuration
int AAXPlugin::getNumInputChannels() const {
    return m_numInputChannels;
}

int AAXPlugin::getNumOutputChannels() const {
    return m_numOutputChannels;
}

bool AAXPlugin::hasEditor() const {
    return m_hasEditor;
}

void* AAXPlugin::openEditor(void* parentWindow) {
    if (!m_hasEditor) {
        return nullptr;
    }
    
    // In a full implementation, this would create the AAX editor
    // and return a handle to it
    std::cout << "Opening editor for " << m_name << std::endl;
    
    // Placeholder
    m_editorHandle = parentWindow;
    return m_editorHandle;
}

void AAXPlugin::closeEditor() {
    if (m_editorHandle) {
        std::cout << "Closing editor for " << m_name << std::endl;
        m_editorHandle = nullptr;
    }
}

// AAXPluginScanner implementation
AAXPluginScanner::AAXPluginScanner() {
    std::cout << "Creating AAX plugin scanner" << std::endl;
}

AAXPluginScanner::~AAXPluginScanner() {
}

std::vector<std::string> AAXPluginScanner::getDefaultAAXPaths() const {
    std::vector<std::string> paths;
    
    #ifdef _WIN32
    paths.push_back("C:\\Program Files\\Common Files\\Avid\\Audio\\Plug-Ins");
    #else
    // macOS paths
    paths.push_back("/Library/Application Support/Avid/Audio/Plug-Ins");
    paths.push_back("~/Library/Application Support/Avid/Audio/Plug-Ins");
    #endif
    
    return paths;
}

std::vector<std::string> AAXPluginScanner::scanDirectory(const std::string& directory, PluginFormat format) {
    std::vector<std::string> results;
    
    // Check if format is correct
    if (format != PluginFormat::AAX) {
        return results;
    }
    
    std::cout << "Scanning for AAX plugins in: " << directory << std::endl;
    
    try {
        // Scan for .aaxplugin files or .aaxdll files in the directory and subdirectories
        for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                auto extension = entry.path().extension().string();
                std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
                
                if (extension == ".aaxplugin" || extension == ".aaxdll" || extension == ".aax") {
                    results.push_back(entry.path().string());
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error scanning directory: " << e.what() << std::endl;
    }
    
    std::cout << "Found " << results.size() << " AAX plugin(s)" << std::endl;
    return results;
}

std::shared_ptr<PluginInstance> AAXPluginScanner::loadPlugin(const std::string& path, PluginFormat format) {
    // Check if format is correct
    if (format != PluginFormat::AAX) {
        return nullptr;
    }
    
    // Create and load an AAX plugin
    auto plugin = std::make_shared<AAXPlugin>(path);
    
    // Return nullptr if loading failed (plugin would set its internal state)
    if (!plugin->getName().empty() && plugin->getName() != "Unknown AAX Plugin") {
        return plugin;
    }
    
    return nullptr;
}