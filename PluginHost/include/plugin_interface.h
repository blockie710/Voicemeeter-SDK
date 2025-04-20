/**
 * plugin_interface.h
 * 
 * Base interface for audio plugins in the Voicemeeter Plugin Host
 * Provides a common interface for all plugin types (VST3, AAX, etc.)
 */

#ifndef PLUGIN_INTERFACE_H
#define PLUGIN_INTERFACE_H

#include <string>
#include <vector>
#include <memory>

// Supported plugin formats
enum class PluginFormat {
    UNKNOWN = 0,
    VST3 = 1,
    AAX = 2,
    AAU = 3, // AudioUnit
    ARA = 4, // ARA2
    LUA = 5, // Lua scripts
    REAPER = 6 // JSFX plugins
};

// Parameter information
struct PluginParameter {
    std::string name;
    std::string label;
    std::string unit;
    float minValue = 0.0f;
    float maxValue = 1.0f;
    float defaultValue = 0.0f;
    float currentValue = 0.0f;
    bool isAutomatable = true;
    bool isDiscrete = false;
    int stepCount = 0;
    
    // For discrete parameters
    std::vector<std::string> valueNames;
};

// Plugin description with basic info
struct PluginDescription {
    std::string name;
    std::string vendor;
    std::string version;
    std::string path;
    std::string uniqueId;
    PluginFormat format;
    int numInputs = 0;
    int numOutputs = 0;
    bool hasMidiInput = false;
    bool hasMidiOutput = false;
    bool hasEditor = false;
};

// Base interface for plugin instances
class PluginInstance {
public:
    virtual ~PluginInstance() = default;
    
    // Basic initialization
    virtual bool initialize() = 0;
    
    // Audio processing
    virtual void process(float** inputs, float** outputs, int numInputs, int numOutputs, int numSamples) = 0;
    virtual void suspend() = 0;
    virtual void resume() = 0;
    
    // Plugin information
    virtual std::string getName() const = 0;
    virtual std::string getVendor() const = 0;
    virtual std::string getVersion() const = 0;
    virtual std::string getUniqueId() const = 0;
    virtual const char* getFormatName() const = 0;
    
    // GUI editor
    virtual bool hasEditor() const = 0;
    virtual bool showEditor(void* parent) = 0;
    virtual void hideEditor() = 0;
    
    // Parameters
    virtual int getParameterCount() const = 0;
    virtual PluginParameter getParameter(int index) const = 0;
    virtual bool setParameter(int index, float value) = 0;
    
    // Presets
    virtual int getPresetCount() const = 0;
    virtual std::string getPresetName(int index) const = 0;
    virtual bool loadPreset(int index) = 0;
    virtual bool savePreset(const std::string& name) = 0;
    
    // Static method to get list of supported plugin formats
    static std::vector<PluginFormat> getSupportedFormats();
};

// Base interface for plugin scanners
class PluginScanner {
public:
    virtual ~PluginScanner() = default;
    
    // Scan a directory for plugins
    virtual std::vector<PluginDescription> scanDirectory(const std::string& directory) = 0;
    
    // Load a specific plugin by path
    virtual std::shared_ptr<PluginInstance> loadPlugin(const std::string& path) = 0;
};

// Factory function to create plugin scanner for a format
std::unique_ptr<PluginScanner> createPluginScanner(PluginFormat format);

#endif // PLUGIN_INTERFACE_H