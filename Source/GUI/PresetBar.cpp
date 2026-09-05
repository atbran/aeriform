#include "PresetBar.h"
#include "Theme.h"

namespace aeriform
{
using namespace theme;

PresetBar::PresetBar (AeriformProcessor& p) : processor (p)
{
    for (auto* b : { &prevButton, &nextButton, &nameButton, &saveButton, &saveAsButton, &initButton })
        addAndMakeVisible (b);

    prevButton.setTooltip ("Previous preset");
    nextButton.setTooltip ("Next preset");
    nameButton.setTooltip ("Click to browse presets");
    saveButton.setTooltip ("Overwrite the current user preset (or save as new if it is a factory preset)");
    saveAsButton.setTooltip ("Save the current sound as a new user preset");
    initButton.setTooltip ("Reset every parameter to the neutral initialisation patch");

    prevButton.onClick = [this] { processor.getPresetManager().loadPrevious(); };
    nextButton.onClick = [this] { processor.getPresetManager().loadNext(); };
    nameButton.onClick = [this] { showPresetMenu(); };
    saveButton.onClick = [this] { processor.getPresetManager().saveCurrent(); refresh(); };
    saveAsButton.onClick = [this] { showSaveAsDialog(); };
    initButton.onClick = [this] { processor.getPresetManager().loadInit(); };
    refresh();
}

PresetBar::~PresetBar()
{
    saveDialog.reset();
    chooser.reset();
}

void PresetBar::refresh()
{
    auto& pm = processor.getPresetManager();
    nameButton.setButtonText ((pm.isDirty() ? "* " : "") + pm.getCurrentName());
    repaint();
}

void PresetBar::resized()
{
    auto r = getLocalBounds();
    const int h = r.getHeight();
    prevButton.setBounds (r.removeFromLeft (h));
    r.removeFromLeft (3);
    nextButton.setBounds (r.removeFromLeft (h));
    r.removeFromLeft (6);
    initButton.setBounds (r.removeFromRight (48));
    r.removeFromRight (4);
    saveAsButton.setBounds (r.removeFromRight (70));
    r.removeFromRight (4);
    saveButton.setBounds (r.removeFromRight (54));
    r.removeFromRight (6);
    nameButton.setBounds (r);
}

void PresetBar::paintOverChildren (juce::Graphics& g)
{
    // category tag drawn over the right side of the name button
    auto& pm = processor.getPresetManager();
    g.setFont (font (10.0f, true));
    g.setColour (brass.withAlpha (0.8f));
    g.drawText (pm.getCurrentCategory().toUpperCase(), nameButton.getBounds().reduced (10, 0), juce::Justification::centredRight);
}

void PresetBar::showPresetMenu()
{
    auto& pm = processor.getPresetManager();
    pm.rescan();
    const auto& entries = pm.getEntries();

    juce::PopupMenu menu;
    juce::StringArray categories;
    for (const auto& e : entries)
        if (! categories.contains (e.category)) categories.add (e.category);

    for (const auto& cat : categories)
    {
        menu.addSectionHeader (cat);
        for (int i = 0; i < (int) entries.size(); ++i)
            if (entries[(size_t) i].category == cat)
                menu.addItem (1000 + i, entries[(size_t) i].name, true, i == pm.getCurrentIndex());
    }
    menu.addSeparator();
    menu.addItem (1, "Load preset file...");
    menu.addItem (2, "Export preset file...");
    menu.addItem (3, "Show user preset folder");
    const bool canDelete = pm.getCurrentIndex() >= 0 && pm.getCurrentIndex() < (int) entries.size() && ! entries[(size_t) pm.getCurrentIndex()].isFactory;
    menu.addItem (4, "Delete user preset \"" + pm.getCurrentName() + "\"", canDelete);

    juce::Component::SafePointer<PresetBar> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&nameButton).withMinimumWidth (240), [safe] (int result)
    {
        if (safe == nullptr || result == 0) return;
        auto& manager = safe->processor.getPresetManager();
        if (result >= 1000) { manager.loadPreset (result - 1000); return; }
        switch (result)
        {
            case 1: safe->importPreset(); break;
            case 2: safe->exportPreset(); break;
            case 3:
                PresetManager::getUserPresetDirectory().createDirectory();
                PresetManager::getUserPresetDirectory().revealToUser();
                break;
            case 4: manager.deleteUserPreset (manager.getCurrentIndex()); manager.loadInit(); break;
            default: break;
        }
    });
}

void PresetBar::showSaveAsDialog()
{
    auto& pm = processor.getPresetManager();
    saveDialog = std::make_unique<juce::AlertWindow> ("Save preset", "Name and category for the new user preset:", juce::MessageBoxIconType::NoIcon);
    saveDialog->addTextEditor ("name", pm.getCurrentName(), "Name");
    saveDialog->addTextEditor ("category", pm.getCurrentCategory() == "Init" ? juce::String ("User") : pm.getCurrentCategory(), "Category");
    saveDialog->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    saveDialog->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    saveDialog->setLookAndFeel (&getLookAndFeel());

    juce::Component::SafePointer<PresetBar> safe (this);
    saveDialog->enterModalState (true, juce::ModalCallbackFunction::create ([safe] (int result)
    {
        if (safe == nullptr || safe->saveDialog == nullptr) return;
        const auto name = safe->saveDialog->getTextEditorContents ("name").trim();
        const auto category = safe->saveDialog->getTextEditorContents ("category").trim();
        auto dialog = std::move (safe->saveDialog);
        dialog->setLookAndFeel (nullptr);
        if (result == 1 && name.isNotEmpty())
            safe->processor.getPresetManager().saveAs (name, category.isEmpty() ? "User" : category);
    }), false);
}

void PresetBar::importPreset()
{
    chooser = std::make_unique<juce::FileChooser> ("Load AERIFORM preset", PresetManager::getUserPresetDirectory(),
                                                   "*" + juce::String (PresetManager::kFileExtension) + ";*.xml");
    juce::Component::SafePointer<PresetBar> safe (this);
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [safe] (const juce::FileChooser& fc)
    {
        if (safe == nullptr) return;
        const auto file = fc.getResult();
        if (file.existsAsFile())
            safe->processor.getPresetManager().loadFromFile (file);
    });
}

void PresetBar::exportPreset()
{
    auto& pm = processor.getPresetManager();
    const auto suggested = PresetManager::getUserPresetDirectory().getChildFile (juce::File::createLegalFileName (pm.getCurrentName()) + PresetManager::kFileExtension);
    chooser = std::make_unique<juce::FileChooser> ("Export AERIFORM preset", suggested, "*" + juce::String (PresetManager::kFileExtension));
    juce::Component::SafePointer<PresetBar> safe (this);
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
                          [safe] (const juce::FileChooser& fc)
    {
        if (safe == nullptr) return;
        auto file = fc.getResult();
        if (file == juce::File()) return;
        if (! file.hasFileExtension (PresetManager::kFileExtension)) file = file.withFileExtension (PresetManager::kFileExtension);
        auto& manager = safe->processor.getPresetManager();
        manager.saveToFile (file, file.getFileNameWithoutExtension(), manager.getCurrentCategory());
    });
}
} // namespace aeriform
