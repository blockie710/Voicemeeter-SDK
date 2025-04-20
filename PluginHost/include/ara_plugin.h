/**
 * ara_plugin.h
 * 
 * ARA (Audio Random Access) Plugin support for Voicemeeter Plugin Host
 * Handles loading and processing ARA-compatible audio plugins
 */

#ifndef ARA_PLUGIN_H
#define ARA_PLUGIN_H

#include "plugin_interface.h"
#include <string>
#include <memory>
#include <vector>

class ARAPluginScanner : public PluginScanner {
public:
    ARAPluginScanner();
    ~ARAPluginScanner();
    
    std::vector<PluginDescription> scanDirectory(const std::string& directory) override;
    std::shared_ptr<PluginInstance> loadPlugin(const std::string& path) override;

private:
    std::vector<std::string> getPluginPaths(const std::string& directory);
};

class ARAPlugin : public PluginInstance {
public:
    ARAPlugin(const std::string& path);
    ~ARAPlugin() override;
    
    bool initialize() override;
    void process(float** inputs, float** outputs, int numInputs, int numOutputs, int numSamples) override;
    void suspend() override;
    void resume() override;
    
    // Implementation of base class methods
    std::string getName() const override;
    std::string getVendor() const override;
    std::string getVersion() const override;
    std::string getUniqueId() const override;
    const char* getFormatName() const override { return "ARA"; }
    
    bool hasEditor() const override;
    bool showEditor(void* parent) override;
    void hideEditor() override;
    
    int getParameterCount() const override;
    PluginParameter getParameter(int index) const override;
    bool setParameter(int index, float value) override;
    
    int getPresetCount() const override;
    std::string getPresetName(int index) const override;
    bool loadPreset(int index) override;
    bool savePreset(const std::string& name) override;
    
private:
    // Path to the plugin file
    std::string m_path;
    
    // Basic plugin info
    std::string m_name;
    std::string m_vendor;
    std::string m_version;
    std::string m_uniqueId;
    
    // Parameter info
    std::vector<PluginParameter> m_parameters;
    
    // Editor state
    bool m_hasEditor = false;
    void* m_editorHandle = nullptr;
    
    // Processing state
    bool m_isSuspended = true;
    
    // ARA-specific implementation details
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

#endif // ARA_PLUGIN_H