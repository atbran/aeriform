#pragma once

#include <array>
#include <cmath>

namespace aeriform::dsp
{
/**
    Two-path polyphase IIR halfband filter (six second-order allpass sections,
    ~ -100 dB stop band, transition ~0.1 fs). Used for 2x interpolation and
    decimation; two instances cascade for 4x. Allocation-free.
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
    struct Section { float x1 = 0.0f, x2 = 0.0f, y1 = 0.0f, y2 = 0.0f; };
    static constexpr float kA0[3] = { 0.07986642623635751f, 0.5453536510711322f, 0.9027213749126163f };
    static constexpr float kA1[3] = { 0.28382934487410993f, 0.7346814251043284f, 0.9773334741405917f };

    static inline float run (std::array<Section, 3>& path, const float* coef, float x) noexcept
    {
        for (int i = 0; i < 3; ++i)
        {
            auto& s = path[(size_t) i];
            const float y = coef[i] * (x + s.y2) - s.x2;
            s.x2 = s.x1; s.x1 = x;
            s.y2 = s.y1; s.y1 = y;
            x = y;
        }
        return x;
    }

    std::array<Section, 3> path0 {}, path1 {};
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
