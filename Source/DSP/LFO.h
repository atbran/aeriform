#pragma once

#include "DspUtils.h"
#include "../Params/ParameterLayout.h"

namespace aeriform::dsp
{
/** Multi-shape LFO with fade-in. Output in [-1, 1]. */
class LFO
{
public:
    void setSampleRate (float sr) noexcept { sampleRate = sr; }
    void seed (uint32_t s) noexcept { rng.seed (s); held = rng.next(); heldPrev = rng.next(); }

    void setShape (LfoShape s) noexcept { shape = s; }
    void setRate (float hz) noexcept { inc = std::clamp (hz, 0.0f, sampleRate * 0.45f) / sampleRate; }
    void setFadeMs (float ms) noexcept { fadeInc = ms <= 0.0f ? 1.0f : 1.0f / std::max (1.0f, ms * 0.001f * sampleRate); }

    /** Restart at the given phase (0..1) and restart the fade-in. */
    void retrigger (float startPhase01) noexcept
    {
        phase = startPhase01 - std::floor (startPhase01);
        fade = 0.0f;
        held = rng.next(); heldPrev = rng.next();
        lastSegment = -1;
    }

    /** Set phase without restarting the fade (used to align free-running LFOs). */
    void setPhase (float p) noexcept { phase = p - std::floor (p); }
    float getPhase() const noexcept { return phase; }
    void resetFade() noexcept { fade = 1.0f; }

    /** Advance by n samples at once (used for block-rate evaluation). */
    inline float advance (int numSamples) noexcept
    {
        const float step = inc * (float) numSamples;
        phase += step;
        if (phase >= 1.0f) { phase -= std::floor (phase); ++cycleCount; }
        fade = std::min (1.0f, fade + fadeInc * (float) numSamples);
        return value() * fade;
    }

    inline float value() noexcept
    {
        switch (shape)
        {
            case LfoShape::Sine:     return std::sin (phase * kTwoPi);
            case LfoShape::Triangle: return 1.0f - 4.0f * std::fabs (phase - 0.5f);
            case LfoShape::SawUp:    return 2.0f * phase - 1.0f;
            case LfoShape::SawDown:  return 1.0f - 2.0f * phase;
            case LfoShape::Square:   return phase < 0.5f ? 1.0f : -1.0f;
            case LfoShape::SampleHold:
            {
                const int seg = cycleCount;
                if (seg != lastSegment) { lastSegment = seg; heldPrev = held; held = rng.next(); }
                return held;
            }
            case LfoShape::SmoothRandom:
            {
                const int seg = cycleCount;
                if (seg != lastSegment) { lastSegment = seg; heldPrev = held; held = rng.next(); }
                const float t = phase * phase * (3.0f - 2.0f * phase);
                return heldPrev + (held - heldPrev) * t;
            }
            case LfoShape::Count:
            default: return 0.0f;
        }
    }

private:
    float sampleRate = 44100.0f, phase = 0.0f, inc = 0.0f, fade = 1.0f, fadeInc = 1.0f;
    float held = 0.0f, heldPrev = 0.0f;
    int cycleCount = 0, lastSegment = -1;
    LfoShape shape = LfoShape::Sine;
    Noise rng;
};
} // namespace aeriform::dsp
