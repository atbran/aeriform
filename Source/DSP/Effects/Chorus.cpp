#include "Chorus.h"

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
    reset();
}

void Chorus::reset()
{
    delayL.clear();
    delayR.clear();
    mixSmooth.reset (mixTarget);
    depthSmooth.reset (depthTarget);
    phase = 0.0f;
}

void Chorus::setParams (float mix, float rateHz, float depth, float w) noexcept
{
    mixTarget = clamp01 (mix);
    depthTarget = clamp01 (depth);
    inc = std::clamp (rateHz, 0.01f, 10.0f) / sampleRate;
    width = clamp01 (w);
    depthSamples = 0.004f * sampleRate;
}

void Chorus::process (float* left, float* right, int numSamples) noexcept
{
    if (mixTarget <= 0.0005f && mixSmooth.getState() <= 0.0005f)
        return;

    constexpr float tapPhase[kTaps] = { 0.0f, 0.333333f, 0.666667f };
    constexpr float tapRate[kTaps]  = { 1.0f, 0.83f, 1.19f };

    for (int i = 0; i < numSamples; ++i)
    {
        const float mix = mixSmooth.process (mixTarget);
        const float depth = depthSmooth.process (depthTarget);
        const float inL = left[i], inR = right[i];
        const float mono = 0.5f * (inL + inR);
        delayL.push (inL + 0.15f * inR);
        delayR.push (inR + 0.15f * inL);

        phase += inc;
        if (phase >= 1.0f) phase -= 1.0f;

        float wetL = 0.0f, wetR = 0.0f;
        for (int t = 0; t < kTaps; ++t)
        {
            const float ph = phase * tapRate[t] + tapPhase[t];
            const float lfoL = std::sin (ph * kTwoPi);
            // right channel LFO is shifted by 90 degrees scaled by width (0 = identical, 1 = quadrature)
            const float lfoR = std::sin ((ph + 0.25f * width) * kTwoPi);
            const float dL = baseDelay * (1.0f + 0.35f * (float) t) + depthSamples * depth * lfoL;
            const float dR = baseDelay * (1.0f + 0.35f * (float) t) + depthSamples * depth * lfoR;
            wetL += delayL.readLagrange (dL);
            wetR += delayR.readLagrange (dR);
        }
        wetL *= 0.45f;
        wetR *= 0.45f;

        const float dryGain = 1.0f - 0.5f * mix;
        left[i]  = inL * dryGain + wetL * mix;
        right[i] = inR * dryGain + wetR * mix;
        juce::ignoreUnused (mono);
    }
}
} // namespace aeriform::dsp
