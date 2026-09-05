#pragma once

#include "../DspUtils.h"
#include "../FractionalDelay.h"

namespace aeriform::dsp
{
/** Three-tap stereo ensemble chorus with quadrature LFOs. */
class Chorus
{
public:
    void prepare (double sampleRate);
    void reset();
    void setParams (float mix, float rateHz, float depth, float width) noexcept;
    void process (float* left, float* right, int numSamples) noexcept;

private:
    static constexpr int kTaps = 3;
    float sampleRate = 44100.0f;
    FractionalDelay delayL, delayR;
    OnePole mixSmooth, depthSmooth;
    float phase = 0.0f, inc = 0.0f, width = 0.8f;
    float mixTarget = 0.0f, depthTarget = 0.0f;
    float baseDelay = 300.0f, depthSamples = 100.0f;
};
} // namespace aeriform::dsp
