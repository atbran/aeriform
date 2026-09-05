#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "../GUI/LookAndFeel.h"
#include "../GUI/PresetBar.h"
#include "../GUI/Visualizer.h"
#include "../GUI/Panels/BreathPanel.h"
#include "../GUI/Panels/ResonatorPanel.h"
#include "../GUI/Panels/MotionPanel.h"
#include "../GUI/Panels/SpacePanel.h"
#include "../GUI/Panels/MasterPanel.h"

/**
    AERIFORM editor: five conceptual regions around a central airflow visualiser.
    The whole interface is laid out at a fixed logical size and scaled uniformly
    (75 % .. 200 %), so it stays crisp on high-DPI displays.
*/
class AeriformEditor : public juce::AudioProcessorEditor,
                       private juce::Timer
{
public:
    explicit AeriformEditor (AeriformProcessor&);
    ~AeriformEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

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

    // regions
    aeriform::Visualizer visualizer;
    aeriform::BreathPanel breath;
    aeriform::ResonatorPanel resonator;
    aeriform::MotionPanel motion;
    aeriform::SpacePanel space;
    aeriform::MasterPanel master;

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
