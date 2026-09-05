#include "ModularFilters.h"
#include "VoiceParams.h"
namespace aeriform::dsp {
namespace {
bool loopPosition(FilterPosition p){return p==FilterPosition::CrossFeedback||p==FilterPosition::EnergyLoop||(p>=FilterPosition::ResALoop&&p<=FilterPosition::ResCLoop);}
float logCosh(float x){float a=std::abs(x);return a+std::log1p(std::exp(-2*a))-0.69314718056f;}
float antialiasedDrive(float x,float& previous,float gain){float delta=x-previous;float y=std::abs(delta)>1e-4f?(logCosh(gain*x)-logCosh(gain*previous))/(gain*gain*delta):std::tanh(gain*.5f*(x+previous))/gain;previous=x;return y;}
constexpr float vowels[5][3]={{800,1150,2900},{400,1600,2700},{350,1700,2700},{450,800,2830},{325,700,2530}};
}
void ModularFilters::Lane::reset() noexcept {for(auto& f:svf)f.reset();for(auto& f:formants)f.reset();for(auto& f:ladder)f.reset();comb.clear();previous=ladderOut=0;delay=100;}
void ModularFilters::prepare(float rate){sr=rate;step=1/(.012f*sr);for(auto& b:blocks)for(auto& e:b.engine)for(auto& l:e.lane)l.comb.prepare((int)(sr*4/20)+8);reset();}
void ModularFilters::reset() noexcept {for(auto& b:blocks){for(auto& e:b.engine)e.reset();b.current=0;b.initialized=false;b.transition=false;b.fade=1;b.wet=0;b.targetWet=0;}anyActive=false;}
void ModularFilters::Engine::configure(MovableFilterParams settings,float rate) {
    p=settings;sr=rate;p.cutoff=std::clamp(p.cutoff,20.0f,.44f*sr);q=.5f+11.5f*clamp01(p.resonance);
    driveGain=std::exp2(5*std::max(p.drive,p.model==FilterModel::DrivenSVF?.2f:0.0f));
    const float peak=q>.7071068f?q/std::sqrt(1-.25f/(q*q)):1;
    loopNorm=loopPosition(p.position)?1/(p.slope?peak*peak:peak):1;
    if(p.model==FilterModel::Formant||p.model==FilterModel::Modal||p.model==FilterModel::Comb||p.model==FilterModel::Ladder)loopNorm=1;
    if(p.model==FilterModel::Tilt&&loopPosition(p.position)){float tilt=(p.morph-.5f)*2;loopNorm=1/(std::exp2(-tilt)*peak+std::exp2(tilt)*(1+peak));}
    for(auto& l:lane){for(auto& f:l.svf){f.setSampleRate(sr);f.set(p.cutoff,q);}for(auto& f:l.ladder)f.setCutoff(p.cutoff,sr);
        for(int i=0;i<3;++i){l.formants[i].setSampleRate(sr);float f=p.model==FilterModel::Formant?vowels[std::clamp(p.vowel,0,4)][i]*p.cutoff/1000:p.cutoff*(i==0?1:i==1?2.756f:5.404f);l.formants[i].set(std::min(f,.44f*sr),2+q);}}
}
float ModularFilters::Engine::next(float input,int channel) noexcept {
    auto& s=lane[(size_t)std::clamp(channel,0,lanes-1)];float x=sanitize(input);
    if(driveGain>1.001f)x=antialiasedDrive(x,s.previous,driveGain);
    float y=x;
    if(p.model==FilterModel::Comb) {
        s.delay+=(sr/p.cutoff-s.delay)*.005f;
        const float fb=(p.morph*2-1)*p.resonance*.97f;
        const float delayed=s.comb.readLinear(s.delay);s.comb.push(x+delayed*fb);
        y=(1-std::abs(fb))*delayed;
    } else if(p.model==FilterModel::Ladder) {
        y=(x-s.ladderOut*p.resonance*.8f)/(1+p.resonance*.8f);
        for(int i=0;i<(p.slope?4:2);++i)y=s.ladder[i].process(y);s.ladderOut=y;
    } else if(p.model==FilterModel::Formant||p.model==FilterModel::Modal) {
        y=0;for(auto& f:s.formants)y+=f.bandpass(x);y/=3*(2+q);
    } else {
        const int stages=p.slope?2:1;
        for(int stage=0;stage<stages;++stage){float lo,bp,hi;s.svf[stage].process(y,lo,bp,hi);
            switch(p.model){
                case FilterModel::Lowpass:y=lo;break;case FilterModel::Highpass:y=hi;break;case FilterModel::Bandpass:y=bp;break;case FilterModel::Notch:y=lo+hi;break;
                case FilterModel::Tilt:{float tilt=(p.morph-.5f)*2;float a=std::exp2(-tilt),b=std::exp2(tilt);y=lo*a+(y-lo)*b;break;}
                default:{float t=p.morph*2;y=t<1?lerp(lo,bp/q,t):lerp(bp/q,hi,t-1);break;}
            }
        }
    }
    y*=loopNorm;
    return std::isfinite(y)?std::clamp(y,-32.0f,32.0f):0;
}
void ModularFilters::set(int slot,MovableFilterParams p,float note,float env,int os) {
    auto& b=blocks[(size_t)std::clamp(slot,0,count-1)];
    if(!p.on&&!b.initialized)return;
    p.cutoff*=std::exp2(p.keytrack*(note-60)/12+p.envelope*env);
    const float rate=sr*((int)p.position<5?std::clamp(os,1,4):1);
    if(!b.initialized){b.engine[0].configure(p,rate);b.initialized=true;}
    const bool first=b.wet==0&&b.targetWet==0;
    if(!first){const auto& old=b.engine[(size_t)b.current].p;const float c=1-std::exp(-controlSamples/(.012f*sr));
        p.cutoff=std::exp(lerp(std::log(std::max(20.0f,old.cutoff)),std::log(std::max(20.0f,p.cutoff)),c));
        p.resonance=lerp(old.resonance,p.resonance,c);p.drive=lerp(old.drive,p.drive,c);p.morph=lerp(old.morph,p.morph,c);}
    auto& current=b.engine[(size_t)b.current];auto& next=b.engine[(size_t)(1-b.current)];
    if(!b.transition&&(current.p.position!=p.position||current.p.model!=p.model||current.p.slope!=p.slope||current.p.vowel!=p.vowel)) {next.reset();next.configure(p,rate);b.transition=true;b.fade=0;}
    if(b.transition){auto old=p;old.position=current.p.position;old.model=current.p.model;old.slope=current.p.slope;old.vowel=current.p.vowel;current.configure(old,sr*((int)old.position<5?os:1));auto target=p;target.position=next.p.position;target.model=next.p.model;target.slope=next.p.slope;target.vowel=next.p.vowel;next.configure(target,sr*((int)target.position<5?os:1));}
    else current.configure(p,rate);
    b.targetWet=p.on?clamp01(p.mix):0;anyActive|=b.targetWet>0;
}
void ModularFilters::update(const VoiceParams& v,float note,float env,int samples) {
    controlSamples=std::max(1,samples);
    for(int i=0;i<count;++i){auto get=[&](int f){return v.get(parameter(i,f));};MovableFilterParams p;
        p.on=get(0)>.5f;p.position=(FilterPosition)std::clamp((int)get(1),0,(int)FilterPosition::Count-1);p.model=(FilterModel)std::clamp((int)get(2),0,(int)FilterModel::Count-1);
        p.cutoff=get(3);p.resonance=get(4);p.drive=get(5);p.keytrack=get(6);p.envelope=get(7);p.morph=get(8);p.slope=(int)get(9);p.vowel=(int)get(10);p.mix=get(11);set(i,p,note,env,v.osFactor);}
}
void ModularFilters::advance() noexcept {
    anyActive=false;for(auto& b:blocks){if(!b.initialized)continue;b.wet+=std::clamp(b.targetWet-b.wet,-step,step);anyActive|=b.wet>1e-6f||b.targetWet>0;
        if(b.transition){b.fade=std::min(1.0f,b.fade+step);if(b.fade>=1){b.current=1-b.current;b.transition=false;}}}
}
float ModularFilters::at(FilterPosition pos,float x,int channel) noexcept {
    if(!anyActive)return x;
    for(auto& b:blocks){if(b.wet<=1e-6f)continue;auto& a=b.engine[(size_t)b.current];auto& c=b.engine[(size_t)(1-b.current)];const float t=b.transition?b.fade:0;
        const float old=(a.p.position==pos)?a.next(x,channel):x;
        const float next=(b.transition&&c.p.position==pos)?c.next(x,channel):x;
        x+=b.wet*((old-x)*(1-t)+(next-x)*t);
    }return x;
}
ModularFilters::FrameWeights ModularFilters::weights() const noexcept {
    FrameWeights w{};for(int i=0;i<count;++i){const auto& b=blocks[(size_t)i];const float t=b.transition?b.fade:0;w[(size_t)i][(size_t)b.current]=b.wet*(1-t);w[(size_t)i][(size_t)(1-b.current)]=b.wet*t;}return w;
}
float ModularFilters::atWeighted(FilterPosition pos,float x,int channel,const FrameWeights& w) noexcept {
    for(int i=0;i<count;++i){float y=x;for(int e=0;e<2;++e){auto& engine=blocks[(size_t)i].engine[(size_t)e];const float amount=w[(size_t)i][(size_t)e];if(amount>1e-6f&&engine.p.position==pos)y+=amount*(engine.next(x,channel)-x);}x=y;}return x;
}
std::complex<double> ModularFilters::Engine::transfer(double f) const noexcept {
    using C=std::complex<double>;const double w=2*3.141592653589793*f/sr;const C z=std::exp(C(0,-w));const C u=(1.0-z)/(1.0+z);const double g=std::tan(3.141592653589793*p.cutoff/sr);const C den=u*u+(g/q)*u+g*g;
    const C lp=g*g/den,bp=g*u/den,hp=u*u/den;C h=1;
    switch(p.model){case FilterModel::Lowpass:h=lp;break;case FilterModel::Highpass:h=hp;break;case FilterModel::Bandpass:h=bp;break;case FilterModel::Notch:h=lp+hp;break;
        case FilterModel::Comb:{double fb=(p.morph*2-1)*p.resonance*.97;C d=std::exp(C(0,-w*sr/p.cutoff));h=(1-std::abs(fb))*d/(1.0-fb*d);break;}
        case FilterModel::Ladder:{double a=1-std::exp(-2*3.141592653589793*p.cutoff/sr);C l=std::pow(a/(1.0-(1-a)*z),p.slope?4:2);h=l/(1.0+p.resonance*.8+p.resonance*.8*z*l);break;}
        case FilterModel::Tilt:{double tilt=(p.morph-.5)*2;h=lp*std::exp2(-tilt)+(1.0-lp)*std::exp2(tilt);break;}
        case FilterModel::Formant:case FilterModel::Modal:h=1;break; // These multimodal cases are intentionally not advertised as exact compensation.
        default:{double t=p.morph*2;h=t<1?lp*(1-t)+bp*(t/q):bp*((2-t)/q)+hp*(t-1);break;}
    }
    if(p.slope&&p.model!=FilterModel::Comb&&p.model!=FilterModel::Ladder&&p.model!=FilterModel::Formant&&p.model!=FilterModel::Modal)h*=h;
    if(driveGain>1.001f)h*=.5*(1.0+z); // Small-signal ADAA response, not a guarantee at high drive.
    return h*(double)loopNorm;
}
float ModularFilters::phaseDelay(FilterPosition pos,float f) const noexcept {
    if(!anyActive)return 0;
    using C=std::complex<double>;C h=1;
    for(const auto& b:blocks){if(!b.initialized)continue;const auto& a=b.engine[(size_t)b.current];const auto& c=b.engine[(size_t)(1-b.current)];const double t=b.transition?b.fade:0;
        C ha=a.p.position==pos?a.transfer(f):C(1);C hb=b.transition&&c.p.position==pos?c.transfer(f):C(1);h*=1.0+(double)b.wet*((ha-1.0)*(1-t)+(hb-1.0)*t);}
    const double omega=2*3.141592653589793*std::max(10.0f,f)/sr;return (float)std::clamp<double>(-std::arg(h)/omega,-sr/std::max(20.0f,f)*.8f,sr/std::max(20.0f,f)*.8f);
}
}
