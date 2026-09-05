#pragma once

#include "../PanelBase.h"

namespace aeriform
{
/** RESONATOR: tuning, tube character, feedback, body. */
class ResonatorPanel : public ParamPanel
{
public:
    explicit ResonatorPanel (AeriformProcessor&);
    void resized() override;

private:
    Knob *coarse, *fine, *length, *keyTrack;
    ChoiceBox* mode;
    Knob *feedback, *damping, *brightness, *dispersion;
    Knob *shape, *reflection, *saturation;
    Knob *bodyFreq, *bodyRes, *bodyMix, *bodyTrack;
    juce::Label *tuneCaption, *tubeCaption, *bodyCaption;
};
} // namespace aeriform
