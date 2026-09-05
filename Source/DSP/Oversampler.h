#pragma once

#include <array>
#include <cmath>

namespace aeriform::dsp
{
/**
    Two-path polyphase IIR halfband filter (eight second-order allpass sections,
    elliptic design: passband to 0.2 fs, > 100 dB stop band from 0.3 fs).
    Used for 2x interpolation and decimation; two instances cascade for 4x.
    Coefficients computed with the classic Valenzuela / Constantinides
    polyphase-IIR method (see scripts/gen_halfband.py). Allocation-free.
*/
class Halfband
{
public:
    void reset() noexcept
    {
        for (auto& s : path0) s = {};
        for (auto& s : path1) s = {};
    }

    /** Interpolate: one input sample -> two output samples (older first). */
    inline void up (float x, float& out0, float& out1) noexcept
    {
        out0 = run (path0, kA0, x);
        out1 = run (path1, kA1, x);
    }

    /** Decimate: two input samples (older first) -> one output sample. */
    inline float down (float x0, float x1) noexcept
    {
        return 0.5f * (run (path0, kA0, x1) + run (path1, kA1, x0));
    }

private:
    // Each allpass is (a + z^-2)/(1 + a z^-2) at the oversampled rate, i.e. first order in the
    // polyphase (decimated) domain in which these sections actually run.
    struct Section { float x1 = 0.0f, y1 = 0.0f; };
    static constexpr int kSections = 4;
    static constexpr float kA0[kSections] = { 0.0358327884310621f, 0.2720401433964576f, 0.5720571972357003f, 0.8271247619973240f };
    static constexpr float kA1[kSections] = { 0.1340901419430669f, 0.4243248712718685f, 0.7062921421386394f, 0.9415030941737551f };

    static inline float run (std::array<Section, kSections>& path, const float* coef, float x) noexcept
    {
        for (int i = 0; i < kSections; ++i)
        {
            auto& s = path[(size_t) i];
            const float y = coef[i] * (x - s.y1) + s.x1;
            s.x1 = x;
            s.y1 = y;
            x = y;
        }
        return x;
    }

    std::array<Section, kSections> path0 {}, path1 {};
};

/** 1x / 2x / 4x oversampling helper for a mono signal path. */
class Oversampler
{
public:
    static constexpr int kMaxFactor = 4;

    void setFactor (int f) noexcept { factor = f == 4 ? 4 : (f == 2 ? 2 : 1); reset(); }
    int  getFactor() const noexcept { return factor; }
    void reset() noexcept { stageA.reset(); stageB.reset(); upA.reset(); upB.reset(); }

    /** Upsample one input sample into `factor` output samples (older first). */
    inline void upsample (float x, float* out) noexcept
    {
        if (factor == 1) { out[0] = x; return; }
        float a, b;
        upA.up (x, a, b);
        if (factor == 2) { out[0] = a; out[1] = b; return; }
        upB.up (a, out[0], out[1]);
        upB.up (b, out[2], out[3]);
    }

    /** Downsample `factor` input samples (older first) into one output sample. */
    inline float downsample (const float* in) noexcept
    {
        if (factor == 1) return in[0];
        if (factor == 2) return stageA.down (in[0], in[1]);
        const float a = stageB.down (in[0], in[1]);
        const float b = stageB.down (in[2], in[3]);
        return stageA.down (a, b);
    }

private:
    int factor = 2;
    Halfband stageA, stageB, upA, upB;
};
} // namespace aeriform::dsp
