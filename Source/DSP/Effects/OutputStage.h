#pragma once

#include "../DspUtils.h"

namespace aeriform::dsp
{
/** Final high-pass / DC blocker, smoothed output gain and a soft peak limiter. */
class OutputStage
{
public:
    void prepare (double sampleRate);
    void reset();
    void setParams (float gainDb, float highpassHz, bool limiterOn) noexcept;
    void process (float* left, float* right, int numSamples) noexcept;

    struct Meter { float prePeak=0, postPeak=0, preRms=0, postRms=0, preMean=0, postMean=0, limitedFraction=0, ceilingFraction=0; };
    const Meter& getMeter() const noexcept { return meter; }
    float getLimiterGain() const noexcept { return limiterGain; }

private:
    Meter meter;
    float sampleRate = 44100.0f;
    SVF hpL, hpR;
    OnePole gainSmooth;
    float gainTarget = 1.0f;
    bool limiter = true;
    float envelope = 0.0f, limiterGain = 1.0f, releaseCoef = 0.001f;
};
} // namespace aeriform::dsp
