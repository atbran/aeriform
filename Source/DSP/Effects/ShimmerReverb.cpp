#include "ShimmerReverb.h"
namespace aeriform::dsp {
void ShimmerReverb::prepare(float rate){sr=rate;step=1/(.02f*sr);feedbackSmooth=1-std::exp(-1/(.03f*sr));room.prepare(sr);for(int i=0;i<2;++i)shift[(size_t)i].prepare(sr,i*.23f);reset();setParams(p,(int)sr);}
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
        // Convex excitation/return blend; the outer return itself is soft bounded.
        const float gain=.35f*feedback;float l,r;
        room.next((1-gain)*sanitize(left[i])+gain*returned[0],(1-gain)*sanitize(right[i])+gain*returned[1],1,l,r);
        const float roomOutput[2]={l,r};
        for(int ch=0;ch<2;++ch){float value=shift[(size_t)ch].next(roomOutput[ch]);value=lowpass[(size_t)ch].process(value);value-=highpass[(size_t)ch].process(value);returned[(size_t)ch]=std::tanh(value);}
        left[i]=lerp(left[i],l,wet);right[i]=lerp(right[i],r,wet);
    }
}
}
