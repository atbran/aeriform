#include "Delay.h"

namespace aeriform::dsp
{
void StereoDelay::prepare (double sr)
{
    sampleRate = (float) sr;
    maxDelaySamples = 2.6f * sampleRate;
    delayL.prepare ((int) maxDelaySamples + 16);
    delayR.prepare ((int) maxDelaySamples + 16);
    toneL.setCutoff (4500.0f, sampleRate);
    toneR.setCutoff (4500.0f, sampleRate);
    hpL.setCutoff (90.0f, sampleRate);
    hpR.setCutoff (90.0f, sampleRate);
    timeSmooth.setCutoff (1.5f, sampleRate);    // slow, tape-like time changes
    mixSmooth.setCutoff (10.0f, sampleRate);
    fbSmooth.setCutoff (10.0f, sampleRate);
    reset();
}

void StereoDelay::reset()
{
    delayL.clear();
    delayR.clear();
    toneL.reset(); toneR.reset(); hpL.reset(); hpR.reset();
    timeSmooth.reset (timeTarget);
    mixSmooth.reset (mixTarget);
    fbSmooth.reset (feedbackTarget);
}

void StereoDelay::setParams (float mix, float timeMs, float feedback, float toneHz, bool pp) noexcept
{
    mixTarget = clamp01 (mix);
    timeTarget = std::clamp (timeMs * 0.001f * sampleRate, 2.0f, maxDelaySamples);
    feedbackTarget = std::clamp (feedback, 0.0f, 0.97f);
    toneL.setCutoff (toneHz, sampleRate);
    toneR.setCutoff (toneHz, sampleRate);
    pingPong = pp;
}

void StereoDelay::process (float* left, float* right, int numSamples) noexcept
{
    if (mixTarget <= 0.0005f && mixSmooth.getState() <= 0.0005f)
    {
        // keep the lines flowing (so a re-enable does not replay stale audio abruptly) but cheaply
        for (int i = 0; i < numSamples; ++i) { delayL.push (0.0f); delayR.push (0.0f); }
        timeSmooth.reset (timeTarget);
        return;
    }

    for (int i = 0; i < numSamples; ++i)
    {
        const float mix = mixSmooth.process (mixTarget);
        const float fb = fbSmooth.process (feedbackTarget);
        const float t = timeSmooth.process (timeTarget);

        const float inL = left[i], inR = right[i];
        float outL = delayL.readLagrange (t);
        float outR = delayR.readLagrange (t);

        // tone shaping in the feedback path
        outL = hpL.processHighpass (toneL.process (outL));
        outR = hpR.processHighpass (toneR.process (outR));

        if (pingPong)
        {
            const float mono = 0.5f * (inL + inR);
            delayL.push (fastTanh (mono + outR * fb));
            delayR.push (fastTanh (outL * fb));
        }
        else
        {
            delayL.push (fastTanh (inL + outL * fb));
            delayR.push (fastTanh (inR + outR * fb));
        }

        const float dryGain = 1.0f - 0.35f * mix;
        left[i]  = inL * dryGain + outL * mix;
        right[i] = inR * dryGain + outR * mix;
    }
}
} // namespace aeriform::dsp
