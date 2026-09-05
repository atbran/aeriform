#pragma once
#include "DspUtils.h"
#include "FractionalDelay.h"
#include "../Params/ParameterLayout.h"
#include <array>
#include <complex>
namespace aeriform::dsp {
struct VoiceParams;
enum class FilterPosition { ExciterA,ExciterB,Combined,BeforeFolder,AfterFolder,NetworkInput,ResAInput,ResBInput,ResCInput,ResAOutput,ResBOutput,ResCOutput,CrossFeedback,EnergyLoop,ResALoop,ResBLoop,ResCLoop,PostBody,PreEffects,PostEffects,Count };
enum class FilterModel { Lowpass,Highpass,Bandpass,Notch,SVFMorph,DrivenSVF,Ladder,Formant,Comb,Modal,Tilt,Count };
struct MovableFilterParams {
    bool on=false; FilterPosition position=FilterPosition::NetworkInput;FilterModel model=FilterModel::Lowpass;
    float cutoff=4000,resonance=.1f,drive=0,keytrack=0,envelope=0,morph=.5f,mix=1;
    int slope=0,vowel=0;
};
/** Three movable blocks. Two independent filter states crossfade on model/position changes.
    Six lanes keep stereo networks and the three feedback destinations independent. */
class ModularFilters {
public:
    static constexpr int count=3,lanes=6,fields=12;
    void prepare(float sampleRate);
    void reset() noexcept;
    void update(const VoiceParams&,float midiNote=60,float envelope=0,int controlSamples=32);
    void set(int slot,MovableFilterParams,float midiNote=60,float envelope=0,int oversampling=1);
    void advance() noexcept;
    float at(FilterPosition,float input,int lane=0) noexcept;
    using FrameWeights=std::array<std::array<float,2>,count>;
    FrameWeights weights() const noexcept;
    float atWeighted(FilterPosition,float,int,const FrameWeights&) noexcept;
    float phaseDelay(FilterPosition,float fundamental) const noexcept;
    bool active() const noexcept {return anyActive;}
    static P parameter(int slot,int field) noexcept {return (P)((int)P::filter1On+slot*fields+field);}
private:
    struct Lane {
        SVF svf[2],formants[3];OnePole ladder[4];FractionalDelay comb;
        float previous=0,ladderOut=0,delay=100;bool combUsed=false;
        void reset() noexcept;
    };
    struct Engine {
        MovableFilterParams p;std::array<Lane,lanes> lane;float sr=48000,q=.707f,driveGain=1,loopNorm=1;
        void configure(MovableFilterParams,float rate);
        float next(float,int) noexcept;
        void reset() noexcept {for(auto& l:lane)l.reset();}
        std::complex<double> transfer(double frequency) const noexcept;
    };
    struct Block {std::array<Engine,2> engine;int current=0;bool initialized=false,transition=false;float fade=1,wet=0,targetWet=0;};
    std::array<Block,count> blocks;
    float sr=48000,step=.001f;int controlSamples=32;bool anyActive=false;
};
}
