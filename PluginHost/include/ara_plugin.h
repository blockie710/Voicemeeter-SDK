#ifndef ARA_PLUGIN_H
#define ARA_PLUGIN_H

#include "plugin_interface.h"
#include <memory>
#include <string>
#include <vector>

/**
 * ARA Plugin Interface
 * 
 * Support for Audio Random Access (ARA) plugins that allow deeper
 * integration between audio hosts and plugins, particularly for
 * time-based audio processing like melodyne, vocalign, etc.
 */
class ARAPluginInstance : public PluginInstance {
public:
    virtual ~ARAPluginInstance() = default;
    
    // ARA-specific functionality
    virtual bool supportsARA() const { return true; }
    virtual bool connectARAHost(void* araHostInterface) = 0;
    virtual bool hasMusicalContext() const = 0;
    virtual bool hasAudioSource() const = 0;
    virtual bool loadAudioFromFile(const std::string& filePath) = 0;
    
    // Musical context for ARA
    struct MusicalContext {
        double tempo;
        int timeSigNumerator;
        int timeSigDenominator;
        double startTimeInSeconds;
        std::vector<std::pair<double, double>> barPositions; // <timeInSeconds, barNumber>
    };
    
    virtual bool setMusicalContext(const MusicalContext& context) = 0;
    virtual MusicalContext getMusicalContext() const = 0;
};

// ARA Plugin Scanner implementation
class ARAPluginScanner : public PluginScanner {
public:
    std::vector<std::string> scanDirectory(const std::string& directory, PluginFormat format) override;
    std::shared_ptr<PluginInstance> loadPlugin(const std::string& path, PluginFormat format) override;
    
private:
    bool isARAPlugin(const std::string& path) const;
};

#endif // ARA_PLUGIN_H