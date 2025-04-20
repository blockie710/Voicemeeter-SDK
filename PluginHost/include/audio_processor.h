#ifndef AUDIO_PROCESSOR_H
#define AUDIO_PROCESSOR_H

#include "plugin_interface.h"
#include <vector>
#include <memory>

/**
 * AudioProcessor - Manages a chain of audio plugins with optimized processing
 * 
 * This class handles the audio processing pipeline, including:
 * - SIMD optimizations when available (AVX on Windows)
 * - Plugin chain management (add, remove, reorder)
 * - Audio buffer routing between plugins
 */
class AudioProcessor {
public:
    AudioProcessor() : m_sampleRate(48000.0), m_blockSize(1024), m_bypassAll(false) {}
    ~AudioProcessor() { releaseResources(); }

    // Main processing function - automatically selects optimized implementation
    void processAudio(float** inputs, float** outputs, int numInputs, int numOutputs, int numSamples);

    // Plugin chain management
    void addPlugin(std::shared_ptr<PluginInstance> plugin);
    bool removePlugin(int index);
    bool movePlugin(int fromIndex, int toIndex);
    void setBypassAll(bool bypass);
    const std::vector<std::shared_ptr<PluginInstance>>& getPlugins() const;
    void clearPlugins();

    // Audio system lifecycle
    bool prepareToPlay(double sampleRate, int maxSamplesPerBlock);
    void releaseResources();

    // Status check
    bool isSIMDAvailable();

private:
    // Implementation variants
    void processAudio_Standard(float** inputs, float** outputs, int numInputs, int numOutputs, int numSamples);
    void processAudio_SIMD(float** inputs, float** outputs, int numInputs, int numOutputs, int numSamples);

    // Member variables
    std::vector<std::shared_ptr<PluginInstance>> m_plugins;
    double m_sampleRate;
    int m_blockSize;
    bool m_bypassAll;
};

#endif // AUDIO_PROCESSOR_H