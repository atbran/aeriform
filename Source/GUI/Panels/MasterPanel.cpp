#include "MasterPanel.h"

namespace aeriform
{
MasterPanel::MasterPanel (AeriformProcessor& p) : ParamPanel (p, "MASTER", theme::copper)
{
    using namespace ids;
    const int s = theme::knobSizeSmall;

    voiceCaption = caption ("VOICES");
    voiceMode   = control<ChoiceBox> (processor, ids::voiceMode, "Mode");
    voices      = knob (voiceCount, "Voices", {}, s);
    glide       = knob (glideTime, "Glide", {}, s);
    glideLegato = control<Toggle> (processor, glideLegatoOnly, "Legato");
    unison      = knob (unisonVoices, "Unison", {}, s);
    detune      = knob (unisonDetune, "Detune", {}, s);
    spread      = knob (unisonSpread, "Spread", {}, s);
    bend        = knob (bendRange, "Bend", {}, s);
    mpe         = control<Toggle> (processor, mpeEnabled, "MPE");

#if AERIFORM_FX
    outCaption = caption ("FX  INPUT / DRY-WET / OUTPUT / ROOT");
#else
    outCaption = caption ("PERFORMANCE / OUTPUT");
#endif
    quality = control<ChoiceBox> (processor, ids::quality, "Quality");
    outGain = knob (ids::outGain, "Output", {}, s);
    outHp   = knob (outHighpass, "High-Pass", {}, s);
    limiter = control<Toggle> (processor, limiterOn, "Limiter");

#if AERIFORM_FX
    fxInput  = knob (ids::fxInputGain,  "In",   {}, s);
    fxWet    = knob (ids::fxMix,        "Mix",  {}, s);
    fxOutput = knob (ids::fxOutputGain, "Out",  {}, s);
    fxRoot   = knob (ids::fxRootNote,   "Root", {}, s);
#endif
}

void MasterPanel::resized()
{
    auto r = getContentArea();
    const int capH = 14;
    const int knobW = 56;
    const int rowH = 66;

    juce::ignoreUnused (knobW);
    // row 1: voice mode + polyphony / glide / unison + legato / MPE
    voiceCaption->setBounds (r.removeFromTop (capH));
    auto row = r.removeFromTop (rowH);
    auto modeArea = row.removeFromLeft (96);
    voiceMode->setBounds (modeArea.withHeight (40));
    row.removeFromLeft (6);
    auto toggles = row.removeFromRight (70);
    glideLegato->setBounds (toggles.removeFromTop (22).withTrimmedTop (2));
    mpe->setBounds (toggles.removeFromTop (22).withTrimmedTop (2));
    knobRow (row, { voices, glide, unison, detune });

    // row 2: quality + output controls + limiter
    r.removeFromTop (6);
    outCaption->setBounds (r.removeFromTop (capH));
    auto row2 = r.removeFromTop (rowH);
    auto qualityArea = row2.removeFromLeft (96);
    quality->setBounds (qualityArea.withHeight (40));
    row2.removeFromLeft (6);
    auto toggles2 = row2.removeFromRight (70);
    limiter->setBounds (toggles2.removeFromTop (22).withTrimmedTop (2));

#if AERIFORM_FX
    // Aeriform FX: the FX I/O strip takes the output row. out_gain / out_hp / unison
    // spread / bend range stay automatable but are not shown on this compact panel.
    knobRow (row2, { fxInput, fxWet, fxOutput, fxRoot });
    for (auto* hidden : { spread, bend, outGain, outHp })
        hidden->setBounds ({});
#else
    knobRow (row2, { spread, bend, outGain, outHp });
#endif
}
} // namespace aeriform
