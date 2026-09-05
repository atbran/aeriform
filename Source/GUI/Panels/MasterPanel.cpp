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

    outCaption = caption ("PERFORMANCE / OUTPUT");
    quality = control<ChoiceBox> (processor, ids::quality, "Quality");
    outGain = knob (ids::outGain, "Output", {}, s);
    outHp   = knob (outHighpass, "High-Pass", {}, s);
    limiter = control<Toggle> (processor, limiterOn, "Limiter");
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

    // row 2: quality + spread / bend / output / high-pass + limiter
    r.removeFromTop (6);
    outCaption->setBounds (r.removeFromTop (capH));
    auto row2 = r.removeFromTop (rowH);
    auto qualityArea = row2.removeFromLeft (96);
    quality->setBounds (qualityArea.withHeight (40));
    row2.removeFromLeft (6);
    auto toggles2 = row2.removeFromRight (70);
    limiter->setBounds (toggles2.removeFromTop (22).withTrimmedTop (2));
    knobRow (row2, { spread, bend, outGain, outHp });
}
} // namespace aeriform
