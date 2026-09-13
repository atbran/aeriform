#include "ShimmerReverb.h"
namespace aeriform::dsp {
void ShimmerReverb::prepare(float rate){sr=rate;step=1/(.02f*sr);feedbackSmooth=1-std::exp(-1/(.03f*sr));room.prepare(sr,false);for(int i=0;i<2;++i)shift[(size_t)i].prepare(sr,i*.23f);reset();setParams(p,(int)sr);}
void ShimmerReverb::reset() noexcept {room.reset();for(auto& s:shift)s.reset();for(auto& f:lowpass)f.reset();for(auto& f:highpass)f.reset();returned.fill(0);wet=feedback=0;wasActive=false;}
void ShimmerReverb::setParams(ShimmerParams next,int samples) noexcept {
    p=next;p.feedback=clamp01(p.feedback);p.mix=clamp01(p.mix);p.spread=clamp01(p.spread);
    RoomParams rp;rp.enabled=true;rp.size=p.size;rp.shape=.63f;rp.wallDamping=p.damping;rp.diffusion=p.diffusion;rp.airAbsorption=p.damping*.5f;rp.send=1;rp.networkReturn=0;rp.feedback=.5f+.45f*p.feedback;rp.width=p.spread;rp.level=1;room.update(rp,samples);
    for(int i=0;i<2;++i){shift[(size_t)i].setSemitones(p.semitones);lowpass[(size_t)i].setCutoff(std::clamp(p.highCutHz,200.0f,sr*.43f),sr);highpass[(size_t)i].setCutoff(std::clamp(p.lowCutHz,20.0f,2000.0f),sr);}
}
void ShimmerReverb::process(float* left,float* right,int samples) noexcept {
    if(!p.enabled&&wet==0)return;
    for(int i=0;i<samples;++i){wet+=std::clamp((p.enabled?p.mix:0.0f)-wet,-step,step);
        if(wet<1e-7f&&!p.enabled){if(wasActive)reset();return;}wasActive=true;feedback+=(p.feedback-feedback)*feedbackSmooth;
        const float returnGain=(1.2f+2.4f*feedback)*feedback;
        float l,r;
        room.next(sanitize(left[i])+returnGain*returned[0],sanitize(right[i])+returnGain*returned[1],1,l,r);
        const float roomOutput[2]={l,r};
        for(int ch=0;ch<2;++ch){
            float value=shift[(size_t)ch].next(roomOutput[ch]);
            value=lowpass[(size_t)ch].process(value);
            value=highpass[(size_t)ch].processHighpass(value);
            returned[(size_t)ch]=fastTanh(value);
        }
        float outL=2.6f*l,outR=2.6f*r;
        if(std::abs(outL)>1.6f)outL=1.6f*fastTanh(outL/1.6f);
        if(std::abs(outR)>1.6f)outR=1.6f*fastTanh(outR/1.6f);
        left[i]=lerp(left[i],outL,wet);right[i]=lerp(right[i],outR,wet);
    }
}
}
