#pragma once

#include <cmath>
#include <cstdint>
#include <algorithm>
#include <juce_core/juce_core.h>

// Small, allocation-free DSP building blocks shared by the voice and effects code.
namespace aeriform::dsp
{
inline constexpr float kPi    = 3.14159265358979323846f;
inline constexpr float kTwoPi = 6.28318530717958647692f;

inline float midiNoteToHz (float note) noexcept
{
    return 440.0f * std::exp2 ((note - 69.0f) / 12.0f);
}

inline float centsToRatio (float cents) noexcept { return std::exp2 (cents / 1200.0f); }
inline float dbToGain (float db) noexcept { return std::pow (10.0f, db * 0.05f); }

inline float clamp01 (float v) noexcept { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
inline float lerp (float a, float b, float t) noexcept { return a + (b - a) * t; }

/** Bounded, cheap tanh approximation (max error ~2e-3 for |x|<4, exact limits +/-1). */
inline float fastTanh (float x) noexcept
{
    if (x > 4.97f)  return 1.0f;
    if (x < -4.97f) return -1.0f;
    const float x2 = x * x;
    return x * (135135.0f + x2 * (17325.0f + x2 * (378.0f + x2)))
             / (135135.0f + x2 * (62370.0f + x2 * (3150.0f + x2 * 28.0f)));
}

/** Replaces NaN / Inf / denormals with 0. */
inline float sanitize (float v) noexcept
{
    if (! std::isfinite (v)) return 0.0f;
    if (std::fabs (v) < 1.0e-20f) return 0.0f;
    return v;
}

// ---------------------------------------------------------------------------
/** Xorshift32 noise generator: white noise in [-1, 1]. */
class Noise
{
public:
    void seed (uint32_t s) noexcept { state = s == 0 ? 0x9E3779B9u : s; }
    inline uint32_t nextUInt() noexcept
    {
        uint32_t x = state;
        x ^= x << 13; x ^= x >> 17; x ^= x << 5;
        state = x;
        return x;
    }
    inline float next() noexcept { return (float) (nextUInt() >> 8) * (1.0f / 8388608.0f) - 1.0f; }
    inline float next01() noexcept { return (float) (nextUInt() >> 8) * (1.0f / 16777216.0f); }
private:
    uint32_t state = 0x12345678u;
};

/** Paul Kellet's economy pink-noise filter (input: white noise). */
class PinkFilter
{
public:
    inline float process (float white) noexcept
    {
        b0 = 0.99765f * b0 + white * 0.0990460f;
        b1 = 0.96300f * b1 + white * 0.2965164f;
        b2 = 0.57000f * b2 + white * 1.0526913f;
        const float pink = b0 + b1 + b2 + white * 0.1848f;
        return pink * 0.25f;
    }
    void reset() noexcept { b0 = b1 = b2 = 0.0f; }
private:
    float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f;
};

// ---------------------------------------------------------------------------
/** One-pole low-pass (also usable as a smoother). */
class OnePole
{
public:
    void setCutoff (float hz, float sampleRate) noexcept
    {
        const float f = std::clamp (hz, 1.0f, sampleRate * 0.49f);
        a = 1.0f - std::exp (-kTwoPi * f / sampleRate);
    }
    void setCoefficient (float coefficient) noexcept { a = std::clamp (coefficient, 0.0f, 1.0f); }
    float getCoefficient() const noexcept { return a; }
    inline float process (float x) noexcept { y += a * (x - y); return y; }
    inline float processHighpass (float x) noexcept { y += a * (x - y); return x - y; }
    void reset (float v = 0.0f) noexcept { y = v; }
    float getState() const noexcept { return y; }

    /** Phase delay (in samples) of this low-pass at the given normalised frequency (radians/sample). */
    float phaseDelay (float omega) const noexcept
    {
        // H(z) = a / (1 - (1-a) z^-1)
        const float b = 1.0f - a;
        const float re = 1.0f - b * std::cos (omega);
        const float im = b * std::sin (omega);
        const float phase = -std::atan2 (im, re);   // arg of denominator inverse
        return omega > 1.0e-6f ? -phase / omega : b / (1.0f - b);
    }
private:
    float a = 1.0f, y = 0.0f;
};

/** DC blocker: y = x - x1 + R * y1 */
class DcBlocker
{
public:
    void setCutoff (float hz, float sampleRate) noexcept { R = 1.0f - kTwoPi * std::clamp (hz, 0.2f, 200.0f) / sampleRate; }
    inline float process (float x) noexcept
    {
        const float y = x - x1 + R * y1;
        x1 = x; y1 = y;
        return y;
    }
    void reset() noexcept { x1 = y1 = 0.0f; }
    float phaseDelay (float omega) const noexcept
    {
        // H(z) = (1 - z^-1) / (1 - R z^-1)
        const float nre = 1.0f - std::cos (omega), nim = std::sin (omega);
        const float dre = 1.0f - R * std::cos (omega), dim = R * std::sin (omega);
        const float phase = std::atan2 (nim, nre) - std::atan2 (dim, dre);
        return omega > 1.0e-6f ? -phase / omega : 0.0f;
    }
private:
    float R = 0.995f, x1 = 0.0f, y1 = 0.0f;
};

/** First-order allpass: y = c*x + x1 - c*y1. Used in a chain for dispersion. */
class Allpass1
{
public:
    void setCoefficient (float coefficient) noexcept { c = std::clamp (coefficient, -0.95f, 0.95f); }
    inline float process (float x) noexcept
    {
        const float y = c * x + x1 - c * y1;
        x1 = x; y1 = y;
        return y;
    }
    void reset() noexcept { x1 = y1 = 0.0f; }
    float phaseDelay (float omega) const noexcept
    {
        // H(z) = (c + z^-1) / (1 + c z^-1)
        const float nre = c + std::cos (omega), nim = -std::sin (omega);
        const float dre = 1.0f + c * std::cos (omega), dim = -c * std::sin (omega);
        const float phase = std::atan2 (nim, nre) - std::atan2 (dim, dre);
        return omega > 1.0e-6f ? -phase / omega : (1.0f - c) / (1.0f + c);
    }
private:
    float c = 0.0f, x1 = 0.0f, y1 = 0.0f;
};

// ---------------------------------------------------------------------------
/** Cytomic / TPT state-variable filter with simultaneous LP / BP / HP outputs. */
class SVF
{
public:
    void setSampleRate (float sr) noexcept { sampleRate = sr; }
    void set (float cutoffHz, float q) noexcept
    {
        const float f = std::clamp (cutoffHz, 5.0f, sampleRate * 0.45f);
        g = std::tan (kPi * f / sampleRate);
        k = 1.0f / std::max (0.1f, q);
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }
    inline void process (float x, float& lp, float& bp, float& hp) noexcept
    {
        const float v3 = x - ic2eq;
        const float v1 = a1 * ic1eq + a2 * v3;
        const float v2 = ic2eq + a2 * ic1eq + a3 * v3;
        ic1eq = 2.0f * v1 - ic1eq;
        ic2eq = 2.0f * v2 - ic2eq;
        lp = v2; bp = v1; hp = x - k * v1 - v2;
    }
    inline float lowpass (float x) noexcept { float l, b, h; process (x, l, b, h); return l; }
    inline float highpass (float x) noexcept { float l, b, h; process (x, l, b, h); return h; }
    inline float bandpass (float x) noexcept { float l, b, h; process (x, l, b, h); return b; }
    void reset() noexcept { ic1eq = ic2eq = 0.0f; }
private:
    float sampleRate = 44100.0f, g = 0.1f, k = 1.0f, a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;
    float ic1eq = 0.0f, ic2eq = 0.0f;
};

// ---------------------------------------------------------------------------
/** Linear ramp smoother for control values (per-sample). */
class Ramp
{
public:
    void setTimeMs (float ms, float sampleRate) noexcept { steps = std::max (1, (int) (ms * 0.001f * sampleRate)); }
    void setImmediate (float v) noexcept { current = target = v; remaining = 0; }
    void setTarget (float t) noexcept
    {
        if (std::fabs (t - target) <= 1.0e-12f) return;
        target = t;
        remaining = steps;
        inc = (target - current) / (float) steps;
    }
    inline float next() noexcept
    {
        if (remaining > 0) { current += inc; if (--remaining == 0) current = target; }
        return current;
    }
    float get() const noexcept { return current; }
    float getTarget() const noexcept { return target; }
    bool isRamping() const noexcept { return remaining > 0; }
private:
    float current = 0.0f, target = 0.0f, inc = 0.0f;
    int steps = 64, remaining = 0;
};

/** Slow band-limited random source (smooth random / sample & hold). */
class SlowRandom
{
public:
    void seed (uint32_t s) noexcept { rng.seed (s); current = rng.next(); target = rng.next(); }
    void setRate (float hz, float sampleRate) noexcept { inc = std::clamp (hz / sampleRate, 0.0f, 0.5f); }
    /** Smoothly interpolated random in [-1, 1]. */
    inline float next() noexcept
    {
        phase += inc;
        if (phase >= 1.0f)
        {
            phase -= 1.0f;
            current = target;
            target = rng.next();
        }
        const float t = phase * phase * (3.0f - 2.0f * phase);
        return current + (target - current) * t;
    }
private:
    Noise rng;
    float phase = 0.0f, inc = 0.001f, current = 0.0f, target = 0.0f;
};
} // namespace aeriform::dsp
