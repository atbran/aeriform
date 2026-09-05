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
    juce::Label *chorusCaption, *delayCaption, *reverbCaption;
    Knob *chorusMix, *chorusRate, *chorusDepth, *chorusWidth;
    Knob *delayMix, *delayTime, *delayFeedback, *delayTone;
    Toggle *delaySync, *delayPingPong;
    ChoiceBox* delayDiv;
    Knob *revMix, *revSize, *revDecay, *revDamp, *revPre, *revWidth, *revMod;
};
} // namespace aeriform
