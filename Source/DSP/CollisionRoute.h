#pragma once
#include "DspUtils.h"
#include "Oversampler.h"
#include <array>

namespace aeriform::dsp {
struct ContactParams {
    bool enabled=false;
    int source=0,destination=1,quality=1;
    float gap=.05f,stiffness=.5f,hardness=1.5f,damping=.2f,friction=0,asymmetry=0,amount=.3f,polarity=1;
};

/** Bounded contact scattering. The instantaneous undelayed map is dissipative;
    oversampling introduces state, so the network additionally scales injections
    by each destination's loss. No claim of exact mechanical energy conservation. */
class CollisionRoute {
public:
    void prepare(float rate) noexcept {sr=rate;step=1/(.012f*sr);reset();}
    void reset() noexcept {for(auto& e:engines)e.reset();current=0;transition=false;initialized=false;fade=1;amount=targetAmount=activity=0;}
    void update(ContactParams p,int samples=32) noexcept {
        p.source=std::clamp(p.source,0,2);p.destination=std::clamp(p.destination,0,2);p.quality=std::clamp(p.quality,0,2);
        if(!initialized&&!p.enabled)return;
        if(!initialized){engines[0].set(p);initialized=true;}
        auto& a=engines[(size_t)current];auto& b=engines[(size_t)(1-current)];
        if(!transition&&(a.p.source!=p.source||a.p.destination!=p.destination||a.p.quality!=p.quality||a.p.polarity!=p.polarity)) {b.set(p);transition=true;fade=0;}
        const float c=1-std::exp(-std::max(1,samples)/(.015f*sr));
        a.smooth(p,c);if(transition)b.smooth(p,c);
        targetAmount=p.enabled?clamp01(p.amount):0;
    }
    static float force(float source,float destination,const ContactParams& p) noexcept {
        const float gap=p.gap*(1+(source<0?-1:1)*.9f*p.asymmetry);
        const float penetration=std::max(0.0f,std::abs(source)-gap);
        if(penetration<=0)return 0;
        const float hardness=std::clamp(p.hardness,1.0f,4.0f);
        const float activation=1-std::exp(-512*clamp01(p.stiffness)*std::pow(std::min(penetration,4.0f),hardness));
        const float buzz=1-clamp01(p.friction)*(.5f+.5f*std::cos(80*penetration));
        const float alpha=.45f*clamp01(p.amount)*std::min(1.0f,activation*(1+clamp01(p.damping)))*buzz;
        const float relative=source-p.polarity*destination;
        return alpha*relative/(1+std::abs(relative));
    }
    void next(const float* displacement,float* injection) noexcept {
        injection[0]=injection[1]=injection[2]=0;
        amount+=std::clamp(targetAmount-amount,-step,step);
        if(!initialized||amount<1e-7f){activity*=.99f;return;}
        float a[3]{},b[3]{};engines[(size_t)current].next(displacement,a);
        float t=0;
        if(transition){t=fade;engines[(size_t)(1-current)].next(displacement,b);fade=std::min(1.0f,fade+step);if(fade>=1){transition=false;current=1-current;}}
        float peak=0;for(int i=0;i<3;++i){injection[i]=amount*lerp(a[i],b[i],t);peak=std::max(peak,std::abs(injection[i]));}
        activity+=.02f*(peak-activity);
    }
    float getActivity() const noexcept {return activity;}
private:
    struct Engine {
        ContactParams p;
        Oversampler sourceUp,destinationUp,down;
        void reset() noexcept {sourceUp.reset();destinationUp.reset();down.reset();}
        void set(ContactParams settings) noexcept {p=settings;p.amount=1;int factor=1<<p.quality;sourceUp.setFactor(factor);destinationUp.setFactor(factor);down.setFactor(factor);}
        void smooth(const ContactParams& target,float c) noexcept {p.gap=lerp(p.gap,target.gap,c);p.stiffness=lerp(p.stiffness,target.stiffness,c);p.hardness=lerp(p.hardness,target.hardness,c);p.damping=lerp(p.damping,target.damping,c);p.friction=lerp(p.friction,target.friction,c);p.asymmetry=lerp(p.asymmetry,target.asymmetry,c);}
        void next(const float* x,float* out) noexcept {
            if(p.source==p.destination)return;
            float a[4],b[4],f[4];sourceUp.upsample(x[p.source],a);destinationUp.upsample(x[p.destination],b);
            for(int k=0;k<sourceUp.getFactor();++k)f[k]=force(a[k],b[k],p);
            const float value=std::clamp(sanitize(down.downsample(f)),-.6f,.6f);
            out[p.source]=-value;out[p.destination]=p.polarity*value;
        }
    };
    std::array<Engine,2> engines;
    float sr=48000,step=.001f,fade=1,amount=0,targetAmount=0,activity=0;
    int current=0;bool initialized=false,transition=false;
};
}
