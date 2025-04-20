#include "../include/plugin_manager.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <chrono>
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
        lastUsed = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
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
    std::mutex pluginsMutex;
    ScanProgressCallback progressCallback;
    PluginsChangedCallback pluginsChangedCallback;

    // Platform-specific plugin paths
    std::vector<std::string> getDefaultPluginPaths() {
        std::vector<std::string> paths;
        
#ifdef _WIN32
        // Windows default plugin paths
        paths.push_back("C:\\Program Files\\Common Files\\VST3");
        paths.push_back("C:\\Program Files\\Common Files\\VST2");
        paths.push_back("C:\\Program Files\\Common Files\\Avid\\Audio\\Plug-Ins");
        paths.push_back("C:\\Program Files\\VSTPlugins");
        paths.push_back("C:\\Program Files\\Steinberg\\VSTPlugins");
        
        // REAPER paths
        const char* appData = getenv("APPDATA");
        if (appData) {
            std::string reaperPath = std::string(appData) + "\\REAPER\\Effects";
            paths.push_back(reaperPath);
        }
#elif __APPLE__
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
        }
#endif

        return paths;
    }
    
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
    bool scanPluginFile(const std::string& path) {
        // Skip if already discovered
        if (isPluginDiscovered(path)) {
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
        if (!plugin) {
            // Try other formats if initial guess failed
            if (format == PluginFormat::VST3 && 
                (extension == ".dll" || extension == ".so" || extension == ".dylib")) {
                
                // Try AAX
                format = PluginFormat::AAX;
                scanner = createPluginScanner(format);
                if (scanner) {
                    plugin = scanner->loadPlugin(path, format);
                }
                
                // Try ARA if still not successful
                if (!plugin) {
                    format = PluginFormat::ARA;
                    scanner = createPluginScanner(format);
                    if (scanner) {
                        plugin = scanner->loadPlugin(path, format);
                    }
                }
                
                // Try REAPER if still not successful
                if (!plugin) {
                    format = PluginFormat::REAPER;
                    scanner = createPluginScanner(format);
                    if (scanner) {
                        plugin = scanner->loadPlugin(path, format);
                    }
                }
            }
            
            // If all attempts failed, this is not a compatible plugin
            if (!plugin) {
                return false;
            }
        }
        
        // Create plugin info
        PluginInfo info(plugin);
        info.path = path;
        
        // Add to discovered plugins
        addDiscoveredPlugin(path, info);
        return true;
    }
    
    // Scan a directory for plugins
    void scanDirectoryInternal(const std::string& directory, bool recursive) {
        try {
            if (!fs::exists(directory) || !fs::is_directory(directory)) {
                return;
            }
            
            // Setup for recursive or non-recursive iteration
            auto begin = recursive ? fs::recursive_directory_iterator(directory) : fs::directory_iterator(directory);
            auto end = recursive ? fs::recursive_directory_iterator() : fs::directory_iterator();
            
            for (auto it = begin; it != end; ++it) {
                // Skip if scanning was cancelled
                if (!scanning) {
                    return;
                }
                
                // Skip directories
                if (!fs::is_regular_file(*it)) {
                    continue;
                }
                
                // Update progress callback
                if (progressCallback) {
                    progressCallback(scanProgress, it->path().string());
                }
                
                // Scan this file
                scanPluginFile(it->path().string());
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error scanning directory " << directory << ": " << e.what() << std::endl;
        }
    }
    
    // Start scanning all plugin paths
    void startScan(bool async) {
        // If already scanning, do nothing
        if (scanning) {
            return;
        }
        
        scanning = true;
        scanProgress = 0.0f;
        
        auto scanFunc = [this]() {
            int totalPaths = pluginPaths.size();
            int currentPath = 0;
            
            for (const auto& path : pluginPaths) {
                // Update scan progress
                scanProgress = static_cast<float>(currentPath) / totalPaths;
                
                if (progressCallback) {
                    progressCallback(scanProgress, path);
                }
                
                // Scan this path
                scanDirectoryInternal(path, true);
                
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

PluginManager::PluginManager() : m_impl(std::make_unique<Impl>()) {
    // Initialize with default plugin paths
    m_impl->pluginPaths = m_impl->getDefaultPluginPaths();
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
    m_impl->startScan(async);
}

void PluginManager::scanDirectory(const std::string& directory, bool recursive, bool async) {
    // If already scanning, do nothing
    if (m_impl->scanning) {
        return;
    }
    
    m_impl->scanning = true;
    m_impl->scanProgress = 0.0f;
    
    auto scanFunc = [this, directory, recursive]() {
        if (m_impl->progressCallback) {
            m_impl->progressCallback(0.0f, directory);
        }
        
        // Scan the directory
        m_impl->scanDirectoryInternal(directory, recursive);
        
        // Update final progress
        m_impl->scanProgress = 1.0f;
        if (m_impl->progressCallback) {
            m_impl->progressCallback(1.0f, "Scan complete");
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
    m_impl->scanning = false;
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
        return nullptr;
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
        mutableInfo.lastUsed = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        mutableInfo.useCount++;
    }
    
    return plugin;
}

void PluginManager::markAsFavorite(const std::string& path, bool favorite) {
    std::lock_guard<std::mutex> lock(m_impl->pluginsMutex);
    
    auto it = m_impl->discoveredPlugins.find(path);
    if (it != m_impl->discoveredPlugins.end()) {
        it->second.favorite = favorite;
    }
}

void PluginManager::setRating(const std::string& path, int rating) {
    std::lock_guard<std::mutex> lock(m_impl->pluginsMutex);
    
    auto it = m_impl->discoveredPlugins.find(path);
    if (it != m_impl->discoveredPlugins.end()) {
        it->second.userRating = std::max(0, std::min(5, rating)); // Clamp to 0-5
    }
}

void PluginManager::addTag(const std::string& path, const std::string& tag) {
    std::lock_guard<std::mutex> lock(m_impl->pluginsMutex);
    
    auto it = m_impl->discoveredPlugins.find(path);
    if (it != m_impl->discoveredPlugins.end()) {
        // Add tag if it doesn't exist
        auto& tags = it->second.tags;
        if (std::find(tags.begin(), tags.end(), tag) == tags.end()) {
            tags.push_back(tag);
        }
    }
}

void PluginManager::removeTag(const std::string& path, const std::string& tag) {
    std::lock_guard<std::mutex> lock(m_impl->pluginsMutex);
    
    auto it = m_impl->discoveredPlugins.find(path);
    if (it != m_impl->discoveredPlugins.end()) {
        // Remove tag if it exists
        auto& tags = it->second.tags;
        tags.erase(std::remove(tags.begin(), tags.end(), tag), tags.end());
    }
}

void PluginManager::updateLastUsed(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_impl->pluginsMutex);
    
    auto it = m_impl->discoveredPlugins.find(path);
    if (it != m_impl->discoveredPlugins.end()) {
        it->second.lastUsed = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    }
}

void PluginManager::incrementUseCount(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_impl->pluginsMutex);
    
    auto it = m_impl->discoveredPlugins.find(path);
    if (it != m_impl->discoveredPlugins.end()) {
        it->second.useCount++;
    }
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