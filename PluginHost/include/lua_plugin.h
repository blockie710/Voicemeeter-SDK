#ifndef LUA_PLUGIN_H
#define LUA_PLUGIN_H

#include "plugin_interface.h"
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <map>

/**
 * LuaPluginInstance - Provides support for LUA scripts as audio plugins
 * 
 * Allows developers to create audio plugins using LUA scripting language,
 * with direct access to audio buffers and parameter management.
 */
class LuaPluginInstance : public PluginInstance {
public:
    LuaPluginInstance();
    ~LuaPluginInstance();

    // PluginInstance interface implementation
    std::string getName() const override;
    std::string getVendor() const override;
    std::string getVersion() const override;
    PluginFormat getFormat() const override { return PluginFormat::LUA; }
    
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
    
    // LUA specific functionality
    bool loadScript(const std::string& scriptPath);
    bool loadScriptFromText(const std::string& scriptText);
    bool reloadScript();
    bool executeFunction(const std::string& functionName);
    void registerCallback(const std::string& eventName, std::function<void()> callback);
    
    // LUA script introspection
    std::vector<std::string> getAvailableFunctions() const;
    bool functionExists(const std::string& functionName) const;

    // Setting values for LUA
    void setGlobalNumber(const std::string& name, double value);
    void setGlobalString(const std::string& name, const std::string& value);
    void setGlobalBoolean(const std::string& name, bool value);
    
    // Getting values from LUA
    double getGlobalNumber(const std::string& name) const;
    std::string getGlobalString(const std::string& name) const;
    bool getGlobalBoolean(const std::string& name) const;
    
    // Error handling
    bool hasError() const;
    std::string getLastError() const;
    void clearError();
    
    // Expose additional functionality to LUA scripts
    void exposeFunction(const std::string& name, std::function<void()> function);
    void exposeObject(const std::string& name, void* object);
    
    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled) { m_enabled = enabled; }

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    bool m_enabled = true;
    
    // Script metadata
    std::string m_name;
    std::string m_vendor;
    std::string m_version;
    std::string m_scriptPath;
    
    // Cache of parameters to avoid LUA lookups in processBlock
    std::vector<PluginParameter> m_parameters;
    std::map<std::string, int> m_parameterNameToIndex;
    
    void initializeLuaEnvironment();
    void extractMetadata();
    void extractParameters();
};

// LUA Plugin Scanner implementation
class LuaPluginScanner : public PluginScanner {
public:
    std::vector<std::string> scanDirectory(const std::string& directory, PluginFormat format) override;
    std::shared_ptr<PluginInstance> loadPlugin(const std::string& path, PluginFormat format) override;
    
private:
    bool isLuaScript(const std::string& path) const;
};

#endif // LUA_PLUGIN_H