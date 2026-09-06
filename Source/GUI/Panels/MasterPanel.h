#pragma once

#include "../PanelBase.h"

#ifndef AERIFORM_FX
 #define AERIFORM_FX 0
#endif

namespace aeriform
{
/** MASTER: voice mode, polyphony, glide, unison, pitch bend, MPE, quality, output.
    Aeriform FX also surfaces the FX I/O strip: Input gain, Dry/Wet, Output gain, Root note. */
class MasterPanel : public ParamPanel
{
public:
    explicit MasterPanel (AeriformProcessor&);
    void resized() override;

private:
    juce::Label *voiceCaption, *outCaption;
    ChoiceBox *voiceMode, *quality;
    Knob *voices, *glide, *unison, *detune, *spread, *bend, *outGain, *outHp;
    Toggle *glideLegato, *mpe, *limiter;
#if AERIFORM_FX
    Knob *fxInput, *fxWet, *fxOutput, *fxRoot;
#endif
};
} // namespace aeriform
