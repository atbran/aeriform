#include "CoupledRoom.h"
namespace aeriform::dsp {
void CoupledRoom::prepare(float rate){sr=rate;step=1/(.02f*sr);lengthSmooth=1-std::exp(-1/(.05f*sr));for(auto& d:delay)d.prepare((int)(sr*.2f)+16);returnRing.prepare((int)(sr*.3f)+64);returnHighpass.setCutoff(30,sr);reset();update(p,(int)sr);length=targetLength;returnDelay=returnDelayTarget;}
void CoupledRoom::clearBuffers() noexcept {for(auto& d:delay)d.clear();for(auto& f:wall)f.reset();returnRing.clear();returnLowpass.reset();returnHighpass.reset();energyValue=returnValue=0;}
void CoupledRoom::reset() noexcept {clearBuffers();wet=freezeAmount=0;wasActive=false;clipCount=0;}
void CoupledRoom::update(RoomParams next,int samples) noexcept {
    if(next.clear!=previousClear){clearBuffers();previousClear=next.clear;}
    const float c=1-std::exp(-std::max(1,samples)/(.03f*sr));
    next.size=lerp(p.size,next.size,c);next.shape=lerp(p.shape,next.shape,c);next.wallDamping=lerp(p.wallDamping,next.wallDamping,c);next.diffusion=lerp(p.diffusion,next.diffusion,c);next.airAbsorption=lerp(p.airAbsorption,next.airAbsorption,c);next.send=lerp(p.send,next.send,c);next.networkReturn=lerp(p.networkReturn,next.networkReturn,c);next.returnFilterHz=std::exp(lerp(std::log(std::max(50.0f,p.returnFilterHz)),std::log(std::max(50.0f,next.returnFilterHz)),c));next.feedback=lerp(p.feedback,next.feedback,c);next.width=lerp(p.width,next.width,c);next.level=lerp(p.level,next.level,c);p=next;
    static constexpr float base[lines]={7,11,13,17,19,23,29,31};
    static constexpr float shape[lines]={.79f,1.23f,.91f,1.11f,1.31f,.87f,1.17f,.73f};
    for(int i=0;i<lines;++i){targetLength[(size_t)i]=std::clamp(sr*.001f*base[i]*(.4f+2.5f*p.size)*lerp(1.0f,shape[i],p.shape),2.0f,sr*.19f);const float cutoff=18000*std::pow(150.0f/18000,.7f*p.wallDamping+.3f*p.airAbsorption*(.4f+i/10.0f));wall[(size_t)i].setCutoff(std::clamp(cutoff,80.0f,sr*.44f),sr);}
    returnDelayTarget=std::clamp(sr*.001f*p.returnDelayMs,(float)quantum,sr*.25f);returnLowpass.setCutoff(std::clamp(p.returnFilterHz,50.0f,sr*.44f),sr);
}
void CoupledRoom::makeReturn(float* dst,int samples,int voices) noexcept {
    const float gain=.02f*clamp01(p.networkReturn)*wet/std::max(1,voices);
    for(int i=0;i<samples;++i){returnDelay+=(returnDelayTarget-returnDelay)*lengthSmooth;dst[i]=gain*returnRing.readLinear(std::max(1.0f,returnDelay-i));}
}
void CoupledRoom::next(float inputL,float inputR,int voices,float& outL,float& outR) noexcept {
    outL=outR=0;wet+=std::clamp((p.enabled?1.0f:0.0f)-wet,-step,step);
    if(wet<1e-7f){if(wasActive)clearBuffers();wasActive=false;return;}wasActive=true;
    freezeAmount+=std::clamp((p.freeze?1.0f:0.0f)-freezeAmount,-step,step);
    const float normalGain=.2f+.78f*clamp01(p.feedback),gain=lerp(normalGain,1.0f,freezeAmount);
    const float inputGain=1-gain,norm=.353553390593f;
    const float inL=std::tanh(sanitize(inputL)*p.send/std::max(1,voices)),inR=std::tanh(sanitize(inputR)*p.send/std::max(1,voices));
    float read[lines],sum=0,sumSq=0;
    for(int i=0;i<lines;++i){auto& len=length[(size_t)i];len+=(targetLength[(size_t)i]-len)*lengthSmooth*(1-freezeAmount);const float fractional=delay[(size_t)i].readLinear(len),integer=delay[(size_t)i].readInteger((int)std::round(len));const float x=lerp(fractional,integer,freezeAmount);read[i]=lerp(wall[(size_t)i].process(x),x,freezeAmount);sum+=read[i];sumSq+=read[i]*read[i];}
    const float diffusion=lerp(clamp01(p.diffusion),1.0f,freezeAmount);
    float l=0,r=0;
    for(int i=0;i<lines;++i){const float sign=i%2?1.0f:-1.0f;const float scatter=read[i]-diffusion*.25f*sum;float value=gain*scatter+inputGain*norm*(inL+sign*inR);
        // Independent internal state guard; never depends on the plug-in output limiter.
        if(!std::isfinite(value)||std::abs(value)>8){++clipCount;value=std::isfinite(value)?std::clamp(value,-8.0f,8.0f):0;}
        delay[(size_t)i].push(value);l+=norm*read[i];r+=norm*sign*read[i];
    }
    const float mid=.5f*(l+r),side=.5f*(l-r)*clamp01(p.width);outL=wet*p.level*(mid+side);outR=wet*p.level*(mid-side);
    const float returned=std::tanh(returnHighpass.process(returnLowpass.process(mid)));
    returnRing.push(returned);energyValue+=.002f*(std::sqrt(sumSq/lines)-energyValue);returnValue+=.002f*(std::abs(returned)-returnValue);
}
}
