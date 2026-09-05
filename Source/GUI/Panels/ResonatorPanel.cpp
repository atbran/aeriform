#include "ResonatorPanel.h"

namespace aeriform
{
ResonatorPanel::ResonatorPanel (AeriformProcessor& p) : ParamPanel (p, "RESONATOR", theme::brass)
{
    using namespace ids;
    tuneCaption = caption ("TUNING");
    coarse   = knob (resCoarse);
    fine     = knob (resFine, semitones (ModDest::Pitch, 2400.0f));   // pitch mod: +/- 24 semitones = +/- 2400 cents
    length   = knob (resLength);
    keyTrack = knob (resKeyTrack);
    mode     = control<ChoiceBox> (processor, resMode, "Mode");

    tubeCaption = caption ("TUBE");
    feedback   = knob (resFeedback, additive (ModDest::Feedback), theme::knobSizeLarge);
    damping    = knob (resDamping, additive (ModDest::Damping), theme::knobSizeLarge);
    brightness = knob (resBrightness, additive (ModDest::Brightness), theme::knobSizeLarge);
    dispersion = knob (resDispersion, additive (ModDest::Dispersion), theme::knobSizeLarge);
    shape      = knob (resShape, additive (ModDest::Shape));
    reflection = knob (resReflection, additive (ModDest::Reflection));
    saturation = knob (resSaturation);

    bodyCaption = caption ("BODY / FORMANT");
    bodyFreq  = knob (resBodyFreq, exponential (ModDest::BodyFreq, 3.0f));
    bodyRes   = knob (resBodyRes);
    bodyMix   = knob (resBodyMix, additive (ModDest::BodyMix));
    bodyTrack = knob (resBodyTrack);
}

void ResonatorPanel::resized()
{
    auto r = getContentArea();
    const int capH = 14;

    tuneCaption->setBounds (r.removeFromTop (capH));
    auto row = r.removeFromTop (76);
    auto modeArea = row.removeFromRight (110).withTrimmedTop (14).withHeight (40);
    mode->setBounds (modeArea);
    knobRow (row, { coarse, fine, length, keyTrack });

    r.removeFromTop (4);
    tubeCaption->setBounds (r.removeFromTop (capH));
    knobRow (r.removeFromTop (92), { feedback, damping, brightness, dispersion });
    knobRow (r.removeFromTop (76), { shape, reflection, saturation });

    r.removeFromTop (4);
    bodyCaption->setBounds (r.removeFromTop (capH));
    knobRow (r.removeFromTop (76), { bodyFreq, bodyRes, bodyMix, bodyTrack });
}
} // namespace aeriform
