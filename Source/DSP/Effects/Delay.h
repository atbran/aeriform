#pragma once

#include "../DspUtils.h"
#include "../FractionalDelay.h"

namespace aeriform::dsp
{
/** Stereo / ping-pong delay with tempo sync, tone filtering and smooth time changes. */
class StereoDelay
{
public:
    void prepare (double sampleRate);
    void reset();

    /** timeMs already resolved from sync/division by the caller. */
    void setParams (float mix, float timeMs, float feedback, float toneHz, bool pingPong) noexcept;
    void process (float* left, float* right, int numSamples) noexcept;

private:
    float sampleRate = 44100.0f;
    FractionalDelay delayL, delayR;
    OnePole toneL, toneR, hpL, hpR, timeSmooth, mixSmooth, fbSmooth;
    float mixTarget = 0.0f, timeTarget = 10000.0f, feedbackTarget = 0.3f;
    bool pingPong = true;
    float maxDelaySamples = 1.0f;
};
} // namespace aeriform::dsp
