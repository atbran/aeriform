#pragma once

#include "../DspUtils.h"
#include "../FractionalDelay.h"
#include "RoomReverb.h"
#include "DattorroPlate.h"
#include "../../Params/ParameterLayout.h"
#include <array>

namespace aeriform::dsp
{

/**
    Multi-algorithm Reverb engine:
    - Hall: 8-line feedback-delay-network (Householder mixing) with input diffusion,
            per-line damping, delay modulation, pre-delay, and width.
    - Room: Early reflection FIR cluster + tight diffuse tank.
    - Plate: Classic Dattorro figure-8 plate with high echo density and metallic shimmer.
*/
class FdnReverb
{
public:
    static constexpr int kLines = 8;
    static constexpr int kDiffusers = 4;

    void prepare (double sampleRate);
    void reset();
    void setType (ReverbType type) noexcept;
    void setParams (float mix, float size, float decay, float damping, float preDelayMs, float width, float modulation) noexcept;
    void process (float* left, float* right, int numSamples) noexcept;

private:
    void processHall (float* left, float* right, int numSamples) noexcept;
    void recomputeGains() noexcept;

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

    RoomReverb room;
    DattorroPlate plate;
    ReverbType currentType = ReverbType::Hall;
    ReverbType targetType  = ReverbType::Hall;
    float xfadeCur[3] { 1.0f, 0.0f, 0.0f };
    float xfadeRamp = 0.005f;
};

using Reverb = FdnReverb;

} // namespace aeriform::dsp
