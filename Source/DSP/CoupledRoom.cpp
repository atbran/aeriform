#include "CoupledRoom.h"
namespace aeriform::dsp {
void CoupledRoom::prepare(float rate,bool audibleVoicing){voiced=audibleVoicing;sr=rate;for(auto& d:earlyDelay)d.prepare((int)(sr*.2f)+16);step=1/(.02f*sr);lengthSmooth=1-std::exp(-1/(.05f*sr));for(auto& d:delay)d.prepare((int)(sr*.2f)+16);returnRing.prepare((int)(sr*.3f)+64);returnHighpass.setCutoff(30,sr);reset();update(p,(int)sr);length=targetLength;returnDelay=returnDelayTarget;}
void CoupledRoom::clearBuffers() noexcept {for(auto& d:earlyDelay)d.clear();for(auto& f:earlyTone)f.reset();for(auto& d:delay)d.clear();for(auto& f:wall)f.reset();returnRing.clear();returnLowpass.reset();returnHighpass.reset();energyValue=returnValue=0;}
void CoupledRoom::reset() noexcept {clearBuffers();wet=freezeAmount=0;wasActive=false;clipCount=0;returnVoiceGain=1;}
void CoupledRoom::update(RoomParams next,int samples) noexcept {
    if(next.clear!=previousClear){clearBuffers();previousClear=next.clear;}
    const float c=1-std::exp(-std::max(1,samples)/(.03f*sr));
    next.size=lerp(p.size,next.size,c);next.shape=lerp(p.shape,next.shape,c);next.wallDamping=lerp(p.wallDamping,next.wallDamping,c);next.diffusion=lerp(p.diffusion,next.diffusion,c);next.airAbsorption=lerp(p.airAbsorption,next.airAbsorption,c);next.send=lerp(p.send,next.send,c);next.networkReturn=lerp(p.networkReturn,next.networkReturn,c);next.returnFilterHz=std::exp(lerp(std::log(std::max(50.0f,p.returnFilterHz)),std::log(std::max(50.0f,next.returnFilterHz)),c));next.feedback=lerp(p.feedback,next.feedback,c);next.width=lerp(p.width,next.width,c);next.level=lerp(p.level,next.level,c);p=next;
    static constexpr float base[lines]={7,11,13,17,19,23,29,31};
    static constexpr float shape[lines]={.79f,1.23f,.91f,1.11f,1.31f,.87f,1.17f,.73f};
    for(int i=0;i<lines;++i){targetLength[(size_t)i]=std::clamp(sr*.001f*base[i]*(.4f+2.5f*p.size)*lerp(1.0f,shape[i],p.shape),2.0f,sr*.19f);const float cutoff=18000*std::pow(150.0f/18000,.7f*p.wallDamping+.3f*p.airAbsorption*(.4f+i/10.0f));wall[(size_t)i].setCutoff(std::clamp(cutoff,80.0f,sr*.44f),sr);}
    for(auto& f:earlyTone)f.setCutoff(std::clamp(18000*std::pow(.05f,.65f*p.wallDamping+.35f*p.airAbsorption),80.0f,sr*.44f),sr);
    returnDelayTarget=std::clamp(sr*.001f*p.returnDelayMs,(float)quantum,sr*.25f);returnLowpass.setCutoff(std::clamp(p.returnFilterHz,50.0f,sr*.44f),sr);
}
void CoupledRoom::makeReturn(float* dst,int samples,int voices) noexcept {
    const float targetVoiceGain=1/std::sqrt((float)std::max(1,voices));
    for(int i=0;i<samples;++i){
        returnVoiceGain+=(targetVoiceGain-returnVoiceGain)*lengthSmooth;
        const float gain=(voiced?returnVoiceGain:.02f/std::max(1,voices))*clamp01(p.networkReturn)*wet;
        returnDelay+=(returnDelayTarget-returnDelay)*lengthSmooth;dst[i]=gain*returnRing.readLinear(std::max(1.0f,returnDelay-i));
    }
}
void CoupledRoom::next(float inputL,float inputR,int voices,float& outL,float& outR) noexcept {
    outL=outR=0;wet+=std::clamp((p.enabled?1.0f:0.0f)-wet,-step,step);
    if(wet<1e-7f){if(wasActive)clearBuffers();wasActive=false;return;}wasActive=true;
    freezeAmount+=std::clamp((p.freeze?1.0f:0.0f)-freezeAmount,-step,step);
    const float normalGain=.2f+.78f*clamp01(p.feedback),gain=lerp(normalGain,1.0f,freezeAmount);
    const float inputGain=1-gain,norm=.353553390593f;
    const float contributors=voiced?1.0f:(float)std::max(1,voices);
    const float inL=std::tanh(sanitize(inputL)*p.send/contributors),inR=std::tanh(sanitize(inputR)*p.send/contributors);
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
    if(voiced) {
        // Feedforward early reflections never enter the physical return or shimmer.
        // All taps have a positive delay; their absolute weights sum to one per side.
        constexpr float weights[4]={.42f,.28f,.18f,.12f};
        float earlyL=0,earlyR=0;
        for(int i=0;i<4;++i) {
            const float a=earlyDelay[(size_t)(i%2)].readLinear(length[(size_t)i]);
            const float b=earlyDelay[(size_t)(1-i%2)].readLinear(length[(size_t)i]+sr*.0007f);
            const float da=earlyDelay[(size_t)(1-i%2)].readLinear(length[(size_t)i+4]);
            const float db=earlyDelay[(size_t)(i%2)].readLinear(length[(size_t)i+4]+sr*.0011f);
            const float sign=i%2?-1.0f:1.0f;
            earlyL+=weights[i]*lerp(a,sign*da,p.diffusion*.65f);
            earlyR+=weights[i]*lerp(b,-sign*db,p.diffusion*.65f);
        }
        earlyDelay[0].push((1-freezeAmount)*earlyTone[0].process(inL));
        earlyDelay[1].push((1-freezeAmount)*earlyTone[1].process(inR));
        const float earlyMid=.5f*(earlyL+earlyR),earlySide=.5f*(earlyL-earlyR)*clamp01(p.width);
        // Calibrated early/late gains are outside the contracting FDN. The gentle
        // output saturation bounds even deliberately excessive enabled EXP patches.
        outL=wet*p.level*2*std::tanh((2*(earlyMid+earlySide)+4*(mid+side))*.65f);
        outR=wet*p.level*2*std::tanh((2*(earlyMid-earlySide)+4*(mid-side))*.65f);
    }
    const float returned=std::tanh(returnHighpass.process(returnLowpass.process(mid)));
    returnRing.push(returned);energyValue+=.002f*(std::sqrt(sumSq/lines)-energyValue);returnValue+=.002f*(std::abs(returned)-returnValue);
}
}
