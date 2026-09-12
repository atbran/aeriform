#include "ResonatorPanel.h"

namespace aeriform
{
ResonatorPanel::ResonatorPanel (AeriformProcessor& p, int slot)
    : ParamPanel (p, "RESONATOR " + juce::String::charToString ((juce::juce_wchar) ('A' + slot)),
                  slot == 0 ? theme::nodeA : slot == 1 ? theme::nodeB : theme::nodeC)
{
    using namespace ids;
    const juce::String prefix = slot == 1 ? "rb" : "rc";
    auto id = [&] (const char* a, const char* suffix) { return slot == 0 ? juce::String (a) : prefix + suffix; };
    auto dest = [&] (ModDest a, ModDest b, ModDest c) { return slot == 0 ? a : slot == 1 ? b : c; };
    tuneCaption = caption ("TUNING");
    coarse = knob (id (resCoarse, "_coarse"), "Coarse");
    fine = knob (id (resFine, "_fine"), "Fine", semitones (dest (ModDest::Pitch, ModDest::ResBPitch, ModDest::ResCPitch), 2400.0f));
    // B/C express physical tuning as a ratio of the played pitch.
    length = knob (id (resLength, "_ratio"), slot == 0 ? "Length" : "Ratio");
    keyTrack = knob (id (resKeyTrack, "_keytrack"), "Key Track");
    mode = control<ChoiceBox> (processor, id (resMode, "_type"), "Model");
    tubeCaption = caption ("RESONANCE / CHARACTER");
    feedback = knob (id (resFeedback, "_feedback"), "Feedback", additive (dest (ModDest::Feedback, ModDest::ResBFeedback, ModDest::ResCFeedback)), 64);
    damping = knob (id (resDamping, "_damping"), "Damping", additive (dest (ModDest::Damping, ModDest::ResBDamping, ModDest::ResCDamping)), 64);
    brightness = knob (id (resBrightness, "_brightness"), "Brightness", additive (dest (ModDest::Brightness, ModDest::ResBBrightness, ModDest::ResCBrightness)), 64);
    dispersion = knob (id (resDispersion, "_dispersion"), "Dispersion", slot == 0 ? additive (ModDest::Dispersion) : Knob::ModMapping(), 64);
    shape = knob (id (resShape, "_shape"), "Shape", slot == 0 ? additive (ModDest::Shape) : Knob::ModMapping());
    reflection = knob (id (resReflection, "_reflect"), "Reflection", slot == 0 ? additive (ModDest::Reflection) : Knob::ModMapping());
    saturation = knob (id (resSaturation, "_saturation"), "Saturation");
    for (auto& k : knobs) k->setAccentColour (getAccent());
}

void ResonatorPanel::resized()
{
    auto r = getContentArea();
    mode->setBounds (r.removeFromTop (40));
    r.removeFromTop (4);
    tuneCaption->setBounds (r.removeFromTop (14));
    knobRow (r.removeFromTop (72), { coarse, fine, length, keyTrack });
    r.removeFromTop (4);
    tubeCaption->setBounds (r.removeFromTop (14));
    knobRow (r.removeFromTop (84), { feedback, damping, brightness, dispersion });
    knobRow (r.removeFromTop (72), { shape, reflection, saturation });
}
} // namespace aeriform
