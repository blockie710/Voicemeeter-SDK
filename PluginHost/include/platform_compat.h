/**
 * platform_compat.h
 *
 * Platform-specific compatibility layer for the Voicemeeter Plugin Host
 * Provides cross-platform abstractions for OS-specific functionality
 */

#ifndef PLATFORM_COMPAT_H
#define PLATFORM_COMPAT_H

#include <string>

#ifdef _WIN32
// Windows-specific includes
#include <windows.h>
#elif defined(__APPLE__)
// macOS-specific includes
#include <dlfcn.h>
#else
// Linux-specific includes
#include <dlfcn.h>
#endif

// Get user's home directory
std::string getHomeDirectory();

// Get application data directory
std::string getAppDataPath();

// Get user documents directory
std::string getUserDocumentsPath();

// Get platform-specific VST3 plugin directory
std::string getVST3PluginDirectory();

// Get platform-specific AAX plugin directory
std::string getAAXPluginDirectory();

#ifdef _WIN32
// Windows-specific string conversion
std::wstring utf8ToWide(const std::string& utf8);
std::string wideToUtf8(const std::wstring& wide);

// Windows-specific window handling
void* createWindow(const std::string& title, int width, int height, void* parentWindow);
void showWindow(void* window);
void hideWindow(void* window);
void destroyWindow(void* window);

// Plugin editor windows
void* createVST3PluginEditorWindow(void* parentWindow, const std::string& title);
void* createAAXPluginEditorWindow(void* parentWindow, const std::string& title);

// Registry access helpers
bool getRegistryValueString(HKEY root, const std::string& subKey, const std::string& valueName, std::string& value);
bool getRegistryValueDword(HKEY root, const std::string& subKey, const std::string& valueName, DWORD& value);
#endif

#endif // PLATFORM_COMPAT_H