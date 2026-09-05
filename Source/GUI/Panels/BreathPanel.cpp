#include "BreathPanel.h"

namespace aeriform
{
BreathPanel::BreathPanel (AeriformProcessor& p) : ParamPanel (p, "BREATH", theme::copper)
{
    using namespace ids;
    excCaption = caption ("EXCITER");
    noise      = knob (excNoise, additive (ModDest::Noise), theme::knobSizeLarge);
    color      = knob (excNoiseColor, additive (ModDest::NoiseColor));
    pressure   = knob (excPressure, additive (ModDest::Pressure), theme::knobSizeLarge);
    reed       = knob (excReed);
    pluck      = knob (excPluck);
    input      = knob (excExternalIn);
    input->setDisplayName ("Sidechain");

    pluckLen   = knob (excPluckLength);
    lowpass    = knob (excLowpass, exponential (ModDest::ExciterLP, 4.0f));
    highpass   = knob (excHighpass, exponential (ModDest::ExciterHP, 4.0f));
    turbulence = knob (excTurbulence, additive (ModDest::Turbulence));
    velocity   = knob (excVelocity);
    keyTrack   = knob (excKeyTrack);
    keyTrack->setDisplayName ("Key Track");

    envCaption = caption ("BREATH ENVELOPE");
    attack     = knob (envAttack);
    decay      = knob (envDecay);
    sustain    = knob (envSustain);
    release    = knob (envRelease);
    velPress   = knob (envVelToPressure);   velPress->setDisplayName ("Vel>Press");
    pressBright= knob (artPressBright);     pressBright->setDisplayName ("Press>Bright");

    artCaption   = caption ("ARTICULATION");
    transient    = knob (excAttackClick, {}, theme::knobSizeSmall);   transient->setDisplayName ("Transient");
    releaseNoise = knob (excReleaseNoise, {}, theme::knobSizeSmall);  releaseNoise->setDisplayName ("Rel. Noise");
    breathRandom = knob (excBreathRandom, {}, theme::knobSizeSmall);  breathRandom->setDisplayName ("Breath Rnd");
    flowPitch    = knob (artFlowPitch, {}, theme::knobSizeSmall);     flowPitch->setDisplayName ("Flow>Pitch");
    instability  = knob (artInstability, {}, theme::knobSizeSmall);
    variation    = knob (artVariation, {}, theme::knobSizeSmall);
    coupling     = knob (artCoupling, {}, theme::knobSizeSmall);
}

void BreathPanel::resized()
{
    auto r = getContentArea();
    const int capH = 14;
    const int rowH = 80, rowSmall = 66;

    excCaption->setBounds (r.removeFromTop (capH));
    knobRow (r.removeFromTop (rowH), { noise, color, pressure, reed, pluck, input });
    knobRow (r.removeFromTop (rowH - 6), { pluckLen, lowpass, highpass, turbulence, velocity, keyTrack });
    r.removeFromTop (4);
    envCaption->setBounds (r.removeFromTop (capH));
    knobRow (r.removeFromTop (rowH - 6), { attack, decay, sustain, release, velPress, pressBright });
    r.removeFromTop (4);
    artCaption->setBounds (r.removeFromTop (capH));
    knobRow (r.removeFromTop (rowSmall), { transient, releaseNoise, breathRandom, flowPitch, instability, variation, coupling });
}
} // namespace aeriform
