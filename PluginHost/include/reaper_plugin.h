#ifndef REAPER_PLUGIN_H
#define REAPER_PLUGIN_H

#include "plugin_interface.h"
#include <memory>
#include <string>
#include <vector>
#include <functional>

/**
 * ReaperPluginInstance - Provides support for REAPER extensions and JSFX plugins
 * 
 * REAPER supports two main types of extensions:
 * 1. JSFX - EEL2 scripted audio effects (primarily used for audio processing)
 * 2. Extension plugins - DLLs that extend REAPER functionality
 * 
 * This class handles both types of REAPER plugins.
 */
class ReaperPluginInstance : public PluginInstance {
public:
    // Types of REAPER plugins
    enum class ReaperPluginType {
        JSFX,      // REAPER's built-in scripting format for audio effects
        EXTENSION  // DLL/SO/DYLIB extensions for REAPER
    };
    
    ReaperPluginInstance();
    ~ReaperPluginInstance();

    // PluginInstance interface implementation
    std::string getName() const override;
    std::string getVendor() const override;
    std::string getVersion() const override;
    PluginFormat getFormat() const override { return PluginFormat::REAPER; }
    
    void prepareToPlay(double sampleRate, int maxSamplesPerBlock) override;
    void processBlock(float** inputBuffers, float** outputBuffers, int numInputs, int numOutputs, int numSamples) override;
    void releaseResources() override;
    
    int getNumParameters() const override;
    PluginParameter getParameter(int index) const override;
    void setParameterValue(int index, double value) override;
    double getParameterValue(int index) const override;
    void setParameterValueByName(const std::string& name, double value) override;
    
    int getNumInputChannels() const override;
    int getNumOutputChannels() const override;
    bool hasEditor() const override;
    void* openEditor(void* parentWindow) override;
    void closeEditor() override;

    // REAPER-specific functionality
    bool isJSFX() const { return m_type == ReaperPluginType::JSFX; }
    bool isExtension() const { return m_type == ReaperPluginType::EXTENSION; }
    ReaperPluginType getReaperPluginType() const { return m_type; }
    
    // JSFX specific methods
    bool loadJSFXScript(const std::string& scriptPath);
    bool recompileJSFX();
    
    // Extension plugin specific methods
    bool loadExtension(const std::string& dllPath);
    void* invokeExtensionFunction(const std::string& functionName, void* data = nullptr);
    
    // REAPER plugin data
    struct ReaperPluginData {
        void* reaperVmInterface;   // For JSFX VM interaction
        void* reaperAPIInterface;  // For extension access to REAPER functions
        int apiVersion;
    };
    
    // Set REAPER-specific data needed for proper plugin operation
    void setReaperData(const ReaperPluginData& data);
    
    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled) { m_enabled = enabled; }
    
private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    bool m_enabled = true;
    ReaperPluginType m_type = ReaperPluginType::JSFX;
    
    // Plugin metadata
    std::string m_name;
    std::string m_vendor;
    std::string m_version;
    std::string m_filePath;
    
    // Parameter handling
    std::vector<PluginParameter> m_parameters;
    
    // Internal initialization
    void scanJSFXParameters();
    void scanExtensionParameters();
    void extractMetadata();
};

/**
 * REAPER Plugin Scanner implementation
 */
class ReaperPluginScanner : public PluginScanner {
public:
    std::vector<std::string> scanDirectory(const std::string& directory, PluginFormat format) override;
    std::shared_ptr<PluginInstance> loadPlugin(const std::string& path, PluginFormat format) override;
    
private:
    bool isJSFXScript(const std::string& path) const;
    bool isReaperExtension(const std::string& path) const;
};

#endif // REAPER_PLUGIN_H