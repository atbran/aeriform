#include "SpacePanel.h"

namespace aeriform
{
SpacePanel::SpacePanel (AeriformProcessor& p) : ParamPanel (p, "SPACE", theme::tealDim.brighter (0.4f))
{
    using namespace ids;
    const int s = theme::knobSizeSmall;

    chorusCaption = caption ("CHORUS");
    chorusMix   = knob (ids::chorusMix, additive (ModDest::ChorusMix), s);   chorusMix->setDisplayName ("Mix");
    chorusRate  = knob (ids::chorusRate, {}, s);                            chorusRate->setDisplayName ("Rate");
    chorusDepth = knob (ids::chorusDepth, {}, s);                           chorusDepth->setDisplayName ("Depth");
    chorusWidth = knob (ids::chorusWidth, {}, s);                           chorusWidth->setDisplayName ("Width");

    delayCaption  = caption ("DELAY");
    delayMix      = knob (ids::delayMix, additive (ModDest::DelayMix), s);   delayMix->setDisplayName ("Mix");
    delayTime     = knob (ids::delayTime, {}, s);                           delayTime->setDisplayName ("Time");
    delayFeedback = knob (ids::delayFeedback, {}, s);                       delayFeedback->setDisplayName ("Feedback");
    delayTone     = knob (ids::delayTone, {}, s);                           delayTone->setDisplayName ("Tone");
    delaySync     = control<Toggle> (processor, ids::delaySync, "Sync");
    delayPingPong = control<Toggle> (processor, ids::delayPingPong, "Ping-Pong");
    delayDiv      = control<ChoiceBox> (processor, ids::delayDiv, "Division");

    reverbCaption = caption ("REVERB");
    revMix   = knob (reverbMix, additive (ModDest::ReverbMix), s);  revMix->setDisplayName ("Mix");
    revSize  = knob (reverbSize, {}, s);                           revSize->setDisplayName ("Size");
    revDecay = knob (reverbDecay, {}, s);                          revDecay->setDisplayName ("Decay");
    revDamp  = knob (reverbDamping, {}, s);                        revDamp->setDisplayName ("Damping");
    revPre   = knob (reverbPreDelay, {}, s);                       revPre->setDisplayName ("Pre-Delay");
    revWidth = knob (reverbWidth, {}, s);                          revWidth->setDisplayName ("Width");
    revMod   = knob (reverbModulation, {}, s);                     revMod->setDisplayName ("Motion");

    for (auto& k : knobs) k->setAccentColour (theme::teal);
}

void SpacePanel::resized()
{
    auto r = getContentArea();
    const int capH = 14;
    const int knobW = 56;
    const int rowH = 66;

    // row 1: chorus + delay, row 2: reverb
    auto top = r.removeFromTop (capH + rowH);
    auto chorusArea = top.removeFromLeft (knobW * 4);
    top.removeFromLeft (16);
    auto delayArea = top;

    chorusCaption->setBounds (chorusArea.removeFromTop (capH));
    knobRow (chorusArea, { chorusMix, chorusRate, chorusDepth, chorusWidth });

    delayCaption->setBounds (delayArea.removeFromTop (capH));
    auto switches = delayArea.removeFromRight (118);
    knobRow (delayArea.removeFromLeft (knobW * 4), { delayMix, delayTime, delayFeedback, delayTone });
    delaySync->setBounds (switches.removeFromTop (20));
    delayPingPong->setBounds (switches.removeFromTop (20));
    delayDiv->setBounds (switches.removeFromTop (24).withTrimmedTop (2));
    delayDiv->setCaptionVisible (false);

    r.removeFromTop (6);
    reverbCaption->setBounds (r.removeFromTop (capH));
    knobRow (r.removeFromTop (rowH).removeFromLeft (knobW * 7), { revMix, revSize, revDecay, revDamp, revPre, revWidth, revMod });
}
} // namespace aeriform
