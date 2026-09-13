#include "PresetBrowser.h"
#include <algorithm>

namespace aeriform
{
using namespace theme;

PresetBrowserComponent::PresetBrowserComponent (AeriformProcessor& p)
    : processor (p)
{
    // Title
    titleLabel.setText ("PRESETS", juce::dontSendNotification);
    titleLabel.setFont (titleFont (14.0f));
    titleLabel.setColour (juce::Label::textColourId, copperBright);
    addAndMakeVisible (titleLabel);

    // Search box
    searchBox.setTextToShowWhenEmpty ("Search presets...", textDim);
    searchBox.setTooltip ("Filter presets by name or category");
    searchBox.onTextChange = [this] { applyFilter(); };
    addAndMakeVisible (searchBox);

    // Category ComboBox (Editable dropdown)
    categoryBox.setTooltip ("Select a category to filter or type to search");
    categoryBox.setTextWhenNothingSelected ("All Categories");
    categoryBox.setTextWhenNoChoicesAvailable ("All Categories");
    categoryBox.onChange = [this] { applyFilter(); };
    addAndMakeVisible (categoryBox);
    refreshCategories();

    // Favorites only toggle
    favToggle.setButtonText (juce::CharPointer_UTF8 ("\xe2\x98\x85 Starred")); // ★ Starred
    favToggle.setTooltip ("Show only starred presets");
    favToggle.onClick = [this] { applyFilter(); };
    addAndMakeVisible (favToggle);

    // Count label
    countLabel.setFont (font (10.5f));
    countLabel.setColour (juce::Label::textColourId, textSecondary);
    countLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (countLabel);

    // Close button
    closeButton.setButtonText (juce::CharPointer_UTF8 ("\xc3\x97")); // ×
    closeButton.setTooltip ("Close preset browser (Esc)");
    closeButton.onClick = [this] {
        setVisible (false);
        if (onClose) onClose();
    };
    addAndMakeVisible (closeButton);

    // Table List Box
    table.getHeader().addColumn (juce::CharPointer_UTF8 ("\xe2\x98\x85"), 1, 38, 32, 48,
                                 juce::TableHeaderComponent::visible | juce::TableHeaderComponent::sortable);
    table.getHeader().addColumn ("NAME", 2, 340, 150, 700,
                                 juce::TableHeaderComponent::visible | juce::TableHeaderComponent::sortable | juce::TableHeaderComponent::resizable);
    table.getHeader().addColumn ("CATEGORY", 3, 200, 100, 400,
                                 juce::TableHeaderComponent::visible | juce::TableHeaderComponent::sortable | juce::TableHeaderComponent::resizable);
    table.setModel (this);
    table.setColour (juce::ListBox::backgroundColourId, inset.withAlpha (0.45f));
    table.setColour (juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
    table.setOutlineThickness (0);
    table.setMultipleSelectionEnabled (false);
    table.setRowHeight (28);
    addAndMakeVisible (table);

    setWantsKeyboardFocus (true);
    refresh();
}

PresetBrowserComponent::~PresetBrowserComponent()
{
    table.setModel (nullptr);
}

void PresetBrowserComponent::refreshCategories()
{
    const auto oldText = categoryBox.getText();
    categoryBox.clear (juce::dontSendNotification);
    categoryBox.addItem ("All Categories", 1);

    const auto cats = processor.getPresetManager().getCategories();
    for (int i = 0; i < cats.size(); ++i)
        categoryBox.addItem (cats[i], i + 2);

    categoryBox.setEditableText (true);

    if (oldText.isNotEmpty() && oldText != "All Categories")
        categoryBox.setText (oldText, juce::dontSendNotification);
    else
        categoryBox.setSelectedId (1, juce::dontSendNotification);
}

void PresetBrowserComponent::setSearchQuery (const juce::String& query)
{
    searchBox.setText (query, false);
    applyFilter();
}

void PresetBrowserComponent::setCategoryFilter (const juce::String& category)
{
    categoryBox.setText (category, juce::dontSendNotification);
    applyFilter();
}

void PresetBrowserComponent::refresh()
{
    refreshCategories();
    applyFilter();
}

void PresetBrowserComponent::applyFilter()
{
    filteredItems.clear();
    auto& pm = processor.getPresetManager();
    const auto& entries = pm.getEntries();
    const auto query = searchBox.getText().trim();
    const auto categoryQuery = categoryBox.getText().trim();
    const bool favoritesOnly = favToggle.getToggleState();

    for (int i = 0; i < (int) entries.size(); ++i)
    {
        const auto& e = entries[(size_t) i];
        const bool isFav = pm.isFavorite (e.stableId);

        if (favoritesOnly && ! isFav)
            continue;

        if (categoryQuery.isNotEmpty() && categoryQuery != "All Categories")
        {
            if (! e.category.equalsIgnoreCase (categoryQuery) && ! e.category.containsIgnoreCase (categoryQuery))
                continue;
        }

        if (query.isNotEmpty())
        {
            if (! e.name.containsIgnoreCase (query) && ! e.category.containsIgnoreCase (query))
                continue;
        }

        filteredItems.push_back ({ i, e.name, e.category, isFav, e.isFactory, e.stableId });
    }

    // Sort items according to active sort column
    if (sortColumnId == 1) // Star
    {
        std::stable_sort (filteredItems.begin(), filteredItems.end(), [this] (const RowItem& a, const RowItem& b) {
            if (a.isFavorite != b.isFavorite)
                return sortAscending ? (a.isFavorite > b.isFavorite) : (a.isFavorite < b.isFavorite);
            return a.name.compareIgnoreCase (b.name) < 0;
        });
    }
    else if (sortColumnId == 2) // Name
    {
        std::stable_sort (filteredItems.begin(), filteredItems.end(), [this] (const RowItem& a, const RowItem& b) {
            int cmp = a.name.compareIgnoreCase (b.name);
            return sortAscending ? (cmp < 0) : (cmp > 0);
        });
    }
    else if (sortColumnId == 3) // Category
    {
        std::stable_sort (filteredItems.begin(), filteredItems.end(), [this] (const RowItem& a, const RowItem& b) {
            int cmp = a.category.compareIgnoreCase (b.category);
            if (cmp != 0)
                return sortAscending ? (cmp < 0) : (cmp > 0);
            return a.name.compareIgnoreCase (b.name) < 0;
        });
    }

    countLabel.setText ("Showing " + juce::String ((int) filteredItems.size()) + " of " + juce::String ((int) entries.size()) + " presets", juce::dontSendNotification);

    syncSelectedRow();
    table.updateContent();
    table.repaint();
}

void PresetBrowserComponent::syncSelectedRow()
{
    const int currentIdx = processor.getPresetManager().getCurrentIndex();
    isSyncingSelection = true;
    for (int r = 0; r < (int) filteredItems.size(); ++r)
    {
        if (filteredItems[(size_t) r].originalIndex == currentIdx)
        {
            table.selectRow (r, false, true);
            isSyncingSelection = false;
            return;
        }
    }
    table.deselectAllRows();
    isSyncingSelection = false;
}

int PresetBrowserComponent::getNumRows()
{
    return (int) filteredItems.size();
}

void PresetBrowserComponent::paintRowBackground (juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
    const bool isCurrent = (rowNumber >= 0 && rowNumber < (int) filteredItems.size()
                            && filteredItems[(size_t) rowNumber].originalIndex == processor.getPresetManager().getCurrentIndex());

    if (rowIsSelected)
    {
        g.fillAll (copper.withAlpha (0.24f));
        g.setColour (copperBright);
        g.fillRect (0, 0, 3, height);
    }
    else if (isCurrent)
    {
        g.fillAll (brass.withAlpha (0.12f));
        g.setColour (amber.withAlpha (0.7f));
        g.fillRect (0, 0, 2, height);
    }
    else if (rowNumber % 2 == 1)
    {
        g.fillAll (inset.withAlpha (0.35f));
    }

    g.setColour (panelBorder.withAlpha (0.25f));
    g.drawHorizontalLine (height - 1, 0.0f, (float) width);
}

void PresetBrowserComponent::paintCell (juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= (int) filteredItems.size()) return;
    const auto& item = filteredItems[(size_t) rowNumber];
    const bool isCurrent = (item.originalIndex == processor.getPresetManager().getCurrentIndex());

    if (columnId == 1) // Star
    {
        g.setFont (font (13.5f, true));
        if (item.isFavorite)
        {
            g.setColour (amber);
            g.drawText (juce::CharPointer_UTF8 ("\xe2\x98\x85"), 0, 0, width, height, juce::Justification::centred); // ★
        }
        else
        {
            g.setColour (textDim.withAlpha (0.35f));
            g.drawText (juce::CharPointer_UTF8 ("\xe2\x98\x86"), 0, 0, width, height, juce::Justification::centred); // ☆
        }
    }
    else if (columnId == 2) // Name
    {
        g.setFont (font (12.5f, isCurrent));
        if (isCurrent)
            g.setColour (copperBright);
        else if (rowIsSelected)
            g.setColour (textPrimary);
        else
            g.setColour (textSecondary);

        juce::String displayName = item.name;
        if (! item.isFactory)
            displayName += " [User]";

        g.drawText (displayName, 8, 0, width - 12, height, juce::Justification::centredLeft, true);
    }
    else if (columnId == 3) // Category
    {
        g.setFont (font (11.0f, false));
        g.setColour (brass.withAlpha (0.85f));
        g.drawText (item.category, 6, 0, width - 10, height, juce::Justification::centredLeft, true);
    }
}

void PresetBrowserComponent::toggleStarForRow (int rowNumber)
{
    if (rowNumber < 0 || rowNumber >= (int) filteredItems.size()) return;
    auto& item = filteredItems[(size_t) rowNumber];
    processor.getPatchTools().toggleFavorite (item.stableId);
    item.isFavorite = processor.getPresetManager().isFavorite (item.stableId);
    if (favToggle.getToggleState())
    {
        applyFilter();
    }
    else
    {
        table.repaintRow (rowNumber);
    }
    if (onFavoriteToggled) onFavoriteToggled();
}

void PresetBrowserComponent::loadPresetForRow (int rowNumber)
{
    if (rowNumber < 0 || rowNumber >= (int) filteredItems.size()) return;
    auto& item = filteredItems[(size_t) rowNumber];
    if (item.originalIndex != processor.getPresetManager().getCurrentIndex())
    {
        processor.getPatchTools().perform ("Load preset", [&] {
            processor.getPresetManager().loadPreset (item.originalIndex);
        });
        if (onPresetLoaded) onPresetLoaded();
        table.repaint();
    }
}

void PresetBrowserComponent::cellClicked (int rowNumber, int columnId, const juce::MouseEvent& e)
{
    if (rowNumber < 0 || rowNumber >= (int) filteredItems.size()) return;

    if (columnId == 1) // Star column clicked
    {
        toggleStarForRow (rowNumber);
        return;
    }

    if (e.mods.isPopupMenu())
    {
        showRowContextMenu (rowNumber);
        return;
    }

    // Left click on row: load preset
    loadPresetForRow (rowNumber);
}

void PresetBrowserComponent::selectedRowsChanged (int lastRowSelected)
{
    if (isSyncingSelection) return;
    if (lastRowSelected >= 0 && lastRowSelected < (int) filteredItems.size())
    {
        const int realIndex = filteredItems[(size_t) lastRowSelected].originalIndex;
        if (realIndex != processor.getPresetManager().getCurrentIndex())
        {
            processor.getPatchTools().perform ("Load preset", [&] {
                processor.getPresetManager().loadPreset (realIndex);
            });
            if (onPresetLoaded) onPresetLoaded();
            table.repaint();
        }
    }
}

void PresetBrowserComponent::sortOrderChanged (int newSortColumnId, bool isForwards)
{
    sortColumnId = newSortColumnId;
    sortAscending = isForwards;
    applyFilter();
}

void PresetBrowserComponent::showRowContextMenu (int rowNumber)
{
    if (rowNumber < 0 || rowNumber >= (int) filteredItems.size()) return;
    const auto item = filteredItems[(size_t) rowNumber];

    juce::PopupMenu menu;
    menu.addItem (1, item.isFavorite ? "Remove from Favorites" : "Add to Favorites");
    menu.addSeparator();
    menu.addItem (2, "Show in File Explorer");
    menu.addItem (3, "Delete Preset", ! item.isFactory);

    juce::Component::SafePointer<PresetBrowserComponent> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options(), [safe, item] (int result)
    {
        if (safe == nullptr || result == 0) return;
        auto& pm = safe->processor.getPresetManager();
        switch (result)
        {
            case 1:
                safe->processor.getPatchTools().toggleFavorite (item.stableId);
                safe->refresh();
                if (safe->onFavoriteToggled) safe->onFavoriteToggled();
                break;
            case 2:
                if (! item.isFactory && item.originalIndex < (int) pm.getEntries().size())
                {
                    pm.getEntries()[(size_t) item.originalIndex].file.revealToUser();
                }
                else
                {
                    PresetManager::getUserPresetDirectory().createDirectory();
                    PresetManager::getUserPresetDirectory().revealToUser();
                }
                break;
            case 3:
                if (! item.isFactory)
                {
                    pm.deleteUserPreset (item.originalIndex);
                    safe->refresh();
                    if (safe->onPresetLoaded) safe->onPresetLoaded();
                }
                break;
            default:
                break;
        }
    });
}

void PresetBrowserComponent::resized()
{
    auto r = getLocalBounds().reduced (8);
    auto toolbar = r.removeFromTop (30);

    closeButton.setBounds (toolbar.removeFromRight (28).reduced (0, 1));
    toolbar.removeFromRight (10);

    countLabel.setBounds (toolbar.removeFromRight (180));
    toolbar.removeFromRight (8);

    favToggle.setBounds (toolbar.removeFromRight (96));
    toolbar.removeFromRight (8);

    titleLabel.setBounds (toolbar.removeFromLeft (90));
    toolbar.removeFromLeft (4);

    searchBox.setBounds (toolbar.removeFromLeft (190).reduced (0, 1));
    toolbar.removeFromLeft (8);

    categoryBox.setBounds (toolbar.removeFromLeft (170).reduced (0, 1));

    r.removeFromTop (6);
    table.setBounds (r);
}

void PresetBrowserComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Background panel
    g.setColour (panel);
    g.fillRoundedRectangle (bounds, cornerRadius);

    // Subtle dark header backing
    auto headerArea = bounds.withHeight (42.0f);
    g.setColour (inset.withAlpha (0.7f));
    g.fillRoundedRectangle (headerArea, cornerRadius);
    g.fillRect (headerArea.withTrimmedTop (headerArea.getHeight() - cornerRadius));

    // Outer border
    g.setColour (panelBorder);
    g.drawRoundedRectangle (bounds.reduced (0.5f), cornerRadius, 1.0f);

    // Header divider line
    g.setColour (panelBorder);
    g.drawHorizontalLine (42, 0.0f, bounds.getWidth());
}

bool PresetBrowserComponent::keyPressed (const juce::KeyPress& key)
{
    if (key.isKeyCode (juce::KeyPress::escapeKey))
    {
        setVisible (false);
        if (onClose) onClose();
        return true;
    }
    return false;
}

void PresetBrowserComponent::visibilityChanged()
{
    if (isVisible())
    {
        refresh();
        table.grabKeyboardFocus();
    }
}

} // namespace aeriform
