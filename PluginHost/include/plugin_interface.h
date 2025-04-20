/**
 * plugin_interface.h
 * 
 * Plugin interface for Voicemeeter Plugin Host
 * Supports VST3, AAX, AAU, ARA, LUA, and REAPER plugin formats
 */

#ifndef PLUGIN_INTERFACE_H
#define PLUGIN_INTERFACE_H

#include <string>
#include <vector>
#include <memory>

// Plugin format types
enum class PluginFormat {
    VST3,
    AAX,
    AAU,
    ARA,    // New: Audio Random Access
    LUA,    // New: LUA script plugins
    REAPER, // New: REAPER extensions and JSFX
    UNKNOWN
};

// Parameter types for plugin parameters
enum class ParameterType {
    FLOAT,
    INT,
    BOOL,
    STRING,
    ENUM
};

// Plugin parameter descriptor
struct PluginParameter {
    std::string id;
    std::string name;
    ParameterType type;
    double minValue;
    double maxValue;
    double defaultValue;
    double currentValue;
    std::vector<std::string> enumValues; // For enum parameters
    bool automatable;
};

// Base plugin interface that will be implemented by specific format handlers
class PluginInstance {
public:
    virtual ~PluginInstance() = default;
    
    // Plugin information
    virtual std::string getName() const = 0;
    virtual std::string getVendor() const = 0;
    virtual std::string getVersion() const = 0;
    virtual PluginFormat getFormat() const = 0;
    
    // Audio processing
    virtual void prepareToPlay(double sampleRate, int maxSamplesPerBlock) = 0;
    virtual void processBlock(float** inputBuffers, float** outputBuffers, int numInputs, int numOutputs, int numSamples) = 0;
    virtual void releaseResources() = 0;
    
    // Parameter handling
    virtual int getNumParameters() const = 0;
    virtual PluginParameter getParameter(int index) const = 0;
    virtual void setParameterValue(int index, double value) = 0;
    virtual double getParameterValue(int index) const = 0;
    virtual void setParameterValueByName(const std::string& name, double value) = 0;
    
    // Plugin I/O configuration
    virtual int getNumInputChannels() const = 0;
    virtual int getNumOutputChannels() const = 0;
    virtual bool hasEditor() const = 0;
    virtual void* openEditor(void* parentWindow) = 0;
    virtual void closeEditor() = 0;
    
    // Advanced features
    virtual bool supportsFeature(const std::string& featureName) const { return false; }
    virtual void* getExtension(const std::string& extensionId) { return nullptr; }
    
    // Enable/disable state - default implementation
    virtual bool isEnabled() const { return true; }
    virtual void setEnabled(bool enabled) {}
    
    // Get supported plugin formats
    static std::vector<PluginFormat> getSupportedFormats();
};

// Plugin scanner interface to discover plugins
class PluginScanner {
public:
    virtual ~PluginScanner() = default;
    
    // Scan for plugins of a specific format in a directory
    virtual std::vector<std::string> scanDirectory(const std::string& directory, PluginFormat format) = 0;
    
    // Load a plugin by path
    virtual std::shared_ptr<PluginInstance> loadPlugin(const std::string& path, PluginFormat format) = 0;
};

// Factory function to create scanner for a specific format
std::unique_ptr<PluginScanner> createPluginScanner(PluginFormat format);

#endif // PLUGIN_INTERFACE_H