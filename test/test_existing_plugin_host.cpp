#include <iostream>
#include <string>
#include <cstring>
#include <memory>
#include <chrono>
#include <cmath>
#include <vector>
#include <cstdlib>
#include <ctime>

// Include the plugin host headers
// Fix the include path
#include "../PluginHost/src/VMPluginHost.h"

// Include shared test utilities
#include "test_utils.h"

#ifdef _WIN32
#include <windows.h>
#define PATH_SEPARATOR "\\"
#else
#define PATH_SEPARATOR "/"
#endif

// Simple sine wave audio generator
void generateSineWave(float* buffer, int numSamples, int sampleRate, float frequency, float amplitude) {
    for (int i = 0; i < numSamples; i++) {
        buffer[i] = amplitude * sin(2.0f * M_PI * frequency * i / sampleRate);
    }
}

// Test function to check if the plugin host can load plugins
bool testLoadPlugin(VMPluginHost* host, const char* pluginPath) {
    std::cout << "=== Testing Plugin Loading ===" << std::endl;
    std::cout << "Loading plugin: " << pluginPath << std::endl;

    bool result = host->LoadPlugin(pluginPath);
    if (result) {
        std::cout << "SUCCESS: Plugin loaded successfully" << std::endl;
        
        // Display plugin info
        std::cout << "Plugin name: " << host->GetPluginName() << std::endl;
        std::cout << "Plugin vendor: " << host->GetPluginVendor() << std::endl;
        std::cout << "Plugin product: " << host->GetPluginProduct() << std::endl;
    } else {
        std::cout << "FAILURE: Plugin failed to load" << std::endl;
    }
    
    return result;
}

// Test function to check plugin parameters
void testPluginParameters(VMPluginHost* host) {
    std::cout << "\n=== Testing Plugin Parameters ===" << std::endl;
    
    // Get parameter count
    int paramCount = host->GetNumParameters();
    std::cout << "Plugin reports " << paramCount << " parameters" << std::endl;
    
    // Display all parameters
    std::cout << "\nParameter list:" << std::endl;
    for (int i = 0; i < paramCount; i++) {
        std::cout << "Parameter " << i << ": " << host->GetParameterName(i) << std::endl;
        std::cout << "  - Value: " << host->GetParameter(i) << std::endl;
        std::cout << "  - Display: " << host->GetParameterDisplay(i) << std::endl;
        
        // Get parameter properties
        float min, max, defaultVal;
        if (host->GetParameterProperties(i, &min, &max, &defaultVal)) {
            std::cout << "  - Range: " << min << " to " << max << " (default: " << defaultVal << ")" << std::endl;
        }
    }
    
    // Test parameter modification
    std::cout << "\nTesting parameter modification:" << std::endl;
    for (int i = 0; i < std::min(3, paramCount); i++) {
        float originalValue = host->GetParameter(i);
        std::cout << "Parameter " << i << " original value: " << originalValue << std::endl;
        
        // Set to 50% value
        float min, max, defaultVal;
        float targetValue = 0.5f;
        
        if (host->GetParameterProperties(i, &min, &max, &defaultVal)) {
            targetValue = min + (max - min) * 0.5f;
        }
        
        host->SetParameter(i, targetValue);
        float newValue = host->GetParameter(i);
        
        std::cout << "  - Set to: " << targetValue << ", Result: " << newValue << std::endl;
        
        // Reset to original value
        host->SetParameter(i, originalValue);
        std::cout << "  - Reset to original: " << host->GetParameter(i) << std::endl;
    }
}

// Test function for audio processing
void testAudioProcessing(VMPluginHost* host) {
    std::cout << "\n=== Testing Audio Processing ===" << std::endl;
    
    // Create test audio buffers
    const int sampleRate = 48000;
    const int bufferSize = 1024;
    
    // Use vectors instead of raw arrays for automatic memory management
    std::vector<float> inputL(bufferSize);
    std::vector<float> inputR(bufferSize);
    std::vector<float> outputL(bufferSize, 0.0f);
    std::vector<float> outputR(bufferSize, 0.0f);
    
    // Generate test signals
    test_utils::generateSineWave(inputL.data(), bufferSize, sampleRate, 440.0f, 0.5f);  // A4 note
    test_utils::generateSineWave(inputR.data(), bufferSize, sampleRate, 587.33f, 0.5f); // D5 note
    
    // Process audio
    std::cout << "Processing " << bufferSize << " samples at " << sampleRate << "Hz" << std::endl;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    try {
        // Call the ProcessAudio method
        host->ProcessAudio(inputL.data(), inputR.data(), outputL.data(), outputR.data(), bufferSize, sampleRate);
    }
    catch (const std::exception& e) {
        std::cerr << "Error processing audio: " << e.what() << std::endl;
        return;
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    
    std::cout << "Processing took " << duration.count() << " microseconds" << std::endl;
    
    // Calculate RMS of input and output to see if there's a difference
    float inputRmsL = test_utils::calculateRMS(inputL.data(), bufferSize);
    float inputRmsR = test_utils::calculateRMS(inputR.data(), bufferSize);
    float outputRmsL = test_utils::calculateRMS(outputL.data(), bufferSize);
    float outputRmsR = test_utils::calculateRMS(outputR.data(), bufferSize);
    
    std::cout << "Input RMS: L=" << inputRmsL << ", R=" << inputRmsR << std::endl;
    std::cout << "Output RMS: L=" << outputRmsL << ", R=" << outputRmsR << std::endl;
    
    // Print a few samples for comparison
    std::cout << "\nFirst 5 samples comparison:" << std::endl;
    for (int i = 0; i < 5; i++) {
        std::cout << "Sample " << i << ": ";
        std::cout << "In(" << inputL[i] << "," << inputR[i] << ") -> ";
        std::cout << "Out(" << outputL[i] << "," << outputR[i] << ")" << std::endl;
    }
}

// Performance test
void testPerformance(VMPluginHost* host) {
    std::cout << "\n=== Testing Plugin Performance ===" << std::endl;
    
    // Use larger buffer for performance testing
    const int sampleRate = 48000;
    const int bufferSize = 4096;  // Larger buffer for performance test
    const int iterations = 100;   // Number of iterations for better measurement
    
    // Use vectors for automatic memory management
    std::vector<float> inputL(bufferSize);
    std::vector<float> inputR(bufferSize);
    std::vector<float> outputL(bufferSize);
    std::vector<float> outputR(bufferSize);
    
    // Generate test signals - white noise for more realistic load
    test_utils::generateWhiteNoise(inputL.data(), bufferSize, 0.5f);
    test_utils::generateWhiteNoise(inputR.data(), bufferSize, 0.5f);
    
    // Run multiple iterations for more accurate timing
    auto startTime = std::chrono::high_resolution_clock::now();
    
    try {
        for (int i = 0; i < iterations; i++) {
            host->ProcessAudio(inputL.data(), inputR.data(), outputL.data(), outputR.data(), bufferSize, sampleRate);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error during performance test: " << e.what() << std::endl;
        return;
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto totalDuration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    
    double avgDuration = totalDuration.count() / static_cast<double>(iterations);
    std::cout << "Average processing time: " << avgDuration << " μs for " << bufferSize << " samples" << std::endl;
    
    double bufferDuration = (bufferSize * 1000000.0) / sampleRate;
    double cpuUsage = (avgDuration / bufferDuration) * 100.0;
    
    std::cout << "Estimated CPU usage: " << cpuUsage << "%" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <plugin_path> [test_type]" << std::endl;
        std::cerr << "Test types: all (default), load, params, audio, perf" << std::endl;
        return 1;
    }
    
    std::string pluginPath = argv[1];
    std::string testType = (argc > 2) ? argv[2] : "all";
    
    // Create the plugin host instance
    std::unique_ptr<VMPluginHost> host(new VMPluginHost());
    
    // Test loading the plugin
    if (!testLoadPlugin(host.get(), pluginPath.c_str())) {
        std::cerr << "Plugin loading failed. Tests aborted." << std::endl;
        return 1;
    }
    
    // Run specific tests based on the test_type argument
    if (testType == "all" || testType == "params") {
        testPluginParameters(host.get());
    }
    
    if (testType == "all" || testType == "audio") {
        testAudioProcessing(host.get());
    }
    
    if (testType == "all" || testType == "perf") {
        testPerformance(host.get());
    }
    
    // Unload the plugin (happens automatically when host is destroyed)
    std::cout << "\nTests completed successfully" << std::endl;
    
    return 0;
}