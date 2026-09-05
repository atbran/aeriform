#include "Reverb.h"

namespace aeriform::dsp
{
namespace
{
    // base delay lengths (samples @ 44.1 kHz) - mutually prime-ish, spread over ~28..63 ms
    constexpr float kBaseLengths[FdnReverb::kLines] = { 1237.0f, 1381.0f, 1607.0f, 1811.0f, 2053.0f, 2251.0f, 2503.0f, 2791.0f };
    constexpr int   kDiffLengths[FdnReverb::kDiffusers] = { 142, 107, 379, 277 };
    constexpr float kDiffCoef = 0.62f;
}

void FdnReverb::prepare (double sr)
{
    sampleRate = (float) sr;
    const float scale = sampleRate / 44100.0f;
    for (int i = 0; i < kLines; ++i)
    {
        lines[(size_t) i].prepare ((int) (kBaseLengths[i] * scale * 1.7f) + 64);
        damp[(size_t) i].setCutoff (6000.0f, sampleRate);
    }
    for (int i = 0; i < kDiffusers; ++i)
    {
        diffLength[(size_t) i] = std::max (4, (int) (kDiffLengths[i] * scale));
        diffusers[(size_t) i].prepare (diffLength[(size_t) i] + 8);
    }
    preDelayL.prepare ((int) (0.25f * sampleRate) + 16);
    preDelayR.prepare ((int) (0.25f * sampleRate) + 16);
    dcL.setCutoff (20.0f, sampleRate);
    dcR.setCutoff (20.0f, sampleRate);
    mixSmooth.setCutoff (8.0f, sampleRate);
    lfoInc = 0.23f / sampleRate;
    lenSmooth = 1.0f - std::exp (-1.0f / (0.08f * sampleRate));
    coefsDirty = true;
    recomputeGains();
    lineLengthCur = lineLength;
    reset();
}

void FdnReverb::reset()
{
    for (auto& l : lines) l.clear();
    for (auto& d : diffusers) d.clear();
    for (auto& d : damp) d.reset();
    preDelayL.clear();
    preDelayR.clear();
    dcL.reset(); dcR.reset();
    lineOut.fill (0.0f);
    mixSmooth.reset (mixTarget);
}

void FdnReverb::setParams (float mix, float size, float decay, float damping, float preDelayMs, float w, float modulation) noexcept
{
    mixTarget = clamp01 (mix);
    const float newSize = 0.45f + 1.2f * clamp01 (size);
    const float newT60 = 0.4f * std::pow (60.0f, clamp01 (decay));      // 0.4 s .. 24 s
    if (std::fabs (newSize - sizeScale) > 1.0e-4f || std::fabs (newT60 - t60Seconds) > 1.0e-4f)
    {
        sizeScale = newSize;
        t60Seconds = newT60;
        coefsDirty = true;
    }
    const float dampHz = 12000.0f * std::pow (1500.0f / 12000.0f, clamp01 (damping));
    for (auto& d : damp) d.setCutoff (dampHz, sampleRate);
    preDelaySamples = std::clamp (preDelayMs * 0.001f * sampleRate, 1.0f, 0.24f * sampleRate);
    width = clamp01 (w);
    modDepth = clamp01 (modulation) * 9.0f * sampleRate / 44100.0f;
}

void FdnReverb::recomputeGains() noexcept
{
    const float scale = sampleRate / 44100.0f;
    for (int i = 0; i < kLines; ++i)
    {
        lineLength[(size_t) i] = kBaseLengths[i] * scale * sizeScale;
        // gain for a T60 decay: g = 10^(-3 L / (T60 fs))
        lineGain[(size_t) i] = std::pow (10.0f, -3.0f * lineLength[(size_t) i] / (t60Seconds * sampleRate));
    }
    coefsDirty = false;
}

void FdnReverb::process (float* left, float* right, int numSamples) noexcept
{
    if (mixTarget <= 0.0005f && mixSmooth.getState() <= 0.0005f)
        return;
    if (coefsDirty) recomputeGains();

    constexpr float householder = -2.0f / (float) kLines;

    for (int i = 0; i < numSamples; ++i)
    {
        const float mix = mixSmooth.process (mixTarget);
        const float inL = left[i], inR = right[i];

        preDelayL.push (inL);
        preDelayR.push (inR);
        float x = 0.5f * (preDelayL.readLinear (preDelaySamples) + preDelayR.readLinear (preDelaySamples));

        // input diffusion (Schroeder allpasses)
        for (int d = 0; d < kDiffusers; ++d)
        {
            const float delayed = diffusers[(size_t) d].readInteger (diffLength[(size_t) d]);
            const float v = x - kDiffCoef * delayed;
            diffusers[(size_t) d].push (v);
            x = delayed + kDiffCoef * v;
        }

        // slow modulation of a few lines removes metallic ringing
        lfoPhase += lfoInc;
        if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
        const float m1 = std::sin (lfoPhase * kTwoPi) * modDepth;
        const float m2 = std::sin ((lfoPhase + 0.37f) * kTwoPi) * modDepth;

        // read all lines
        float sum = 0.0f;
        for (int l = 0; l < kLines; ++l)
        {
            const float mod = (l == 1) ? m1 : (l == 5) ? m2 : (l == 3) ? -m1 * 0.7f : (l == 7) ? -m2 * 0.7f : 0.0f;
            lineLengthCur[(size_t) l] += (lineLength[(size_t) l] - lineLengthCur[(size_t) l]) * lenSmooth;
            const float out = lines[(size_t) l].readLagrange (lineLengthCur[(size_t) l] + mod);
            lineOut[(size_t) l] = damp[(size_t) l].process (out) * lineGain[(size_t) l];
            sum += lineOut[(size_t) l];
        }

        // Householder feedback: y_i = x_i + householder * sum
        const float mixTerm = householder * sum;
        for (int l = 0; l < kLines; ++l)
        {
            const float fb = lineOut[(size_t) l] + mixTerm;
            lines[(size_t) l].push (fastTanh (fb + x * (l % 2 == 0 ? 0.5f : -0.5f)));
        }

        float wetL = 0.0f, wetR = 0.0f;
        for (int l = 0; l < kLines; ++l)
        {
            if (l % 2 == 0) wetL += lineOut[(size_t) l];
            else            wetR += lineOut[(size_t) l];
        }
        wetL *= 0.5f; wetR *= 0.5f;
        const float mid = 0.5f * (wetL + wetR);
        const float side = 0.5f * (wetL - wetR) * width;
        wetL = dcL.process (mid + side);
        wetR = dcR.process (mid - side);

        const float dryGain = 1.0f - 0.4f * mix;
        left[i]  = inL * dryGain + wetL * mix;
        right[i] = inR * dryGain + wetR * mix;
    }
}
} // namespace aeriform::dsp
