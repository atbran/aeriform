#include "SpectralFreeze.h"
namespace aeriform::dsp {
void SpectralFreeze::prepare(float rate){sr=rate;step=1/(.02f*sr);fft.prepare();for(int i=0;i<size;++i)window[(size_t)i]=.5f-.5f*std::cos(kTwoPi*i/size);reset();}
void SpectralFreeze::reset() noexcept {for(auto& c:channels){c.input.fill(0);c.output.fill(0);c.work.fill({});c.previousPhase.fill(0);c.magnitude.fill(0);c.omega.fill(0);c.phase.fill(0);c.blurred.fill(0);}write=history=captureCount=0;untilFrame=hop;wet=0;ratio=1;holding=captured=pendingCapture=wasActive=false;rng=0x5F00A123u;p.freeze=p.capture=p.release=false;}
float SpectralFreeze::random() noexcept {rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return (float)(rng>>8)*(1.0f/16777216.0f);}
void SpectralFreeze::setParams(SpectralParams next) noexcept {
    if(next.freeze!=p.freeze||(next.enabled&&!p.enabled&&next.freeze)){holding=next.freeze;if(holding)pendingCapture=true;}
    if(next.capture!=p.capture){holding=true;pendingCapture=true;}
    if(next.release!=p.release)holding=false;
    next.blur=clamp01(next.blur);next.randomPhase=clamp01(next.randomPhase);next.mix=clamp01(next.mix);next.semitones=std::clamp(next.semitones,-24.0f,24.0f);next.decayMs=std::clamp(next.decayMs,0.0f,60000.0f);p=next;
}
float SpectralFreeze::bandEnergy(int band) const noexcept {const int from=1+std::clamp(band,0,63)*(bins-2)/64,to=1+(std::clamp(band,0,63)+1)*(bins-2)/64;float energy=0;for(int k=from;k<to;++k)energy+=channels[0].magnitude[(size_t)k]+channels[1].magnitude[(size_t)k];return energy/size;}
void SpectralFreeze::frame() noexcept {
    const bool take=pendingCapture&&history>=size;const float smoothing=1-std::exp(-hop/(.04f*sr));ratio+=(std::exp2(p.semitones/12)-ratio)*smoothing;
    for(auto& c:channels){
        for(int i=0;i<size;++i)c.work[(size_t)i]={c.input[(size_t)((write+i)&(size-1))]*window[(size_t)i],0};fft.transform(c.work);
        for(int k=0;k<bins;++k){const float phase=std::arg(c.work[(size_t)k]),base=kTwoPi*k/size;
            const float delta=std::remainder(phase-c.previousPhase[(size_t)k]-base*hop,kTwoPi);c.previousPhase[(size_t)k]=phase;
            if(take){c.magnitude[(size_t)k]=(k==0||k==bins-1)?0:std::abs(c.work[(size_t)k]);c.omega[(size_t)k]=std::clamp(base+delta/hop,0.0f,kPi);c.phase[(size_t)k]=std::remainder(phase+c.omega[(size_t)k]*size*ratio,kTwoPi);}}
        if(!(take||(captured&&(holding||wet>0))))continue;
        // Blur bin energy, then renormalize to retain captured spectral energy.
        const int radius=(int)std::round(p.blur*12);double before=0,after=0;
        for(int k=1;k<bins-1;++k){double energy=0,weight=0;for(int j=std::max(1,k-radius);j<=std::min(bins-2,k+radius);++j){const float w=(float)(radius+1-std::abs(j-k));energy+=w*c.magnitude[(size_t)j]*c.magnitude[(size_t)j];weight+=w;}c.blurred[(size_t)k]=(float)std::sqrt(energy/std::max(1.0,weight));before+=c.magnitude[(size_t)k]*c.magnitude[(size_t)k];after+=c.blurred[(size_t)k]*c.blurred[(size_t)k];}
        const float normalization=after>1e-20?(float)std::sqrt(before/after):0;c.work.fill({});
        for(int k=1;k<bins-1;++k){auto& phase=c.phase[(size_t)k];if(!take)phase=std::remainder(phase+c.omega[(size_t)k]*hop*ratio+(random()-.5f)*p.randomPhase*.5f,kTwoPi);
            const float mapped=k*ratio;const int bin=(int)mapped;if(bin<1||bin>=bins-1)continue;const float fraction=mapped-bin,amp=c.blurred[(size_t)k]*normalization;
            const FixedFFT<11>::Complex value=std::polar(amp,phase);c.work[(size_t)bin]+=value*(1-fraction);if(bin+1<bins-1)c.work[(size_t)(bin+1)]+=value*fraction;
        }
        // Normalize any coincident shifted bins to the original frame energy.
        double shifted=0;for(int k=1;k<bins-1;++k)shifted+=std::norm(c.work[(size_t)k]);const float cap=shifted>before&&shifted>1e-20?(float)std::sqrt(before/shifted):1;
        for(int k=1;k<bins-1;++k){c.work[(size_t)k]*=cap;c.work[(size_t)(size-k)]=std::conj(c.work[(size_t)k]);}fft.transform(c.work,true);
        if(holding||wet>0)for(int i=0;i<size;++i)c.output[(size_t)((write+i)&(size-1))]+=c.work[(size_t)i].real()*window[(size_t)i]*(2.0f/3);
        if(p.decayMs>0){const float decay=std::exp(-6.907755f*hop/(sr*.001f*std::max(10.0f,p.decayMs)));for(auto& magnitude:c.magnitude)magnitude*=decay;}
    }
    if(take){pendingCapture=false;captured=true;++captureCount;}
}
void SpectralFreeze::process(float* left,float* right,int samples) noexcept {
    if(!p.enabled&&wet==0)return;
    for(int i=0;i<samples;++i){wet+=std::clamp((p.enabled&&holding&&captured?p.mix:0.0f)-wet,-step,step);
        if(!p.enabled&&wet<1e-7f){if(wasActive){const auto saved=p;reset();p=saved;}return;}wasActive=true;
        for(int ch=0;ch<2;++ch){float& sample=ch?right[i]:left[i];auto& c=channels[(size_t)ch];c.input[(size_t)write]=std::clamp(sanitize(sample),-4.0f,4.0f);const float frozen=c.output[(size_t)write];c.output[(size_t)write]=0;sample=lerp(sample,std::clamp(sanitize(frozen),-4.0f,4.0f),wet);}
        write=(write+1)&(size-1);history=std::min(size,history+1);if(--untilFrame==0){untilFrame=hop;frame();}
    }
}
}
