#include "ExcitersOverviewPanel.h"

namespace aeriform
{
ExcitersOverviewPanel::ExcitersOverviewPanel (AeriformProcessor& p) : ParamPanel (p, "EXCITERS", theme::copper)
{
    using namespace ids;
    slotA = control<ExciterSlotPanel> (processor, 0, true);
    slotB = control<ExciterSlotPanel> (processor, 1, true);

    mixCaption  = caption ("INTERACTION");
    mixMode     = control<ChoiceBox> (processor, ids::mixMode, "");
    mixMode->setCaptionVisible (false);
    interaction = knob (mixInteraction, "Interaction", additive (ModDest::Interaction));
    depth       = knob (mixDepth, "Depth");
    balance     = knob (mixBalance, "Balance", additive (ModDest::Balance));

    foldCaption = caption ("WAVEFOLDER");
    foldOn      = control<Toggle> (processor, wfOn, "On");
    foldMode    = control<ChoiceBox> (processor, wfMode, "");
    foldMode->setCaptionVisible (false);
    fold        = knob (wfFold, "Fold", additive (ModDest::Fold));
    drive       = knob (wfDrive, "Drive", additive (ModDest::FoldDrive));
    shape       = knob (wfShape, "Shape");
    for (auto* k : { fold, drive, shape }) k->setAccentColour (theme::folder);

    envCaption  = caption ("BREATH ENVELOPE");
    attack      = knob (envAttack, "Attack");
    decay       = knob (envDecay, "Decay");
    sustain     = knob (envSustain, "Sustain");
    release     = knob (envRelease, "Release");
    velPress    = knob (envVelToPressure, "Vel > Press");
    pressBright = knob (artPressBright, "Press > Bright");
}

void ExcitersOverviewPanel::collectPanels (std::vector<ParamPanel*>& out)
{
    out.push_back (this);
    out.push_back (slotA);
    out.push_back (slotB);
}

void ExcitersOverviewPanel::resized()
{
    auto r = getContentArea();
    const int capH = 14;
    slotA->setBounds (r.removeFromTop (110));
    r.removeFromTop (4);
    slotB->setBounds (r.removeFromTop (110));
    r.removeFromTop (6);

    // interaction: mode box on the left, three knobs
    mixCaption->setBounds (r.removeFromTop (capH));
    auto mixRow = r.removeFromTop (70);
    mixMode->setBounds (mixRow.removeFromLeft (150).withTrimmedTop (22).withHeight (26));
    mixRow.removeFromLeft (6);
    knobRow (mixRow, { interaction, depth, balance });
    r.removeFromTop (4);

    // wavefolder: on + mode on the left, three knobs
    foldCaption->setBounds (r.removeFromTop (capH));
    auto foldRow = r.removeFromTop (70);
    auto foldLeft = foldRow.removeFromLeft (150);
    foldOn->setBounds (foldLeft.removeFromTop (22).withTrimmedTop (2));
    foldMode->setBounds (foldLeft.withTrimmedTop (4).withHeight (26));
    foldRow.removeFromLeft (6);
    knobRow (foldRow, { fold, drive, shape });
    r.removeFromTop (4);

    envCaption->setBounds (r.removeFromTop (capH));
    knobRow (r.removeFromTop (74), { attack, decay, sustain, release, velPress, pressBright });
}
} // namespace aeriform
