#include <iostream>
#include <memory>
#include <string>
#include "../VMPlugin.h"

class TestPluginHost {
private:
    HMODULE hDLL;
    LPVOID lpPluginContext;
    
    // Function pointers for plugin interface
    T_VMPLUGIN_INIT pfInit;
    T_VMPLUGIN_END pfEnd;
    T_VMPLUGIN_GETPARAMETER pfGetParameter;
    T_VMPLUGIN_SETPARAMETER pfSetParameter;
    T_VMPLUGIN_PROCESSAUDIO pfProcessAudio;
    
public:
    TestPluginHost() : hDLL(NULL), lpPluginContext(NULL), 
                       pfInit(NULL), pfEnd(NULL), 
                       pfGetParameter(NULL), pfSetParameter(NULL),
                       pfProcessAudio(NULL) {}
    
    ~TestPluginHost() {
        if (pfEnd && lpPluginContext) {
            pfEnd(lpPluginContext);
        }
        if (hDLL) {
            FreeLibrary(hDLL);
        }
    }
    
    bool loadPlugin(const std::string& pluginPath) {
        // Load the DLL
        hDLL = LoadLibraryA(pluginPath.c_str());
        if (!hDLL) {
            std::cerr << "Failed to load plugin: " << pluginPath << std::endl;
            return false;
        }
        
        // Get function pointers
        pfInit = (T_VMPLUGIN_INIT)GetProcAddress(hDLL, "VMPLUGIN_Init");
        pfEnd = (T_VMPLUGIN_END)GetProcAddress(hDLL, "VMPLUGIN_End");
        pfGetParameter = (T_VMPLUGIN_GETPARAMETER)GetProcAddress(hDLL, "VMPLUGIN_GetParameter");
        pfSetParameter = (T_VMPLUGIN_SETPARAMETER)GetProcAddress(hDLL, "VMPLUGIN_SetParameter");
        pfProcessAudio = (T_VMPLUGIN_PROCESSAUDIO)GetProcAddress(hDLL, "VMPLUGIN_ProcessAudio");
        
        if (!pfInit || !pfEnd || !pfGetParameter || !pfSetParameter || !pfProcessAudio) {
            std::cerr << "Failed to get plugin function pointers" << std::endl;
            FreeLibrary(hDLL);
            hDLL = NULL;
            return false;
        }
        
        // Initialize the plugin
        lpPluginContext = pfInit();
        if (!lpPluginContext) {
            std::cerr << "Failed to initialize plugin" << std::endl;
            FreeLibrary(hDLL);
            hDLL = NULL;
            return false;
        }
        
        std::cout << "Plugin loaded successfully" << std::endl;
        return true;
    }
    
    bool testParameters() {
        if (!pfGetParameter || !pfSetParameter || !lpPluginContext) {
            return false;
        }
        
        // Test getting a parameter
        float value = 0.0f;
        if (pfGetParameter(lpPluginContext, 0, &value) != 0) {
            std::cout << "Parameter 0: " << value << std::endl;
        }
        
        // Test setting a parameter
        if (pfSetParameter(lpPluginContext, 0, 0.5f) == 0) {
            std::cout << "Set parameter 0 to 0.5" << std::endl;
            
            // Verify the parameter was set
            if (pfGetParameter(lpPluginContext, 0, &value) != 0) {
                std::cout << "Parameter 0 (after set): " << value << std::endl;
            }
        }
        
        return true;
    }
    
    bool testAudioProcessing() {
        if (!pfProcessAudio || !lpPluginContext) {
            return false;
        }
        
        // Create test audio buffers
        const int numSamples = 1024;
        float* inBuffer = new float[numSamples];
        float* outBuffer = new float[numSamples];
        
        // Fill input buffer with a sine wave for testing
        for (int i = 0; i < numSamples; i++) {
            inBuffer[i] = 0.5f * sinf(2.0f * 3.14159f * i / 64.0f);
        }
        
        // Process audio
        T_AUDIO_IO audioIO;
        audioIO.inputBuffer = inBuffer;
        audioIO.outputBuffer = outBuffer;
        audioIO.nbs = numSamples;
        
        if (pfProcessAudio(lpPluginContext, &audioIO) == 0) {
            std::cout << "Audio processing successful" << std::endl;
            
            // Check a few output samples
            std::cout << "First few output samples: ";
            for (int i = 0; i < 5; i++) {
                std::cout << outBuffer[i] << " ";
            }
            std::cout << std::endl;
        }
        
        delete[] inBuffer;
        delete[] outBuffer;
        return true;
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <plugin_path>" << std::endl;
        return 1;
    }
    
    std::string pluginPath = argv[1];
    TestPluginHost host;
    
    if (!host.loadPlugin(pluginPath)) {
        std::cerr << "Failed to load plugin" << std::endl;
        return 1;
    }
    
    std::cout << "Testing parameters..." << std::endl;
    host.testParameters();
    
    std::cout << "Testing audio processing..." << std::endl;
    host.testAudioProcessing();
    
    std::cout << "Plugin test completed successfully" << std::endl;
    return 0;
}