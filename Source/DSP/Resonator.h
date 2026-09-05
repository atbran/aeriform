#pragma once

#include "DspUtils.h"
#include "FractionalDelay.h"
#include "../Params/ParameterLayout.h"

namespace aeriform::dsp
{
struct ResonatorParams
{
    float freqHz = 440.0f;       // target fundamental (already includes tuning, bend, glide, modulation)
    float feedback = 0.9f;       // 0..1
    float damping = 0.35f;       // 0..1
    float brightness = 0.5f;     // 0..1
    float dispersion = 0.0f;     // 0..1
    float shape = 0.5f;          // 0..1 excitation position / bore shape
    float reflection = 0.3f;     // 0..1 end reflection (hard -> open)
    float saturation = 0.25f;    // 0..1
    ResMode mode = ResMode::OpenPipe;
    float bodyFreqHz = 900.0f;
    float bodyRes = 0.4f;
    float bodyMix = 0.3f;
    float reed = 0.0f;           // 0..1 reed / jet non-linearity at the junction
    float pressure = 0.0f;       // mouth pressure driving the reed (0..~1.2)
    float variationDamping = 0.0f;   // per-voice offsets (added by the voice)
    float variationBright = 0.0f;
};

/**
    Tuned digital waveguide: fractional delay loop with end-reflection loss,
    damping, dispersion allpasses, DC blocking, saturation and an optional reed
    non-linearity at the excitation junction. The loop length is compensated
    for the phase delay of every in-loop filter so the tube stays in tune.

    Stability: the linear loop gain never exceeds 1.0 and every in-loop filter is
    passive, the loop signal passes through a bounded saturator and the injected
    excitation is soft-limited, so energy is bounded for any parameter combination.
*/
class Resonator
{
public:
    static constexpr int kNumDispersionStages = 4;

    void prepare (float sampleRate);
    void reset();

    /** Control-rate update (once per sub-block). */
    void update (const ResonatorParams& p, bool snapLength);

    /** Runs one sample. excitation = exciter output (already filtered); pressureNow = breath pressure 0..1. Returns the tube output (pre-VCA). */
    inline float next (float excitation, float pressureNow) noexcept
    {
        // smooth the loop length towards its target (pitch glide / modulation without zipper noise)
        delayLen += (targetLen - delayLen) * lenSmooth;

        // ---- read the returning wave and apply in-loop losses ----------------
        float d = delay.readLagrange (delayLen);

        const float lp2 = reflLP.process (d);          // end reflection: HF radiated out of the open end
        d = lp2 + reflHF * (d - lp2);
        d = dampLP.process (d);                        // frequency-dependent damping
        if (ksBlend > 0.0f)                            // string: two-point average (extra HF loss)
        {
            const float avg = 0.5f * (d + ksPrev);
            ksPrev = d;
            d = lerp (d, avg, ksBlend);
        }
        if (dispersionActive)
            for (auto& ap : dispersion) d = ap.process (d);
        d = dcBlock.process (d);

        // saturation with pressure-dependent bias (asymmetry -> even harmonics under pressure)
        const float sat = (fastTanh (d * drive + satBias) - satBiasOut) * invDrive;
        const float reflected = sat * loopGain;

        // ---- excitation shaping: brightness tilt + position comb -----------
        const float tl = tiltLP.process (excitation);
        float in = tl + tiltHF * (excitation - tl);
        exciteDelay.push (in);
        in -= 0.85f * exciteDelay.readLinear (combDelay);
        in = fastTanh (in * 0.5f) * 2.0f;              // the mouth cannot inject unbounded pressure

        // ---- junction: linear injection blended with a reed non-linearity ---
        float x;
        if (reedAmount > 0.0f)
        {
            const float pMouth = pressureNow * 1.2f + in;
            const float dp = pMouth - reflected;
            const float r = std::clamp (0.7f - 0.3f * dp, -1.0f, 1.0f);
            const float reedOut = reflected + dp * r;
            x = lerp (reflected + in, reedOut, reedAmount);
        }
        else
        {
            x = reflected + in;
        }

        delay.push (x);
        energy += 0.002f * (std::fabs (x) - energy);

        // ---- output tap + body filter ---------------------------------------
        const float tap = stringMode ? d : x;
        const float bp = bodySVF.bandpass (tap) * bodyK;
        const float out = tap * (1.0f - 0.7f * bodyMix) + bp * bodyGain;
        lastOut = out * outputComp;
        return lastOut;
    }

    float getEnergy() const noexcept { return energy; }
    float getLastOutput() const noexcept { return lastOut; }
    float getDelayLength() const noexcept { return delayLen; }
    bool  isFinite() const noexcept { return std::isfinite (lastOut) && std::isfinite (delayLen) && std::isfinite (energy); }

private:
    float sampleRate = 44100.0f;
    FractionalDelay delay, exciteDelay;
    OnePole dampLP, reflLP, tiltLP;
    Allpass1 dispersion[kNumDispersionStages];
    DcBlocker dcBlock;
    SVF bodySVF;

    float delayLen = 100.0f, targetLen = 100.0f, lenSmooth = 0.01f;
    float reflHF = 1.0f, tiltHF = 1.0f, ksBlend = 0.0f, ksPrev = 0.0f;
    bool dispersionActive = false, stringMode = false;
    float drive = 1.0f, invDrive = 1.0f, satBias = 0.0f, satBiasOut = 0.0f;
    float loopGain = 0.9f, reedAmount = 0.0f;
    float combDelay = 20.0f;
    float bodyK = 1.0f, bodyGain = 0.0f, bodyMix = 0.0f, outputComp = 1.0f;
    float energy = 0.0f, lastOut = 0.0f;

    float loopPhaseDelay (float omega) const noexcept;
};
} // namespace aeriform::dsp
