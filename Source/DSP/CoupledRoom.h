#pragma once
#include "DspUtils.h"
#include "FractionalDelay.h"
#include <array>
namespace aeriform::dsp {
struct RoomParams {
    bool enabled=false,freeze=false,clear=false;
    float size=.3f,shape=.5f,wallDamping=.4f,diffusion=.7f,airAbsorption=.3f,send=.4f,networkReturn=.2f,returnDelayMs=15,returnFilterHz=3000,feedback=.6f,width=1,level=.3f;
};
/** Shared short-room FDN with an explicitly bounded return to the voices.
    The return ring is sampled before each <=32-sample voice segment, so its
    minimum causal delay is independent of the host buffer size. */
class CoupledRoom {
public:
    static constexpr int lines=8,quantum=32;
    // Voiced output belongs to the instrument room. Shimmer explicitly keeps the legacy core.
    void prepare(float sampleRate,bool audibleVoicing=true);
    void reset() noexcept;
    void update(RoomParams,int samples=32) noexcept;
    bool active() const noexcept {return p.enabled||wet>1e-7f;}
    void makeReturn(float* destination,int samples,int voices) noexcept;
    void next(float inputL,float inputR,int voices,float& outputL,float& outputR) noexcept;
    float energy() const noexcept {return energyValue;}
    float returnEnergy() const noexcept {return returnValue;}
    int safetyClips() const noexcept {return clipCount;}
    float minimumReturnDelay() const noexcept {return (float)quantum;}
private:
    std::array<FractionalDelay,lines> delay;
    std::array<OnePole,lines> wall;
    std::array<float,lines> targetLength{},length{};
    FractionalDelay returnRing;
    std::array<FractionalDelay,2> earlyDelay;
    std::array<OnePole,2> earlyTone;
    bool voiced=true;
    OnePole returnLowpass;
    DcBlocker returnHighpass;
    RoomParams p;
    float sr=48000,wet=0,freezeAmount=0,step=.001f,lengthSmooth=.001f,returnDelay=720,returnDelayTarget=720,energyValue=0,returnValue=0;
    bool previousClear=false,wasActive=false;
    int clipCount=0;
    float returnVoiceGain=1;
    void clearBuffers() noexcept;
};
}
