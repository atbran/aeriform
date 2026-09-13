#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Plugin/PluginProcessor.h"
#include "Theme.h"

namespace aeriform
{

class PresetBrowserComponent : public juce::Component,
                               public juce::TableListBoxModel
{
public:
    explicit PresetBrowserComponent (AeriformProcessor& processor);
    ~PresetBrowserComponent() override;

    void refresh();
    void refreshCategories();
    void applyFilter();

    void resized() override;
    void paint (juce::Graphics& g) override;
    bool keyPressed (const juce::KeyPress& key) override;
    void visibilityChanged() override;

    // TableListBoxModel overrides
    int getNumRows() override;
    void paintRowBackground (juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell (juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
    void cellClicked (int rowNumber, int columnId, const juce::MouseEvent& e) override;
    void selectedRowsChanged (int lastRowSelected) override;
    void sortOrderChanged (int newSortColumnId, bool isForwards) override;

    std::function<void()> onClose;
    std::function<void()> onPresetLoaded;
    std::function<void()> onFavoriteToggled;

    // Accessors and action helpers for testing / inspection
    juce::TableListBox& getTable() noexcept { return table; }
    juce::TextEditor& getSearchBox() noexcept { return searchBox; }
    juce::ComboBox& getCategoryBox() noexcept { return categoryBox; }
    juce::ToggleButton& getFavToggle() noexcept { return favToggle; }
    int getFilteredCount() const noexcept { return (int) filteredItems.size(); }
    void setSearchQuery (const juce::String& query);
    void setCategoryFilter (const juce::String& category);
    void toggleStarForRow (int rowNumber);
    void loadPresetForRow (int rowNumber);

private:
    struct RowItem
    {
        int originalIndex = 0;
        juce::String name;
        juce::String category;
        bool isFavorite = false;
        bool isFactory = true;
        juce::String stableId;
    };

    AeriformProcessor& processor;

    // Header / Toolbar
    juce::Label titleLabel;
    juce::TextEditor searchBox;
    juce::ComboBox categoryBox;
    juce::ToggleButton favToggle;
    juce::Label countLabel;
    juce::TextButton closeButton;

    // Table
    juce::TableListBox table;
    std::vector<RowItem> filteredItems;

    int sortColumnId = 0;
    bool sortAscending = true;
    bool isSyncingSelection = false;

    void syncSelectedRow();
    void showRowContextMenu (int rowNumber);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetBrowserComponent)
};

} // namespace aeriform
