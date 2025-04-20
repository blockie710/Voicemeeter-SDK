#include "../include/ara_plugin.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <algorithm>

// For Linux compatibility
#ifndef _WIN32
#include <dlfcn.h>
#endif

namespace fs = std::filesystem;

// Function typedefs for ARA plugin entry points
typedef void* (*ARAFactoryFunction)(void*);
typedef int (*ARAGetAPIVersionFunction)();
typedef const char* (*ARAGetPluginNameFunction)();
typedef const char* (*ARAGetPluginVendorFunction)();
typedef const char* (*ARAGetPluginVersionFunction)();

// ARA Plugin implementation
class ARAPluginInstanceImpl : public ARAPluginInstance {
public:
    ARAPluginInstanceImpl(const std::string& path) : m_path(path) {
        m_name = "ARA Plugin";
        m_vendor = "Unknown";
        m_version = "1.0";
        m_numInputChannels = 2;
        m_numOutputChannels = 2;

        // Load the library
        loadLibrary(path);

        // Initialize ARA-specific state
        m_musicalContext.tempo = 120.0;
        m_musicalContext.timeSigNumerator = 4;
        m_musicalContext.timeSigDenominator = 4;
        m_musicalContext.startTimeInSeconds = 0.0;
    }

    ~ARAPluginInstanceImpl() {
        if (m_libraryHandle) {
            #ifdef _WIN32
            FreeLibrary((HMODULE)m_libraryHandle);
            #else
            dlclose(m_libraryHandle);
            #endif
        }
    }

    // PluginInstance interface implementation
    std::string getName() const override { return m_name; }
    std::string getVendor() const override { return m_vendor; }
    std::string getVersion() const override { return m_version; }
    PluginFormat getFormat() const override { return PluginFormat::ARA; }
    
    void prepareToPlay(double sampleRate, int maxSamplesPerBlock) override {
        m_sampleRate = sampleRate;
        m_blockSize = maxSamplesPerBlock;
        
        if (m_araInterface && m_araInterface->prepareToPlay) {
            m_araInterface->prepareToPlay(m_araInterface->handle, sampleRate, maxSamplesPerBlock);
        }
    }
    
    void processBlock(float** inputBuffers, float** outputBuffers, int numInputs, int numOutputs, int numSamples) override {
        if (!m_enabled || !m_araInterface || !m_araInterface->processAudio) {
            // Pass through audio if plugin is disabled or can't process
            for (int i = 0; i < std::min(numInputs, numOutputs); i++) {
                std::copy(inputBuffers[i], inputBuffers[i] + numSamples, outputBuffers[i]);
            }
            return;
        }
        
        // Call the ARA plugin's process function
        m_araInterface->processAudio(
            m_araInterface->handle, 
            inputBuffers, outputBuffers,
            numInputs, numOutputs, numSamples
        );
    }
    
    void releaseResources() override {
        if (m_araInterface && m_araInterface->releaseResources) {
            m_araInterface->releaseResources(m_araInterface->handle);
        }
    }
    
    int getNumParameters() const override { return m_parameters.size(); }
    
    PluginParameter getParameter(int index) const override {
        if (index >= 0 && index < m_parameters.size()) {
            return m_parameters[index];
        }
        
        // Return a default parameter if out of bounds
        PluginParameter param;
        param.id = "invalid";
        param.name = "Invalid Parameter";
        param.type = ParameterType::FLOAT;
        param.minValue = 0.0;
        param.maxValue = 1.0;
        param.defaultValue = 0.0;
        param.currentValue = 0.0;
        param.automatable = false;
        return param;
    }
    
    void setParameterValue(int index, double value) override {
        if (index >= 0 && index < m_parameters.size()) {
            m_parameters[index].currentValue = value;
            
            if (m_araInterface && m_araInterface->setParameterValue) {
                m_araInterface->setParameterValue(m_araInterface->handle, index, value);
            }
        }
    }
    
    double getParameterValue(int index) const override {
        if (index >= 0 && index < m_parameters.size()) {
            return m_parameters[index].currentValue;
        }
        return 0.0;
    }
    
    void setParameterValueByName(const std::string& name, double value) override {
        for (int i = 0; i < m_parameters.size(); i++) {
            if (m_parameters[i].name == name || m_parameters[i].id == name) {
                setParameterValue(i, value);
                return;
            }
        }
    }
    
    int getNumInputChannels() const override { return m_numInputChannels; }
    int getNumOutputChannels() const override { return m_numOutputChannels; }
    
    bool hasEditor() const override { return m_hasEditor; }
    
    void* openEditor(void* parentWindow) override {
        if (!m_hasEditor || !m_araInterface || !m_araInterface->openEditor) {
            return nullptr;
        }
        
        m_editorOpen = true;
        return m_araInterface->openEditor(m_araInterface->handle, parentWindow);
    }
    
    void closeEditor() override {
        if (m_editorOpen && m_araInterface && m_araInterface->closeEditor) {
            m_araInterface->closeEditor(m_araInterface->handle);
            m_editorOpen = false;
        }
    }
    
    bool isEnabled() const override { return m_enabled; }
    void setEnabled(bool enabled) override { m_enabled = enabled; }

    // ARA-specific functionality
    bool connectARAHost(void* araHostInterface) override {
        if (!m_araInterface || !m_araInterface->connectHost) {
            return false;
        }
        
        return m_araInterface->connectHost(m_araInterface->handle, araHostInterface);
    }
    
    bool hasMusicalContext() const override {
        return m_araInterface && m_araInterface->supportsMusicalContext;
    }
    
    bool hasAudioSource() const override {
        return m_araInterface && m_araInterface->supportsAudioSource;
    }
    
    bool loadAudioFromFile(const std::string& filePath) override {
        if (!m_araInterface || !m_araInterface->loadAudioFromFile) {
            return false;
        }
        
        return m_araInterface->loadAudioFromFile(m_araInterface->handle, filePath.c_str());
    }
    
    bool setMusicalContext(const MusicalContext& context) override {
        if (!m_araInterface || !m_araInterface->setMusicalContext) {
            return false;
        }
        
        m_musicalContext = context;
        
        // Convert to ARA's internal musical context representation
        void* araContext = m_araInterface->createMusicalContext(
            m_araInterface->handle,
            context.tempo,
            context.timeSigNumerator,
            context.timeSigDenominator,
            context.startTimeInSeconds
        );
        
        if (!araContext) {
            return false;
        }
        
        // Add bar positions
        for (const auto& barPos : context.barPositions) {
            m_araInterface->addBarPosition(araContext, barPos.first, barPos.second);
        }
        
        bool result = m_araInterface->setMusicalContext(m_araInterface->handle, araContext);
        m_araInterface->destroyMusicalContext(araContext);
        
        return result;
    }
    
    MusicalContext getMusicalContext() const override {
        return m_musicalContext;
    }

private:
    // ARA interface structure used to communicate with the plugin
    struct ARAInterface {
        void* handle;                // Plugin instance handle
        void* libraryHandle;         // Loaded library handle
        
        // Core functions
        void (*prepareToPlay)(void* handle, double sampleRate, int blockSize);
        void (*processAudio)(void* handle, float** inputs, float** outputs, int numInputs, int numOutputs, int numSamples);
        void (*releaseResources)(void* handle);
        
        // Parameter handling
        int (*getNumParameters)(void* handle);
        const char* (*getParameterName)(void* handle, int index);
        double (*getParameterValue)(void* handle, int index);
        void (*setParameterValue)(void* handle, int index, double value);
        
        // Editor
        void* (*openEditor)(void* handle, void* parent);
        void (*closeEditor)(void* handle);
        
        // ARA-specific functions
        bool (*connectHost)(void* handle, void* hostInterface);
        bool (*supportsMusicalContext);
        bool (*supportsAudioSource);
        bool (*loadAudioFromFile)(void* handle, const char* filePath);
        void* (*createMusicalContext)(void* handle, double tempo, int timeSigNum, int timeSigDenom, double startTime);
        void (*addBarPosition)(void* musicalCtx, double timeInSeconds, double barNumber);
        bool (*setMusicalContext)(void* handle, void* musicalContext);
        void (*destroyMusicalContext)(void* musicalContext);
    };

    bool loadLibrary(const std::string& path) {
        // Load the plugin DLL/SO
        #ifdef _WIN32
        m_libraryHandle = LoadLibraryA(path.c_str());
        #else
        m_libraryHandle = dlopen(path.c_str(), RTLD_LAZY);
        #endif

        if (!m_libraryHandle) {
            std::cerr << "Failed to load ARA plugin: " << path << std::endl;
            return false;
        }

        // Get plugin factory function
        #ifdef _WIN32
        ARAFactoryFunction factoryFunc = (ARAFactoryFunction)GetProcAddress((HMODULE)m_libraryHandle, "ARAPluginFactory");
        ARAGetAPIVersionFunction versionFunc = (ARAGetAPIVersionFunction)GetProcAddress((HMODULE)m_libraryHandle, "ARAGetAPIVersion");
        ARAGetPluginNameFunction nameFunc = (ARAGetPluginNameFunction)GetProcAddress((HMODULE)m_libraryHandle, "ARAGetPluginName");
        ARAGetPluginVendorFunction vendorFunc = (ARAGetPluginVendorFunction)GetProcAddress((HMODULE)m_libraryHandle, "ARAGetPluginVendor");
        ARAGetPluginVersionFunction pluginVersionFunc = (ARAGetPluginVersionFunction)GetProcAddress((HMODULE)m_libraryHandle, "ARAGetPluginVersion");
        #else
        ARAFactoryFunction factoryFunc = (ARAFactoryFunction)dlsym(m_libraryHandle, "ARAPluginFactory");
        ARAGetAPIVersionFunction versionFunc = (ARAGetAPIVersionFunction)dlsym(m_libraryHandle, "ARAGetAPIVersion");
        ARAGetPluginNameFunction nameFunc = (ARAGetPluginNameFunction)dlsym(m_libraryHandle, "ARAGetPluginName");
        ARAGetPluginVendorFunction vendorFunc = (ARAGetPluginVendorFunction)dlsym(m_libraryHandle, "ARAGetPluginVendor");
        ARAGetPluginVersionFunction pluginVersionFunc = (ARAGetPluginVersionFunction)dlsym(m_libraryHandle, "ARAGetPluginVersion");
        #endif

        if (!factoryFunc) {
            std::cerr << "ARA plugin is missing factory function" << std::endl;
            return false;
        }

        // Get plugin metadata
        if (nameFunc) m_name = nameFunc();
        if (vendorFunc) m_vendor = vendorFunc();
        if (pluginVersionFunc) m_version = pluginVersionFunc();

        // Create ARA interface and instance
        m_araInterface = std::make_unique<ARAInterface>();
        m_araInterface->libraryHandle = m_libraryHandle;
        m_araInterface->handle = factoryFunc(nullptr); // nullptr for default configuration

        if (!m_araInterface->handle) {
            std::cerr << "Failed to create ARA plugin instance" << std::endl;
            return false;
        }

        // Load ARA interface functions
        loadARAFunctions();
        
        // Scan plugin parameters
        scanParameters();
        
        return true;
    }

    void loadARAFunctions() {
        #ifdef _WIN32
        #define LOAD_FUNCTION(name) \
            m_araInterface->name = (decltype(m_araInterface->name))GetProcAddress( \
                (HMODULE)m_libraryHandle, "ARA" #name)
        #else
        #define LOAD_FUNCTION(name) \
            m_araInterface->name = (decltype(m_araInterface->name))dlsym( \
                m_libraryHandle, "ARA" #name)
        #endif

        // Load core functions
        LOAD_FUNCTION(prepareToPlay);
        LOAD_FUNCTION(processAudio);
        LOAD_FUNCTION(releaseResources);
        
        // Load parameter functions
        LOAD_FUNCTION(getNumParameters);
        LOAD_FUNCTION(getParameterName);
        LOAD_FUNCTION(getParameterValue);
        LOAD_FUNCTION(setParameterValue);
        
        // Load editor functions
        LOAD_FUNCTION(openEditor);
        LOAD_FUNCTION(closeEditor);
        
        // Load ARA-specific functions
        LOAD_FUNCTION(connectHost);
        LOAD_FUNCTION(supportsMusicalContext);
        LOAD_FUNCTION(supportsAudioSource);
        LOAD_FUNCTION(loadAudioFromFile);
        LOAD_FUNCTION(createMusicalContext);
        LOAD_FUNCTION(addBarPosition);
        LOAD_FUNCTION(setMusicalContext);
        LOAD_FUNCTION(destroyMusicalContext);

        // Determine if plugin has an editor
        m_hasEditor = (m_araInterface->openEditor != nullptr);
        
        #undef LOAD_FUNCTION
    }

    void scanParameters() {
        m_parameters.clear();
        
        if (!m_araInterface || !m_araInterface->getNumParameters) {
            return;
        }
        
        int numParams = m_araInterface->getNumParameters(m_araInterface->handle);
        for (int i = 0; i < numParams; i++) {
            PluginParameter param;
            param.id = std::to_string(i);
            
            if (m_araInterface->getParameterName) {
                param.name = m_araInterface->getParameterName(m_araInterface->handle, i);
            } else {
                param.name = "Parameter " + std::to_string(i);
            }
            
            // Default parameter ranges for ARA plugins
            param.type = ParameterType::FLOAT;
            param.minValue = 0.0;
            param.maxValue = 1.0;
            param.defaultValue = 0.0;
            
            if (m_araInterface->getParameterValue) {
                param.currentValue = m_araInterface->getParameterValue(m_araInterface->handle, i);
            } else {
                param.currentValue = 0.0;
            }
            
            param.automatable = true;
            m_parameters.push_back(param);
        }
    }

    // Member variables
    std::string m_path;
    std::string m_name;
    std::string m_vendor;
    std::string m_version;
    void* m_libraryHandle = nullptr;
    std::unique_ptr<ARAInterface> m_araInterface;
    std::vector<PluginParameter> m_parameters;
    bool m_enabled = true;
    bool m_hasEditor = false;
    bool m_editorOpen = false;
    int m_numInputChannels = 2;
    int m_numOutputChannels = 2;
    double m_sampleRate = 44100.0;
    int m_blockSize = 1024;
    MusicalContext m_musicalContext;
};

// ARA Plugin Scanner implementation
std::vector<std::string> ARAPluginScanner::scanDirectory(const std::string& directory, PluginFormat format) {
    std::vector<std::string> pluginPaths;
    
    if (format != PluginFormat::ARA) {
        return pluginPaths;
    }
    
    try {
        if (!fs::exists(directory) || !fs::is_directory(directory)) {
            return pluginPaths;
        }
        
        // Look for ARA plugins in directory
        for (const auto& entry : fs::recursive_directory_iterator(directory)) {
            if (fs::is_regular_file(entry) && isARAPlugin(entry.path().string())) {
                pluginPaths.push_back(entry.path().string());
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error scanning for ARA plugins: " << e.what() << std::endl;
    }
    
    return pluginPaths;
}

std::shared_ptr<PluginInstance> ARAPluginScanner::loadPlugin(const std::string& path, PluginFormat format) {
    std::cout << "Loading ARA plugin: " << path << std::endl;
    
    // Check if the format is correct when specified
    if (format != PluginFormat::UNKNOWN && format != PluginFormat::ARA) {
        std::cerr << "Wrong format requested for ARA plugin" << std::endl;
        return nullptr;
    }
    
    try {
        if (!std::filesystem::exists(path)) {
            std::cerr << "Plugin file does not exist: " << path << std::endl;
            return nullptr;
        }
        
        // Create the plugin instance
        auto plugin = std::make_shared<ARAPlugin>(path);
        if (!plugin->initialize()) {
            std::cerr << "Failed to initialize ARA plugin: " << path << std::endl;
            return nullptr;
        }
        
        return plugin;
    }
    catch (const std::exception& e) {
        std::cerr << "Error loading ARA plugin: " << e.what() << std::endl;
        return nullptr;
    }
}

bool ARAPluginScanner::isARAPlugin(const std::string& path) const {
    // Check if file has the correct extension for an ARA plugin
    std::string extension = fs::path(path).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    if (extension != ".dll" && extension != ".so" && extension != ".dylib") {
        return false;
    }
    
    // Try to load the library and check for ARA entry points
    void* libHandle = nullptr;
    
    #ifdef _WIN32
    libHandle = LoadLibraryA(path.c_str());
    #else
    libHandle = dlopen(path.c_str(), RTLD_LAZY);
    #endif
    
    if (!libHandle) {
        return false;
    }
    
    // Check for ARA entry point
    bool isARA = false;
    
    #ifdef _WIN32
    isARA = (GetProcAddress((HMODULE)libHandle, "ARAPluginFactory") != nullptr);
    FreeLibrary((HMODULE)libHandle);
    #else
    isARA = (dlsym(libHandle, "ARAPluginFactory") != nullptr);
    dlclose(libHandle);
    #endif
    
    return isARA;
}