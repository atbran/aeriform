#include "OutputStage.h"

namespace aeriform::dsp
{
namespace
{
    constexpr float kThreshold = 0.92f;
}

void OutputStage::prepare (double sr)
{
    sampleRate = (float) sr;
    hpL.setSampleRate (sampleRate);
    hpR.setSampleRate (sampleRate);
    hpL.set (24.0f, 0.707f);
    hpR.set (24.0f, 0.707f);
    gainSmooth.setCutoff (12.0f, sampleRate);
    releaseCoef = 1.0f - std::exp (-1.0f / (0.12f * sampleRate));
    reset();
}

void OutputStage::reset()
{
    hpL.reset();
    hpR.reset();
    gainSmooth.reset (gainTarget);
    envelope = 0.0f;
    limiterGain = 1.0f;
}

void OutputStage::setParams (float gainDb, float highpassHz, bool limiterOn) noexcept
{
    gainTarget = dbToGain (std::clamp (gainDb, -80.0f, 24.0f));
    hpL.set (highpassHz, 0.707f);
    hpR.set (highpassHz, 0.707f);
    limiter = limiterOn;
}

void OutputStage::process (float* left, float* right, int numSamples) noexcept
{
    for (int i = 0; i < numSamples; ++i)
    {
        const float g = gainSmooth.process (gainTarget);
        float l = hpL.highpass (sanitize (left[i])) * g;
        float r = hpR.highpass (sanitize (right[i])) * g;

        if (limiter)
        {
            // peak follower: instant attack, slow release
            const float peak = std::max (std::fabs (l), std::fabs (r));
            if (peak > envelope) envelope = peak;
            else                 envelope += releaseCoef * (peak - envelope);
            limiterGain = envelope > kThreshold ? kThreshold / envelope : 1.0f;
            l *= limiterGain;
            r *= limiterGain;
            // gentle safety saturation for anything that still overshoots (attack transients)
            l = fastTanh (l * 0.85f) * 1.1765f;
            r = fastTanh (r * 0.85f) * 1.1765f;
        }
        else
        {
            // limiter off: still never emit anything beyond +/- 4 (safety)
            l = std::clamp (l, -4.0f, 4.0f);
            r = std::clamp (r, -4.0f, 4.0f);
        }
        left[i] = l;
        right[i] = r;
    }
}
} // namespace aeriform::dsp
