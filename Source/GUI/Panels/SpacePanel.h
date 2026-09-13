#pragma once

#include "../PanelBase.h"

namespace aeriform
{
/** SPACE: chorus, tempo-synced delay, reverb. Full mode (SPACE page) uses three wide columns. */
class SpacePanel : public ParamPanel
{
public:
    SpacePanel (AeriformProcessor&, bool full);
    void resized() override;

private:
    bool full;
    struct TabButton : public juce::Button
    {
        TabButton (const juce::String& name = {}) : juce::Button (name) {}
        void paintButton (juce::Graphics& g, bool isMouseOver, bool) override
        {
            auto bounds = getLocalBounds().toFloat();
            const bool active = getToggleState();
            if (active)
            {
                g.setColour (theme::tealDim.withAlpha (0.45f));
                g.fillRoundedRectangle (bounds.reduced (1.0f), 3.0f);
                g.setColour (theme::teal);
                g.fillRect (bounds.getX() + 5.0f, bounds.getBottom() - 2.0f, bounds.getWidth() - 10.0f, 2.0f);
            }
            else if (isMouseOver)
            {
                g.setColour (juce::Colours::white.withAlpha (0.05f));
                g.fillRoundedRectangle (bounds.reduced (1.0f), 3.0f);
            }
            g.setColour (active ? theme::textPrimary : (isMouseOver ? theme::textPrimary : theme::textSecondary));
            g.setFont (theme::titleFont (11.0f));
            g.drawText (getButtonText(), bounds, juce::Justification::centred);
        }
    };
    std::array<TabButton, 3> tabs;
    std::array<TabButton, 5> chorusTypeButtons;
    std::unique_ptr<juce::ParameterAttachment> chorusTypeAttachment;

    int selected = 0;
    void selectEffect (int);
    juce::Label *chorusCaption, *delayCaption, *reverbCaption;
    Knob *chorusMix, *chorusRate, *chorusDepth, *chorusWidth;
    Knob *delayMix, *delayTime, *delayFeedback, *delayTone;
    Toggle *delaySync, *delayPingPong;
    ChoiceBox* delayDiv;
    ChoiceBox* reverbType;
    Knob *revMix, *revSize, *revDecay, *revDamp, *revPre, *revWidth, *revMod;
};
} // namespace aeriform
