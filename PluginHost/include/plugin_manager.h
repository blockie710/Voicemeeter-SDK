#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include "plugin_interface.h"
#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

/**
 * PluginManager - Handles plugin discovery, sorting, and filtering
 * 
 * This class provides a centralized way to:
 * - Auto-scan for plugins in standard locations
 * - Sort plugins by various criteria
 * - Search and filter plugins
 * - Load plugins and manage their lifecycle
 */
class PluginManager {
public:
    // Plugin metadata for display and filtering
    struct PluginInfo {
        std::string path;           // File path to the plugin
        std::string name;           // Plugin name
        std::string vendor;         // Plugin vendor
        std::string version;        // Plugin version
        PluginFormat format;        // Plugin format type
        std::string category;       // Plugin category (if available)
        std::string description;    // Plugin description (if available)
        std::vector<std::string> tags; // Tags for filtering
        int inputChannels;          // Number of input channels
        int outputChannels;         // Number of output channels
        bool hasEditor;             // Whether the plugin has a UI
        bool favorite;              // User marked as favorite
        int userRating;             // User rating (0-5)
        std::chrono::system_clock::time_point lastUsed; // When the plugin was last used
        int useCount;               // How many times the plugin was used

        // Constructors
        PluginInfo() : format(PluginFormat::UNKNOWN), inputChannels(0), outputChannels(0),
                      hasEditor(false), favorite(false), userRating(0),
                      lastUsed(std::chrono::system_clock::now()), useCount(0) {}
        
        // Create from existing plugin instance
        PluginInfo(const std::shared_ptr<PluginInstance>& plugin);
    };

    // Sort criteria for plugin list
    enum class SortCriteria {
        NAME,           // Sort by name
        VENDOR,         // Sort by vendor
        FORMAT,         // Sort by plugin format
        RATING,         // Sort by user rating
        LAST_USED,      // Sort by last used date
        USE_COUNT,      // Sort by how often used
        CATEGORY,       // Sort by category
        INPUT_CHANNELS, // Sort by number of input channels
        OUTPUT_CHANNELS // Sort by number of output channels
    };

    // Filter criteria for plugin list
    struct FilterCriteria {
        std::string searchText;             // Text to search for in name/vendor/description
        std::set<PluginFormat> formats;     // Formats to include
        std::set<std::string> categories;   // Categories to include
        std::set<std::string> vendors;      // Vendors to include
        std::set<std::string> tags;         // Tags to include
        bool favoritesOnly = false;         // Show only favorites
        int minRating = 0;                  // Minimum rating (0-5)
        int minInputChannels = 0;           // Minimum number of input channels
        int minOutputChannels = 0;          // Minimum number of output channels
        bool hasEditorOnly = false;         // Show only plugins with UI
        
        // Helper to reset all filters
        void reset() {
            searchText.clear();
            formats.clear();
            categories.clear();
            vendors.clear();
            tags.clear();
            favoritesOnly = false;
            minRating = 0;
            minInputChannels = 0;
            minOutputChannels = 0;
            hasEditorOnly = false;
        }
    };

    // Constructor will set up platform-specific default paths
    PluginManager();
    // Virtual destructor for proper cleanup in derived classes
    virtual ~PluginManager();

    // Copy/move semantics
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;
    PluginManager(PluginManager&&) noexcept = default;
    PluginManager& operator=(PluginManager&&) noexcept = default;

    // Path management for plugin scanning
    void addPluginPath(const std::string& path);
    void removePluginPath(const std::string& path);
    std::vector<std::string> getPluginPaths() const;
    
    // Auto-scan for plugins in standard locations and user-defined paths
    void autoScanForPlugins(bool async = true);
    
    // Manual scan for plugins in specific location
    void scanDirectory(const std::string& directory, bool recursive = true, bool async = false);
    
    // Get plugin scanning status
    bool isScanningPlugins() const;
    float getScanningProgress() const;
    void cancelScanning();
    
    // Register callback for scan progress updates
    using ScanProgressCallback = std::function<void(float progress, const std::string& currentPath)>;
    void setScanProgressCallback(ScanProgressCallback callback);
    
    // Plugin discovery and loading
    std::vector<PluginInfo> getAllPlugins() const;
    std::vector<PluginInfo> getFilteredPlugins(const FilterCriteria& criteria) const;
    std::vector<PluginInfo> getSortedPlugins(SortCriteria criteria, bool ascending = true) const;
    std::shared_ptr<PluginInstance> loadPlugin(const std::string& path);
    
    // Plugin metadata management
    void markAsFavorite(const std::string& path, bool favorite);
    void setRating(const std::string& path, int rating);
    void addTag(const std::string& path, const std::string& tag);
    void removeTag(const std::string& path, const std::string& tag);
    void updateLastUsed(const std::string& path);
    void incrementUseCount(const std::string& path);
    
    // Get available filter options from discovered plugins
    std::set<std::string> getAllCategories() const;
    std::set<std::string> getAllVendors() const;
    std::set<std::string> getAllTags() const;
    std::set<PluginFormat> getAllFormats() const;
    
    // Settings persistence
    bool saveSettings(const std::string& path) const;
    bool loadSettings(const std::string& path);
    
    // Events
    using PluginsChangedCallback = std::function<void()>;
    void setPluginsChangedCallback(PluginsChangedCallback callback);

protected:
    // Platform-specific implementations can override these methods
    virtual std::vector<std::string> getDefaultPluginPaths() const;
    virtual bool isValidPluginFile(const std::string& path) const;

private:
    // Internal implementation
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

#endif // PLUGIN_MANAGER_H