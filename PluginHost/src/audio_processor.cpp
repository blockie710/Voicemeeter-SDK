#include "../include/audio_processor.h"
#include <cstring>
#include <algorithm>
#include <cmath>

#ifdef _WIN32
#include <immintrin.h>  // For AVX, AVX2
#endif

// Check if SIMD optimizations are available at runtime
bool AudioProcessor::isSIMDAvailable() {
#ifdef _WIN32
    // Check for AVX support
    int cpuInfo[4] = {0};
    __cpuid(cpuInfo, 1);
    return (cpuInfo[2] & (1 << 28)) != 0; // AVX support bit
#else
    return false; // Non-Windows platforms need different detection
#endif
}

// Process audio buffers with automatic selection of optimized implementation
void AudioProcessor::processAudio(float** inputs, float** outputs, int numInputs, int numOutputs, int numSamples) {
    // Choose appropriate implementation based on available CPU features
    if (isSIMDAvailable()) {
        processAudio_SIMD(inputs, outputs, numInputs, numOutputs, numSamples);
    } else {
        processAudio_Standard(inputs, outputs, numInputs, numOutputs, numSamples);
    }
}

// Standard implementation without SIMD optimizations
void AudioProcessor::processAudio_Standard(float** inputs, float** outputs, int numInputs, int numOutputs, int numSamples) {
    if (!m_plugins.empty() && !m_bypassAll) {
        // Process each plugin in the chain
        for (auto& plugin : m_plugins) {
            if (plugin && plugin->isEnabled()) {
                plugin->processBlock(inputs, outputs, numInputs, numOutputs, numSamples);
                
                // The outputs of this plugin become the inputs for the next
                for (int ch = 0; ch < std::min(numInputs, numOutputs); ch++) {
                    std::memcpy(inputs[ch], outputs[ch], numSamples * sizeof(float));
                }
            }
        }
    } else {
        // Simply copy input to output (bypass)
        for (int ch = 0; ch < std::min(numInputs, numOutputs); ch++) {
            std::memcpy(outputs[ch], inputs[ch], numSamples * sizeof(float));
        }
    }
}

// SIMD optimized implementation
void AudioProcessor::processAudio_SIMD(float** inputs, float** outputs, int numInputs, int numOutputs, int numSamples) {
#ifdef _WIN32
    if (!m_plugins.empty() && !m_bypassAll) {
        // Process each plugin in the chain
        for (auto& plugin : m_plugins) {
            if (plugin && plugin->isEnabled()) {
                plugin->processBlock(inputs, outputs, numInputs, numOutputs, numSamples);
                
                // The outputs of this plugin become the inputs for the next
                // Using AVX to optimize the copy operation
                int channelsToCopy = std::min(numInputs, numOutputs);
                
                for (int ch = 0; ch < channelsToCopy; ch++) {
                    float* src = outputs[ch];
                    float* dst = inputs[ch];
                    
                    // Process 8 floats at a time using AVX
                    int i = 0;
                    for (; i <= numSamples - 8; i += 8) {
                        __m256 values = _mm256_loadu_ps(&src[i]);
                        _mm256_storeu_ps(&dst[i], values);
                    }
                    
                    // Handle remaining samples
                    for (; i < numSamples; i++) {
                        dst[i] = src[i];
                    }
                }
            }
        }
    } else {
        // Simply copy input to output (bypass) with AVX optimization
        int channelsToCopy = std::min(numInputs, numOutputs);
        
        for (int ch = 0; ch < channelsToCopy; ch++) {
            float* src = inputs[ch];
            float* dst = outputs[ch];
            
            // Process 8 floats at a time using AVX
            int i = 0;
            for (; i <= numSamples - 8; i += 8) {
                __m256 values = _mm256_loadu_ps(&src[i]);
                _mm256_storeu_ps(&dst[i], values);
            }
            
            // Handle remaining samples
            for (; i < numSamples; i++) {
                dst[i] = src[i];
            }
        }
    }
#else
    // Fallback to standard implementation on non-Windows platforms
    processAudio_Standard(inputs, outputs, numInputs, numOutputs, numSamples);
#endif
}

// Add a plugin to the processing chain
void AudioProcessor::addPlugin(std::shared_ptr<PluginInstance> plugin) {
    if (plugin) {
        m_plugins.push_back(plugin);
    }
}

// Remove a plugin from the processing chain
bool AudioProcessor::removePlugin(int index) {
    if (index >= 0 && index < m_plugins.size()) {
        m_plugins.erase(m_plugins.begin() + index);
        return true;
    }
    return false;
}

// Reorder plugins in the chain
bool AudioProcessor::movePlugin(int fromIndex, int toIndex) {
    if (fromIndex >= 0 && fromIndex < m_plugins.size() && 
        toIndex >= 0 && toIndex < m_plugins.size() && 
        fromIndex != toIndex) {
        
        auto plugin = m_plugins[fromIndex];
        m_plugins.erase(m_plugins.begin() + fromIndex);
        
        if (toIndex > fromIndex) {
            toIndex--; // Adjust index after removal
        }
        
        m_plugins.insert(m_plugins.begin() + toIndex, plugin);
        return true;
    }
    return false;
}

// Bypass all plugins
void AudioProcessor::setBypassAll(bool bypass) {
    m_bypassAll = bypass;
}

// Get the list of plugins
const std::vector<std::shared_ptr<PluginInstance>>& AudioProcessor::getPlugins() const {
    return m_plugins;
}

// Clear all plugins from the chain
void AudioProcessor::clearPlugins() {
    m_plugins.clear();
}

// Initialize all plugins in the chain
bool AudioProcessor::prepareToPlay(double sampleRate, int maxSamplesPerBlock) {
    m_sampleRate = sampleRate;
    m_blockSize = maxSamplesPerBlock;
    
    bool success = true;
    for (auto& plugin : m_plugins) {
        if (plugin) {
            plugin->prepareToPlay(sampleRate, maxSamplesPerBlock);
        }
    }
    return success;
}

// Clean up resources
void AudioProcessor::releaseResources() {
    for (auto& plugin : m_plugins) {
        if (plugin) {
            plugin->releaseResources();
        }
    }
}