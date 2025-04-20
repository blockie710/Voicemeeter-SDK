/**
 * aax_plugin.h
 * 
 * AAX plugin format implementation
 * Handles loading, processing, and parameter management for AAX plugins
 */

#ifndef AAX_PLUGIN_H
#define AAX_PLUGIN_H

#include "plugin_interface.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>

// Forward declarations for AAX SDK types to avoid including the whole SDK in header
namespace AAX {
    class IACFPluginDefinition;
    class IController;
    class IEffectParameters;
    class IComponentDescriptor;
}

class AAXPlugin : public PluginInstance {
public:
    AAXPlugin(const std::string& path);
    ~AAXPlugin() override;

    // PluginInstance interface implementation
    std::string getName() const override;
    std::string getVendor() const override;
    std::string getVersion() const override;
    PluginFormat getFormat() const override { return PluginFormat::AAX; }

    // Audio processing
    void prepareToPlay(double sampleRate, int maxSamplesPerBlock) override;
    void processBlock(float** inputBuffers, float** outputBuffers, int numInputs, int numOutputs, int numSamples) override;
    void releaseResources() override;

    // Parameter handling
    int getNumParameters() const override;
    PluginParameter getParameter(int index) const override;
    void setParameterValue(int index, double value) override;
    double getParameterValue(int index) const override;
    void setParameterValueByName(const std::string& name, double value) override;

    // Plugin I/O configuration
    int getNumInputChannels() const override;
    int getNumOutputChannels() const override;
    bool hasEditor() const override;
    void* openEditor(void* parentWindow) override;
    void closeEditor() override;

private:
    // Path to the AAX module
    std::string m_path;
    
    // Module/DLL handle
    void* m_moduleHandle = nullptr;
    
    // AAX interfaces
    AAX::IACFPluginDefinition* m_pluginDefinition = nullptr;
    AAX::IController* m_controller = nullptr;
    AAX::IEffectParameters* m_parameters = nullptr;
    
    // AAX processing state
    double m_sampleRate = 0.0;
    int m_blockSize = 0;
    bool m_isActive = false;
    
    // Parameter cache for quick lookups
    std::unordered_map<int, PluginParameter> m_parameterCache;
    std::unordered_map<std::string, int> m_parameterNameToIndex;
    
    // Plugin information
    std::string m_name;
    std::string m_vendor;
    std::string m_version;
    int m_numInputChannels = 0;
    int m_numOutputChannels = 0;
    
    // Editor
    void* m_editorHandle = nullptr;
    bool m_hasEditor = false;
    
    // Initialize and load the AAX plugin
    bool loadPlugin();
    bool initializePlugin();
    void cacheParameters();
    
    // Process context for AAX
    struct ProcessContext {
        float** inputBuffers;
        float** outputBuffers;
        int numSamples;
    };
    
    // Buffer management for AAX format conversion
    std::vector<float> m_inputInterleavedBuffer;
    std::vector<float> m_outputInterleavedBuffer;
};

// AAX Plugin Scanner implementation
class AAXPluginScanner : public PluginScanner {
public:
    AAXPluginScanner();
    ~AAXPluginScanner() override;
    
    // Scan directories for AAX plugins
    std::vector<std::string> scanDirectory(const std::string& directory, PluginFormat format) override;
    
    // Load a specific plugin
    std::shared_ptr<PluginInstance> loadPlugin(const std::string& path, PluginFormat format) override;
    
private:
    // Standard AAX plugin locations
    std::vector<std::string> getDefaultAAXPaths() const;
};

#endif // AAX_PLUGIN_H