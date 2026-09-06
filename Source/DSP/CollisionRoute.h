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

/** Two-part contact: a nonlinear, dissipative pickup junction makes impacts audible,
    while a separate loss-scaled force drives the physical resonators. Oversampled
    correction paths have memory; the instantaneous map's contraction is not a
    claim of exact mechanical passivity for the complete delayed network. */
class CollisionRoute {
public:
    void prepare(float rate) noexcept {sr=rate;step=1/(.012f*sr);reset();}
    void reset() noexcept {for(auto& e:engines)e.reset();current=0;transition=false;initialized=false;fade=1;amount=targetAmount=pickupAmount=activity=0;}
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
    static float activation(float source,const ContactParams& p) noexcept {
        const float gap=p.gap*p.gap*(1+(source<0?-1:1)*.9f*p.asymmetry);
        const float penetration=std::max(0.0f,std::abs(source)-gap);
        const float travel=std::min(4.0f,penetration/(.025f+gap));
        const float edge=1-std::exp(-12*clamp01(p.stiffness)*std::pow(travel,std::clamp(p.hardness,1.0f,4.0f)));
        return edge*(1-clamp01(p.friction)*(.5f+.5f*std::cos(32*travel)));
    }
    static void scatter(float a,float b,const ContactParams& p,float& outA,float& outB) noexcept {
        const float hit=activation(a,p);
        const float relative=a-p.polarity*b;
        const float exchange=.45f*hit*relative/(1+std::abs(relative));
        // A compliant stop reflects the source; damping makes that reflection
        // lossy. Both gains have magnitude <= 1 and the exchange is contractive.
        const float sourceGain=1-hit*(1.8f-1.0f*clamp01(p.damping));
        const float destinationGain=1-.35f*hit*clamp01(p.damping);
        outA=(a-exchange)*sourceGain;
        outB=(b+p.polarity*exchange)*destinationGain;
    }
    void next(const float* displacement,float* injection,float* pickupCorrection=nullptr) noexcept {
        injection[0]=injection[1]=injection[2]=0;
        if(pickupCorrection)pickupCorrection[0]=pickupCorrection[1]=pickupCorrection[2]=0;
        amount+=std::clamp(targetAmount-amount,-step,step);
        pickupAmount+=std::clamp(targetAmount*(2-targetAmount)-pickupAmount,-step,step);
        if(!initialized||(amount<1e-7f&&pickupAmount<1e-7f)){activity*=.99f;return;}
        float a[3]{},b[3]{},pa[3]{},pb[3]{};engines[(size_t)current].next(displacement,a,pa);
        float t=0;
        if(transition){t=fade;engines[(size_t)(1-current)].next(displacement,b,pb);fade=std::min(1.0f,fade+step);if(fade>=1){transition=false;current=1-current;}}
        float peak=0;for(int i=0;i<3;++i){injection[i]=amount*lerp(a[i],b[i],t);const float correction=pickupAmount*lerp(pa[i],pb[i],t);if(pickupCorrection)pickupCorrection[i]=correction;peak=std::max(peak,std::abs(correction));}
        activity+=.02f*(peak-activity);
    }
    float getActivity() const noexcept {return activity;}
private:
    struct Engine {
        ContactParams p;
        Oversampler sourceUp,destinationUp,down,pickupA,pickupB;
        void reset() noexcept {sourceUp.reset();destinationUp.reset();down.reset();pickupA.reset();pickupB.reset();}
        void set(ContactParams settings) noexcept {p=settings;p.amount=1;int factor=1<<p.quality;sourceUp.setFactor(factor);destinationUp.setFactor(factor);down.setFactor(factor);pickupA.setFactor(factor);pickupB.setFactor(factor);}
        void smooth(const ContactParams& target,float c) noexcept {p.gap=lerp(p.gap,target.gap,c);p.stiffness=lerp(p.stiffness,target.stiffness,c);p.hardness=lerp(p.hardness,target.hardness,c);p.damping=lerp(p.damping,target.damping,c);p.friction=lerp(p.friction,target.friction,c);p.asymmetry=lerp(p.asymmetry,target.asymmetry,c);}
        void next(const float* x,float* out,float* pickup) noexcept {
            if(p.source==p.destination)return;
            float a[4],b[4],f[4],da[4],db[4];sourceUp.upsample(x[p.source],a);destinationUp.upsample(x[p.destination],b);
            for(int k=0;k<sourceUp.getFactor();++k){
                float sa,sb;scatter(a[k],b[k],p,sa,sb);da[k]=sa-a[k];db[k]=sb-b[k];
                f[k]=force(a[k],b[k],p);
            }
            pickup[p.source]=sanitize(pickupA.downsample(da));
            pickup[p.destination]=sanitize(pickupB.downsample(db));
            const float value=std::clamp(sanitize(down.downsample(f)),-.6f,.6f);
            out[p.source]=-value;out[p.destination]=p.polarity*value;
        }
    };
    std::array<Engine,2> engines;
    float sr=48000,step=.001f,fade=1,amount=0,targetAmount=0,pickupAmount=0,activity=0;
    int current=0;bool initialized=false,transition=false;
};
}
