/**
 * Platform-specific utilities for Voicemeeter Plugin Host
 * 
 * Provides cross-platform helper functions for file paths, system directories, etc.
 */
#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#include <knownfolders.h>
#else
#include <cstdlib>
#include <pwd.h>
#include <unistd.h>
#include <sys/types.h>
#endif

namespace PlatformUtils {

/**
 * Get the user's home directory
 * 
 * @return Path to the home directory
 */
inline std::string getHomeDirectory() {
#ifdef _WIN32
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, path))) {
        return std::string(path);
    }
    return "";
#else
    const char* home = getenv("HOME");
    if (home) {
        return std::string(home);
    }
    
    struct passwd* pw = getpwuid(getuid());
    if (pw) {
        return std::string(pw->pw_dir);
    }
    
    return "";
#endif
}

/**
 * Get the user's documents directory
 * 
 * @return Path to the documents directory
 */
inline std::string getDocumentsDirectory() {
#ifdef _WIN32
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_MYDOCUMENTS, NULL, 0, path))) {
        return std::string(path);
    }
    return "";
#else
    // On macOS/Linux, typically it's ~/Documents
    std::string home = getHomeDirectory();
    if (!home.empty()) {
        return home + "/Documents";
    }
    return "";
#endif
}

/**
 * Get standard plugin directories for the current platform
 * 
 * @return Vector of standard plugin directory paths
 */
inline std::vector<std::string> getStandardPluginDirectories() {
    std::vector<std::string> dirs;
    
#ifdef _WIN32
    // Windows standard VST3 locations
    dirs.push_back("C:\\Program Files\\Common Files\\VST3");
    dirs.push_back("C:\\Program Files (x86)\\Common Files\\VST3");
    
    // Windows standard AAX locations
    dirs.push_back("C:\\Program Files\\Common Files\\Avid\\Audio\\Plug-Ins");
    
    // User plugin directories
    std::string userProfile = getHomeDirectory();
    if (!userProfile.empty()) {
        dirs.push_back(userProfile + "\\Documents\\VST3");
    }
#elif defined(__APPLE__)
    // macOS standard locations
    dirs.push_back("/Library/Audio/Plug-Ins/VST3");
    dirs.push_back("/Library/Audio/Plug-Ins/Components");
    dirs.push_back("/Library/Application Support/Avid/Audio/Plug-Ins");
    
    // User plugin directories
    std::string home = getHomeDirectory();
    if (!home.empty()) {
        dirs.push_back(home + "/Library/Audio/Plug-Ins/VST3");
        dirs.push_back(home + "/Library/Audio/Plug-Ins/Components");
    }
#else
    // Linux standard locations
    dirs.push_back("/usr/lib/vst3");
    dirs.push_back("/usr/local/lib/vst3");
    
    // User plugin directories
    std::string home = getHomeDirectory();
    if (!home.empty()) {
        dirs.push_back(home + "/.vst3");
    }
#endif

    // Remove directories that don't exist
    dirs.erase(
        std::remove_if(dirs.begin(), dirs.end(),
            [](const std::string& dir) { 
                return !std::filesystem::exists(dir); 
            }),
        dirs.end()
    );

    return dirs;
}

} // namespace PlatformUtils
