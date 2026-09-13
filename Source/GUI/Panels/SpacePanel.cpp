#include "SpacePanel.h"

namespace aeriform
{
SpacePanel::SpacePanel (AeriformProcessor& p, bool f) : ParamPanel (p, "EFFECTS", theme::teal), full (f)
{
    using namespace ids;
    const int s = full ? theme::knobSizeLarge : theme::knobSizeSmall;

    chorusCaption = caption ("CHORUS");
    chorusMix   = knob (ids::chorusMix, "Mix", additive (ModDest::ChorusMix), s);
    chorusRate  = knob (ids::chorusRate, "Rate", additive (ModDest::ChorusRate), s);
    chorusDepth = knob (ids::chorusDepth, "Depth", additive (ModDest::ChorusDepth), s);
    chorusWidth = knob (ids::chorusWidth, "Width", {}, s);

    const char* chorusNames[5] = { "ENSEMBLE", "JUNO I", "JUNO II", "JUNO I+II", "DIMENSION" };
    for (int i = 0; i < 5; ++i)
    {
        auto& btn = chorusTypeButtons[(size_t) i];
        btn.setButtonText (chorusNames[i]);
        btn.onClick = [this, &p, i]
        {
            p.getPatchTools().setParameter (ids::chorusType, (float) i);
        };
        addAndMakeVisible (btn);
    }

    if (auto* param = p.getAPVTS().getParameter (ids::chorusType))
    {
        chorusTypeAttachment = std::make_unique<juce::ParameterAttachment> (*param, [this] (float v)
        {
            const int idx = std::clamp ((int) std::lround (v), 0, 4);
            for (int i = 0; i < 5; ++i)
                chorusTypeButtons[(size_t) i].setToggleState (i == idx, juce::dontSendNotification);
        });
        chorusTypeAttachment->sendInitialUpdate();
    }

    delayCaption  = caption ("DELAY");
    delayMix      = knob (ids::delayMix, "Mix", additive (ModDest::DelayMix), s);
    delayTime     = knob (ids::delayTime, "Time", additive (ModDest::DelayTimeL), s);
    delayFeedback = knob (ids::delayFeedback, "Feedback", additive (ModDest::DelayFeedback), s);
    delayTone     = knob (ids::delayTone, "Tone", additive (ModDest::DelayFilter), s);
    delaySync     = control<Toggle> (processor, ids::delaySync, "Sync");
    delayPingPong = control<Toggle> (processor, ids::delayPingPong, "Ping-Pong");
    delayDiv      = control<ChoiceBox> (processor, ids::delayDiv, "Division");

    reverbCaption = caption ("REVERB");
    reverbType    = control<ChoiceBox> (processor, ids::reverbType, "Type");
    revMix   = knob (reverbMix, "Mix", additive (ModDest::ReverbMix), s);
    revSize  = knob (reverbSize, "Size", additive (ModDest::ReverbSize), s);
    revDecay = knob (reverbDecay, "Decay", additive (ModDest::ReverbDecay), s);
    revDamp  = knob (reverbDamping, "Damping", additive (ModDest::ReverbDamp), s);
    revPre   = knob (reverbPreDelay, "Pre-Delay", additive (ModDest::ReverbPredelay), s);
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
    for (auto* c : std::initializer_list<juce::Component*> { chorusMix, chorusRate, chorusDepth, chorusWidth })
        c->setVisible (index == 0);
    for (auto& btn : chorusTypeButtons)
        btn.setVisible (index == 0);

    for (auto* c : std::initializer_list<juce::Component*> { delayMix, delayTime, delayFeedback, delayTone, delaySync, delayPingPong, delayDiv })
        c->setVisible (index == 1);

    for (auto* c : { revMix, revSize, revDecay, revDamp, revPre, revWidth, revMod })
        c->setVisible (index == 2);
    reverbType->setVisible (index == 2);

    chorusCaption->setVisible (false);
    delayCaption->setVisible (false);
    reverbCaption->setVisible (false);

    for (int i = 0; i < 3; ++i)
        tabs[(size_t) i].setToggleState (i == index, juce::dontSendNotification);

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
        auto delayArea  = r.removeFromLeft (colW); r.removeFromLeft (gap);
        auto reverbArea = r;

        const int rowH = 96;

        // Chorus Column
        chorusCaption->setBounds (chorusArea.removeFromTop (capH));
        knobRow (chorusArea.removeFromTop (rowH), { chorusMix, chorusRate, chorusDepth, chorusWidth });
        chorusArea.removeFromTop (10);
        auto cStrip = chorusArea.removeFromTop (26);
        const int cBtnW = cStrip.getWidth() / 5;
        for (auto& btn : chorusTypeButtons)
            btn.setBounds (cStrip.removeFromLeft (cBtnW).reduced (2, 0));

        // Delay Column
        delayCaption->setBounds (delayArea.removeFromTop (capH));
        knobRow (delayArea.removeFromTop (rowH), { delayMix, delayTime, delayFeedback, delayTone });
        delayArea.removeFromTop (10);
        auto switches = delayArea.removeFromTop (44);
        delaySync->setBounds (switches.removeFromLeft (60).withTrimmedTop (14));
        delayPingPong->setBounds (switches.removeFromLeft (90).withTrimmedTop (14));
        switches.removeFromLeft (8);
        delayDiv->setBounds (switches);
        delayDiv->setCaptionVisible (true);

        // Reverb Column
        reverbCaption->setBounds (reverbArea.removeFromTop (capH));
        auto revTypeRow = reverbArea.removeFromTop (26);
        reverbType->setBounds (revTypeRow.removeFromLeft (130));
        reverbType->setCaptionVisible (false);
        reverbArea.removeFromTop (4);
        knobRow (reverbArea.removeFromTop (rowH), { revMix, revSize, revDecay, revDamp });
        knobRow (reverbArea.removeFromTop (rowH), { revPre, revWidth, revMod, nullptr });
        return;
    }

    auto bar = getLocalBounds().removeFromTop (theme::sectionTitleHeight).withTrimmedLeft (74).reduced (4, 2);
    const int w = bar.getWidth() / 3;
    for (auto& button : tabs)
        button.setBounds (bar.removeFromLeft (w).reduced (2, 0));

    r.removeFromTop (4);

    // Tab 0: Chorus
    auto chorusArea = r;
    auto cStrip = chorusArea.removeFromTop (24);
    const int cBtnW = cStrip.getWidth() / 5;
    for (auto& btn : chorusTypeButtons)
        btn.setBounds (cStrip.removeFromLeft (cBtnW).reduced (1, 0));
    chorusArea.removeFromTop (4);
    knobRow (chorusArea.removeFromTop (70), { chorusMix, chorusRate, chorusDepth, chorusWidth });

    // Tab 1: Delay
    auto delayArea = r;
    knobRow (delayArea.removeFromTop (70), { delayMix, delayTime, delayFeedback, delayTone });
    auto switches = delayArea.removeFromTop (40);
    delaySync->setBounds (switches.removeFromLeft (66).withTrimmedTop (12));
    delayPingPong->setBounds (switches.removeFromLeft (96).withTrimmedTop (12));
    delayDiv->setBounds (switches);

    // Tab 2: Reverb
    auto revArea = r;
    auto revTop = revArea.removeFromTop (24);
    reverbType->setBounds (revTop.removeFromLeft (120));
    reverbType->setCaptionVisible (false);
    revArea.removeFromTop (4);
    knobRow (revArea.removeFromTop (70), { revMix, revSize, revDecay, revDamp });
    knobRow (revArea.removeFromTop (70), { revPre, revWidth, revMod, nullptr });
}
} // namespace aeriform