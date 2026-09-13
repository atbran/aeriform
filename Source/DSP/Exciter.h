#pragma once

#include "DspUtils.h"

namespace aeriform::dsp
{
struct ExciterParams
{
    float mouth=.35f, swellMs=65, settleMs=280, contour=.4f, edge=0, texturePressure=.4f;
    float noise = 0.6f;          // 0..1 continuous noise level
    float noiseColor = 0.35f;    // 0 white .. 1 pink
    float pluck = 0.0f;          // 0..1 impulse burst level
    float pluckLengthMs = 5.0f;
    float lowpassHz = 7000.0f;
    float highpassHz = 40.0f;
    float turbulence = 0.12f;
    float velocityAmount = 0.5f;
    float externalIn = 0.0f;
    float keyTrack = 0.5f;
    float attackClick = 0.02f;
    float releaseNoise = 0.025f;
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

        mouthNow += smooth * (mouthTarget-mouthNow);
        edgeNow += smooth * (edgeTarget-edgeNow);
        if ((shapeTick++ & 15u)==0) {
            const float drift = 1.0f + 0.03f * slowDrift.next();
            const float f1Hz = 320.0f + 500.0f * std::sin (kPi * mouthNow);
            const float f2Hz = 850.0f * std::exp2 (1.6f * mouthNow);
            const float f3Hz = 2200.0f + 1100.0f * mouthNow;
            const float edgeHz = 4800.0f + 2000.0f * mouthNow;
            f1Filter.set (std::clamp (f1Hz * drift, 50.0f, sampleRate * 0.45f), 3.0f);
            f2Filter.set (std::clamp (f2Hz * drift, 100.0f, sampleRate * 0.45f), 3.5f);
            f3Filter.set (std::clamp (f3Hz * drift, 200.0f, sampleRate * 0.45f), 3.0f);
            edgeFilter.set (std::clamp (edgeHz, 300.0f, sampleRate * 0.45f), 2.0f);
        }
        // Resonant vocal tract formant coloration plus embouchure/edge air band.
        const float f1 = f1Filter.bandpass (breathNoise);
        const float f2 = f2Filter.bandpass (breathNoise);
        const float f3 = f3Filter.bandpass (breathNoise);
        const float edgeBand = edgeFilter.bandpass (white);
        breathNoise = 0.9f * f1 + 0.7f * f2 + 0.4f * f3 + (edgeNow * 0.6f) * edgeBand;
        onset += onsetRate*(1-onset);
        emphasis *= settleRate;
        contourNow += smooth*(contourAmount-contourNow);
        airGainNow += smooth*(noiseGain-airGainNow);
        const float airContour=onset*(1+contourNow*emphasis);

        // turbulence: slow chaotic amplitude / pressure fluctuation
        const float turb = slowTurb.next();
        const float turbGain = std::clamp(1.0f + turbAmount * (0.9f * turb + 0.35f * fastTurb.next()),.1f,2.0f);
        breathNoise *= turbGain;

        // slow breath drift (human unsteadiness)
        const float drift = 1.0f + breathRandomAmount * 0.25f * slowDrift.next();
        float out = breathNoise * airGainNow * drift * breath * airContour;

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
    SVF lpFilter, hpFilter, f1Filter, f2Filter, f3Filter, edgeFilter;
    float smooth=.001f,mouthNow=.35f,mouthTarget=.35f,edgeNow=0,edgeTarget=0;
    float onset=0,emphasis=1,onsetRate=.001f,settleRate=.999f,contourAmount=.4f;
    unsigned shapeTick=0; float contourNow=.4f,airGainNow=0;

    float color = 0.0f, noiseGain = 0.0f, turbAmount = 0.0f, breathRandomAmount = 0.0f, externalGain = 0.0f;
    float velocity = 1.0f;
    float noteRandom = 1.0f;

    int pluckRemaining = 0; float pluckLevel = 0.0f, pluckEnv = 1.0f, pluckDecay = 0.99f;
    int clickRemaining = 0, clickLength = 1; float clickLevel = 0.0f, clickEnv = 1.0f, clickDecay = 0.99f;
    int puffRemaining = 0; float puffLevel = 0.0f, puffEnv = 1.0f, puffDecay = 0.999f;

    ExciterParams cached;
};
} // namespace aeriform::dsp

