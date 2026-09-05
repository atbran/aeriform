#include "MasterPanel.h"

namespace aeriform
{
MasterPanel::MasterPanel (AeriformProcessor& p) : ParamPanel (p, "MASTER", theme::copper)
{
    using namespace ids;
    const int s = theme::knobSizeSmall;

    voiceCaption = caption ("VOICES");
    voiceMode   = control<ChoiceBox> (processor, ids::voiceMode, "Mode");
    voices      = knob (voiceCount, {}, s);      voices->setDisplayName ("Voices");
    glide       = knob (glideTime, {}, s);       glide->setDisplayName ("Glide");
    glideLegato = control<Toggle> (processor, glideLegatoOnly, "Legato only");
    unison      = knob (unisonVoices, {}, s);    unison->setDisplayName ("Unison");
    detune      = knob (unisonDetune, {}, s);    detune->setDisplayName ("Detune");
    spread      = knob (unisonSpread, {}, s);    spread->setDisplayName ("Spread");
    bend        = knob (bendRange, {}, s);       bend->setDisplayName ("Bend");
    mpe         = control<Toggle> (processor, mpeEnabled, "MPE");

    outCaption = caption ("PERFORMANCE / OUTPUT");
    outGain = knob (ids::outGain, {}, s);        outGain->setDisplayName ("Output");
    outHp   = knob (outHighpass, {}, s);         outHp->setDisplayName ("High-Pass");
    limiter = control<Toggle> (processor, limiterOn, "Limiter");
}

void MasterPanel::resized()
{
    auto r = getContentArea();
    const int capH = 14;
    const int knobW = 56;
    const int rowH = 66;

    // row 1: voice mode + polyphony / glide / unison
    voiceCaption->setBounds (r.removeFromTop (capH));
    auto row = r.removeFromTop (rowH);
    auto modeArea = row.removeFromLeft (96);
    voiceMode->setBounds (modeArea.withHeight (40));
    glideLegato->setBounds (modeArea.withTrimmedTop (44).withHeight (20));
    row.removeFromLeft (6);
    knobRow (row.removeFromLeft (knobW * 4), { voices, glide, unison, detune });

    // row 2: spread / bend / MPE + output
    r.removeFromTop (6);
    auto captions = r.removeFromTop (capH);
    outCaption->setBounds (captions);
    auto row2 = r.removeFromTop (rowH);
    knobRow (row2.removeFromLeft (knobW * 4), { spread, bend, outGain, outHp });
    row2.removeFromLeft (10);
    auto toggles = row2.removeFromLeft (110);
    mpe->setBounds (toggles.removeFromTop (22).withTrimmedTop (2));
    limiter->setBounds (toggles.removeFromTop (22).withTrimmedTop (2));
}
} // namespace aeriform
