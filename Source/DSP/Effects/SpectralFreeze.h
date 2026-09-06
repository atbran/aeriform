#pragma once
#include "FixedFFT.h"
#include "../DspUtils.h"
namespace aeriform::dsp {
struct SpectralParams {
    bool enabled=false,freeze=false,capture=false,release=false;
    float blur=0,semitones=0,randomPhase=0,decayMs=0,mix=1;
};
class SpectralFreeze {
public:
    static constexpr int size=2048,hop=512,bins=size/2+1;
    void prepare(float sampleRate);
    void reset() noexcept;
    void setParams(SpectralParams) noexcept;
    void process(float* left,float* right,int samples) noexcept;
    bool isFrozen() const noexcept {return holding&&captured;}
    float bandEnergy(int band) const noexcept;
    int captures() const noexcept {return captureCount;}
private:
    struct Channel {
        std::array<float,size> input{},output{};
        std::array<FixedFFT<11>::Complex,size> work{};
        std::array<float,bins> previousPhase{},magnitude{},omega{},phase{},blurred{};
    };
    FixedFFT<11> fft;
    std::array<Channel,2> channels{};
    std::array<float,size> window{};
    SpectralParams p;
    float sr=48000,wet=0,step=.001f,ratio=1;
    int write=0,untilFrame=hop,history=0,captureCount=0;
    bool holding=false,captured=false,pendingCapture=false,wasActive=false;
    uint32_t rng=0x5F00A123u;
    void frame() noexcept;
    float random() noexcept;
};
}
