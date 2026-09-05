#include "MotionPanel.h"

namespace aeriform
{
MotionPanel::MotionPanel (AeriformProcessor& p) : ParamPanel (p, "MOTION", theme::teal)
{
    using namespace ids;
    const ModDest rateDest[ids::numLFOs] = { ModDest::Lfo1Rate, ModDest::Lfo2Rate, ModDest::Lfo3Rate };
    for (int i = 0; i < numLFOs; ++i)
    {
        auto& l = lfos[i];
        const int n = i + 1;
        l.caption  = caption ("LFO " + juce::String (n));
        l.shape    = control<ChoiceBox> (processor, lfoParam (n, lfoShapeSuffix), "Shape");
        l.rate     = knob (lfoParam (n, lfoRateSuffix), exponential (rateDest[i], 3.0f), theme::knobSizeSmall);
        l.rate->setDisplayName ("Rate");
        l.sync     = control<Toggle> (processor, lfoParam (n, lfoSyncSuffix), "Sync");
        l.division = control<ChoiceBox> (processor, lfoParam (n, lfoDivSuffix), "Division");
        l.mode     = control<ChoiceBox> (processor, lfoParam (n, lfoModeSuffix), "Trigger");
        l.fade     = knob (lfoParam (n, lfoFadeSuffix), {}, theme::knobSizeSmall);
        l.fade->setDisplayName ("Fade");
        l.phase    = knob (lfoParam (n, lfoPhaseSuffix), {}, theme::knobSizeSmall);
        l.phase->setDisplayName ("Phase");
    }
    for (auto& l : lfos)
        for (auto* k : { l.rate, l.fade, l.phase })
            k->setAccentColour (theme::teal);

    envCaption = caption ("MOD ENVELOPE");
    menvA = knob (menvAttack, {}, theme::knobSizeSmall);  menvA->setDisplayName ("Attack");
    menvD = knob (menvDecay, {}, theme::knobSizeSmall);   menvD->setDisplayName ("Decay");
    menvS = knob (menvSustain, {}, theme::knobSizeSmall); menvS->setDisplayName ("Sustain");
    menvR = knob (menvRelease, {}, theme::knobSizeSmall); menvR->setDisplayName ("Release");

    matrixCaption = caption ("MODULATION MATRIX");
    matrix = control<ModMatrixPanel> (processor);
}

void MotionPanel::resized()
{
    auto r = getContentArea();
    const int capH = 14;
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
