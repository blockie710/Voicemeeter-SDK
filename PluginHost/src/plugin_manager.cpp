#include "../include/plugin_manager.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <chrono>
#include <cstring>

// For dynamic library loading
#ifdef _WIN32
    #include <windows.h>
    #define DL_HANDLE HMODULE
    #define DL_OPEN(path) LoadLibraryA(path)
    #define DL_CLOSE(handle) FreeLibrary(handle)
    #define DL_SYM(handle, name) GetProcAddress(handle, name)
    #define DL_ERROR() GetLastError()
    #define PATH_SEPARATOR "\\"
#else
    #include <dlfcn.h>
    #define DL_HANDLE void*
    #define DL_OPEN(path) dlopen(path, RTLD_LAZY)
    #define DL_CLOSE(handle) dlclose(handle)
    #define DL_SYM(handle, name) dlsym(handle, name)
    #define DL_ERROR() dlerror()
    #define PATH_SEPARATOR "/"
#endif

// For JSON serialization - using a header-only JSON library
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

// Constructor implementation for PluginInfo
PluginManager::PluginInfo::PluginInfo(const std::shared_ptr<PluginInstance>& plugin) {
    if (plugin) {
        name = plugin->getName();
        vendor = plugin->getVendor();
        version = plugin->getVersion();
        format = plugin->getFormat();
        inputChannels = plugin->getNumInputChannels();
        outputChannels = plugin->getNumOutputChannels();
        hasEditor = plugin->hasEditor();
        
        // Default values for user metadata
        favorite = false;
        userRating = 0;
        lastUsed = std::chrono::system_clock::now();
        useCount = 1;
    }
}

// Implementation of PluginManager
struct PluginManager::Impl {
    std::vector<std::string> pluginPaths;
    std::map<std::string, PluginInfo> discoveredPlugins;
    std::atomic<bool> scanning{false};
    std::atomic<float> scanProgress{0.0f};
    std::thread scanThread;
    mutable std::mutex pluginsMutex;
    ScanProgressCallback progressCallback;
    PluginsChangedCallback pluginsChangedCallback;

    // Check if a plugin at the given path is already discovered
    bool isPluginDiscovered(const std::string& path) {
        std::lock_guard<std::mutex> lock(pluginsMutex);
        return discoveredPlugins.find(path) != discoveredPlugins.end();
    }
    
    // Add a plugin to the discovered plugins list
    void addDiscoveredPlugin(const std::string& path, const PluginInfo& info) {
        std::lock_guard<std::mutex> lock(pluginsMutex);
        discoveredPlugins[path] = info;
        
        // Notify that plugins have changed
        if (pluginsChangedCallback) {
            pluginsChangedCallback();
        }
    }
    
    // Scan a single file to see if it's a plugin
    bool scanPluginFile(const std::string& path, PluginManager* manager) {
        // Skip if already discovered
        if (isPluginDiscovered(path)) {
            return false;
        }
        
        // Check if this is a valid plugin file according to platform-specific rules
        if (!manager->isValidPluginFile(path)) {
            return false;
        }
        
        // Get file extension to determine plugin type
        std::string extension = fs::path(path).extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        
        // Try to map extension to plugin format
        PluginFormat format = PluginFormat::UNKNOWN;
        
        if (extension == ".vst3") {
            format = PluginFormat::VST3;
        } else if (extension == ".dll" || extension == ".so" || extension == ".dylib") {
            // Could be VST3, AAX, ARA, or REAPER extension - need to inspect
            // For now, try VST3 first
            format = PluginFormat::VST3;
        } else if (extension == ".component" || extension == ".vst") {
            format = PluginFormat::AAU;
        } else if (extension == ".lua") {
            format = PluginFormat::LUA;
        } else if (extension == ".jsfx") {
            format = PluginFormat::REAPER;
        }
        
        if (format == PluginFormat::UNKNOWN) {
            return false;
        }
        
        // Create appropriate scanner for this format
        auto scanner = createPluginScanner(format);
        if (!scanner) {
            return false;
        }
        
        // Try to load the plugin to get its info
        auto plugin = scanner->loadPlugin(path, format);
        
        // Try other formats if initial guess failed
        if (!plugin && (extension == ".dll" || extension == ".so" || extension == ".dylib")) {
            // Try each supported format
            std::vector<PluginFormat> formatsToTry = {
                PluginFormat::VST, 
                PluginFormat::AAX, 
                PluginFormat::ARA, 
                PluginFormat::REAPER
            };
            
            for (auto tryFormat : formatsToTry) {
                auto formatScanner = createPluginScanner(tryFormat);
                if (!formatScanner) continue;
                
                try {
                    plugin = formatScanner->loadPlugin(path, tryFormat);
                    if (plugin) {
                        format = tryFormat;
                        break;
                    }
                } catch (const std::exception& e) {
                    // Log error and continue with next format
                    g_logger.log(PluginLogger::Level::Warning, 
                        "Failed to load plugin as " + std::to_string(static_cast<int>(tryFormat)) + 
                        ": " + e.what());
                }
            }
        }
        
        // If all attempts failed, this is not a compatible plugin
        if (!plugin) {
            return false;
        }
        
        // Create plugin info
        PluginInfo info(plugin);
        info.path = path;
        info.format = format;
        
        // Add to discovered plugins
        addDiscoveredPlugin(path, info);
        return true;
    }
    
    // Scan a directory for plugins
    void scanDirectoryInternal(const std::string& directory, bool recursive, PluginManager* manager) {
        try {
            if (!fs::exists(directory) || !fs::is_directory(directory)) {
                return;
            }
            
            // Setup for recursive or non-recursive iteration
            if (recursive) {
                for (const auto& entry : fs::recursive_directory_iterator(directory)) {
                    // Skip if scanning was cancelled
                    if (!scanning) {
                        return;
                    }
                    
                    // Skip directories
                    if (!fs::is_regular_file(entry)) {
                        continue;
                    }
                    
                    // Update progress callback
                    if (progressCallback) {
                        progressCallback(scanProgress, entry.path().string());
                    }
                    
                    // Scan this file
                    scanPluginFile(entry.path().string(), manager);
                }
            } else {
                for (const auto& entry : fs::directory_iterator(directory)) {
                    // Skip if scanning was cancelled
                    if (!scanning) {
                        return;
                    }
                    
                    // Skip directories
                    if (!fs::is_regular_file(entry)) {
                        continue;
                    }
                    
                    // Update progress callback
                    if (progressCallback) {
                        progressCallback(scanProgress, entry.path().string());
                    }
                    
                    // Scan this file
                    scanPluginFile(entry.path().string(), manager);
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error scanning directory " << directory << ": " << e.what() << std::endl;
        }
    }
    
    // Start scanning all plugin paths
    void startScan(bool async, PluginManager* manager) {
        // If already scanning, do nothing
        if (scanning) {
            return;
        }
        
        scanning = true;
        scanProgress = 0.0f;
        
        auto scanFunc = [this, manager]() {
            int totalPaths = pluginPaths.size();
            int currentPath = 0;
            
            for (const auto& path : pluginPaths) {
                // Update scan progress
                scanProgress = static_cast<float>(currentPath) / totalPaths;
                
                if (progressCallback) {
                    progressCallback(scanProgress, path);
                }
                
                // Scan this path
                scanDirectoryInternal(path, true, manager);
                
                // Stop if scanning was cancelled
                if (!scanning) {
                    break;
                }
                
                currentPath++;
            }
            
            // Set final progress
            scanProgress = 1.0f;
            if (progressCallback) {
                progressCallback(1.0f, "Scan complete");
            }
            
            scanning = false;
        };
        
        if (async) {
            // Stop previous scan thread if it's still running
            if (scanThread.joinable()) {
                scanThread.join();
            }
            
            // Start a new scan thread
            scanThread = std::thread(scanFunc);
        } else {
            // Run scan synchronously
            scanFunc();
        }
    }
};

// Platform-specific default plugin paths
std::vector<std::string> PluginManager::getDefaultPluginPaths() const {
    std::vector<std::string> paths;
    
#ifdef _WIN32
    // Windows default plugin paths
    paths.push_back("C:\\Program Files\\Common Files\\VST3");
    paths.push_back("C:\\Program Files\\Common Files\\VST2");
    paths.push_back("C:\\Program Files\\Common Files\\Avid\\Audio\\Plug-Ins");
    paths.push_back("C:\\Program Files\\VSTPlugins");
    paths.push_back("C:\\Program Files\\Steinberg\\VSTPlugins");
    
    // User-specific paths
    const char* appData = getenv("APPDATA");
    if (appData) {
        // REAPER paths
        std::string reaperPath = std::string(appData) + "\\REAPER\\Effects";
        paths.push_back(reaperPath);
        
        // VST3 paths
        std::string vst3Path = std::string(appData) + "\\VST3";
        paths.push_back(vst3Path);
    }
#elif defined(__APPLE__)
    // macOS default plugin paths
    paths.push_back("/Library/Audio/Plug-Ins/VST3");
    paths.push_back("/Library/Audio/Plug-Ins/VST");
    paths.push_back("/Library/Audio/Plug-Ins/Components");
    
    // User plugin paths
    const char* home = getenv("HOME");
    if (home) {
        paths.push_back(std::string(home) + "/Library/Audio/Plug-Ins/VST3");
        paths.push_back(std::string(home) + "/Library/Audio/Plug-Ins/VST");
        paths.push_back(std::string(home) + "/Library/Audio/Plug-Ins/Components");
    }
#else
    // Linux default plugin paths
    paths.push_back("/usr/lib/vst3");
    paths.push_back("/usr/lib/vst");
    paths.push_back("/usr/lib/lv2");
    
    // User plugin paths
    const char* home = getenv("HOME");
    if (home) {
        paths.push_back(std::string(home) + "/.vst3");
        paths.push_back(std::string(home) + "/.vst");
        paths.push_back(std::string(home) + "/.lv2");
        
        // XDG paths
        const char* xdgData = getenv("XDG_DATA_HOME");
        if (xdgData) {
            paths.push_back(std::string(xdgData) + "/vst3");
        } else if (home) {
            paths.push_back(std::string(home) + "/.local/share/vst3");
        }
    }
#endif

    return paths;
}

bool PluginManager::isValidPluginFile(const std::string& path) const {
    // Check if file exists and is readable
    if (!fs::exists(path) || !fs::is_regular_file(path)) {
        return false;
    }

    // Get file extension
    std::string extension = fs::path(path).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    // Check extension based on platform
#ifdef _WIN32
    // Windows supported extensions
    return extension == ".dll" || extension == ".vst3" || extension == ".lua" || extension == ".jsfx";
#elif defined(__APPLE__)
    // macOS supported extensions
    return extension == ".vst" || extension == ".vst3" || extension == ".component" || 
           extension == ".dylib" || extension == ".lua" || extension == ".jsfx";
#else
    // Linux supported extensions
    return extension == ".so" || extension == ".vst3" || extension == ".lua" || extension == ".jsfx";
#endif
}

PluginManager::PluginManager() : m_impl(std::make_unique<Impl>()) {
    // Initialize with default plugin paths
    m_impl->pluginPaths = getDefaultPluginPaths();
}

PluginManager::~PluginManager() {
    // Cancel any ongoing scan
    cancelScanning();
    
    // Wait for scan thread to finish
    if (m_impl->scanThread.joinable()) {
        m_impl->scanThread.join();
    }
}

void PluginManager::addPluginPath(const std::string& path) {
    // Check if path already exists
    auto it = std::find(m_impl->pluginPaths.begin(), m_impl->pluginPaths.end(), path);
    if (it == m_impl->pluginPaths.end()) {
        m_impl->pluginPaths.push_back(path);
    }
}

void PluginManager::removePluginPath(const std::string& path) {
    auto it = std::find(m_impl->pluginPaths.begin(), m_impl->pluginPaths.end(), path);
    if (it != m_impl->pluginPaths.end()) {
        m_impl->pluginPaths.erase(it);
    }
}

std::vector<std::string> PluginManager::getPluginPaths() const {
    return m_impl->pluginPaths;
}

void PluginManager::autoScanForPlugins(bool async) {
    // Start scanning all paths
    m_impl->startScan(async, this);
}

void PluginManager::scanDirectory(const std::string& directory, bool recursive, bool async) {
    // If already scanning, do nothing
    if (m_impl->scanning) {
        return;
    }
    
    m_impl->scanning = true;
    m_impl->scanProgress = 0.0f;
    
    auto scanFunc = [this, directory, recursive]() {
        try {
            if (m_impl->progressCallback) {
                m_impl->progressCallback(0.0f, directory);
            }
            
            // Scan the directory
            m_impl->scanDirectoryInternal(directory, recursive, this);
            
            // Update final progress
            m_impl->scanProgress = 1.0f;
            if (m_impl->progressCallback) {
                m_impl->progressCallback(1.0f, "Scan complete");
            }
        }
        catch (const std::exception& e) {
            g_logger.log(PluginLogger::Level::Error, "Exception during plugin scan: " + std::string(e.what()));
        }
        
        m_impl->scanning = false;
    };
    
    if (async) {
        // Stop previous scan thread if it's still running
        if (m_impl->scanThread.joinable()) {
            m_impl->scanThread.join();
        }
        
        // Start a new scan thread
        m_impl->scanThread = std::thread(scanFunc);
    } else {
        // Run scan synchronously
        scanFunc();
    }
}

bool PluginManager::isScanningPlugins() const {
    return m_impl->scanning;
}

float PluginManager::getScanningProgress() const {
    return m_impl->scanProgress;
}

void PluginManager::cancelScanning() {
    std::lock_guard<std::mutex> lock(m_impl->pluginsMutex);
    
    if (m_impl->scanning) {
        m_impl->scanning = false;
        
        // Only join if the thread is joinable to prevent issues
        if (m_impl->scanThread.joinable()) {
            m_impl->scanThread.join();
        }
    }
}

void PluginManager::setScanProgressCallback(ScanProgressCallback callback) {
    m_impl->progressCallback = callback;
}

std::vector<PluginManager::PluginInfo> PluginManager::getAllPlugins() const {
    std::lock_guard<std::mutex> lock(m_impl->pluginsMutex);
    std::vector<PluginInfo> plugins;
    plugins.reserve(m_impl->discoveredPlugins.size());
    
    for (const auto& pair : m_impl->discoveredPlugins) {
        plugins.push_back(pair.second);
    }
    
    return plugins;
}

std::vector<PluginManager::PluginInfo> PluginManager::getFilteredPlugins(const FilterCriteria& criteria) const {
    std::vector<PluginInfo> allPlugins = getAllPlugins();
    std::vector<PluginInfo> filteredPlugins;
    
    // Apply filters
    for (const auto& plugin : allPlugins) {
        bool include = true;
        
        // Search text filter (in name, vendor, description)
        if (!criteria.searchText.empty()) {
            // Case insensitive search
            std::string search = criteria.searchText;
            std::string name = plugin.name;
            std::string vendor = plugin.vendor;
            std::string description = plugin.description;
            
            std::transform(search.begin(), search.end(), search.begin(), ::tolower);
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            std::transform(vendor.begin(), vendor.end(), vendor.begin(), ::tolower);
            std::transform(description.begin(), description.end(), description.begin(), ::tolower);
            
            if (name.find(search) == std::string::npos && 
                vendor.find(search) == std::string::npos && 
                description.find(search) == std::string::npos) {
                include = false;
            }
        }
        
        // Format filter
        if (!criteria.formats.empty() && 
            criteria.formats.find(plugin.format) == criteria.formats.end()) {
            include = false;
        }
        
        // Category filter
        if (!criteria.categories.empty() && 
            criteria.categories.find(plugin.category) == criteria.categories.end()) {
            include = false;
        }
        
        // Vendor filter
        if (!criteria.vendors.empty() && 
            criteria.vendors.find(plugin.vendor) == criteria.vendors.end()) {
            include = false;
        }
        
        // Tags filter
        if (!criteria.tags.empty()) {
            bool hasTag = false;
            for (const auto& tag : plugin.tags) {
                if (criteria.tags.find(tag) != criteria.tags.end()) {
                    hasTag = true;
                    break;
                }
            }
            if (!hasTag) {
                include = false;
            }
        }
        
        // Favorites filter
        if (criteria.favoritesOnly && !plugin.favorite) {
            include = false;
        }
        
        // Rating filter
        if (plugin.userRating < criteria.minRating) {
            include = false;
        }
        
        // Input channels filter
        if (plugin.inputChannels < criteria.minInputChannels) {
            include = false;
        }
        
        // Output channels filter
        if (plugin.outputChannels < criteria.minOutputChannels) {
            include = false;
        }
        
        // Has editor filter
        if (criteria.hasEditorOnly && !plugin.hasEditor) {
            include = false;
        }
        
        if (include) {
            filteredPlugins.push_back(plugin);
        }
    }
    
    return filteredPlugins;
}

std::vector<PluginManager::PluginInfo> PluginManager::getSortedPlugins(SortCriteria criteria, bool ascending) const {
    std::vector<PluginInfo> plugins = getAllPlugins();
    
    // Define comparison function based on sort criteria
    auto compare = [criteria, ascending](const PluginInfo& a, const PluginInfo& b) -> bool {
        bool result = false;
        
        switch (criteria) {
            case SortCriteria::NAME:
                result = a.name < b.name;
                break;
            case SortCriteria::VENDOR:
                result = a.vendor < b.vendor;
                break;
            case SortCriteria::FORMAT:
                result = static_cast<int>(a.format) < static_cast<int>(b.format);
                break;
            case SortCriteria::RATING:
                result = a.userRating < b.userRating;
                break;
            case SortCriteria::LAST_USED:
                result = a.lastUsed < b.lastUsed;
                break;
            case SortCriteria::USE_COUNT:
                result = a.useCount < b.useCount;
                break;
            case SortCriteria::CATEGORY:
                result = a.category < b.category;
                break;
            case SortCriteria::INPUT_CHANNELS:
                result = a.inputChannels < b.inputChannels;
                break;
            case SortCriteria::OUTPUT_CHANNELS:
                result = a.outputChannels < b.outputChannels;
                break;
        }
        
        // Invert result if descending
        return ascending ? result : !result;
    };
    
    // Sort the plugins
    std::sort(plugins.begin(), plugins.end(), compare);
    
    return plugins;
}

std::shared_ptr<PluginInstance> PluginManager::loadPlugin(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_impl->pluginsMutex);
    
    // Check if plugin is discovered
    auto it = m_impl->discoveredPlugins.find(path);
    if (it == m_impl->discoveredPlugins.end()) {
        // Try to detect format from file extension
        PluginFormat format = PluginFormat::UNKNOWN;
        std::string extension = std::filesystem::path(path).extension().string();
        
        if (extension == ".vst3") format = PluginFormat::VST3;
        else if (extension == ".aaxplugin") format = PluginFormat::AAX;
        else if (extension == ".component") format = PluginFormat::AAU;
        else if (extension == ".jsfx") format = PluginFormat::REAPER;
        else if (extension == ".lua") format = PluginFormat::LUA;
        
        // Create scanner for detected format
        auto scanner = createPluginScanner(format);
        if (!scanner) {
            return nullptr;
        }
        
        // Load the plugin
        return scanner->loadPlugin(path, format);
    }
    
    // Get plugin info
    const PluginInfo& info = it->second;
    
    // Create scanner for this format
    auto scanner = createPluginScanner(info.format);
    if (!scanner) {
        return nullptr;
    }
    
    // Load the plugin
    auto plugin = scanner->loadPlugin(path, info.format);
    
    // Update usage metadata if successful
    if (plugin) {
        PluginInfo& mutableInfo = m_impl->discoveredPlugins[path];
        mutableInfo.lastUsed = std::chrono::system_clock::now();
        mutableInfo.useCount++;
    }
    
    return plugin;
}

std::set<std::string> PluginManager::getAllCategories() const {
    std::lock_guard<std::mutex> lock(m_impl->pluginsMutex);
    std::set<std::string> categories;
    
    for (const auto& pair : m_impl->discoveredPlugins) {
        if (!pair.second.category.empty()) {
            categories.insert(pair.second.category);
        }
    }
    
    return categories;
}

std::set<std::string> PluginManager::getAllVendors() const {
    std::lock_guard<std::mutex> lock(m_impl->pluginsMutex);
    std::set<std::string> vendors;
    
    for (const auto& pair : m_impl->discoveredPlugins) {
        if (!pair.second.vendor.empty()) {
            vendors.insert(pair.second.vendor);
        }
    }
    
    return vendors;
}

std::set<std::string> PluginManager::getAllTags() const {
    std::lock_guard<std::mutex> lock(m_impl->pluginsMutex);
    std::set<std::string> tags;
    
    for (const auto& pair : m_impl->discoveredPlugins) {
        for (const auto& tag : pair.second.tags) {
            tags.insert(tag);
        }
    }
    
    return tags;
}

std::set<PluginFormat> PluginManager::getAllFormats() const {
    std::lock_guard<std::mutex> lock(m_impl->pluginsMutex);
    std::set<PluginFormat> formats;
    
    for (const auto& pair : m_impl->discoveredPlugins) {
        formats.insert(pair.second.format);
    }
    
    return formats;
}

bool PluginManager::saveSettings(const std::string& path) const {
    std::lock_guard<std::mutex> lock(m_impl->pluginsMutex);
    
    try {
        json j;
        
        // Save plugin paths
        j["paths"] = m_impl->pluginPaths;
        
        // Save plugin metadata
        json plugins = json::array();
        
        for (const auto& pair : m_impl->discoveredPlugins) {
            const auto& info = pair.second;
            
            json plugin;
            plugin["path"] = info.path;
            plugin["favorite"] = info.favorite;
            plugin["rating"] = info.userRating;
            plugin["lastUsed"] = info.lastUsed;
            plugin["useCount"] = info.useCount;
            plugin["tags"] = info.tags;
            
            plugins.push_back(plugin);
        }
        
        j["plugins"] = plugins;
        
        // Write to file
        std::ofstream file(path);
        if (!file.is_open()) {
            return false;
        }
        
        file << j.dump(4);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error saving plugin manager settings: " << e.what() << std::endl;
        return false;
    }
}

bool PluginManager::loadSettings(const std::string& path) {
    try {
        // Read file
        std::ifstream file(path);
        if (!file.is_open()) {
            return false;
        }
        
        json j = json::parse(file);
        
        // Load plugin paths
        if (j.contains("paths") && j["paths"].is_array()) {
            std::vector<std::string> paths;
            for (const auto& item : j["paths"]) {
                if (item.is_string()) {
                    paths.push_back(item.get<std::string>());
                }
            }
            m_impl->pluginPaths = paths;
        }
        
        // Load plugin metadata
        if (j.contains("plugins") && j["plugins"].is_array()) {
            std::lock_guard<std::mutex> lock(m_impl->pluginsMutex);
            
            for (const auto& item : j["plugins"]) {
                std::string path;
                
                if (item.contains("path") && item["path"].is_string()) {
                    path = item["path"].get<std::string>();
                } else {
                    continue;
                }
                
                // Update existing plugin or create a new entry
                auto it = m_impl->discoveredPlugins.find(path);
                PluginInfo info;
                
                if (it != m_impl->discoveredPlugins.end()) {
                    info = it->second;
                } else {
                    info.path = path;
                }
                
                // Update metadata
                if (item.contains("favorite") && item["favorite"].is_boolean()) {
                    info.favorite = item["favorite"].get<bool>();
                }
                
                if (item.contains("rating") && item["rating"].is_number()) {
                    info.userRating = item["rating"].get<int>();
                }
                
                if (item.contains("lastUsed") && item["lastUsed"].is_number()) {
                    info.lastUsed = item["lastUsed"].get<time_t>();
                }
                
                if (item.contains("useCount") && item["useCount"].is_number()) {
                    info.useCount = item["useCount"].get<int>();
                }
                
                if (item.contains("tags") && item["tags"].is_array()) {
                    info.tags.clear();
                    for (const auto& tag : item["tags"]) {
                        if (tag.is_string()) {
                            info.tags.push_back(tag.get<std::string>());
                        }
                    }
                }
                
                m_impl->discoveredPlugins[path] = info;
            }
        }
        
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error loading plugin manager settings: " << e.what() << std::endl;
        return false;
    }
}

void PluginManager::setPluginsChangedCallback(PluginsChangedCallback callback) {
    m_impl->pluginsChangedCallback = callback;
}