#pragma once

#include "../DspUtils.h"
#include "../FractionalDelay.h"
#include "JunoChorus.h"
#include "../../Params/ParameterLayout.h"

namespace aeriform::dsp
{

/** Chorus effect coordinator supporting Ensemble, Juno I, Juno II, Juno I+II, and Dimension. */
class Chorus
{
public:
    void prepare (double sampleRate);
    void reset();
    void setType (ChorusType type) noexcept;
    void setParams (float mix, float rateHz, float depth, float width) noexcept;
    void process (float* left, float* right, int numSamples) noexcept;

private:
    void processEnsemble (float* left, float* right, int numSamples) noexcept;

    static constexpr int kTaps = 3;
    float sampleRate = 44100.0f;
    FractionalDelay delayL, delayR;
    OnePole mixSmooth, depthSmooth;
    float phase = 0.0f, inc = 0.0f, width = 0.8f;
    float mixTarget = 0.0f, depthTarget = 0.0f;
    float baseDelay = 300.0f, depthSamples = 100.0f;

    JunoChorus juno;
    ChorusType currentType = ChorusType::Ensemble;
    ChorusType targetType  = ChorusType::Ensemble;
    float xfade = 0.0f; // 0 = ensemble, 1 = juno/dimension
    float xfadeRamp = 0.005f;
};

} // namespace aeriform::dsp
