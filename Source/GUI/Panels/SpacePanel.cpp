#include "SpacePanel.h"

namespace aeriform
{
SpacePanel::SpacePanel (AeriformProcessor& p, bool f) : ParamPanel (p, "EFFECTS", theme::teal), full (f)
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
    if (! full)
    {
        const char* names[] = { "CHORUS", "DELAY", "REVERB" };
        for (int i = 0; i < 3; ++i)
        {
            auto& button = tabs[(size_t) i];
            button.setButtonText (names[i]);
            button.onClick = [this, i] { selectEffect (i); };
            addAndMakeVisible (button);
        }
        selectEffect (0);
    }
}

void SpacePanel::selectEffect (int index)
{
    selected = index;
    for (auto* c : std::initializer_list<juce::Component*> { chorusMix, chorusRate, chorusDepth, chorusWidth }) c->setVisible (index == 0);
    for (auto* c : std::initializer_list<juce::Component*> { delayMix, delayTime, delayFeedback, delayTone, delaySync, delayPingPong, delayDiv }) c->setVisible (index == 1);
    for (auto* c : { revMix, revSize, revDecay, revDamp, revPre, revWidth, revMod }) c->setVisible (index == 2);
    chorusCaption->setVisible (false); delayCaption->setVisible (false); reverbCaption->setVisible (false);
    for (int i = 0; i < 3; ++i) tabs[(size_t) i].setToggleState (i == index, juce::dontSendNotification);
    resized();
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
        delaySync->setBounds (switches.removeFromLeft (60).withTrimmedTop (14));
        delayPingPong->setBounds (switches.removeFromLeft (90).withTrimmedTop (14));
        switches.removeFromLeft (8);
        delayDiv->setBounds (switches);
        delayDiv->setCaptionVisible (true);

        reverbCaption->setBounds (reverbArea.removeFromTop (capH));
        knobRow (reverbArea.removeFromTop (rowH), { revMix, revSize, revDecay, revDamp });
        knobRow (reverbArea.removeFromTop (rowH), { revPre, revWidth, revMod, nullptr });
        return;
    }

    auto bar = getLocalBounds().removeFromTop (theme::sectionTitleHeight).withTrimmedLeft (74).reduced (4, 2);
    const int w = bar.getWidth() / 3;
    for (auto& button : tabs) button.setBounds (bar.removeFromLeft (w).reduced (2, 0));
    r.removeFromTop (4);
    knobRow (r.withHeight (70), { chorusMix, chorusRate, chorusDepth, chorusWidth });
    auto delayArea = r;
    knobRow (delayArea.removeFromTop (70), { delayMix, delayTime, delayFeedback, delayTone });
    auto switches = delayArea.removeFromTop (40);
    delaySync->setBounds (switches.removeFromLeft (66).withTrimmedTop (12));
    delayPingPong->setBounds (switches.removeFromLeft (96).withTrimmedTop (12));
    delayDiv->setBounds (switches);
    knobRow (r.removeFromTop (70), { revMix, revSize, revDecay, revDamp });
    knobRow (r.removeFromTop (70), { revPre, revWidth, revMod, nullptr });
}
} // namespace aeriform