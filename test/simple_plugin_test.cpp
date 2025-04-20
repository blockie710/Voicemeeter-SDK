/**
 * Simple Plugin Host Tester
 * 
 * A streamlined tool for testing plugins with the Voicemeeter Plugin Host
 */
#include <iostream>
#include <string>
#include <memory>
#include <chrono>
#include <cmath>
#include <vector>
#include <filesystem>

// Include the plugin host
#include "../PluginHost/src/VMPluginHost.h"

// Helper function to generate test audio
void generateTestAudio(float* buffer, int numSamples, float frequency, float sampleRate, float amplitude) {
    for (int i = 0; i < numSamples; i++) {
        buffer[i] = amplitude * sinf(2.0f * M_PI * frequency * i / sampleRate);
    }
}

// Print plugin details
void printPluginInfo(VMPluginHost* host) {
    std::cout << "=============================================" << std::endl;
    std::cout << "PLUGIN INFORMATION" << std::endl;
    std::cout << "=============================================" << std::endl;
    std::cout << "Name: " << host->GetPluginName() << std::endl;
    std::cout << "Vendor: " << host->GetPluginVendor() << std::endl;
    std::cout << "Product: " << host->GetPluginProduct() << std::endl;
    std::cout << "=============================================" << std::endl;
}

// Test parameters
void testParameters(VMPluginHost* host) {
    int paramCount = host->GetNumParameters();
    std::cout << "Total parameters: " << paramCount << std::endl;
    
    // Show first 10 parameters only to keep output manageable
    int maxToShow = std::min(10, paramCount);
    for (int i = 0; i < maxToShow; i++) {
        std::string name = host->GetParameterName(i);
        float value = host->GetParameter(i);
        std::string display = host->GetParameterDisplay(i);
        
        float min, max, defaultVal;
        bool hasProps = host->GetParameterProperties(i, &min, &max, &defaultVal);
        
        std::cout << "Parameter " << i << ": " << name << std::endl;
        std::cout << "  Current: " << value << " (" << display << ")" << std::endl;
        if (hasProps) {
            std::cout << "  Range: " << min << " to " << max << " (default: " << defaultVal << ")" << std::endl;
        }
    }
    
    if (paramCount > maxToShow) {
        std::cout << "... and " << (paramCount - maxToShow) << " more parameters." << std::endl;
    }
}

// Test audio processing
void testAudioProcessing(VMPluginHost* host, int bufferSize = 1024, int sampleRate = 48000) {
    // Create audio buffers
    std::vector<float> inputL(bufferSize);
    std::vector<float> inputR(bufferSize);
    std::vector<float> outputL(bufferSize, 0.0f);
    std::vector<float> outputR(bufferSize, 0.0f);
    
    // Generate test signals: 440Hz (A4) in left channel, 587Hz (D5) in right channel
    generateTestAudio(inputL.data(), bufferSize, 440.0f, sampleRate, 0.5f);
    generateTestAudio(inputR.data(), bufferSize, 587.33f, sampleRate, 0.5f);
    
    // Process the audio
    auto startTime = std::chrono::high_resolution_clock::now();
    host->ProcessAudio(inputL.data(), inputR.data(), outputL.data(), outputR.data(), bufferSize, sampleRate);
    auto endTime = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    
    // Calculate some basic audio stats
    float inRmsL = 0.0f, inRmsR = 0.0f;
    float outRmsL = 0.0f, outRmsR = 0.0f;
    
    for (int i = 0; i < bufferSize; i++) {
        inRmsL += inputL[i] * inputL[i];
        inRmsR += inputR[i] * inputR[i];
        outRmsL += outputL[i] * outputL[i];
        outRmsR += outputR[i] * outputR[i];
    }
    
    inRmsL = std::sqrt(inRmsL / bufferSize);
    inRmsR = std::sqrt(inRmsR / bufferSize);
    outRmsL = std::sqrt(outRmsL / bufferSize);
    outRmsR = std::sqrt(outRmsR / bufferSize);
    
    // Print results
    std::cout << "=============================================" << std::endl;
    std::cout << "AUDIO PROCESSING TEST" << std::endl;
    std::cout << "=============================================" << std::endl;
    std::cout << "Buffer size: " << bufferSize << " samples at " << sampleRate << "Hz" << std::endl;
    std::cout << "Processing time: " << duration.count() << " μs" << std::endl;
    
    double bufferDuration = (bufferSize * 1000000.0) / sampleRate;
    double cpuUsage = (duration.count() / bufferDuration) * 100.0;
    std::cout << "Estimated CPU: " << cpuUsage << "%" << std::endl;
    
    std::cout << "Input RMS: L=" << inRmsL << ", R=" << inRmsR << std::endl;
    std::cout << "Output RMS: L=" << outRmsL << ", R=" << outRmsR << std::endl;
    std::cout << "Gain change: L=" << 20 * std::log10(outRmsL/inRmsL) << "dB, R=" << 20 * std::log10(outRmsR/inRmsR) << "dB" << std::endl;
    std::cout << "=============================================" << std::endl;
}

// Scan directory for plugins
std::vector<std::string> findPlugins(const std::string& directory, VMPluginHost* host) {
    std::vector<std::string> pluginPaths;
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string path = entry.path().string();
                // Try to load each file as a plugin
                if (host->LoadPlugin(path)) {
                    std::cout << "Found plugin: " << host->GetPluginName() << " at " << path << std::endl;
                    pluginPaths.push_back(path);
                    host->UnloadPlugin(); // Unload so we can continue scanning
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error scanning directory: " << e.what() << std::endl;
    }
    
    return pluginPaths;
}

void displayUsage(const char* programName) {
    std::cout << "Usage: " << programName << " <command> [options]" << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  scan <directory>     - Scan directory for plugins" << std::endl;
    std::cout << "  info <plugin_path>   - Show information about a plugin" << std::endl;
    std::cout << "  params <plugin_path> - List plugin parameters" << std::endl;
    std::cout << "  test <plugin_path>   - Run audio processing test" << std::endl;
    std::cout << "  full <plugin_path>   - Run all tests (info, params, audio)" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        displayUsage(argv[0]);
        return 1;
    }
    
    std::string command = argv[1];
    
    std::unique_ptr<VMPluginHost> host(new VMPluginHost());
    
    if (command == "scan" && argc >= 3) {
        std::string directory = argv[2];
        std::cout << "Scanning for plugins in: " << directory << std::endl;
        auto plugins = findPlugins(directory, host.get());
        std::cout << "Found " << plugins.size() << " plugins." << std::endl;
    }
    else if (command == "info" && argc >= 3) {
        std::string pluginPath = argv[2];
        if (host->LoadPlugin(pluginPath)) {
            printPluginInfo(host.get());
        } else {
            std::cerr << "Failed to load plugin: " << pluginPath << std::endl;
            return 1;
        }
    }
    else if (command == "params" && argc >= 3) {
        std::string pluginPath = argv[2];
        if (host->LoadPlugin(pluginPath)) {
            printPluginInfo(host.get());
            testParameters(host.get());
        } else {
            std::cerr << "Failed to load plugin: " << pluginPath << std::endl;
            return 1;
        }
    }
    else if (command == "test" && argc >= 3) {
        std::string pluginPath = argv[2];
        if (host->LoadPlugin(pluginPath)) {
            printPluginInfo(host.get());
            testAudioProcessing(host.get());
        } else {
            std::cerr << "Failed to load plugin: " << pluginPath << std::endl;
            return 1;
        }
    }
    else if (command == "full" && argc >= 3) {
        std::string pluginPath = argv[2];
        if (host->LoadPlugin(pluginPath)) {
            printPluginInfo(host.get());
            testParameters(host.get());
            testAudioProcessing(host.get());
        } else {
            std::cerr << "Failed to load plugin: " << pluginPath << std::endl;
            return 1;
        }
    }
    else {
        displayUsage(argv[0]);
        return 1;
    }
    
    return 0;
}
