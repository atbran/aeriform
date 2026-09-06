#pragma once
#include <juce_dsp/juce_dsp.h>
#include "../DspUtils.h"
#include "../Oversampler.h"

namespace aeriform::dsp {
struct SaturationParams {
    struct Band {float drive=0,mix=1,output=0;int model=0;};
    bool enabled=false;
    float lowHz=250,highHz=2500,mix=1;
    int quality=1;
    std::array<Band,3> bands{};
};
/** LR4 split with the low band's upper-crossover allpass compensation.
    Neutral bands sum to a flat-magnitude allpass, not a sample-aligned raw wire.
    Dry and distorted band paths use identical oversampling filters. Bypass is
    an exact wire after the short enable/quality fade. All storage is prepared. */
class MultibandSaturation {
public:
    void prepare(float rate){
        sr=rate;step=1/(.02f*sr);smooth=1-std::exp(-1/(.015f*sr));
        juce::dsp::ProcessSpec spec{rate,1,2};
        lowSplit.prepare(spec);highSplit.prepare(spec);lowPhase.prepare(spec);
        lowPhase.setType(juce::dsp::LinkwitzRileyFilterType::allpass);
        for(auto& b:bands)for(auto& c:b.channels)c.dc.setCutoff(8,sr);
        reset();
    }
    void reset() noexcept {
        lowSplit.reset();highSplit.reset();lowPhase.reset();active=false;wet=0;mix=1;tick=0;
        lowHz=250;highHz=2500;quality=std::clamp(p.quality,0,2);setQuality(quality);
        for(auto& b:bands){b.drive=1;b.mix=1;b.output=1;b.weights={1,0,0,0};}
        updateCrossovers();
    }
    void setParams(SaturationParams next) noexcept {
        next.lowHz=std::clamp(next.lowHz,40.0f,4000.0f);next.highHz=std::clamp(next.highHz,200.0f,std::min(16000.0f,sr*.45f));
        next.lowHz=std::min(next.lowHz,next.highHz/1.25f);next.mix=clamp01(next.mix);next.quality=std::clamp(next.quality,0,2);
        for(auto& b:next.bands){b.drive=std::clamp(b.drive,0.0f,36.0f);b.mix=clamp01(b.mix);b.output=std::clamp(b.output,-24.0f,12.0f);b.model=std::clamp(b.model,0,3);}
        p=next;
        for(size_t i=0;i<3;++i){targetDrive[i]=std::pow(10.0f,p.bands[i].drive/20);targetOutput[i]=std::pow(10.0f,p.bands[i].output/20);}
    }
    void process(float* left,float* right,int samples) noexcept {
        for(int i=0;i<samples;++i){
            const bool changing=quality!=p.quality;
            wet+=std::clamp((p.enabled&&!changing?1.0f:0.0f)-wet,-step,step);
            if(wet<1e-7f){
                if(changing)setQuality(p.quality);
                if(!p.enabled){if(active){lowSplit.reset();highSplit.reset();lowPhase.reset();setQuality(quality);active=false;}return;}
            }
            active=true;
            lowHz+=smooth*(p.lowHz-lowHz);highHz+=smooth*(p.highHz-highHz);mix+=smooth*(p.mix-mix);
            if((tick++&15)==0)updateCrossovers();
            for(size_t b=0;b<3;++b){auto& state=bands[b];state.drive+=smooth*(targetDrive[b]-state.drive);state.mix+=smooth*(p.bands[b].mix-state.mix);state.output+=smooth*(targetOutput[b]-state.output);
                for(int m=0;m<4;++m)state.weights[(size_t)m]+=smooth*((p.bands[b].model==m?1.0f:0.0f)-state.weights[(size_t)m]);}
            for(int ch=0;ch<2;++ch){float& sample=ch?right[i]:left[i];const float input=sanitize(sample);float low,upper,mid,high;
                lowSplit.processSample(ch,input,low,upper);highSplit.processSample(ch,upper,mid,high);low=lowPhase.processSample(ch,low);
                const float split[3]{low,mid,high};float sum=0;
                for(size_t b=0;b<3;++b){auto& state=bands[b];auto& channel=state.channels[(size_t)ch];float up[4],delta[4];channel.up.upsample(split[b],up);
                    const float gain=std::max(1.0f,state.drive),comp=1/std::sqrt(gain),strength=std::min(1.0f,(gain-1)*2);
                    for(int k=0;k<channel.up.getFactor();++k){const float x=std::clamp(up[k]*gain,-64.0f,64.0f);float shaped=0;
                        if(state.weights[0]>1e-6f)shaped+=state.weights[0]*std::tanh(x);
                        if(state.weights[1]>1e-6f)shaped+=state.weights[1]*(std::tanh(x+.35f)-.336375544f);
                        if(state.weights[2]>1e-6f)shaped+=state.weights[2]*std::clamp(x,-1.0f,1.0f);
                        if(state.weights[3]>1e-6f)shaped+=state.weights[3]*std::sin(x);
                        delta[k]=strength*(shaped*comp-up[k]);
                    }
                    const float dry=channel.dry.downsample(up),distortion=channel.dc.process(channel.delta.downsample(delta));
                    sum+=dry+mix*state.mix*((dry+distortion)*state.output-dry);
                }
                sample=lerp(input,sanitize(sum),wet);
            }
        }
    }
private:
    struct Channel {Oversampler up,dry,delta;DcBlocker dc;};
    struct Band {std::array<Channel,2> channels;float drive=1,mix=1,output=1;std::array<float,4> weights{1,0,0,0};};
    std::array<Band,3> bands;
    std::array<float,3> targetDrive{1,1,1},targetOutput{1,1,1};
    juce::dsp::LinkwitzRileyFilter<float> lowSplit,highSplit,lowPhase;
    SaturationParams p;
    float sr=48000,step=.001f,smooth=.001f,wet=0,mix=1,lowHz=250,highHz=2500;
    unsigned tick=0;int quality=1;bool active=false;
    void updateCrossovers() noexcept {lowSplit.setCutoffFrequency(lowHz);highSplit.setCutoffFrequency(highHz);lowPhase.setCutoffFrequency(highHz);}
    void setQuality(int next) noexcept {quality=next;for(auto& b:bands)for(auto& c:b.channels){c.up.setFactor(1<<quality);c.dry.setFactor(1<<quality);c.delta.setFactor(1<<quality);c.dc.reset();}}
};
}
