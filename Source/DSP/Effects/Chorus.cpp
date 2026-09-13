#include "Chorus.h"
#include <algorithm>

namespace aeriform::dsp
{

void Chorus::prepare (double sr)
{
    sampleRate = (float) sr;
    const int maxDelay = (int) (0.040f * sampleRate) + 16;
    delayL.prepare (maxDelay);
    delayR.prepare (maxDelay);
    mixSmooth.setCutoff (8.0f, sampleRate);
    depthSmooth.setCutoff (8.0f, sampleRate);
    baseDelay = 0.008f * sampleRate;
    xfadeRamp = 1.0f - std::exp (-1.0f / (0.020f * sampleRate));

    juno.prepare (sr);
    reset();
}

void Chorus::reset()
{
    delayL.clear();
    delayR.clear();
    mixSmooth.reset (mixTarget);
    depthSmooth.reset (depthTarget);
    phase = 0.0f;

    juno.reset();
    currentType = targetType;
    xfade = (currentType == ChorusType::Ensemble) ? 0.0f : 1.0f;
}

void Chorus::setType (ChorusType type) noexcept
{
    if (targetType == type)
        return;

    targetType = type;
    switch (type)
    {
        case ChorusType::JunoI:     juno.setMode (JunoChorus::Mode::JunoI);     break;
        case ChorusType::JunoII:    juno.setMode (JunoChorus::Mode::JunoII);    break;
        case ChorusType::JunoDual:  juno.setMode (JunoChorus::Mode::JunoDual);  break;
        case ChorusType::Dimension: juno.setMode (JunoChorus::Mode::Dimension); break;
        case ChorusType::Ensemble:
        default:                    juno.setMode (JunoChorus::Mode::Off);       break;
    }
}

void Chorus::setParams (float mix, float rateHz, float depth, float w) noexcept
{
    mixTarget = clamp01 (mix);
    depthTarget = clamp01 (depth);
    inc = std::clamp (rateHz, 0.01f, 10.0f) / sampleRate;
    width = clamp01 (w);
    depthSamples = 0.004f * sampleRate;

    juno.setMix (mixTarget);
    juno.setRateScale (rateHz / 0.4f);
    juno.setDepth (depth / 0.4f);
    juno.setWidth (width);
}

void Chorus::processEnsemble (float* left, float* right, int numSamples) noexcept
{
    constexpr float tapPhase[kTaps] = { 0.0f, 0.333333f, 0.666667f };
    constexpr float tapRate[kTaps]  = { 1.0f, 0.83f, 1.19f };

    for (int i = 0; i < numSamples; ++i)
    {
        const float mix = mixSmooth.process (mixTarget);
        const float dep = depthSmooth.process (depthTarget);
        const float inL = left[i], inR = right[i];
        delayL.push (inL + 0.15f * inR);
        delayR.push (inR + 0.15f * inL);

        phase += inc;
        if (phase >= 1.0f) phase -= 1.0f;

        float wetL = 0.0f, wetR = 0.0f;
        for (int t = 0; t < kTaps; ++t)
        {
            const float ph = phase * tapRate[t] + tapPhase[t];
            const float lfoL = std::sin (ph * kTwoPi);
            const float lfoR = std::sin ((ph + 0.25f * width) * kTwoPi);
            const float dL = baseDelay * (1.0f + 0.35f * (float) t) + depthSamples * dep * lfoL;
            const float dR = baseDelay * (1.0f + 0.35f * (float) t) + depthSamples * dep * lfoR;
            wetL += delayL.readLagrange (dL);
            wetR += delayR.readLagrange (dR);
        }
        wetL *= 0.45f;
        wetR *= 0.45f;

        const float dryGain = 1.0f - 0.5f * mix;
        left[i]  = inL * dryGain + wetL * mix;
        right[i] = inR * dryGain + wetR * mix;
    }
}

void Chorus::process (float* left, float* right, int numSamples) noexcept
{
    if (mixTarget <= 0.0005f && mixSmooth.getState() <= 0.0005f)
        return;

    const float targetXfade = (targetType == ChorusType::Ensemble) ? 0.0f : 1.0f;

    // Fast-path: steady state Ensemble
    if (targetType == ChorusType::Ensemble && currentType == ChorusType::Ensemble && xfade <= 0.0001f)
    {
        processEnsemble (left, right, numSamples);
        return;
    }

    // Fast-path: steady state Juno/Dimension
    if (targetType != ChorusType::Ensemble && currentType != ChorusType::Ensemble && xfade >= 0.9999f)
    {
        juno.process (left, right, numSamples);
        return;
    }

    // Crossfade between Ensemble and Juno/Dimension
    float origL[128], origR[128];
    float ensL[128], ensR[128];
    float junL[128], junR[128];

    for (int offset = 0; offset < numSamples; offset += 128)
    {
        const int n = std::min (128, numSamples - offset);
        float* L = left + offset;
        float* R = right + offset;

        std::copy (L, L + n, origL);
        std::copy (R, R + n, origR);
        std::copy (origL, origL + n, ensL);
        std::copy (origR, origR + n, ensR);
        std::copy (origL, origL + n, junL);
        std::copy (origR, origR + n, junR);

        processEnsemble (ensL, ensR, n);
        juno.process (junL, junR, n);

        for (int i = 0; i < n; ++i)
        {
            xfade += (targetXfade - xfade) * xfadeRamp;
            L[i] = (1.0f - xfade) * ensL[i] + xfade * junL[i];
            R[i] = (1.0f - xfade) * ensR[i] + xfade * junR[i];
        }
    }

    if (std::abs (xfade - targetXfade) < 0.001f)
    {
        xfade = targetXfade;
        currentType = targetType;
    }
}

} // namespace aeriform::dsp
