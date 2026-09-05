#pragma once

#include "../PanelBase.h"

namespace aeriform
{
/** BREATH: exciter, breath envelope and articulation. */
class BreathPanel : public ParamPanel
{
public:
    explicit BreathPanel (AeriformProcessor&);
    void resized() override;

private:
    Knob *noise, *color, *pressure, *reed, *pluck, *input;
    Knob *pluckLen, *lowpass, *highpass, *turbulence, *velocity, *keyTrack;
    Knob *attack, *decay, *sustain, *release, *velPress, *pressBright;
    Knob *transient, *releaseNoise, *breathRandom, *flowPitch, *instability, *variation, *coupling;
    juce::Label *excCaption, *envCaption, *artCaption;
};
} // namespace aeriform
