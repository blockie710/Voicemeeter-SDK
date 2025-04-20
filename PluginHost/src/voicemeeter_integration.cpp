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

// Thread-safe implementation with proper resource management
class VoicemeeterClientImpl {
public:
    VoicemeeterClientImpl() 
        : m_initialized(false)
        , m_running(false)
        , m_audioCallback(nullptr)
    {
        // Initialize function pointers to nullptr
        VBVMR_Login = nullptr;
        VBVMR_Logout = nullptr;
        VBVMR_GetVoicemeeterType = nullptr;
        VBVMR_GetVoicemeeterVersion = nullptr;
        VBVMR_IsParametersDirty = nullptr;
        VBVMR_GetParameterFloat = nullptr;
        VBVMR_SetParameterFloat = nullptr;
        VBVMR_GetParameterStringA = nullptr;
        VBVMR_SetParameterStringA = nullptr;
        VBVMR_GetLevel = nullptr;
        VBVMR_MacroButtonGetStatus = nullptr;
        VBVMR_MacroButtonSetStatus = nullptr;
        VBVMR_AudioCallbackRegister = nullptr;
        VBVMR_AudioCallbackStart = nullptr;
        VBVMR_AudioCallbackStop = nullptr;
        VBVMR_AudioCallbackUnregister = nullptr;
    }
    
    ~VoicemeeterClientImpl() {
        stopAudioProcessing();
        
        // Clean up Voicemeeter
        if (m_initialized) {
            if (VBVMR_Logout) {
                VBVMR_Logout();
            }
            
            #ifdef _WIN32
            unloadVoicemeeterRemoteDLL();
            #endif
        }
    }
    
    bool initialize() {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (m_initialized) {
            return true;
        }
        
        #ifdef _WIN32
        // Load the DLL
        if (!loadVoicemeeterRemoteDLL()) {
            std::cerr << "Failed to load Voicemeeter Remote DLL" << std::endl;
            return false;
        }
        
        // Get function pointers
        if (!loadFunctionPointers()) {
            std::cerr << "Failed to get Voicemeeter Remote function pointers" << std::endl;
            unloadVoicemeeterRemoteDLL();
            return false;
        }
        #else
        // For non-Windows platforms, we don't currently support Voicemeeter
        std::cerr << "Voicemeeter is only supported on Windows" << std::endl;
        return false;
        #endif
        
        // Login to Voicemeeter
        if (VBVMR_Login) {
            long result = VBVMR_Login();
            if (result != 0) {
                std::cerr << "Failed to login to Voicemeeter, error: " << result << std::endl;
                
                #ifdef _WIN32
                unloadVoicemeeterRemoteDLL();
                #endif
                
                return false;
            }
        }
        else {
            std::cerr << "Login function pointer is null" << std::endl;
            
            #ifdef _WIN32
            unloadVoicemeeterRemoteDLL();
            #endif
            
            return false;
        }
        
        m_initialized = true;
        return true;
    }
    
    VoicemeeterType getVoicemeeterType() {
        if (!m_initialized || !VBVMR_GetVoicemeeterType) {
            return VoicemeeterType::STANDARD; // Default
        }
        
        long type = 0;
        long result = VBVMR_GetVoicemeeterType(&type);
        
        if (result != 0) {
            std::cerr << "Failed to get Voicemeeter type" << std::endl;
            return VoicemeeterType::STANDARD;
        }
        
        switch (type) {
            case 1: return VoicemeeterType::STANDARD;
            case 2: return VoicemeeterType::BANANA;
            case 3: return VoicemeeterType::POTATO;
            case 6: return VoicemeeterType::POTATO_X64;
            default: return VoicemeeterType::STANDARD;
        }
    }
    
    int getNumBuses() {
        VoicemeeterType type = getVoicemeeterType();
        
        switch (type) {
            case VoicemeeterType::STANDARD: return 3; // 2 physical + 1 virtual
            case VoicemeeterType::BANANA: return 5;   // 3 physical + 2 virtual
            case VoicemeeterType::POTATO: 
            case VoicemeeterType::POTATO_X64: 
                return 8;   // 5 physical + 3 virtual
            default: return 3;
        }
    }
    
    bool setAudioCallback(AudioProcessCallback callback) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (!m_initialized) {
            std::cerr << "Cannot set audio callback: not initialized" << std::endl;
            return false;
        }
        
        m_audioCallback = callback;
        return true;
    }
    
    bool startAudioProcessing() {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (!m_initialized) {
            std::cerr << "Cannot start audio processing: not initialized" << std::endl;
            return false;
        }
        
        if (m_running) {
            return true;
        }
        
        if (!VBVMR_AudioCallbackRegister || !VBVMR_AudioCallbackStart) {
            std::cerr << "Audio callback functions not available" << std::endl;
            return false;
        }
        
        // Register the audio callback
        long result = VBVMR_AudioCallbackRegister(
            VBVMR_AUDIOCALLBACK_IN | VBVMR_AUDIOCALLBACK_OUT,
            audioCallbackStatic,
            this
        );
        
        if (result != 0) {
            std::cerr << "Failed to register audio callback" << std::endl;
            return false;
        }
        
        // Start the audio callback
        result = VBVMR_AudioCallbackStart();
        if (result != 0) {
            std::cerr << "Failed to start audio callback" << std::endl;
            VBVMR_AudioCallbackUnregister();
            return false;
        }
        
        m_running = true;
        return true;
    }
    
    bool stopAudioProcessing() {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (!m_initialized || !m_running) {
            return true;
        }
        
        if (!VBVMR_AudioCallbackStop || !VBVMR_AudioCallbackUnregister) {
            std::cerr << "Audio callback functions not available" << std::endl;
            return false;
        }
        
        // Stop the audio callback
        long result = VBVMR_AudioCallbackStop();
        if (result != 0) {
            std::cerr << "Failed to stop audio callback" << std::endl;
            return false;
        }
        
        // Unregister the audio callback
        result = VBVMR_AudioCallbackUnregister();
        if (result != 0) {
            std::cerr << "Failed to unregister audio callback" << std::endl;
            return false;
        }
        
        m_running = false;
        return true;
    }
    
    bool isRunning() const {
        return m_running;
    }
    
private:
    // Mutex for thread safety
    std::mutex m_mutex;
    
    // State
    bool m_initialized;
    bool m_running;
    
    // Audio callback
    AudioProcessCallback m_audioCallback;
    
    // Voicemeeter Remote API function pointers
    #ifdef _WIN32
    T_VBVMR_Login VBVMR_Login;
    T_VBVMR_Logout VBVMR_Logout;
    T_VBVMR_GetVoicemeeterType VBVMR_GetVoicemeeterType;
    T_VBVMR_GetVoicemeeterVersion VBVMR_GetVoicemeeterVersion;
    T_VBVMR_IsParametersDirty VBVMR_IsParametersDirty;
    T_VBVMR_GetParameterFloat VBVMR_GetParameterFloat;
    T_VBVMR_SetParameterFloat VBVMR_SetParameterFloat;
    T_VBVMR_GetParameterStringA VBVMR_GetParameterStringA;
    T_VBVMR_SetParameterStringA VBVMR_SetParameterStringA;
    T_VBVMR_GetLevel VBVMR_GetLevel;
    T_VBVMR_MacroButtonGetStatus VBVMR_MacroButtonGetStatus;
    T_VBVMR_MacroButtonSetStatus VBVMR_MacroButtonSetStatus;
    T_VBVMR_AudioCallbackRegister VBVMR_AudioCallbackRegister;
    T_VBVMR_AudioCallbackStart VBVMR_AudioCallbackStart;
    T_VBVMR_AudioCallbackStop VBVMR_AudioCallbackStop;
    T_VBVMR_AudioCallbackUnregister VBVMR_AudioCallbackUnregister;
    #endif
    
    // Load all function pointers from the DLL
    bool loadFunctionPointers() {
        #ifdef _WIN32
        VBVMR_Login = (T_VBVMR_Login)getVoicemeeterProcAddress("VBVMR_Login");
        VBVMR_Logout = (T_VBVMR_Logout)getVoicemeeterProcAddress("VBVMR_Logout");
        VBVMR_GetVoicemeeterType = (T_VBVMR_GetVoicemeeterType)getVoicemeeterProcAddress("VBVMR_GetVoicemeeterType");
        VBVMR_GetVoicemeeterVersion = (T_VBVMR_GetVoicemeeterVersion)getVoicemeeterProcAddress("VBVMR_GetVoicemeeterVersion");
        VBVMR_IsParametersDirty = (T_VBVMR_IsParametersDirty)getVoicemeeterProcAddress("VBVMR_IsParametersDirty");
        VBVMR_GetParameterFloat = (T_VBVMR_GetParameterFloat)getVoicemeeterProcAddress("VBVMR_GetParameterFloat");
        VBVMR_SetParameterFloat = (T_VBVMR_SetParameterFloat)getVoicemeeterProcAddress("VBVMR_SetParameterFloat");
        VBVMR_GetParameterStringA = (T_VBVMR_GetParameterStringA)getVoicemeeterProcAddress("VBVMR_GetParameterStringA");
        VBVMR_SetParameterStringA = (T_VBVMR_SetParameterStringA)getVoicemeeterProcAddress("VBVMR_SetParameterStringA");
        VBVMR_GetLevel = (T_VBVMR_GetLevel)getVoicemeeterProcAddress("VBVMR_GetLevel");
        VBVMR_MacroButtonGetStatus = (T_VBVMR_MacroButtonGetStatus)getVoicemeeterProcAddress("VBVMR_MacroButtonGetStatus");
        VBVMR_MacroButtonSetStatus = (T_VBVMR_MacroButtonSetStatus)getVoicemeeterProcAddress("VBVMR_MacroButtonSetStatus");
        VBVMR_AudioCallbackRegister = (T_VBVMR_AudioCallbackRegister)getVoicemeeterProcAddress("VBVMR_AudioCallbackRegister");
        VBVMR_AudioCallbackStart = (T_VBVMR_AudioCallbackStart)getVoicemeeterProcAddress("VBVMR_AudioCallbackStart");
        VBVMR_AudioCallbackStop = (T_VBVMR_AudioCallbackStop)getVoicemeeterProcAddress("VBVMR_AudioCallbackStop");
        VBVMR_AudioCallbackUnregister = (T_VBVMR_AudioCallbackUnregister)getVoicemeeterProcAddress("VBVMR_AudioCallbackUnregister");
        
        return (VBVMR_Login != nullptr && 
                VBVMR_Logout != nullptr &&
                VBVMR_GetVoicemeeterType != nullptr &&
                VBVMR_GetVoicemeeterVersion != nullptr &&
                VBVMR_IsParametersDirty != nullptr &&
                VBVMR_GetParameterFloat != nullptr &&
                VBVMR_SetParameterFloat != nullptr &&
                VBVMR_GetParameterStringA != nullptr &&
                VBVMR_SetParameterStringA != nullptr &&
                VBVMR_GetLevel != nullptr &&
                VBVMR_MacroButtonGetStatus != nullptr &&
                VBVMR_MacroButtonSetStatus != nullptr &&
                VBVMR_AudioCallbackRegister != nullptr &&
                VBVMR_AudioCallbackStart != nullptr &&
                VBVMR_AudioCallbackStop != nullptr &&
                VBVMR_AudioCallbackUnregister != nullptr);
        #else
        return false;
        #endif
    }
    
    // Static audio callback function
    static void audioCallbackStatic(void* userData, float** inputs, float** outputs, long numInputs, long numOutputs, long numSamples) {
        VoicemeeterClientImpl* self = static_cast<VoicemeeterClientImpl*>(userData);
        if (self && self->m_audioCallback) {
            self->m_audioCallback(inputs, outputs, static_cast<int>(numInputs), static_cast<int>(numOutputs), static_cast<int>(numSamples));
        }
    }
};

// VoicemeeterClient implementation
VoicemeeterClient::VoicemeeterClient()
    : m_impl(std::make_unique<VoicemeeterClientImpl>())
{
}

VoicemeeterClient::~VoicemeeterClient() = default;

bool VoicemeeterClient::initialize() {
    return m_impl->initialize();
}

VoicemeeterType VoicemeeterClient::getVoicemeeterType() {
    return m_impl->getVoicemeeterType();
}

int VoicemeeterClient::getNumBuses() {
    return m_impl->getNumBuses();
}

bool VoicemeeterClient::setAudioCallback(AudioProcessCallback callback) {
    return m_impl->setAudioCallback(callback);
}

bool VoicemeeterClient::startAudioProcessing() {
    return m_impl->startAudioProcessing();
}

bool VoicemeeterClient::stopAudioProcessing() {
    return m_impl->stopAudioProcessing();
}

bool VoicemeeterClient::isRunning() const {
    return m_impl->isRunning();
}

} // namespace VoicemeeterIntegration