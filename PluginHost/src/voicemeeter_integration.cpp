#include "../include/voicemeeter_integration.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>

namespace VoicemeeterIntegration {

// Static data to hold callback information
static struct CallbackData {
    AudioProcessCallback userCallback;
    bool initialized = false;
} g_callbackData;

// Constructor
VoicemeeterClient::VoicemeeterClient() 
    : m_initialized(false), m_type(VoicemeeterType::STANDARD), m_version(0) {
}

// Destructor - ensure we clean up
VoicemeeterClient::~VoicemeeterClient() {
    if (m_initialized) {
        shutdown();
    }
}

// Initialize connection to Voicemeeter
bool VoicemeeterClient::initialize() {
    // Try to login to Voicemeeter
    long result = VBVMR_Login();
    
    if (result < 0) {
        std::cerr << "Failed to login to Voicemeeter: " << result << std::endl;
        return false;
    } else if (result == 1) {
        std::cout << "Voicemeeter is not running. Please start Voicemeeter or use launchVoicemeeter()." << std::endl;
        return false;
    }
    
    // Get Voicemeeter type and version
    long type = 0;
    if (VBVMR_GetVoicemeeterType(&type) == 0) {
        m_type = static_cast<VoicemeeterType>(type);
    } else {
        std::cerr << "Failed to get Voicemeeter type." << std::endl;
    }
    
    if (VBVMR_GetVoicemeeterVersion(&m_version) != 0) {
        std::cerr << "Failed to get Voicemeeter version." << std::endl;
    }
    
    m_initialized = true;
    return true;
}

// Close connection to Voicemeeter
void VoicemeeterClient::shutdown() {
    if (!m_initialized) return;
    
    // Stop audio processing if running
    stopAudioProcessing();
    
    // Logout from Voicemeeter
    VBVMR_Logout();
    m_initialized = false;
}

// Get Voicemeeter type (Standard, Banana, Potato)
VoicemeeterType VoicemeeterClient::getVoicemeeterType() {
    return m_type;
}

// Get Voicemeeter version
long VoicemeeterClient::getVoicemeeterVersion() {
    return m_version;
}

// Launch Voicemeeter if not already running
bool VoicemeeterClient::launchVoicemeeter(VoicemeeterType type) {
    long result = VBVMR_RunVoicemeeter(static_cast<long>(type));
    
    if (result != 0) {
        std::cerr << "Failed to launch Voicemeeter: " << result << std::endl;
        return false;
    }
    
    return true;
}

// The static audio callback function that will be registered with Voicemeeter
long __stdcall VoicemeeterClient::audioCallback(void* lpUser, long nCommand, void* lpData, long nnn) {
    // Handle different command types from Voicemeeter
    switch (nCommand) {
        case VBVMR_CBCOMMAND_STARTING:
        {
            // Audio engine is starting
            VBVMR_LPT_AUDIOINFO pAudioInfo = (VBVMR_LPT_AUDIOINFO)lpData;
            std::cout << "Audio engine starting: " << pAudioInfo->samplerate << " Hz, " 
                      << pAudioInfo->nbSamplePerFrame << " samples per frame" << std::endl;
            return 0;
        }
        
        case VBVMR_CBCOMMAND_ENDING:
        {
            // Audio engine is stopping
            std::cout << "Audio engine stopping" << std::endl;
            return 0;
        }
        
        case VBVMR_CBCOMMAND_CHANGE:
        {
            // Audio configuration changed
            std::cout << "Audio configuration changed" << std::endl;
            return 0;
        }
        
        case VBVMR_CBCOMMAND_BUFFER_IN:
        case VBVMR_CBCOMMAND_BUFFER_OUT:
        case VBVMR_CBCOMMAND_BUFFER_MAIN:
        {
            // Process audio buffers
            if (!g_callbackData.initialized || !g_callbackData.userCallback) {
                return 0;
            }
            
            VBVMR_LPT_AUDIOBUFFER pAudioBuffer = (VBVMR_LPT_AUDIOBUFFER)lpData;
            
            // Call the user's audio processing callback
            g_callbackData.userCallback(
                pAudioBuffer->audiobuffer_r, 
                pAudioBuffer->audiobuffer_w,
                pAudioBuffer->audiobuffer_nbi,
                pAudioBuffer->audiobuffer_nbo,
                pAudioBuffer->audiobuffer_nbs);
            
            return 0;
        }
        
        default:
            return 0;
    }
}

// Register an audio callback to process Voicemeeter audio
bool VoicemeeterClient::registerAudioCallback(AudioProcessCallback callback, 
                                              bool processInputs, 
                                              bool processOutputs) {
    if (!m_initialized) {
        std::cerr << "Cannot register audio callback: Voicemeeter not initialized" << std::endl;
        return false;
    }
    
    // Store the callback in the global data
    g_callbackData.userCallback = callback;
    g_callbackData.initialized = true;
    
    // Determine which audio streams we want to process
    long mode = 0;
    if (processInputs && processOutputs) {
        mode = VBVMR_AUDIOCALLBACK_MAIN;
    } else if (processInputs) {
        mode = VBVMR_AUDIOCALLBACK_IN;
    } else if (processOutputs) {
        mode = VBVMR_AUDIOCALLBACK_OUT;
    }
    
    // Register the callback with Voicemeeter
    char clientName[64] = "VoicemeeterPluginHost";
    long result = VBVMR_AudioCallbackRegister(mode, audioCallback, nullptr, clientName);
    
    if (result != 0) {
        std::cerr << "Failed to register audio callback: " << result << std::endl;
        g_callbackData.initialized = false;
        return false;
    }
    
    return true;
}

// Start audio processing
bool VoicemeeterClient::startAudioProcessing() {
    if (!m_initialized) {
        std::cerr << "Cannot start audio processing: Voicemeeter not initialized" << std::endl;
        return false;
    }
    
    long result = VBVMR_AudioCallbackStart();
    
    if (result != 0) {
        std::cerr << "Failed to start audio processing: " << result << std::endl;
        return false;
    }
    
    return true;
}

// Stop audio processing
bool VoicemeeterClient::stopAudioProcessing() {
    if (!m_initialized) {
        return false;
    }
    
    long result = VBVMR_AudioCallbackStop();
    
    if (result != 0) {
        std::cerr << "Failed to stop audio processing: " << result << std::endl;
        return false;
    }
    
    return true;
}

// Set a Voicemeeter parameter value
bool VoicemeeterClient::setParameter(const std::string& paramName, float value) {
    if (!m_initialized) {
        return false;
    }
    
    long result = VBVMR_SetParameterFloat(const_cast<char*>(paramName.c_str()), value);
    
    if (result != 0) {
        std::cerr << "Failed to set parameter " << paramName << ": " << result << std::endl;
        return false;
    }
    
    return true;
}

// Get a Voicemeeter parameter value
bool VoicemeeterClient::getParameter(const std::string& paramName, float& value) {
    if (!m_initialized) {
        return false;
    }
    
    long result = VBVMR_GetParameterFloat(const_cast<char*>(paramName.c_str()), &value);
    
    if (result != 0) {
        std::cerr << "Failed to get parameter " << paramName << ": " << result << std::endl;
        return false;
    }
    
    return true;
}

// Get level of input channel
bool VoicemeeterClient::getInputLevel(int channel, float& level) {
    if (!m_initialized) {
        return false;
    }
    
    // Get pre-fader input level (type 0)
    long result = VBVMR_GetLevel(0, channel, &level);
    
    if (result != 0) {
        return false;
    }
    
    return true;
}

// Get level of output channel
bool VoicemeeterClient::getOutputLevel(int channel, float& level) {
    if (!m_initialized) {
        return false;
    }
    
    // Get output level (type 3)
    long result = VBVMR_GetLevel(3, channel, &level);
    
    if (result != 0) {
        return false;
    }
    
    return true;
}

// Get number of strips (inputs) based on Voicemeeter type
int VoicemeeterClient::getNumStrips() const {
    switch (m_type) {
        case VoicemeeterType::STANDARD:
            return 3;  // 2 physical + 1 virtual
        case VoicemeeterType::BANANA:
            return 5;  // 3 physical + 2 virtual
        case VoicemeeterType::POTATO:
        case VoicemeeterType::POTATO_X64:
            return 8;  // 5 physical + 3 virtual
        default:
            return 0;
    }
}

// Get number of buses (outputs) based on Voicemeeter type
int VoicemeeterClient::getNumBuses() const {
    switch (m_type) {
        case VoicemeeterType::STANDARD:
            return 2;  // A + B
        case VoicemeeterType::BANANA:
            return 5;  // 3 A + 2 B
        case VoicemeeterType::POTATO:
        case VoicemeeterType::POTATO_X64:
            return 8;  // 5 A + 3 B
        default:
            return 0;
    }
}

// Check if parameters have changed (for UI updates)
bool VoicemeeterClient::haveParametersChanged() {
    if (!m_initialized) {
        return false;
    }
    
    long result = VBVMR_IsParametersDirty();
    
    if (result > 0) {
        return true;
    }
    
    return false;
}

} // namespace VoicemeeterIntegration