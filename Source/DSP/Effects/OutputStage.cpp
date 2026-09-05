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
    meter = {};
    double preEnergy=0, postEnergy=0, preSum=0, postSum=0;
    int limited=0, ceiling=0;
    for (int i = 0; i < numSamples; ++i)
    {
        const float g = gainSmooth.process (gainTarget);
        float l = hpL.highpass (sanitize (left[i])) * g;
        float r = hpR.highpass (sanitize (right[i])) * g;

        meter.prePeak=std::max(meter.prePeak,std::max(std::abs(l),std::abs(r)));
        preEnergy+=(double)l*l+(double)r*r; preSum+=l+r;
        if (limiter)
        {
            // peak follower: instant attack, slow release
            const float peak = std::max (std::fabs (l), std::fabs (r));
            if (peak > envelope) envelope = peak;
            else                 envelope += releaseCoef * (peak - envelope);
            limiterGain = envelope > kThreshold ? kThreshold / envelope : 1.0f;
            if(limiterGain<0.98f) ++limited;
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
        meter.postPeak=std::max(meter.postPeak,std::max(std::abs(l),std::abs(r)));
        postEnergy+=(double)l*l+(double)r*r; postSum+=l+r;
        if(std::max(std::abs(l),std::abs(r))>0.76f) ++ceiling;
        left[i] = l;
        right[i] = r;
    }
    const double count=std::max(1,2*numSamples);
    meter.preRms=(float)std::sqrt(preEnergy/count); meter.postRms=(float)std::sqrt(postEnergy/count);
    meter.preMean=(float)(preSum/count); meter.postMean=(float)(postSum/count);
    meter.limitedFraction=(float)limited/(float)std::max(1,numSamples);
    meter.ceilingFraction=(float)ceiling/(float)std::max(1,numSamples);
}
} // namespace aeriform::dsp
