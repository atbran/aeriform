#pragma once
#include "ResonatorNetwork.h"
namespace aeriform::dsp {
struct StereoNetworkParams {
    bool enabled=false;
    float divergence=8,coupling=.1f,exciterSpread=0,pickupSpread=.1f,dampingDivergence=0,rotation=0,width=1,monoBass=120;
};
/** Two physical networks. Economy bypasses the second network entirely after
    the mode transition. There is no output Haas delay or phase-inverting widener. */
class StereoResonatorNetwork {
public:
    void prepare(float rate){sr=rate;step=1/(.015f*sr);left.prepare(rate);right.prepare(rate);reset();}
    void setFilters(ModularFilters* f,int=0) noexcept {left.setFilters(f,0);right.setFilters(f,3);}
    void reset(){left.reset();right.reset();sideBass.reset();blend=target=0;rightRunning=false;previousL=previousR=exciterSide=energyL=energyR=0;}
    void setStereo(StereoNetworkParams p,int samples=32) noexcept {
        desired=p;target=p.enabled?1.0f:0.0f;
        const float c=1-std::exp(-std::max(1,samples)/(.02f*sr));
        current.divergence=lerp(current.divergence,p.divergence,c);current.coupling=lerp(current.coupling,p.coupling,c);current.exciterSpread=lerp(current.exciterSpread,p.exciterSpread,c);current.pickupSpread=lerp(current.pickupSpread,p.pickupSpread,c);current.dampingDivergence=lerp(current.dampingDivergence,p.dampingDivergence,c);current.rotation=lerp(current.rotation,p.rotation,c);current.width=lerp(current.width,p.width,c);current.monoBass=lerp(current.monoBass,p.monoBass,c);
    }
    void update(const NetworkParams& original,bool snap) {
        if(snap){current=desired;blend=target;}
        if(target>0&&!rightRunning){right.reset();rightRunning=true;}
        if(target==0&&blend<=0)rightRunning=false;
        auto a=original,b=original;
        crossGain=.001f;
        for(int i=0;i<3;++i){const float halfCents=current.divergence*.5f*blend;
            a.res[i].freqHz*=std::exp2(-halfCents/1200);b.res[i].freqHz*=std::exp2(halfCents/1200);
            a.res[i].pickup=clamp01(a.res[i].pickup-current.pickupSpread*.5f*blend);b.res[i].pickup=clamp01(b.res[i].pickup+current.pickupSpread*.5f*blend);
            // Main waveguide output is independent of pickup; make its physical pickup
            // audible in each channel with the existing second-tap width mechanism.
            a.width3[i]=std::max(a.width3[i],current.pickupSpread*blend);b.width3[i]=std::max(b.width3[i],current.pickupSpread*blend);
            a.res[i].damping=clamp01(a.res[i].damping-current.dampingDivergence*.5f*blend);b.res[i].damping=clamp01(b.res[i].damping+current.dampingDivergence*.5f*blend);
            const float loss=ResonatorSlot::isModalType(a.res[i].type)?.1f:std::max(.002f,.3f*(1-clamp01(a.res[i].feedback)));
            crossGain=std::min(crossGain,loss*.05f);
        }
        crossGain*=current.coupling;left.update(a,snap);if(rightRunning)right.update(b,snap);
        sideBass.setCutoff(std::max(20.0f,current.monoBass),sr);
        const float angle=current.rotation*.25f*kPi;cosRotation=std::cos(angle);sinRotation=std::sin(angle);
    }
    bool stereoActive() const noexcept {return rightRunning;}
    void setExciterSide(float side) noexcept {exciterSide=side;}
    void next(float excitation,float loop,float pressure,float& l,float& r) noexcept {
        blend+=std::clamp(target-blend,-step,step);
        if(!rightRunning){left.next(excitation,loop,pressure,l,r);energyL+=.002f*(std::abs(l)-energyL);energyR+=.002f*(std::abs(r)-energyR);return;}
        const float side=current.exciterSpread*exciterSide*blend;
        const float cross=crossGain*std::tanh(previousR-previousL)*blend;
        float al,ar,bl,br;left.next(excitation+side+cross,loop,pressure,al,ar);right.next(excitation-side-cross,loop,pressure,bl,br);
        // A side-specific physical pickup feeds its channel. Original pan controls
        // still affect this combination, and centered identical networks match unity.
        const float physicalL=al,physicalR=br;
        previousL=.5f*(al+ar);previousR=.5f*(bl+br);
        float mid=.5f*(physicalL+physicalR),stereoSide=.5f*(physicalL-physicalR);
        // Rotate in mid/side coordinates, then remove bass from the side only.
        float m=mid*cosRotation-stereoSide*sinRotation;
        float s=mid*sinRotation+stereoSide*cosRotation;
        const float low=sideBass.process(s);if(current.monoBass>20.1f)s-=low;
        s*=current.width;
        l=lerp(al,m+s,blend);r=lerp(ar,m-s,blend);
        energyL+=.002f*(std::abs(l)-energyL);energyR+=.002f*(std::abs(r)-energyR);
    }
    float loopReturn() const noexcept {return rightRunning?lerp(left.loopReturn(),.5f*(left.loopReturn()+right.loopReturn()),blend):left.loopReturn();}
    float energy(int i) const noexcept {return rightRunning?.5f*(left.energy(i)+right.energy(i)):left.energy(i);}
    float netEnergy() const noexcept {return rightRunning?.5f*(left.netEnergy()+right.netEnergy()):left.netEnergy();}
    float governor() const noexcept {return rightRunning?std::min(left.governor(),right.governor()):left.governor();}
    float contactActivity() const noexcept {return rightRunning?.5f*(left.contactActivity()+right.contactActivity()):left.contactActivity();}
    bool slotRunning(int i) const noexcept {return left.slotRunning(i);}
    bool isFinite() const noexcept {return left.isFinite()&&(!rightRunning||right.isFinite());}
    float leftEnergy() const noexcept {return energyL;}
    float rightEnergy() const noexcept {return energyR;}
private:
    ResonatorNetwork left,right;
    StereoNetworkParams current,desired;
    OnePole sideBass;
    float sr=48000,step=.001f,blend=0,target=0,previousL=0,previousR=0,exciterSide=0,crossGain=0,cosRotation=1,sinRotation=0,energyL=0,energyR=0;
    bool rightRunning=false;
};
}
