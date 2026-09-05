#include "SpacePanel.h"

namespace aeriform
{
SpacePanel::SpacePanel (AeriformProcessor& p, bool f) : ParamPanel (p, "SPACE", theme::tealDim.brighter (0.4f)), full (f)
{
    using namespace ids;
    const int s = full ? theme::knobSizeLarge : theme::knobSizeSmall;

    chorusCaption = caption ("CHORUS");
    chorusMix   = knob (ids::chorusMix, "Mix", additive (ModDest::ChorusMix), s);
    chorusRate  = knob (ids::chorusRate, "Rate", {}, s);
    chorusDepth = knob (ids::chorusDepth, "Depth", {}, s);
    chorusWidth = knob (ids::chorusWidth, "Width", {}, s);

    delayCaption  = caption ("DELAY");
    delayMix      = knob (ids::delayMix, "Mix", additive (ModDest::DelayMix), s);
    delayTime     = knob (ids::delayTime, "Time", {}, s);
    delayFeedback = knob (ids::delayFeedback, "Feedback", {}, s);
    delayTone     = knob (ids::delayTone, "Tone", {}, s);
    delaySync     = control<Toggle> (processor, ids::delaySync, "Sync");
    delayPingPong = control<Toggle> (processor, ids::delayPingPong, "Ping-Pong");
    delayDiv      = control<ChoiceBox> (processor, ids::delayDiv, "Division");

    reverbCaption = caption ("REVERB");
    revMix   = knob (reverbMix, "Mix", additive (ModDest::ReverbMix), s);
    revSize  = knob (reverbSize, "Size", {}, s);
    revDecay = knob (reverbDecay, "Decay", {}, s);
    revDamp  = knob (reverbDamping, "Damping", {}, s);
    revPre   = knob (reverbPreDelay, "Pre-Delay", {}, s);
    revWidth = knob (reverbWidth, "Width", {}, s);
    revMod   = knob (reverbModulation, "Motion", {}, s);

    for (auto& k : knobs) k->setAccentColour (theme::teal);
}

void SpacePanel::resized()
{
    auto r = getContentArea();
    const int capH = 14;

    if (full)
    {
        const int gap = 16;
        const int colW = (r.getWidth() - 2 * gap) / 3;
        auto chorusArea = r.removeFromLeft (colW); r.removeFromLeft (gap);
        auto delayArea = r.removeFromLeft (colW);  r.removeFromLeft (gap);
        auto reverbArea = r;

        const int rowH = 96;
        chorusCaption->setBounds (chorusArea.removeFromTop (capH));
        knobRow (chorusArea.removeFromTop (rowH), { chorusMix, chorusRate, chorusDepth, chorusWidth });

        delayCaption->setBounds (delayArea.removeFromTop (capH));
        knobRow (delayArea.removeFromTop (rowH), { delayMix, delayTime, delayFeedback, delayTone });
        delayArea.removeFromTop (10);
        auto switches = delayArea.removeFromTop (44);
        delaySync->setBounds (switches.removeFromLeft (80).withTrimmedTop (14));
        delayPingPong->setBounds (switches.removeFromLeft (110).withTrimmedTop (14));
        switches.removeFromLeft (8);
        delayDiv->setBounds (switches.removeFromLeft (150));
        delayDiv->setCaptionVisible (true);

        reverbCaption->setBounds (reverbArea.removeFromTop (capH));
        knobRow (reverbArea.removeFromTop (rowH), { revMix, revSize, revDecay, revDamp });
        knobRow (reverbArea.removeFromTop (rowH), { revPre, revWidth, revMod, nullptr });
        return;
    }

    const int knobW = 56;
    const int rowH = 66;
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
