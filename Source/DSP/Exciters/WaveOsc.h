#pragma once

#include "../DspUtils.h"

namespace aeriform::dsp
{
/**
    Band-limited morphing oscillator (runs at the oversampled rate).
    Shape 0..1 morphs sine -> triangle -> saw -> pulse; PolyBLEP corrects the
    saw and pulse discontinuities. Includes phase distortion, a sine sub
    oscillator, hard sync input and per-sample FM / PM inputs.
*/
class WaveOsc
{
public:
    void prepare (float sampleRate) noexcept { sr = sampleRate; }
    void setFrequency (float hz) noexcept { inc = std::clamp (hz, 0.0f, sr * 0.45f) / sr; }
    void reset (float phase01) noexcept { phase = phase01 - std::floor (phase01); subPhase = phase * 0.5f; wrappedFlag = false; }
    void hardSync() noexcept { phase = 0.0f; }
    bool wrapped() const noexcept { return wrappedFlag; }
    float getPhase() const noexcept { return phase; }

    /** shape 0..1, pw 0.05..0.95, sub 0..1, pd 0..1, fmOctaves: pitch offset, pmPhase: phase offset (cycles). */
    inline float next (float shape, float pw, float sub, float pd, float fmOctaves, float pmPhase) noexcept
    {
        const float step = inc * std::exp2 (std::clamp (fmOctaves, -6.0f, 6.0f));
        wrappedFlag = false;
        phase += step;
        if (phase >= 1.0f) { phase -= 1.0f; wrappedFlag = true; }
        subPhase += step * 0.5f;
        if (subPhase >= 1.0f) subPhase -= 1.0f;

        float p = phase + pmPhase;
        p -= std::floor (p);

        // primitives
        const float pdAmount = 0.5f * (1.0f - std::clamp (pd, 0.0f, 1.0f) * 0.92f);
        const float pdPhase = p < pdAmount ? p * (0.5f / pdAmount) : 0.5f + (p - pdAmount) * (0.5f / (1.0f - pdAmount));
        const float sine = std::sin (kTwoPi * pdPhase);
        const float tri  = 4.0f * std::fabs (p - 0.5f) - 1.0f;
        const float saw  = (2.0f * p - 1.0f) - polyBlep (p, step);
        float pulse = p < pw ? 1.0f : -1.0f;
        pulse += polyBlep (p, step);
        float p2 = p - pw; p2 -= std::floor (p2);
        pulse -= polyBlep (p2, step);

        float out;
        const float s = std::clamp (shape, 0.0f, 1.0f) * 3.0f;
        if (s < 1.0f)      out = lerp (sine, tri, s);
        else if (s < 2.0f) out = lerp (tri, saw, s - 1.0f);
        else               out = lerp (saw, pulse * 0.9f, s - 2.0f);

        if (sub > 0.0005f)
            out = out * (1.0f - 0.5f * sub) + sub * std::sin (kTwoPi * subPhase);
        return out;
    }

private:
    static inline float polyBlep (float t, float dt) noexcept
    {
        if (dt <= 0.0f) return 0.0f;
        if (t < dt)            { t /= dt; return t + t - t * t - 1.0f; }
        if (t > 1.0f - dt)     { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
        return 0.0f;
    }

    float sr = 88200.0f, inc = 0.0f, phase = 0.0f, subPhase = 0.0f;
    bool wrappedFlag = false;
};
} // namespace aeriform::dsp
