#pragma once

#include "../PanelBase.h"

namespace aeriform
{
/** Shared main-page resonator controls; body EQ remains on Network. */
class ResonatorPanel : public ParamPanel
{
public:
    ResonatorPanel (AeriformProcessor&, int slot = 0);
    void resized() override;

private:
    Knob *coarse, *fine, *length, *keyTrack;
    ChoiceBox* mode;
    Knob *feedback, *damping, *brightness, *dispersion;
    Knob *shape, *reflection, *saturation;
    juce::Label *tuneCaption, *tubeCaption;
};
} // namespace aeriform
