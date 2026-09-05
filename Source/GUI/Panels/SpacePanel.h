#pragma once

#include "../PanelBase.h"

namespace aeriform
{
/** SPACE: chorus, tempo-synced delay, reverb. */
class SpacePanel : public ParamPanel
{
public:
    explicit SpacePanel (AeriformProcessor&);
    void resized() override;

private:
    juce::Label *chorusCaption, *delayCaption, *reverbCaption;
    Knob *chorusMix, *chorusRate, *chorusDepth, *chorusWidth;
    Knob *delayMix, *delayTime, *delayFeedback, *delayTone;
    Toggle *delaySync, *delayPingPong;
    ChoiceBox* delayDiv;
    Knob *revMix, *revSize, *revDecay, *revDamp, *revPre, *revWidth, *revMod;
};
} // namespace aeriform
