#pragma once

#include "DSP/DspUtils.h"

namespace aeriform::dsp::baseline
{
struct ExciterParams
{
    float noise = 0.6f;          // 0..1 continuous noise level
    float noiseColor = 0.35f;    // 0 white .. 1 pink
    float pluck = 0.0f;          // 0..1 impulse burst level
    float pluckLengthMs = 5.0f;
    float lowpassHz = 7000.0f;
    float highpassHz = 40.0f;
    float turbulence = 0.25f;
    float velocityAmount = 0.5f;
    float externalIn = 0.0f;
    float keyTrack = 0.5f;
    float attackClick = 0.15f;
    float releaseNoise = 0.1f;
    float breathRandom = 0.15f;
    float pressureBright = 0.4f; // pressure-dependent brightness
};

/**
    Oscillator-free excitation source: breath noise (white/pink, turbulent),
    pluck / strike impulse bursts, tongue transient at note-on and a release
    puff, filtered by key-tracked low-pass / high-pass filters. Optionally mixes
    an external audio input as excitation.
*/
class Exciter
{
public:
    void prepare (float sampleRate, uint32_t seed);
    void reset();

    void noteOn (float velocity, float noteHz);
    void noteOff();

    /** Update per control block. pressureNow = current breath pressure (0..1) for pressure-dependent brightness. */
    void update (const ExciterParams& p, float noteHz, float pressureNow, float turbulenceMod);

    /** Next excitation sample. external = external-input sample (0 if none), breath = per-sample breath envelope (0..1+). */
    inline float next (float external, float breath) noexcept
    {
        // --- breath noise ---------------------------------------------------
        const float white = rng.next();
        const float pink = pinkFilter.process (white);
        float breathNoise = lerp (white, pink * 2.2f, color);

        // turbulence: slow chaotic amplitude / pressure fluctuation
        const float turb = slowTurb.next();
        const float turbGain = 1.0f + turbAmount * (0.9f * turb + 0.35f * fastTurb.next());
        breathNoise *= turbGain;

        // slow breath drift (human unsteadiness)
        const float drift = 1.0f + breathRandomAmount * 0.25f * slowDrift.next();
        float out = breathNoise * noiseGain * drift * breath;

        // --- one-shot components -------------------------------------------
        if (pluckRemaining > 0)
        {
            out += white * pluckLevel * pluckEnv;
            pluckEnv *= pluckDecay;
            --pluckRemaining;
        }
        if (clickRemaining > 0)
        {
            out += (white * 0.7f + (clickRemaining == clickLength ? 1.0f : 0.0f)) * clickLevel * clickEnv;
            clickEnv *= clickDecay;
            --clickRemaining;
        }
        if (puffRemaining > 0)
        {
            out += pink * 2.0f * puffLevel * puffEnv;
            puffEnv *= puffDecay;
            --puffRemaining;
        }

        out += external * externalGain * breath;

        // --- filters --------------------------------------------------------
        out = hpFilter.highpass (out);
        out = lpFilter.lowpass (out);
        return out;
    }

    bool hasPendingTransient() const noexcept { return pluckRemaining > 0 || clickRemaining > 0 || puffRemaining > 0; }

private:
    float sampleRate = 44100.0f;
    Noise rng;
    PinkFilter pinkFilter;
    SlowRandom slowTurb, fastTurb, slowDrift;
    SVF lpFilter, hpFilter;

    float color = 0.0f, noiseGain = 0.0f, turbAmount = 0.0f, breathRandomAmount = 0.0f, externalGain = 0.0f;
    float velocity = 1.0f;
    float noteRandom = 1.0f;

    int pluckRemaining = 0; float pluckLevel = 0.0f, pluckEnv = 1.0f, pluckDecay = 0.99f;
    int clickRemaining = 0, clickLength = 1; float clickLevel = 0.0f, clickEnv = 1.0f, clickDecay = 0.99f;
    int puffRemaining = 0; float puffLevel = 0.0f, puffEnv = 1.0f, puffDecay = 0.999f;

    ExciterParams cached;
};
} // namespace aeriform::dsp::baseline



namespace aeriform::dsp::baseline
{
void Exciter::prepare (float sr, uint32_t seed)
{
    sampleRate = sr;
    rng.seed (seed);
    slowTurb.seed (seed * 7u + 1u);
    fastTurb.seed (seed * 13u + 5u);
    slowDrift.seed (seed * 31u + 9u);
    slowTurb.setRate (6.0f, sr);
    fastTurb.setRate (45.0f, sr);
    slowDrift.setRate (0.35f, sr);
    lpFilter.setSampleRate (sr);
    hpFilter.setSampleRate (sr);
    lpFilter.set (7000.0f, 0.707f);
    hpFilter.set (40.0f, 0.707f);
    reset();
}

void Exciter::reset()
{
    pinkFilter.reset();
    lpFilter.reset();
    hpFilter.reset();
    pluckRemaining = clickRemaining = puffRemaining = 0;
    noiseGain = 0.0f;
}

void Exciter::noteOn (float vel, float noteHz)
{
    velocity = clamp01 (vel);
    noteRandom = 1.0f + cached.breathRandom * 0.3f * rng.next();

    const float velScale = lerp (1.0f, velocity * velocity, cached.velocityAmount);

    // pluck burst: noise with exponential decay over the burst length
    const int len = std::max (4, (int) (cached.pluckLengthMs * 0.001f * sampleRate));
    pluckRemaining = cached.pluck > 0.001f ? len : 0;
    pluckLevel = cached.pluck * 1.6f * velScale;
    pluckEnv = 1.0f;
    pluckDecay = std::exp (-4.0f / (float) len);

    // tongue / chiff transient: short bright click scaled by attack transient amount
    clickLength = std::max (8, (int) (0.0025f * sampleRate + 0.004f * sampleRate * (1.0f - clamp01 (noteHz / 2000.0f))));
    clickRemaining = cached.attackClick > 0.001f ? clickLength : 0;
    clickLevel = cached.attackClick * 0.9f * velScale;
    clickEnv = 1.0f;
    clickDecay = std::exp (-5.0f / (float) clickLength);

    puffRemaining = 0;
}

void Exciter::noteOff()
{
    if (cached.releaseNoise > 0.001f)
    {
        const int len = (int) (0.09f * sampleRate);
        puffRemaining = len;
        puffLevel = cached.releaseNoise * 0.35f * lerp (1.0f, velocity, cached.velocityAmount);
        puffEnv = 1.0f;
        puffDecay = std::exp (-4.5f / (float) len);
    }
}

void Exciter::update (const ExciterParams& p, float noteHz, float pressureNow, float turbulenceMod)
{
    cached = p;
    color = clamp01 (p.noiseColor);
    turbAmount = clamp01 (p.turbulence + turbulenceMod);
    breathRandomAmount = p.breathRandom;

    const float velScale = lerp (1.0f, 0.25f + 0.75f * velocity, p.velocityAmount);
    noiseGain = p.noise * 0.5f * velScale * noteRandom;
    externalGain = p.externalIn * 2.0f;

    // key tracking of both filters around middle C, plus pressure-dependent brightness
    const float track = std::pow (std::max (noteHz, 20.0f) / 261.63f, 0.6f * p.keyTrack);
    const float pressBright = std::exp2 (p.pressureBright * 2.5f * (pressureNow - 0.5f));
    const float lp = std::clamp (p.lowpassHz * track * pressBright, 60.0f, sampleRate * 0.45f);
    const float hp = std::clamp (p.highpassHz * track, 5.0f, sampleRate * 0.3f);
    lpFilter.set (lp, 0.65f);
    hpFilter.set (hp, 0.6f);
}
} // namespace aeriform::dsp::baseline
