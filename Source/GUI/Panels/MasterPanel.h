#pragma once

#include "../PanelBase.h"

namespace aeriform
{
/** MASTER: voice mode, polyphony, glide, unison, pitch bend, MPE, output. */
class MasterPanel : public ParamPanel
{
public:
    explicit MasterPanel (AeriformProcessor&);
    void resized() override;

private:
    juce::Label *voiceCaption, *outCaption;
    ChoiceBox* voiceMode;
    Knob *voices, *glide, *unison, *detune, *spread, *bend, *outGain, *outHp;
    Toggle *glideLegato, *mpe, *limiter;
};
} // namespace aeriform
