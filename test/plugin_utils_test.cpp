/**
 * Unit tests for the plugin utility functions
 */
#include <gtest/gtest.h>
#include <filesystem>
#include <vector>
#include <string>

#include "../PluginHost/include/platform_utils.h"
#include "../PluginHost/include/plugin_interface.h"
#include "test_utils.h"

// Test home directory retrieval
TEST(PlatformUtilsTest, GetHomeDirectory) {
    std::string home = PlatformUtils::getHomeDirectory();
    ASSERT_FALSE(home.empty()) << "Home directory should not be empty";
    ASSERT_TRUE(std::filesystem::exists(home)) << "Home directory should exist";
}

// Test documents directory retrieval
TEST(PlatformUtilsTest, GetDocumentsDirectory) {
    std::string docs = PlatformUtils::getDocumentsDirectory();
    ASSERT_FALSE(docs.empty()) << "Documents directory should not be empty";
}

// Test standard plugin directories
TEST(PlatformUtilsTest, GetStandardPluginDirectories) {
    auto dirs = PlatformUtils::getStandardPluginDirectories();
    // At least some directories should exist on the system
    ASSERT_FALSE(dirs.empty()) << "Should find at least one standard plugin directory";
    
    // Each returned directory should exist
    for (const auto& dir : dirs) {
        ASSERT_TRUE(std::filesystem::exists(dir)) << "Directory should exist: " << dir;
    }
}

// Test audio utilities
TEST(TestUtilsTest, GenerateSineWave) {
    const int bufferSize = 1024;
    std::vector<float> buffer(bufferSize);
    
    test_utils::generateSineWave(buffer.data(), bufferSize, 48000, 440.0f, 0.5f);
    
    // Check that the buffer contains a valid sine wave
    // First sample should be 0 (sine starts at zero)
    ASSERT_NEAR(buffer[0], 0.0f, 0.01f);
    
    // Check RMS value is close to expected for sine wave with amplitude 0.5
    // RMS of sine wave is amplitude / sqrt(2)
    float rms = test_utils::calculateRMS(buffer.data(), bufferSize);
    float expectedRms = 0.5f / std::sqrt(2.0f);
    ASSERT_NEAR(rms, expectedRms, 0.01f);
}

// Test RMS calculation
TEST(TestUtilsTest, CalculateRMS) {
    const int bufferSize = 1024;
    std::vector<float> buffer(bufferSize, 0.5f);  // All samples at 0.5
    
    float rms = test_utils::calculateRMS(buffer.data(), bufferSize);
    ASSERT_NEAR(rms, 0.5f, 0.0001f);
}

// Test plugin scanner creation
TEST(PluginInterfaceTest, CreatePluginScanner) {
    auto vst3Scanner = createPluginScanner(PluginFormat::VST3);
    ASSERT_NE(vst3Scanner, nullptr) << "VST3 scanner should be created";
    
    auto aaxScanner = createPluginScanner(PluginFormat::AAX);
    ASSERT_NE(aaxScanner, nullptr) << "AAX scanner should be created";
    
    // AAU is platform-specific
    #ifdef __APPLE__
    auto aauScanner = createPluginScanner(PluginFormat::AAU);
    ASSERT_NE(aauScanner, nullptr) << "AAU scanner should be created on macOS";
    #endif
}

// Main function will be provided by gtest_main
