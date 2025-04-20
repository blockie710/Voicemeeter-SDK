/**
 * voicemeeter_integration.h
 * 
 * Integration with Voicemeeter Remote API
 * Handles communication with Voicemeeter (Standard, Banana, Potato)
 */

#ifndef VOICEMEETER_INTEGRATION_H
#define VOICEMEETER_INTEGRATION_H

#include <string>
#include <vector>
#include <functional>
#include <memory>

// Include the Voicemeeter Remote API header
#include "../../VoicemeeterRemote.h"

namespace VoicemeeterIntegration {

// Voicemeeter types
enum class VoicemeeterType {
    STANDARD = 1,    // Voicemeeter
    BANANA = 2,      // Voicemeeter Banana
    POTATO = 3,      // Voicemeeter Potato
    POTATO_X64 = 6   // Voicemeeter Potato x64
};

// Audio callback for processing
using AudioProcessCallback = std::function<void(float**, float**, int, int, int)>;

// Forward declaration for implementation details
class VoicemeeterClientImpl;

class VoicemeeterClient {
public:
    VoicemeeterClient();
    ~VoicemeeterClient();

    // Initialize the connection to Voicemeeter
    bool initialize();
    
    // Close the connection to Voicemeeter
    void shutdown();

    // Get info about the running Voicemeeter instance
    VoicemeeterType getVoicemeeterType();
    long getVoicemeeterVersion();
    
    // Launch Voicemeeter if not running
    bool launchVoicemeeter(VoicemeeterType type);

    // Register for audio callbacks
    bool registerAudioCallback(AudioProcessCallback callback, 
                              bool processInputs = true,
                              bool processOutputs = true);
    
    // Start/stop audio processing
    bool startAudioProcessing();
    bool stopAudioProcessing();
    
    // Set and get parameters
    bool setParameter(const std::string& paramName, float value);
    bool getParameter(const std::string& paramName, float& value);
    
    // Get audio levels
    bool getInputLevel(int channel, float& level);
    bool getOutputLevel(int channel, float& level);
    
    // Get available strip and bus counts
    int getNumStrips() const;
    int getNumBuses() const;
    
    // Check if parameters have changed (to update UI)
    bool haveParametersChanged();

private:
    // Implementation details hidden with PIMPL pattern
    std::unique_ptr<VoicemeeterClientImpl> m_impl;
    
    // Audio callback registered with Voicemeeter
    static long __stdcall audioCallback(void* lpUser, long nCommand, void* lpData, long nnn);
    
    // Member variables for backward compatibility
    bool m_initialized;
    VoicemeeterType m_type;
    long m_version;
};

} // namespace VoicemeeterIntegration

#endif // VOICEMEETER_INTEGRATION_H