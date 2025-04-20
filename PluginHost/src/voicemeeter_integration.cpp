#include "../include/voicemeeter_integration.h"
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <iostream>
#include <vector>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#endif

// Forward declarations from plugin_interface.cpp
#ifdef _WIN32
extern bool loadVoicemeeterRemoteDLL();
extern void unloadVoicemeeterRemoteDLL();
extern void* getVoicemeeterProcAddress(const char* procName);
#endif

namespace VoicemeeterIntegration {

// Forward declaration of callback data structure
struct AudioCallbackData;

// Thread-safe implementation with proper resource management
class VoicemeeterClientImpl {
public:
    std::mutex m_mutex;
    std::condition_variable m_cvAudioCallback;
    std::atomic<bool> m_isProcessing{false};
    bool m_initialized{false};
    VoicemeeterType m_type{VoicemeeterType::STANDARD};
    long m_version{0};
    AudioProcessCallback m_audioCallback;
    std::unique_ptr<AudioCallbackData> m_callbackData;
    
    // Audio buffer management
    std::vector<float*> m_inputBuffers;
    std::vector<float*> m_outputBuffers;
    int m_maxBufferSize{0};
    double m_sampleRate{48000};
    
    // Parameters monitoring
    std::thread m_parameterThread;
    std::atomic<bool> m_parameterThreadRunning{false};
    std::atomic<bool> m_parametersChanged{false};
    
    #ifdef _WIN32
    // Voicemeeter Remote API function pointers for Windows
    T_VBVMR_Login VBVMR_Login = nullptr;
    T_VBVMR_Logout VBVMR_Logout = nullptr;
    T_VBVMR_RunVoicemeeter VBVMR_RunVoicemeeter = nullptr;
    T_VBVMR_GetVoicemeeterType VBVMR_GetVoicemeeterType = nullptr;
    T_VBVMR_GetVoicemeeterVersion VBVMR_GetVoicemeeterVersion = nullptr;
    T_VBVMR_IsParametersDirty VBVMR_IsParametersDirty = nullptr;
    T_VBVMR_GetParameterFloat VBVMR_GetParameterFloat = nullptr;
    T_VBVMR_GetLevel VBVMR_GetLevel = nullptr;
    T_VBVMR_SetParameterFloat VBVMR_SetParameterFloat = nullptr;
    T_VBVMR_AudioCallbackRegister VBVMR_AudioCallbackRegister = nullptr;
    T_VBVMR_AudioCallbackStart VBVMR_AudioCallbackStart = nullptr;
    T_VBVMR_AudioCallbackStop VBVMR_AudioCallbackStop = nullptr;
    T_VBVMR_AudioCallbackUnregister VBVMR_AudioCallbackUnregister = nullptr;
    
    // Load all function pointers from the DLL
    bool loadFunctionPointers() {
        VBVMR_Login = reinterpret_cast<T_VBVMR_Login>(getVoicemeeterProcAddress("VBVMR_Login"));
        VBVMR_Logout = reinterpret_cast<T_VBVMR_Logout>(getVoicemeeterProcAddress("VBVMR_Logout"));
        VBVMR_RunVoicemeeter = reinterpret_cast<T_VBVMR_RunVoicemeeter>(getVoicemeeterProcAddress("VBVMR_RunVoicemeeter"));
        VBVMR_GetVoicemeeterType = reinterpret_cast<T_VBVMR_GetVoicemeeterType>(getVoicemeeterProcAddress("VBVMR_GetVoicemeeterType"));
        VBVMR_GetVoicemeeterVersion = reinterpret_cast<T_VBVMR_GetVoicemeeterVersion>(getVoicemeeterProcAddress("VBVMR_GetVoicemeeterVersion"));
        VBVMR_IsParametersDirty = reinterpret_cast<T_VBVMR_IsParametersDirty>(getVoicemeeterProcAddress("VBVMR_IsParametersDirty"));
        VBVMR_GetParameterFloat = reinterpret_cast<T_VBVMR_GetParameterFloat>(getVoicemeeterProcAddress("VBVMR_GetParameterFloat"));
        VBVMR_GetLevel = reinterpret_cast<T_VBVMR_GetLevel>(getVoicemeeterProcAddress("VBVMR_GetLevel"));
        VBVMR_SetParameterFloat = reinterpret_cast<T_VBVMR_SetParameterFloat>(getVoicemeeterProcAddress("VBVMR_SetParameterFloat"));
        VBVMR_AudioCallbackRegister = reinterpret_cast<T_VBVMR_AudioCallbackRegister>(getVoicemeeterProcAddress("VBVMR_AudioCallbackRegister"));
        VBVMR_AudioCallbackStart = reinterpret_cast<T_VBVMR_AudioCallbackStart>(getVoicemeeterProcAddress("VBVMR_AudioCallbackStart"));
        VBVMR_AudioCallbackStop = reinterpret_cast<T_VBVMR_AudioCallbackStop>(getVoicemeeterProcAddress("VBVMR_AudioCallbackStop"));
        VBVMR_AudioCallbackUnregister = reinterpret_cast<T_VBVMR_AudioCallbackUnregister>(getVoicemeeterProcAddress("VBVMR_AudioCallbackUnregister"));
        
        // Check if all essential functions were loaded
        return (VBVMR_Login != nullptr &&
                VBVMR_Logout != nullptr &&
                VBVMR_GetVoicemeeterType != nullptr &&
                VBVMR_GetVoicemeeterVersion != nullptr &&
                VBVMR_IsParametersDirty != nullptr &&
                VBVMR_GetParameterFloat != nullptr &&
                VBVMR_GetLevel != nullptr &&
                VBVMR_SetParameterFloat != nullptr &&
                VBVMR_AudioCallbackRegister != nullptr &&
                VBVMR_AudioCallbackStart != nullptr &&
                VBVMR_AudioCallbackStop != nullptr &&
                VBVMR_AudioCallbackUnregister != nullptr);
    }
    #endif
};

// Data passed to audio callback
struct AudioCallbackData {
    VoicemeeterClientImpl* client;
    AudioProcessCallback callback;
};

// Static callback function for Voicemeeter
long __stdcall VoicemeeterClient::audioCallback(void* lpUser, long nCommand, void* lpData, long nnn) {
    auto data = static_cast<AudioCallbackData*>(lpUser);
    if (!data || !data->client) return -1;
    
    VoicemeeterClientImpl* client = data->client;
    
    switch(nCommand) {
        case VBVMR_CBCOMMAND_STARTING:
        {
            // Initialize audio processing
            auto info = static_cast<VBVMR_LPT_AUDIOINFO>(lpData);
            std::lock_guard<std::mutex> lock(client->m_mutex);
            client->m_sampleRate = info->samplerate;
            client->m_maxBufferSize = info->nbSamplePerFrame;
            
            // Allocate buffers
            client->m_inputBuffers.resize(64, nullptr);  // Reserve space for max possible channels
            client->m_outputBuffers.resize(64, nullptr); // Reserve space for max possible channels
            
            for (int i = 0; i < 64; i++) {
                client->m_inputBuffers[i] = new float[client->m_maxBufferSize];
                client->m_outputBuffers[i] = new float[client->m_maxBufferSize];
            }
            
            client->m_isProcessing = true;
            client->m_cvAudioCallback.notify_all();
            return 0;
        }
        
        case VBVMR_CBCOMMAND_ENDING:
        {
            // Clean up audio processing
            std::lock_guard<std::mutex> lock(client->m_mutex);
            client->m_isProcessing = false;
            
            // Free buffers
            for (auto& buffer : client->m_inputBuffers) {
                delete[] buffer;
                buffer = nullptr;
            }
            for (auto& buffer : client->m_outputBuffers) {
                delete[] buffer;
                buffer = nullptr;
            }
            
            client->m_inputBuffers.clear();
            client->m_outputBuffers.clear();
            return 0;
        }
        
        case VBVMR_CBCOMMAND_BUFFER_IN:
        case VBVMR_CBCOMMAND_BUFFER_OUT:
        case VBVMR_CBCOMMAND_BUFFER_MAIN:
        {
            // Process audio
            auto audiobuffer = static_cast<VBVMR_LPT_AUDIOBUFFER>(lpData);
            
            // Call the client's audio callback if registered
            if (data->callback) {
                // Copy input data to our buffers for thread safety
                for (int i = 0; i < audiobuffer->audiobuffer_nbi; i++) {
                    if (audiobuffer->audiobuffer_r[i]) {
                        std::copy(audiobuffer->audiobuffer_r[i], 
                                 audiobuffer->audiobuffer_r[i] + audiobuffer->audiobuffer_nbs, 
                                 client->m_inputBuffers[i]);
                    }
                }
                
                // Call user callback
                data->callback(client->m_inputBuffers.data(), 
                              client->m_outputBuffers.data(), 
                              audiobuffer->audiobuffer_nbi,
                              audiobuffer->audiobuffer_nbo,
                              audiobuffer->audiobuffer_nbs);
                
                // Copy our output buffers to Voicemeeter's buffers
                for (int i = 0; i < audiobuffer->audiobuffer_nbo; i++) {
                    if (audiobuffer->audiobuffer_w[i]) {
                        std::copy(client->m_outputBuffers[i],
                                 client->m_outputBuffers[i] + audiobuffer->audiobuffer_nbs,
                                 audiobuffer->audiobuffer_w[i]);
                    }
                }
            }
            return 0;
        }
        
        default:
            return 0;
    }
    return 0;
}

// Constructor for VoicemeeterClient
VoicemeeterClient::VoicemeeterClient() 
    : m_initialized(false)
    , m_type(VoicemeeterType::STANDARD)
    , m_version(0)
{
    // Create implementation
    m_impl = std::make_unique<VoicemeeterClientImpl>();
}

// Destructor for VoicemeeterClient
VoicemeeterClient::~VoicemeeterClient() {
    shutdown();
}

// Initialize the connection to Voicemeeter
bool VoicemeeterClient::initialize() {
    if (m_impl->m_initialized) return true;
    
    #ifdef _WIN32
    // On Windows, dynamically load the DLL
    if (!loadVoicemeeterRemoteDLL()) {
        std::cerr << "Failed to load Voicemeeter Remote DLL" << std::endl;
        return false;
    }
    
    // Load function pointers
    if (!m_impl->loadFunctionPointers()) {
        std::cerr << "Failed to load Voicemeeter Remote API functions" << std::endl;
        unloadVoicemeeterRemoteDLL();
        return false;
    }
    
    // Login to Voicemeeter
    long result = m_impl->VBVMR_Login();
    if (result < 0) {
        std::cerr << "Failed to login to Voicemeeter API: " << result << std::endl;
        unloadVoicemeeterRemoteDLL();
        return false;
    }
    
    // Get Voicemeeter type
    long vmType = 0;
    result = m_impl->VBVMR_GetVoicemeeterType(&vmType);
    if (result == 0) {
        m_impl->m_type = static_cast<VoicemeeterType>(vmType);
    } else {
        std::cerr << "Failed to get Voicemeeter type: " << result << std::endl;
        m_impl->VBVMR_Logout();
        unloadVoicemeeterRemoteDLL();
        return false;
    }
    
    // Get Voicemeeter version
    result = m_impl->VBVMR_GetVoicemeeterVersion(&m_impl->m_version);
    if (result != 0) {
        std::cerr << "Failed to get Voicemeeter version: " << result << std::endl;
        m_impl->VBVMR_Logout();
        unloadVoicemeeterRemoteDLL();
        return false;
    }
    
    // Start parameter monitoring thread
    m_impl->m_parameterThreadRunning = true;
    m_impl->m_parameterThread = std::thread([this]() {
        while (m_impl->m_parameterThreadRunning) {
            // Check if parameters have changed
            long dirty = m_impl->VBVMR_IsParametersDirty();
            if (dirty == 1) {
                m_impl->m_parametersChanged = true;
            }
            // Sleep for a short time to avoid high CPU usage
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });
    #else
    // For Linux/macOS, we directly use the API functions
    long result = VBVMR_Login();
    if (result < 0) {
        std::cerr << "Failed to login to Voicemeeter API: " << result << std::endl;
        return false;
    }
    
    // Get Voicemeeter type
    long vmType = 0;
    result = VBVMR_GetVoicemeeterType(&vmType);
    if (result == 0) {
        m_impl->m_type = static_cast<VoicemeeterType>(vmType);
    } else {
        std::cerr << "Failed to get Voicemeeter type: " << result << std::endl;
        VBVMR_Logout();
        return false;
    }
    
    // Get Voicemeeter version
    result = VBVMR_GetVoicemeeterVersion(&m_impl->m_version);
    if (result != 0) {
        std::cerr << "Failed to get Voicemeeter version: " << result << std::endl;
        VBVMR_Logout();
        return false;
    }
    
    // Start parameter monitoring thread
    m_impl->m_parameterThreadRunning = true;
    m_impl->m_parameterThread = std::thread([this]() {
        while (m_impl->m_parameterThreadRunning) {
            // Check if parameters have changed
            long dirty = VBVMR_IsParametersDirty();
            if (dirty == 1) {
                m_impl->m_parametersChanged = true;
            }
            // Sleep for a short time to avoid high CPU usage
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });
    #endif
    
    m_impl->m_initialized = true;
    return true;
}

// Close the connection to Voicemeeter
void VoicemeeterClient::shutdown() {
    if (!m_impl->m_initialized) return;
    
    // Stop parameter monitoring thread
    m_impl->m_parameterThreadRunning = false;
    if (m_impl->m_parameterThread.joinable()) {
        m_impl->m_parameterThread.join();
    }
    
    // Stop audio processing
    stopAudioProcessing();
    
    // Logout from Voicemeeter
    #ifdef _WIN32
    if (m_impl->VBVMR_Logout) {
        m_impl->VBVMR_Logout();
    }
    unloadVoicemeeterRemoteDLL();
    #else
    VBVMR_Logout();
    #endif
    
    m_impl->m_initialized = false;
}

// Get Voicemeeter type
VoicemeeterType VoicemeeterClient::getVoicemeeterType() {
    return m_impl->m_type;
}

// Get Voicemeeter version
long VoicemeeterClient::getVoicemeeterVersion() {
    return m_impl->m_version;
}

// Launch Voicemeeter if not running
bool VoicemeeterClient::launchVoicemeeter(VoicemeeterType type) {
    #ifdef _WIN32
    if (!m_impl->VBVMR_RunVoicemeeter) return false;
    long result = m_impl->VBVMR_RunVoicemeeter(static_cast<long>(type));
    #else
    long result = VBVMR_RunVoicemeeter(static_cast<long>(type));
    #endif
    return (result == 0);
}

// Register for audio callbacks
bool VoicemeeterClient::registerAudioCallback(AudioProcessCallback callback, 
                                            bool processInputs,
                                            bool processOutputs) {
    if (!m_impl->m_initialized) return false;
    
    // Create callback data
    m_impl->m_callbackData = std::make_unique<AudioCallbackData>();
    m_impl->m_callbackData->client = m_impl.get();
    m_impl->m_callbackData->callback = callback;
    
    // Register with Voicemeeter
    long mode = 0;
    if (processInputs) mode |= VBVMR_AUDIOCALLBACK_IN;
    if (processOutputs) mode |= VBVMR_AUDIOCALLBACK_OUT;
    
    // If both are true, use MAIN mode which is more efficient
    if (processInputs && processOutputs) {
        mode = VBVMR_AUDIOCALLBACK_MAIN;
    }
    
    char clientName[64] = "VoicemeeterPluginHost";
    
    #ifdef _WIN32
    long result = m_impl->VBVMR_AudioCallbackRegister(mode, audioCallback, m_impl->m_callbackData.get(), clientName);
    #else
    long result = VBVMR_AudioCallbackRegister(mode, audioCallback, m_impl->m_callbackData.get(), clientName);
    #endif
    
    return (result == 0);
}

// Start audio processing
bool VoicemeeterClient::startAudioProcessing() {
    if (!m_impl->m_initialized) return false;
    
    // Start audio
    #ifdef _WIN32
    long result = m_impl->VBVMR_AudioCallbackStart();
    #else
    long result = VBVMR_AudioCallbackStart();
    #endif
    
    if (result != 0) {
        std::cerr << "Failed to start audio processing: " << result << std::endl;
        return false;
    }
    
    // Wait for processing to actually start
    {
        std::unique_lock<std::mutex> lock(m_impl->m_mutex);
        m_impl->m_cvAudioCallback.wait_for(lock, std::chrono::seconds(5), 
            [this] { return m_impl->m_isProcessing.load(); });
    }
    
    return m_impl->m_isProcessing;
}

// Stop audio processing
bool VoicemeeterClient::stopAudioProcessing() {
    if (!m_impl->m_initialized) return true;
    
    #ifdef _WIN32
    long result = m_impl->VBVMR_AudioCallbackStop();
    #else
    long result = VBVMR_AudioCallbackStop();
    #endif
    
    if (result != 0) {
        std::cerr << "Failed to stop audio processing: " << result << std::endl;
        return false;
    }
    
    // Wait for processing to actually stop
    {
        std::unique_lock<std::mutex> lock(m_impl->m_mutex);
        m_impl->m_cvAudioCallback.wait_for(lock, std::chrono::seconds(5), 
            [this] { return !m_impl->m_isProcessing.load(); });
    }
    
    return !m_impl->m_isProcessing;
}

// Set parameter
bool VoicemeeterClient::setParameter(const std::string& paramName, float value) {
    if (!m_impl->m_initialized) return false;
    
    #ifdef _WIN32
    long result = m_impl->VBVMR_SetParameterFloat(const_cast<char*>(paramName.c_str()), value);
    #else
    long result = VBVMR_SetParameterFloat(const_cast<char*>(paramName.c_str()), value);
    #endif
    
    return (result == 0);
}

// Get parameter
bool VoicemeeterClient::getParameter(const std::string& paramName, float& value) {
    if (!m_impl->m_initialized) return false;
    
    #ifdef _WIN32
    long result = m_impl->VBVMR_GetParameterFloat(const_cast<char*>(paramName.c_str()), &value);
    #else
    long result = VBVMR_GetParameterFloat(const_cast<char*>(paramName.c_str()), &value);
    #endif
    
    return (result == 0);
}

// Get input level
bool VoicemeeterClient::getInputLevel(int channel, float& level) {
    if (!m_impl->m_initialized) return false;
    
    #ifdef _WIN32
    long result = m_impl->VBVMR_GetLevel(0, channel, &level);
    #else
    long result = VBVMR_GetLevel(0, channel, &level);
    #endif
    
    return (result == 0);
}

// Get output level
bool VoicemeeterClient::getOutputLevel(int channel, float& level) {
    if (!m_impl->m_initialized) return false;
    
    #ifdef _WIN32
    long result = m_impl->VBVMR_GetLevel(3, channel, &level);
    #else
    long result = VBVMR_GetLevel(3, channel, &level);
    #endif
    
    return (result == 0);
}

// Get number of strips
int VoicemeeterClient::getNumStrips() const {
    switch (m_impl->m_type) {
        case VoicemeeterType::STANDARD: return 3; // 2 physical + 1 virtual
        case VoicemeeterType::BANANA: return 5;   // 3 physical + 2 virtual
        case VoicemeeterType::POTATO:
        case VoicemeeterType::POTATO_X64: return 8; // 5 physical + 3 virtual
        default: return 0;
    }
}

// Get number of buses
int VoicemeeterClient::getNumBuses() const {
    switch (m_impl->m_type) {
        case VoicemeeterType::STANDARD: return 2; // A+B
        case VoicemeeterType::BANANA: return 5;   // 3 physical + 2 virtual
        case VoicemeeterType::POTATO:
        case VoicemeeterType::POTATO_X64: return 8; // 5 physical + 3 virtual
        default: return 0;
    }
}

// Check if parameters have changed
bool VoicemeeterClient::haveParametersChanged() {
    bool changed = m_impl->m_parametersChanged.exchange(false);
    return changed;
}

} // namespace VoicemeeterIntegration