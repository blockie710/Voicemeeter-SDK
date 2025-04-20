/**
 * Shared test utilities for Voicemeeter Plugin Host tests
 */
#pragma once

#include <cmath>
#include <vector>
#include <string>
#include <iostream>
#include <memory>
#include <chrono>
#include <random>
#include <filesystem>

namespace test_utils {

/**
 * Generate a sine wave for testing audio processing
 * 
 * @param buffer Output buffer to fill with samples
 * @param numSamples Number of samples to generate
 * @param sampleRate Sample rate in Hz
 * @param frequency Frequency of sine wave in Hz
 * @param amplitude Amplitude of sine wave (0.0 to 1.0)
 */
inline void generateSineWave(float* buffer, int numSamples, int sampleRate, float frequency, float amplitude) {
    const float twoPi = 6.28318530718f;
    const float phase = twoPi * frequency / sampleRate;
    
    for (int i = 0; i < numSamples; i++) {
        buffer[i] = amplitude * std::sin(i * phase);
    }
}

/**
 * Generate white noise for testing
 * 
 * @param buffer Output buffer to fill with samples
 * @param numSamples Number of samples to generate
 * @param amplitude Amplitude of noise (0.0 to 1.0)
 */
inline void generateWhiteNoise(float* buffer, int numSamples, float amplitude) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-amplitude, amplitude);
    
    for (int i = 0; i < numSamples; i++) {
        buffer[i] = dist(gen);
    }
}

/**
 * Calculate RMS (Root Mean Square) value of a signal
 * 
 * @param buffer Input buffer with samples
 * @param numSamples Number of samples to process
 * @return RMS value of the signal
 */
inline float calculateRMS(const float* buffer, int numSamples) {
    float sum = 0.0f;
    
    for (int i = 0; i < numSamples; i++) {
        sum += buffer[i] * buffer[i];
    }
    
    return std::sqrt(sum / numSamples);
}

/**
 * Find plugins in a directory
 * 
 * @param directory Directory to scan
 * @param extensions List of file extensions to consider
 * @return List of full paths to potential plugin files
 */
inline std::vector<std::string> findPluginFiles(const std::string& directory, 
                                                const std::vector<std::string>& extensions) {
    std::vector<std::string> result;
    
    try {
        if (!std::filesystem::exists(directory)) {
            std::cerr << "Directory does not exist: " << directory << std::endl;
            return result;
        }
        
        for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                
                // Convert to lowercase for case-insensitive comparison
                std::transform(ext.begin(), ext.end(), ext.begin(), 
                               [](unsigned char c) { return std::tolower(c); });
                
                for (const auto& validExt : extensions) {
                    if (ext == validExt) {
                        result.push_back(entry.path().string());
                        break;
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error scanning directory: " << e.what() << std::endl;
    }
    
    return result;
}

} // namespace test_utils
