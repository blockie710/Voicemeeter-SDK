#include "../include/platform_utils.h"
#include <iostream>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>
#else
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#endif

namespace PlatformUtils {

std::string getHomeDirectory() {
#ifdef _WIN32
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, path))) {
        return std::string(path);
    }
    // Fallback to environment variable
    const char* home = getenv("USERPROFILE");
    if (home) {
        return std::string(home);
    }
    return "";
#else
    const char* home = getenv("HOME");
    if (home) {
        return std::string(home);
    }
    
    // Fallback to pwd database
    struct passwd* pw = getpwuid(getuid());
    if (pw) {
        return std::string(pw->pw_dir);
    }
    
    return "";
#endif
}

std::vector<std::string> getStandardPluginDirectories() {
    std::vector<std::string> paths;
    
#ifdef _WIN32
    // Windows default plugin paths
    paths.push_back("C:\\Program Files\\Common Files\\VST3");
    paths.push_back("C:\\Program Files\\Common Files\\VST2");
    paths.push_back("C:\\Program Files\\Common Files\\Avid\\Audio\\Plug-Ins");
    
    // User-specific paths
    std::string appData = getAppDataDirectory();
    if (!appData.empty()) {
        paths.push_back(appData + "\\VST3");
    }
#elif defined(__APPLE__)
    // macOS default plugin paths
    paths.push_back("/Library/Audio/Plug-Ins/VST3");
    paths.push_back("/Library/Audio/Plug-Ins/Components");
    
    // User plugin paths
    std::string home = getHomeDirectory();
    if (!home.empty()) {
        paths.push_back(home + "/Library/Audio/Plug-Ins/VST3");
        paths.push_back(home + "/Library/Audio/Plug-Ins/Components");
    }
#else
    // Linux default plugin paths
    paths.push_back("/usr/lib/vst3");
    
    // User plugin paths
    std::string home = getHomeDirectory();
    if (!home.empty()) {
        paths.push_back(home + "/.vst3");
        paths.push_back(home + "/.lv2");
    }
#endif

    return paths;
}

std::string getAppDataDirectory() {
#ifdef _WIN32
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, path))) {
        return std::string(path);
    }
    // Fallback to environment variable
    const char* appdata = getenv("APPDATA");
    if (appdata) {
        return std::string(appdata);
    }
    return "";
#elif defined(__APPLE__)
    std::string home = getHomeDirectory();
    if (!home.empty()) {
        return home + "/Library/Application Support";
    }
    return "";
#else
    // Linux: Use XDG_CONFIG_HOME if available, otherwise ~/.config
    const char* xdgConfig = getenv("XDG_CONFIG_HOME");
    if (xdgConfig && *xdgConfig) {
        return std::string(xdgConfig);
    }
    
    std::string home = getHomeDirectory();
    if (!home.empty()) {
        return home + "/.config";
    }
    return "";
#endif
}

} // namespace PlatformUtils
