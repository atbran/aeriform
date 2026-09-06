#pragma once
#include "../DspUtils.h"
#include "../FractionalDelay.h"
#include <array>
namespace aeriform::dsp {
struct ResonantDelayParams {
    bool enabled=false;
    int type=0;
    float timeMs=375,feedback=.5f,tuningHz=220,damping=.4f,dispersion=0,amount=.7f,saturation=.2f,stereoOffsetMs=7,mix=.3f;
};
/** Stereo delay with a contracting six-mode processor inside each feedback path.
    All storage is prepared in advance; bypass fades then releases stored energy. */
class ResonantDelay {
public:
    void prepare(float sampleRate);
    void reset() noexcept;
    void setParams(ResonantDelayParams next) noexcept;
    void process(float* left,float* right,int samples) noexcept;
    float modeFrequency(int mode) const noexcept {return frequencies[(size_t)std::clamp(mode,0,5)];}
    int safetyClips() const noexcept {return clips;}
private:
    struct Mode {double re=0,im=0;float a=0,b=0,input=0,weight=0;};
    std::array<std::array<Mode,6>,2> modes{};
    std::array<float,6> frequencies{};
    std::array<FractionalDelay,2> delays;
    ResonantDelayParams target,current;
    float sr=48000,wet=0,step=.001f,smooth=.001f,controlSmooth=.001f,time=18000;
    int counter=0,clips=0;
    bool wasActive=false;
    void coefficients() noexcept;
    void clear() noexcept;
    float colour(float input,int channel) noexcept;
};
}
