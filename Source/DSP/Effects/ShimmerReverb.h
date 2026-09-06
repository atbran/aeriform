#pragma once
#include "../CoupledRoom.h"
#include "PitchShift.h"
namespace aeriform::dsp {
struct ShimmerParams {
    bool enabled=false;
    float semitones=12,feedback=.55f,diffusion=.8f,damping=.3f,size=.7f,spread=1,lowCutHz=150,highCutHz=8000,mix=.3f;
};
/** Shared FDN core with a filtered pitch-shifted outer return. The return is
    bounded independently of the master limiter and the original reverb. */
class ShimmerReverb {
public:
    void prepare(float sampleRate);
    void reset() noexcept;
    void setParams(ShimmerParams,int samples=256) noexcept;
    void process(float* left,float* right,int samples) noexcept;
    int safetyClips() const noexcept {return room.safetyClips();}
private:
    CoupledRoom room;
    std::array<PitchShift,2> shift;
    std::array<OnePole,2> lowpass,highpass;
    ShimmerParams p;
    float sr=48000,wet=0,step=.001f,feedback=0,feedbackSmooth=.001f;
    std::array<float,2> returned{};
    bool wasActive=false;
};
}
