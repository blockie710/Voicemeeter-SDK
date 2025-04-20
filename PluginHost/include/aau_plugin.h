/**
 * aau_plugin.h
 * 
 * Audio Units (AU/AAU) plugin format implementation
 * Handles loading, processing, and parameter management for Audio Units plugins
 * Note: Audio Units are macOS-specific, but we include the interface for cross-platform compatibility
 */

#ifndef AAU_PLUGIN_H
#define AAU_PLUGIN_H

#include "plugin_interface.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>

// Forward declarations for AudioUnit types
// These are placeholders since AAU is macOS specific
struct AudioUnitStruct;
typedef AudioUnitStruct* AudioUnit;
typedef struct AudioComponentDescription AudioComponentDescription;
typedef struct AUParameterInfo AUParameterInfo;

class AAUPlugin : public PluginInstance {
public:
    AAUPlugin(const std::string& path);
    ~AAUPlugin() override;

    // PluginInstance interface implementation
    std::string getName() const override;
    std::string getVendor() const override;
    std::string getVersion() const override;
    PluginFormat getFormat() const override { return PluginFormat::AAU; }

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
    // Path or identifier for the AU component
    std::string m_path;
    
    // Audio Unit handle (macOS specific)
    AudioUnit m_audioUnit = nullptr;
    
    // AU component description
    AudioComponentDescription* m_componentDesc = nullptr;
    
    // AU processing state
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
    void* m_editorView = nullptr;
    bool m_hasEditor = false;
    
    // Initialize and load the AU plugin (not implemented on Windows)
    bool loadPlugin();
    bool initializePlugin();
    void cacheParameters();
    
    // Buffer management for AU format
    std::vector<float> m_inputBuffers;
    std::vector<float> m_outputBuffers;
    std::vector<float*> m_inputChannels;
    std::vector<float*> m_outputChannels;
};

// Audio Unit Plugin Scanner (not fully functional on Windows)
class AAUPluginScanner : public PluginScanner {
public:
    AAUPluginScanner();
    ~AAUPluginScanner() override;
    
    // AU components are registered in the system on macOS, not stored in directories
    std::vector<std::string> scanDirectory(const std::string& directory, PluginFormat format) override;
    
    // Load a specific plugin
    std::shared_ptr<PluginInstance> loadPlugin(const std::string& path, PluginFormat format) override;
    
private:
    // For macOS, this would access the system component registry
    std::vector<std::string> getRegisteredAUComponents() const;
};

#endif // AAU_PLUGIN_H