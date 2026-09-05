#pragma once

#include "../DspUtils.h"
#include "../FractionalDelay.h"
#include <array>

namespace aeriform::dsp
{
/**
    8-line feedback-delay-network reverb (Householder mixing) with input
    diffusion, per-line damping, slow delay modulation, pre-delay and width.
    Tuned for long, smooth tails behind wind and drone sounds.
*/
class FdnReverb
{
public:
    static constexpr int kLines = 8;
    static constexpr int kDiffusers = 4;

    void prepare (double sampleRate);
    void reset();
    void setParams (float mix, float size, float decay, float damping, float preDelayMs, float width, float modulation) noexcept;
    void process (float* left, float* right, int numSamples) noexcept;

private:
    float sampleRate = 44100.0f;
    std::array<FractionalDelay, kLines> lines;
    std::array<OnePole, kLines> damp;
    std::array<float, kLines> lineLength {}, lineLengthCur {}, lineGain {}, lineOut {};
    float lenSmooth = 0.001f;
    std::array<FractionalDelay, kDiffusers> diffusers;
    std::array<int, kDiffusers> diffLength {};
    FractionalDelay preDelayL, preDelayR;
    DcBlocker dcL, dcR;
    OnePole mixSmooth;
    float mixTarget = 0.0f, width = 1.0f, modDepth = 0.0f, preDelaySamples = 0.0f;
    float lfoPhase = 0.0f, lfoInc = 0.0f;
    float sizeScale = 1.0f, t60Seconds = 2.0f;
    bool coefsDirty = true;

    void recomputeGains() noexcept;
};
} // namespace aeriform::dsp
