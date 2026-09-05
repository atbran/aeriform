#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "../GUI/LookAndFeel.h"
#include "../GUI/PresetBar.h"
#include "../GUI/Pages.h"

/**
    AERIFORM editor: a top bar (title, presets, status, size), a page strip and
    five pages (MAIN, EXCITERS, NETWORK, MOTION, SPACE). The whole interface is
    laid out at a fixed logical size and scaled uniformly (75 % .. 200 %).
*/
class AeriformEditor : public juce::AudioProcessorEditor,
                       private juce::Timer
{
public:
    explicit AeriformEditor (AeriformProcessor&);
    ~AeriformEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    /** Shows a page (0 = MAIN .. 4 = SPACE). */
    void showPage (int index);
    int getCurrentPage() const noexcept { return currentPage; }

private:
    class Content : public juce::Component
    {
    public:
        explicit Content (AeriformEditor& e) : editor (e) {}
        void paint (juce::Graphics&) override;
        void resized() override;
    private:
        AeriformEditor& editor;
    };

    AeriformProcessor& processor;
    aeriform::AeriformLookAndFeel lookAndFeel;
    juce::TooltipWindow tooltips;
    Content content;

    // top bar
    juce::Label titleLabel, subtitleLabel, statusLabel;
    aeriform::PresetBar presetBar;
    juce::TextButton scaleButton { "100 %" };
    aeriform::PageTabs tabs;

    // pages
    std::array<std::unique_ptr<aeriform::Page>, 5> pages;
    int currentPage = 0;

    float scale = 1.0f;
    std::array<float, (size_t) aeriform::ModDest::Count> liveMod {};
    int midiActivityCounter = 0;
    bool presetDirtyFlag = false;

    void timerCallback() override;
    void applyScale (float newScale);
    void showScaleMenu();
    void layoutContent();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AeriformEditor)
};
