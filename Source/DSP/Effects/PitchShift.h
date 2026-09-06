#pragma once
#include "../DspUtils.h"
#include "../FractionalDelay.h"
namespace aeriform::dsp {
/** Two overlapping variable-delay grains. Positive intervals read faster.
    A fixed 80 ms window trades transient smear for smooth feedback textures. */
class PitchShift {
public:
    void prepare(float sampleRate,float initialPhase=0) {
        sr=sampleRate;window=.08f*sr;delay.prepare((int)window+64);startPhase=initialPhase;ratioSmooth=1-std::exp(-1/(.03f*sr));reset();
    }
    void reset() noexcept {delay.clear();phase=startPhase;ratio=target;antiAlias.reset();}
    void setSemitones(float value) noexcept {
        target=std::exp2(std::clamp(value,-24.0f,24.0f)/12);antiAlias.setCutoff(sr*.4f/std::max(1.0f,target),sr);
    }
    float next(float input) noexcept {
        ratio+=(target-ratio)*ratioSmooth;delay.push(antiAlias.process(input));
        phase+=(1-ratio)/window;phase-=std::floor(phase);float other=phase+.5f;other-=std::floor(other);
        const float weight=.5f-.5f*std::cos(kTwoPi*phase);
        return weight*delay.readLinear(16+window*phase)+(1-weight)*delay.readLinear(16+window*other);
    }
private:
    FractionalDelay delay;OnePole antiAlias;
    float sr=48000,window=3840,phase=0,startPhase=0,ratio=2,target=2,ratioSmooth=.001f;
};
}
