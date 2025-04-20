#ifndef VM_PLUGIN_HOST_H
#define VM_PLUGIN_HOST_H

#include <string>
#include <memory>
#include "../include/plugin_interface.h"

/**
 * VMPluginHost - Simple wrapper class for plugin handling used by tests
 *
 * This provides a simpler interface to load and test plugins compared to
 * the complete plugin host implementation.
 */
class VMPluginHost {
public:
    VMPluginHost();
    ~VMPluginHost();
    
    // Plugin loading
    bool LoadPlugin(const std::string& path);
    void UnloadPlugin();
    
    // Plugin info
    std::string GetPluginName() const;
    std::string GetPluginVendor() const;
    std::string GetPluginProduct() const;
    
    // Parameters
    int GetNumParameters() const;
    std::string GetParameterName(int index) const;
    float GetParameter(int index) const;
    bool SetParameter(int index, float value);
    std::string GetParameterDisplay(int index) const;
    bool GetParameterProperties(int index, float* min, float* max, float* defaultVal);
    
    // Audio processing
    void ProcessAudio(float* inL, float* inR, float* outL, float* outR, int numSamples, float sampleRate);
    
private:
    std::shared_ptr<PluginInstance> m_plugin;
    PluginFormat detectFormat(const std::string& path);
};

#endif // VM_PLUGIN_HOST_H
