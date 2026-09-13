#pragma once
#include "ResonantDelay.h"
#include "ShimmerReverb.h"
#include "SpectralFreeze.h"
#include "MultibandSaturation.h"
#include "../VoiceParams.h"
#include "../../Params/RackParameters.h"
namespace aeriform::dsp {
/** All algorithms are prepared off-thread. Structural edits fade through dry;
    order changes retain the same instance objects. Callback storage is fixed. */
class ModularRack {
    struct Slot {
        ResonantDelay resonantDelay; ShimmerReverb shimmer; SpectralFreeze spectral; MultibandSaturation saturation;
        int type=0; float wet=0; bool primeSpectral=true;
        void prepare(float sr){resonantDelay.prepare(sr);shimmer.prepare(sr);spectral.prepare(sr);saturation.prepare(sr);reset();}
        void reset(){resonantDelay.reset();shimmer.reset();spectral.reset();saturation.reset();type=0;wet=0;primeSpectral=true;}
        void clearType(int t){switch(t){case 1:resonantDelay.reset();break;case 2:shimmer.reset();break;case 3:spectral.reset();primeSpectral=true;break;case 4:saturation.reset();break;default:break;}}
        void run(VoiceParams& params,float* L,float* R,int numSamples,double bpm,float filterNote){
            switch(type){
            case 1: {        ResonantDelayParams rd;rd.enabled=true;rd.timeMs=params.get(P::rdTime);
        if(params.getb(P::rdSync))rd.timeMs=(float)(60000.0/(bpm>1?bpm:120)*choices::syncDivisionBeats(params.geti(P::rdDiv)));
        rd.feedback=params.get(P::rdFeedback);rd.type=params.geti(P::rdType);rd.tuningHz=params.get(P::rdTuning)*std::exp2((filterNote-57)*params.get(P::rdTrack)/12);
        rd.damping=params.get(P::rdDamping);rd.dispersion=params.get(P::rdDispersion);rd.amount=params.get(P::rdAmount);rd.saturation=params.get(P::rdSaturation);rd.stereoOffsetMs=params.get(P::rdOffset);rd.mix=params.get(P::rdMix);
        resonantDelay.setParams(rd);resonantDelay.process(L,R,numSamples);

break;}
            case 2: {        ShimmerParams sh;sh.enabled=true;sh.semitones=params.get(P::shInterval);sh.feedback=params.get(P::shFeedback);sh.diffusion=params.get(P::shDiffusion);sh.damping=params.get(P::shDamping);sh.size=params.get(P::shSize);sh.spread=params.get(P::shSpread);sh.lowCutHz=params.get(P::shLowCut);sh.highCutHz=params.get(P::shHighCut);sh.mix=params.get(P::shMix);
        shimmer.setParams(sh,numSamples);shimmer.process(L,R,numSamples);
break;}
            case 3: {        SpectralParams sf;sf.enabled=true;sf.freeze=params.getb(P::sfFreeze);sf.capture=params.getb(P::sfCapture);sf.release=params.getb(P::sfRelease);sf.blur=params.get(P::sfBlur);sf.semitones=params.get(P::sfShift);sf.randomPhase=params.get(P::sfRandom);sf.decayMs=params.get(P::sfDecay);sf.mix=params.get(P::sfMix);if(primeSpectral){spectral.primeCommandEdges(sf.capture,sf.release);primeSpectral=false;}spectral.setParams(sf);spectral.process(L,R,numSamples);
break;}
            case 4: {        SaturationParams sat;sat.enabled=true;sat.lowHz=params.get(P::satLow);sat.highHz=params.get(P::satHigh);sat.mix=params.get(P::satMix);sat.quality=params.geti(P::satQuality);
        for(int b=0;b<3;++b){const int base=(int)P::satLowDrive+b*4;auto& band=sat.bands[(size_t)b];band.drive=params.get((P)base);band.model=params.geti((P)(base+1));band.mix=params.get((P)(base+2));band.output=params.get((P)(base+3));}
        saturation.setParams(sat);saturation.process(L,R,numSamples);
break;}
            default:break;
            }
        }
    };
    std::array<Slot,4> slots;
    float step=.001f, orderWet=1; int orderCode=0;
public:
    bool isFrozen(int slot) const noexcept {return slots[(size_t)std::clamp(slot,0,3)].spectral.isFrozen();}
    void prepare(float sr){step=1/(.02f*sr);for(auto& s:slots)s.prepare(sr);orderCode=0;orderWet=1;}
    void reset(){for(auto& s:slots)s.reset();orderCode=0;orderWet=1;}
    void process(float* left,float* right,int samples,const VoiceParams& source,const ModValues& mod,double bpm,float note){
        VoiceParams params=source;
        for(auto b:advancedBindings) if(b.parameter>=P::rack1Type && b.parameter<P::breathMouth)
            params.v[(size_t)b.parameter]=std::clamp(source.get(b.parameter)+mod[(size_t)b.destination]*(b.hi-b.lo),b.lo,b.hi);
        const int requestedOrder=std::clamp(source.geti(P::rackOrder),0,23);
        for(int offset=0;offset<samples;offset+=32){
            const int n=std::min(32,samples-offset);
            float dryL[32],dryR[32],slotL[32],slotR[32];
            std::copy_n(left+offset,n,dryL);std::copy_n(right+offset,n,dryR);
            if(orderWet==0)orderCode=requestedOrder;
            for(int index:rackPermutation(orderCode)){
                auto& slot=slots[(size_t)index];const int wanted=std::clamp(source.geti(rackTypes[index]),0,4);
                if(slot.wet==0 && slot.type!=wanted){slot.clearType(wanted);slot.type=wanted;}
                const bool enabled=source.getb(rackEnables[index]) && wanted!=0;
                const float target=enabled && wanted==slot.type?1.0f:0.0f;
                if(slot.type==0 || (slot.wet==0 && target==0))continue;
                for(size_t f=0;f<std::size(originalRackFields);++f)params.v[(size_t)originalRackFields[f]]=params.get(rackFields[index][f]);
                std::copy_n(left+offset,n,slotL);std::copy_n(right+offset,n,slotR);
                slot.run(params,left+offset,right+offset,n,bpm,note);
                for(int i=0;i<n;++i){slot.wet+=std::clamp(target-slot.wet,-step,step);left[offset+i]=lerp(slotL[i],left[offset+i],slot.wet);right[offset+i]=lerp(slotR[i],right[offset+i],slot.wet);}
                if(slot.wet==0 && !enabled)slot.clearType(slot.type);
            }
            const float target=requestedOrder==orderCode?1.0f:0.0f;
            for(int i=0;i<n;++i){orderWet+=std::clamp(target-orderWet,-step,step);left[offset+i]=lerp(dryL[i],left[offset+i],orderWet);right[offset+i]=lerp(dryR[i],right[offset+i],orderWet);}
        }
    }
};
}
