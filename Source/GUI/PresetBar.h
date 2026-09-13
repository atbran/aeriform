#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Plugin/PluginProcessor.h"

namespace aeriform
{
/** Preset browser strip: previous / next, name display with menu, save, save as, init, file import / export. */
class PresetBar : public juce::Component
{
public:
    explicit PresetBar (AeriformProcessor&);
    ~PresetBar() override;

    void resized() override;
    void paintOverChildren (juce::Graphics&) override;

    /** Called by the editor when the preset manager reports a change. */
    void refresh();

    std::function<void()> onOpenBrowser;

    class PresetNameButton : public juce::TextButton
    {
    public:
        using juce::TextButton::TextButton;
        using juce::TextButton::clicked;
        std::function<void()> onLeftClick;
        std::function<void()> onRightClick;

        void clicked (const juce::ModifierKeys& modifiers) override
        {
            if (modifiers.isPopupMenu() && onRightClick)
                onRightClick();
            else if (onLeftClick)
                onLeftClick();
        }
    };

private:
    AeriformProcessor& processor;
    juce::TextButton prevButton { "<" }, nextButton { ">" };
    PresetNameButton nameButton;
    juce::TextButton saveButton { "SAVE" }, saveAsButton { "SAVE AS" }, initButton { "INIT" };
    juce::TextButton favoriteButton{"STAR"};
    juce::ToggleButton favoritesOnly{"Favorites"};
    juce::TextEditor search;
    std::unique_ptr<juce::FileChooser> chooser;
    std::unique_ptr<juce::AlertWindow> saveDialog;

    void showPresetMenu();
    void showSaveAsDialog();
    void importPreset();
    void exportPreset();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetBar)
};
} // namespace aeriform
