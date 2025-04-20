/**
 * vst3_plugin.h
 * 
 * VST3 plugin format implementation
 * Handles loading, processing, and parameter management for VST3 plugins
 */

#ifndef VST3_PLUGIN_H
#define VST3_PLUGIN_H

#include "plugin_interface.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <atomic>

// Forward declarations for VST3 SDK types to avoid including the whole SDK in the header
namespace Steinberg {
    class IPluginFactory;
    namespace Vst {
        class IComponent;
        class IAudioProcessor;
        class IEditController;
        class IConnectionPoint;
    }
}

class VST3Plugin : public PluginInstance {
public:
    VST3Plugin(const std::string& path);
    ~VST3Plugin() override;

    // PluginInstance interface implementation
    std::string getName() const override;
    std::string getVendor() const override;
    std::string getVersion() const override;
    PluginFormat getFormat() const override { return PluginFormat::VST3; }

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
    // Path to the VST3 module
    std::string m_path;
    
    // Module/DLL handle
    void* m_moduleHandle = nullptr;
    
    // VST3 interfaces
    Steinberg::IPluginFactory* m_factory = nullptr;
    Steinberg::Vst::IComponent* m_component = nullptr;
    Steinberg::Vst::IAudioProcessor* m_processor = nullptr;
    Steinberg::Vst::IEditController* m_controller = nullptr;
    Steinberg::Vst::IConnectionPoint* m_connectionPoint = nullptr;
    
    // VST3 processing state
    double m_sampleRate = 0.0;
    int m_blockSize = 0;
    bool m_isActive = false;
    
    // Parameter cache for quick lookups
    std::unordered_map<int, PluginParameter> m_parameters;
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
    
    // Initialize and load the VST3 plugin
    bool loadPlugin();
    bool initializePlugin();
    void cacheParameters();
    void connectControllerAndComponent();
};

// VST3 Plugin Scanner implementation
class VST3PluginScanner : public PluginScanner {
public:
    VST3PluginScanner();
    ~VST3PluginScanner() override;
    
    // Scan directories for VST3 plugins
    std::vector<std::string> scanDirectory(const std::string& directory, PluginFormat format) override;
    
    // Load a specific plugin
    std::shared_ptr<PluginInstance> loadPlugin(const std::string& path, PluginFormat format) override;
};

#endif // VST3_PLUGIN_H