#include "MotionPanel.h"

namespace aeriform
{
MotionPanel::MotionPanel (AeriformProcessor& p, bool f) : ParamPanel (p, "MOTION", theme::teal), full (f)
{
    using namespace ids;
    const ModDest rateDest[ids::numLFOs] = { ModDest::Lfo1Rate, ModDest::Lfo2Rate, ModDest::Lfo3Rate };
    const int d = full ? theme::knobSize : theme::knobSizeSmall;
    for (int i = 0; i < numLFOs; ++i)
    {
        auto& l = lfos[i];
        const int n = i + 1;
        l.caption  = caption ("LFO " + juce::String (n));
        l.shape    = control<ChoiceBox> (processor, lfoParam (n, lfoShapeSuffix), "Shape");
        l.rate     = knob (lfoParam (n, lfoRateSuffix), "Rate", exponential (rateDest[i], 3.0f), d);
        l.sync     = control<Toggle> (processor, lfoParam (n, lfoSyncSuffix), "Sync");
        l.division = control<ChoiceBox> (processor, lfoParam (n, lfoDivSuffix), "Division");
        l.mode     = control<ChoiceBox> (processor, lfoParam (n, lfoModeSuffix), "Trigger");
        l.fade     = knob (lfoParam (n, lfoFadeSuffix), "Fade", {}, d);
        l.phase    = knob (lfoParam (n, lfoPhaseSuffix), "Phase", {}, d);
    }
    envCaption = caption ("MOD ENVELOPE");
    menvA = knob (menvAttack, "Attack", {}, d);
    menvD = knob (menvDecay, "Decay", {}, d);
    menvS = knob (menvSustain, "Sustain", {}, d);
    menvR = knob (menvRelease, "Release", {}, d);
    for (auto& k : knobs) k->setAccentColour (theme::teal);

    matrixCaption = caption (full ? "MODULATION MATRIX  (16 slots)" : "MODULATION MATRIX  (slots 1-8, all 16 on the MOTION page)");
    matrix = control<ModMatrixPanel> (processor, 1, full ? ids::numModSlots : ids::numModSlotsV01, full ? 2 : 1);
}

void MotionPanel::resized()
{
    auto r = getContentArea();
    const int capH = 14;

    if (full)
    {
        // three LFO columns
        auto lfoArea = r.removeFromTop (150);
        const int colW = (lfoArea.getWidth() - 2 * 12) / 3;
        for (int i = 0; i < ids::numLFOs; ++i)
        {
            auto& l = lfos[i];
            auto col = juce::Rectangle<int> (lfoArea.getX() + i * (colW + 12), lfoArea.getY(), colW, lfoArea.getHeight());
            l.caption->setBounds (col.removeFromTop (capH));
            auto boxes = col.removeFromTop (44);
            const int bw = (boxes.getWidth() - 16) / 3;
            l.shape->setBounds (boxes.removeFromLeft (bw)); boxes.removeFromLeft (8);
            l.mode->setBounds (boxes.removeFromLeft (bw)); boxes.removeFromLeft (8);
            l.division->setBounds (boxes);
            col.removeFromTop (4);
            auto row = col.removeFromTop (80);
            l.sync->setBounds (row.removeFromLeft (70).withTrimmedTop (24).withHeight (22));
            knobRow (row, { l.rate, l.fade, l.phase });
        }
        r.removeFromTop (8);
        auto bottom = r;
        auto envArea = bottom.removeFromLeft (150);
        envCaption->setBounds (envArea.removeFromTop (capH));
        auto envRow1 = envArea.removeFromTop (84);
        knobRow (envRow1, { menvA, menvD });
        knobRow (envArea.removeFromTop (84), { menvS, menvR });
        bottom.removeFromLeft (16);
        matrixCaption->setBounds (bottom.removeFromTop (capH));
        matrix->setBounds (bottom);
        return;
    }

    const int lfoRowH = 62;
    for (auto& l : lfos)
    {
        l.caption->setBounds (r.removeFromTop (capH));
        auto row = r.removeFromTop (lfoRowH);
        const int knobW = 52;
        auto knobsArea = row.removeFromRight (knobW * 3);
        knobRow (knobsArea, { l.rate, l.fade, l.phase });
        auto boxes = row.withTrimmedRight (6);
        const int colW = boxes.getWidth() / 3;
        auto c1 = boxes.removeFromLeft (colW).reduced (2, 0);
        auto c2 = boxes.removeFromLeft (colW).reduced (2, 0);
        auto c3 = boxes.reduced (2, 0);
        l.shape->setBounds (c1.withHeight (38));
        l.mode->setBounds (c2.withHeight (38));
        l.division->setBounds (c3.withHeight (38));
        l.sync->setBounds (c1.withTrimmedTop (40).withHeight (20));
        r.removeFromTop (2);
    }

    envCaption->setBounds (r.removeFromTop (capH));
    auto envRow = r.removeFromTop (60);
    knobRow (envRow.removeFromLeft (52 * 4), { menvA, menvD, menvS, menvR });

    r.removeFromTop (4);
    matrixCaption->setBounds (r.removeFromTop (capH));
    matrix->setBounds (r);
}
} // namespace aeriform
