#include "ResonantDelay.h"
namespace aeriform::dsp {
void ResonantDelay::prepare(float sampleRate) {
    sr=sampleRate;step=1/(.015f*sr);smooth=1-std::exp(-1/(.04f*sr));controlSmooth=1-std::exp(-16/(.04f*sr));
    for(auto& d:delays)d.prepare((int)(sr*4.1f)+16);
    current=target;time=(float)(sr*.001*target.timeMs);frequencies.fill(target.tuningHz);reset();coefficients();
}
void ResonantDelay::clear() noexcept {for(auto& d:delays)d.clear();for(auto& channel:modes)for(auto& m:channel)m.re=m.im=0;}
void ResonantDelay::reset() noexcept {clear();wet=0;counter=clips=0;wasActive=false;}
void ResonantDelay::setParams(ResonantDelayParams p) noexcept {
    p.type=std::clamp(p.type,0,3);p.timeMs=std::clamp(p.timeMs,1.0f,4000.0f);p.feedback=std::clamp(p.feedback,0.0f,.98f);
    p.tuningHz=std::clamp(p.tuningHz,20.0f,sr*.4f);p.damping=clamp01(p.damping);p.dispersion=clamp01(p.dispersion);
    p.amount=clamp01(p.amount);p.saturation=clamp01(p.saturation);p.stereoOffsetMs=std::clamp(p.stereoOffsetMs,-50.0f,50.0f);p.mix=clamp01(p.mix);target=p;
}
void ResonantDelay::coefficients() noexcept {
    auto follow=[&](float& value,float t){value=lerp(value,t,controlSmooth);};
    follow(current.feedback,target.feedback);follow(current.amount,target.amount);follow(current.saturation,target.saturation);
    follow(current.damping,target.damping);follow(current.dispersion,target.dispersion);follow(current.stereoOffsetMs,target.stereoOffsetMs);
    static constexpr float ratios[4][6]={{1,2,3,4,5,6},{1,2.756f,5.404f,8.933f,13.34f,18.64f},{1,1.594f,2.136f,2.296f,2.653f,2.918f},{1,1.66f,4.08f,4.46f,5,6.1f}};
    float weights[6],total=0;
    for(int i=0;i<6;++i){weights[i]=std::exp(-current.damping*i*.7f);total+=weights[i];}
    for(int i=0;i<6;++i){float ratio=ratios[target.type][i];ratio*=std::sqrt(1+.012f*current.dispersion*current.dispersion*ratio*ratio);
        const float f=std::clamp(target.tuningHz*ratio,20.0f,sr*.43f);auto& hz=frequencies[(size_t)i];hz=std::exp(lerp(std::log(std::max(20.0f,hz)),std::log(f),controlSmooth));
        const float radius=std::exp(-1/(sr*(.008f+.22f*(1-current.damping))/(1+.1f*i))),theta=kTwoPi*hz/sr;
        for(auto& channel:modes){auto& m=channel[(size_t)i];m.a=radius*std::cos(theta);m.b=radius*std::sin(theta);m.input=1-radius;m.weight=weights[i]/total;}
    }
}
float ResonantDelay::colour(float input,int channel) noexcept {
    double sum=0;
    for(auto& m:modes[(size_t)channel]){const double re=m.a*m.re-m.b*m.im+m.input*input;m.im=m.b*m.re+m.a*m.im;m.re=re;sum+=m.weight*re;}
    return (float)sum;
}
void ResonantDelay::process(float* left,float* right,int samples) noexcept {
    if(!target.enabled&&wet==0)return;
    for(int i=0;i<samples;++i){wet+=std::clamp((target.enabled?target.mix:0.0f)-wet,-step,step);
        if(wet<1e-7f&&!target.enabled){wet=0;if(wasActive)clear();wasActive=false;return;}wasActive=true;
        if(counter++%16==0)coefficients();if(counter>=16)counter=0;
        time+=((float)(sr*.001*target.timeMs)-time)*smooth;
        for(int ch=0;ch<2;++ch){float& sample=ch?right[i]:left[i];const float delayed=delays[(size_t)ch].readLinear(time+(ch?1:-1)*.0005f*sr*current.stereoOffsetMs);
            const float coloured=colour(delayed,ch),feedback=lerp(delayed,coloured,current.amount);
            float value=(1-current.feedback)*sanitize(sample)+current.feedback*feedback;
            if(!std::isfinite(value)||std::abs(value)>4){++clips;value=std::isfinite(value)?std::clamp(value,-4.0f,4.0f):0;}
            const float drive=1+8*current.saturation;value=lerp(value,std::tanh(value*drive)/drive,current.saturation);
            delays[(size_t)ch].push(value);sample=lerp(sample,delayed,wet);
        }
    }
}
}
