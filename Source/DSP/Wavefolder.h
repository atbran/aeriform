#pragma once

#include "DspUtils.h"
#include "../Params/ParameterLayout.h"

namespace aeriform::dsp
{
/**
    Wavefolder with seven original folding functions, symmetry, bias, 1-4
    cascaded stages, continuous shape, wet / dry mix, output compensation and a
    post low-pass. Runs at the oversampled rate; every fold function is bounded
    and the DC created by bias / asymmetry is removed after folding.

    foldSample() is a pure function so the GUI can draw the exact transfer curve.
*/
class Wavefolder
{
public:
    struct Params
    {
        bool on = false;
        FoldMode mode = FoldMode::Smooth;
        float fold = 0.3f, drive = 0.2f, symmetry = 0.0f, bias = 0.0f, shape = 0.5f, mix = 1.0f, comp = 1.0f, postLpHz = 20000.0f;
        int stages = 1;
    };

    // ---- pure folding functions (x already includes drive / fold gain) ------
    static inline float triFold (float x) noexcept
    {
        // reflect into [-1, 1]: period 4
        float t = (x + 1.0f) * 0.25f;
        t -= std::floor (t);
        return 4.0f * std::fabs (t - 0.5f) - 1.0f;
    }
    static inline float wrapFold (float x) noexcept
    {
        float t = (x + 1.0f) * 0.5f;
        t -= std::floor (t);
        return 2.0f * t - 1.0f;
    }
    static inline float sineFold (float x, float shape) noexcept
    {
        return std::sin (kPi * 0.5f * x + shape * 0.8f * std::sin (kPi * x));
    }
    static inline float chebyshevFold (float x, float fold, float shape) noexcept
    {
        const float u = fastTanh (x);
        const float t2 = 2.0f * u * u - 1.0f;
        const float t3 = 4.0f * u * u * u - 3.0f * u;
        const float t4 = 8.0f * u * u * u * u - 8.0f * u * u + 1.0f;
        const float t5 = 16.0f * u * u * u * u * u - 20.0f * u * u * u + 5.0f * u;
        const float even = shape, odd = 1.0f - shape;
        const float f = clamp01 (fold);
        const float w1 = 1.0f - 0.6f * f;
        const float w2 = f * even * 0.8f, w3 = f * odd * 0.8f, w4 = f * f * even * 0.6f, w5 = f * f * odd * 0.6f;
        const float y = w1 * u + w2 * t2 + w3 * t3 + w4 * t4 + w5 * t5;
        return y / (w1 + w2 + w3 + w4 + w5);
    }
    static inline float diodeFold (float x, float shape) noexcept
    {
        // asymmetric thresholds with soft knees: positive folds at 1, negative at a lower threshold
        const float thPos = 1.0f, thNeg = 1.0f - 0.7f * shape;
        const float kneeW = 0.15f + 0.35f * shape;
        auto knee = [kneeW] (float v, float th)
        {
            const float over = v - th;
            if (over <= -kneeW) return v;
            if (over >= kneeW)  return th - over;                           // reflected
            const float t = (over + kneeW) / (2.0f * kneeW);                 // smooth turn-around
            return th - kneeW + 2.0f * kneeW * (t - t * t);
        };
        if (x >= 0.0f) return knee (x, thPos);
        return -knee (-x, thNeg);
    }

    /** One fold stage of mode `m` at input x (gain already applied). */
    static inline float foldStage (FoldMode m, float x, float fold, float shape) noexcept
    {
        switch (m)
        {
            case FoldMode::Smooth:    return lerp (triFold (x), std::sin (kPi * 0.5f * x), 0.3f + 0.7f * shape);
            case FoldMode::Triangle:  return triFold (x * (1.0f + 0.3f * shape) + 0.2f * shape);
            case FoldMode::Sine:      return sineFold (x, shape);
            case FoldMode::Diode:     return std::clamp (diodeFold (x, shape), -3.0f, 3.0f);
            case FoldMode::Chebyshev: return chebyshevFold (x, fold, shape);
            case FoldMode::Hard:      return lerp (triFold (x), wrapFold (x), shape);
            case FoldMode::Hybrid:    return lerp (fastTanh (x), triFold (x), shape);
            default:                  return triFold (x);
        }
    }

    /** Complete static transfer function (no filters / DC block): used by the GUI display and tests. */
    static float foldSample (const Params& p, float x) noexcept
    {
        const float g = (1.0f + 7.0f * p.fold * p.fold * p.fold + 3.0f * p.fold) * (1.0f + 3.0f * p.drive);
        const float gPos = g * (1.0f + 0.8f * p.symmetry), gNeg = g * (1.0f - 0.8f * p.symmetry);
        float y = (x >= 0.0f ? x * gPos : x * gNeg) + p.bias * 0.9f;
        for (int s = 0; s < std::clamp (p.stages, 1, 4); ++s)
        {
            y = foldStage (p.mode, y, p.fold, p.shape);
            if (s + 1 < p.stages) y *= 1.0f + 1.5f * p.fold;
        }
        const float compGain = lerp (1.0f, 1.0f / std::sqrt (g), p.comp);
        return lerp (x, y * compGain, p.mix);
    }

    // ---- real-time processing --------------------------------------------------
    void prepare (float sampleRate) noexcept { sr = sampleRate; dc.setCutoff (8.0f, sr); reset(); }
    void reset() noexcept { dc.reset(); postLP.reset(); }
    void update (const Params& p) noexcept
    {
        params = p;
        params.stages = std::clamp (p.stages, 1, 4);
        postLP.setCutoff (std::clamp (p.postLpHz, 100.0f, sr * 0.45f), sr);
        gain = (1.0f + 7.0f * p.fold * p.fold * p.fold + 3.0f * p.fold) * (1.0f + 3.0f * p.drive);
        gainPos = gain * (1.0f + 0.8f * p.symmetry);
        gainNeg = gain * (1.0f - 0.8f * p.symmetry);
        compGain = lerp (1.0f, 1.0f / std::sqrt (gain), p.comp);
        biasValue = p.bias * 0.9f;
        // static DC of the bias through one stage, removed before the blocker for faster settling
        biasOut = foldStage (p.mode, biasValue, p.fold, p.shape) * compGain * p.mix * 0.5f;
    }

    inline float next (float x) noexcept
    {
        const auto& p = params;
        if (! p.on) return x;
        float y = (x >= 0.0f ? x * gainPos : x * gainNeg) + biasValue;
        for (int s = 0; s < p.stages; ++s)
        {
            y = foldStage (p.mode, y, p.fold, p.shape);
            if (s + 1 < p.stages) y *= 1.0f + 1.5f * p.fold;
        }
        y = y * compGain - biasOut;
        y = lerp (x, y, p.mix);
        y = dc.process (y);
        if (p.postLpHz < 19000.0f) y = postLP.process (y);   // the maximum setting bypasses the filter
        return std::clamp (y, -4.0f, 4.0f);
    }

    bool isActive() const noexcept { return params.on; }

private:
    float sr = 88200.0f;
    Params params;
    DcBlocker dc;
    OnePole postLP;
    float gain = 1.0f, gainPos = 1.0f, gainNeg = 1.0f, compGain = 1.0f, biasValue = 0.0f, biasOut = 0.0f;
};
} // namespace aeriform::dsp
