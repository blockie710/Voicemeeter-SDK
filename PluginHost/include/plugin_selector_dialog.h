#ifndef PLUGIN_SELECTOR_DIALOG_H
#define PLUGIN_SELECTOR_DIALOG_H

#include "plugin_manager.h"
#include <memory>
#include <functional>
#include <string>
#include <vector>

// Forward declarations for GTK+ types to avoid including GTK headers in the interface
typedef struct _GtkWidget GtkWidget;
typedef struct _GtkListStore GtkListStore;
typedef struct _GtkTreeModel GtkTreeModel;

/**
 * PluginSelectorDialog - A dialog for selecting, filtering, and managing plugins
 * 
 * Features:
 * - Search and filter plugins by name, vendor, format, etc.
 * - Sort plugins by different criteria
 * - View plugin details and metadata
 * - Rate plugins and mark as favorites
 * - Add custom tags to plugins for easier organization
 * - Select plugins to add to the processing chain
 */
class PluginSelectorDialog {
public:
    // Constructor takes a plugin manager instance
    PluginSelectorDialog(PluginManager& pluginManager);
    ~PluginSelectorDialog();

    // Callback for selected plugin(s)
    using PluginSelectedCallback = std::function<void(const std::vector<std::string>& selectedPluginPaths)>;
    
    // Show the dialog modally
    void showDialog(GtkWidget* parentWindow, PluginSelectedCallback callback);
    
    // Close the dialog
    void closeDialog();
    
    // Refresh the plugin list with current filters
    void refreshPluginList();
    
    // Update progress during scanning
    void updateScanProgress(float progress, const std::string& currentPath);
    
private:
    // Implementation class to hide GTK+ details
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    
    // Reference to the plugin manager
    PluginManager& m_pluginManager;
    
    // Selected plugin callback
    PluginSelectedCallback m_callback;
    
    // Initialize UI components
    void initializeUI();
    
    // Setup signal handlers
    void setupSignals();
    
    // UI Actions
    void onSearch(const std::string& text);
    void onSortChanged(PluginManager::SortCriteria criteria, bool ascending);
    void onFilterChanged(const PluginManager::FilterCriteria& criteria);
    void onPluginSelected(const std::string& path);
    void onAddPluginClicked();
    void onPluginDoubleClicked(const std::string& path);
    void onScanForPluginsClicked();
    void onCancelScanClicked();
    void onAddPluginPathClicked();
    void onRemovePluginPathClicked();
    void onRefreshPluginsClicked();
    void onPluginInfoClicked(const std::string& path);
    void onFavoriteToggled(const std::string& path, bool favorite);
    void onRatingChanged(const std::string& path, int rating);
    void onAddTagClicked(const std::string& path);
    void onRemoveTagClicked(const std::string& path, const std::string& tag);
};

#endif // PLUGIN_SELECTOR_DIALOG_H