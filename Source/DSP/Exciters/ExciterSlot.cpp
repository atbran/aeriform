#include "ExciterSlot.h"

namespace aeriform::dsp
{
void ExciterSlot::prepare (float osRate, int oversampleFactor, int vIndex, int sIndex)
{
    sr = osRate;
    osFactor = std::max (1, oversampleFactor);
    voiceIndex = vIndex;
    slotIndex = sIndex;
    const uint32_t seed = 0x2545F491u + (uint32_t) vIndex * 0x9E3779B9u + (uint32_t) sIndex * 0x7F4A7C15u;
    rng.seed (seed);
    breathModel.prepare (osRate, seed ^ 0x51ED2Fu);
    wave.prepare (osRate);
    complex.prepare (osRate);
    complex.seed (seed ^ 0xC0FFEEu);
    noise.prepare (osRate, osFactor);
    physical.prepare (osRate);
    physical.seed (seed ^ 0xBEEFu);
    scLp.setSampleRate (osRate); scHp.setSampleRate (osRate);
    scLp.set (20000.0f, 0.707f); scHp.set (20.0f, 0.707f);
    scTransLP.setCutoff (1500.0f, osRate);
    toneLP.setCutoff (1200.0f, osRate);
    driftRnd.seed (seed ^ 0xD41F7u);
    driftRnd.setRate (0.4f, osRate);
    breathComp = std::sqrt ((float) osFactor) * (osFactor > 1 ? 1.15f : 1.0f);   // matches the v0.1 level after decimation
    varTune = rng.next(); varTone = rng.next(); varParam = rng.next();
    reset();
}

void ExciterSlot::reset()
{
    breathModel.reset();
    wave.reset (0.0f);
    complex.reset (0.0f);
    noise.reset();
    physical.reset();
    scLp.reset(); scHp.reset(); scTransLP.reset(); toneLP.reset();
    scEnv = outEnv = lastOut = 0.0f;
}

void ExciterSlot::noteOn (float vel, float midiNote, const Params& p)
{
    velocity = clamp01 (vel);
    randomPhase = rng.next01();
    const float startPhase = p.retrig == RetrigMode::Random ? randomPhase : p.phaseDeg / 360.0f;
    if (p.retrig != RetrigMode::Free)
    {
        wave.reset (startPhase);
        complex.reset (startPhase);
    }
    noise.seed (p.nz.seed, voiceIndex * 2 + slotIndex);
    noise.reset();
    physical.trigger (velocity);
    breathModel.update (p.breath, midiNoteToHz (midiNote), 0.0f, 0.0f);
    breathModel.noteOn (velocity, midiNoteToHz (midiNote));
}

void ExciterSlot::noteOff()
{
    breathModel.noteOff();
}

void ExciterSlot::update (const Params& p, float midiNote, float envelope, float pressureNow, float aftertouch, float vel, float)
{
    params = p;
    if (p.model != lastModel)
    {
        // model switch: clear the state of the newly selected model so no stale energy leaks in
        lastModel = p.model;
        if (p.model == ExciterModel::Complex) complex.reset (0.0f);
        if (isPhysical (p.model)) { physical.reset(); physical.trigger (velocity); }
        if (isNoise (p.model)) { noise.seed (p.nz.seed, voiceIndex * 2 + slotIndex); noise.reset(); }
    }
    velocity = clamp01 (vel);
    selfSustaining = isPhysical (p.model) && physical.isSelfSustaining();
    envGate = 0.0f;

    // pitch: key tracking around middle C, slot tuning, matrix pitch, drift, per-voice variation
    const float tracked = 60.0f + (midiNote - 60.0f) * p.keyTrack;
    const float driftCents = p.drift * 25.0f * driftRnd.next() + p.variation * 6.0f * varTune;
    const float semis = tracked + p.coarse + p.fine / 100.0f + p.pitchModSemis + driftCents / 100.0f;
    freqHz = midiNoteToHz (std::clamp (semis, -24.0f, 140.0f));
    driftOct = 0.0f;
    wave.setFrequency (freqHz);
    complex.setFrequency (freqHz);

    // level: velocity and pressure response
    const float velGain = lerp (1.0f, 0.15f + 0.85f * velocity, p.velAmount);
    const float pressGain = 1.0f + p.pressAmount * aftertouch;
    gain = p.level * velGain * pressGain;

    // tone tilt (+/- 9 dB) with variation
    const float tone = std::clamp (p.tone + p.variation * 0.15f * varTone, -1.0f, 1.0f);
    toneHF = std::exp2 (tone * 3.0f);
    toneLP.setCutoff (std::clamp (900.0f * std::pow (freqHz / 261.63f, 0.3f), 300.0f, 6000.0f), sr);

    switch (p.model)
    {
        case ExciterModel::Breath:
        {
            // the breath model's own filters are bypassed: the pre-shaper performs that filtering now
            ExciterParams b = p.breath;
            b.lowpassHz = 1.0e6f; b.highpassHz = 1.0f; b.keyTrack = 0.0f; b.pressureBright = 0.0f;
            b.noise = clamp01 (b.noise * p.level);
            breathModel.update (b, midiNoteToHz (midiNote), pressureNow, 0.0f);
            break;
        }
        case ExciterModel::Sidechain:
            scLp.set (std::clamp (p.scLp, 50.0f, sr * 0.45f), 0.707f);
            scHp.set (std::clamp (p.scHp, 5.0f, sr * 0.3f), 0.707f);
            break;
        default:
            if (isNoise (p.model))
            {
                NoiseLab::Params n = p.nz;
                n.color = std::clamp (n.color + p.variation * 0.2f * varParam, -1.0f, 1.0f);
                noise.update (p.model, n, p.keyTrack > 0.01f ? freqHz : 0.0f);
                noise.setColorCutoff (std::clamp (2000.0f * std::pow (freqHz / 261.63f, 0.3f), 200.0f, 8000.0f));
            }
            else if (isPhysical (p.model))
            {
                PhysicalExciter::Params q = p.ph;
                q.speed = clamp01 (q.speed * (1.0f + p.pressAmount * aftertouch) + p.variation * 0.05f * varParam);
                physical.update (p.model, q, freqHz, selfSustaining ? std::max (envelope, 0.0f) : 1.0f);
            }
            break;
    }
    juce::ignoreUnused (pressureNow);
}
} // namespace aeriform::dsp
