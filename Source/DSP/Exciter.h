#pragma once

#include "DspUtils.h"

namespace aeriform::dsp
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
} // namespace aeriform::dsp
