#include "SympatheticBank.h"
namespace aeriform::dsp {
SympatheticBank::SympatheticBank() noexcept {captured.fill(-1);lastHeld.fill(-1);for(auto& x:published)x.store(-1);for(auto& x:requested)x.store(-1);for(auto& x:publishedHeld)x.store(-1);}
void SympatheticBank::prepare(float sampleRate) noexcept {sr=sampleRate;step=1/(.015f*sr);envelopeAttack=1-std::exp(-1/(.0005f*sr));envelopeRelease=std::exp(-1/(.07f*sr));reset();tune(true);}
void SympatheticBank::clearEnergy() noexcept {for(auto& m:modes){m.re=m.im=0;m.weight=0;}energy.fill(0);fastEnvelope=peakEnvelope=0;controllerGain=1;}
void SympatheticBank::reset() noexcept {clearEnergy();held.fill(0);sustain.fill(false);wet=freezeAmount=0;outputNorm=1;clipCount=0;for(auto& x:publishedHeld)x.store(-1,std::memory_order_relaxed);}
SympatheticBank::Chord SympatheticBank::heldChord() const noexcept {
    Chord chord;chord.fill(-1);int count=0;
    for(int note=0;note<128&&count<12;++note){bool down=false;for(int ch=0;ch<16;++ch)down|=held[(size_t)(ch*128+note)]!=0;if(down)chord[(size_t)count++]=note;}return chord;
}
void SympatheticBank::publish() noexcept {publication.fetch_add(1,std::memory_order_acq_rel);for(int i=0;i<12;++i)published[(size_t)i].store(captured[(size_t)i],std::memory_order_relaxed);publication.fetch_add(1,std::memory_order_release);}
void SympatheticBank::requestCapturedChord(const Chord& chord) noexcept {requestRevision.fetch_add(1,std::memory_order_acq_rel);for(int i=0;i<12;++i)requested[(size_t)i].store(std::clamp(chord[(size_t)i],-1,127),std::memory_order_relaxed);requestRevision.fetch_add(1,std::memory_order_release);}
SympatheticBank::Chord SympatheticBank::capturedChord() const noexcept {
    Chord chord;chord.fill(-1);
    if(requestRevision.load(std::memory_order_acquire)!=appliedRevision.load(std::memory_order_acquire)){for(int i=0;i<12;++i)chord[(size_t)i]=requested[(size_t)i].load(std::memory_order_relaxed);return chord;}
    for(int attempt=0;attempt<3;++attempt){unsigned before=publication.load(std::memory_order_acquire);if(before&1)continue;for(int i=0;i<12;++i)chord[(size_t)i]=published[(size_t)i].load(std::memory_order_relaxed);if(before==publication.load(std::memory_order_acquire))return chord;}
    return chord;
}
void SympatheticBank::update(SympatheticParams next,int samples) noexcept {
    const unsigned request=requestRevision.load(std::memory_order_acquire);
    if(!(request&1u)&&request!=consumedRequest){Chord c;for(int i=0;i<12;++i)c[(size_t)i]=requested[(size_t)i].load(std::memory_order_relaxed);if(request==requestRevision.load(std::memory_order_acquire)){captured=c;consumedRequest=request;publish();appliedRevision.store(request,std::memory_order_release);}}
    if(next.clear!=previousClear){clearEnergy();previousClear=next.clear;}
    if(next.capture!=previousCapture){auto chord=heldChord();if(chord[0]>=0){captured=chord;publish();}previousCapture=next.capture;}
    const float c=1-std::exp(-std::max(1,samples)/(.02f*sr));
    next.send=lerp(p.send,next.send,c);next.returnLevel=lerp(p.returnLevel,next.returnLevel,c);next.damper=lerp(p.damper,next.damper,c);next.decayMs=std::exp(lerp(std::log(std::max(20.0f,p.decayMs)),std::log(std::max(20.0f,next.decayMs)),c));next.damping=lerp(p.damping,next.damping,c);next.brightness=lerp(p.brightness,next.brightness,c);next.detune=lerp(p.detune,next.detune,c);next.spread=lerp(p.spread,next.spread,c);next.thresholdDb=lerp(p.thresholdDb,next.thresholdDb,c);
    frequencySmooth=c;threshold=std::pow(10.0f,next.thresholdDb/20);
    p=next;p.count=std::clamp(p.count,1,12);p.tuning=std::clamp(p.tuning,0,8);if(active())tune();
}
void SympatheticBank::tune(bool immediate) noexcept {
    static constexpr int major[]={0,2,4,5,7,9,11},minor[]={0,2,3,5,7,8,10},pent[]={0,2,4,7,9};
    const Chord& chord=p.tuning==8?captured:lastHeld;int chordCount=0;for(int n:chord)chordCount+=n>=0;
    for(int i=0;i<12;++i){float note=(float)p.root;switch(p.tuning){case 1:note+=major[i%7]+12*(i/7);break;case 2:note+=minor[i%7]+12*(i/7);break;case 3:note+=pent[i%5]+12*(i/5);break;case 4:note+=i*2;break;case 5:note+=(float)p.intervals[(size_t)i];break;case 6:note+=12*std::log2((float)(i+1));break;case 7:case 8:note=chordCount?chord[(size_t)(i%chordCount)]+12.0f*(i/chordCount):note+i;break;default:note+=i;break;}
        note+=p.detune*((i*7)%13-6)/600.0f;const float hz=std::clamp(midiNoteToHz(note),16.0f,sr*.44f);
        float& f=frequencyHz[(size_t)i];f=immediate||f<=0?hz:std::exp(lerp(std::log(f),std::log(hz),frequencySmooth));
        auto& m=modes[(size_t)i];const double w=2*3.141592653589793*f/sr;m.cosine=std::cos(w);m.sine=std::sin(w);
        const double seconds=std::max(.02,p.decayMs*.001/(1+39*p.damper)*std::pow(.04,p.damping*(.2+.8*std::sqrt(std::min(1.0f,f/4000)))));m.radius=std::exp(-6.907755278982137/(seconds*sr));
        const float pan=p.spread*((i*7)%12/5.5f-1);const float angle=(pan+1)*.25f*kPi;m.panL=std::cos(angle);m.panR=std::sin(angle);m.gain=std::exp2((p.brightness-.5f)*2*(i/11.0f));
    }
    float normSquared=0;
    for(int i=0;i<p.count;++i) {
        normSquared+=modes[(size_t)i].gain*modes[(size_t)i].gain;
        for(int j=0;j<i;++j) {
            const float separation=std::abs(frequencyHz[(size_t)i]-frequencyHz[(size_t)j]);
            const float coherence=std::exp(-separation/5.0f);
            normSquared+=2*coherence*modes[(size_t)i].gain*modes[(size_t)j].gain;
        }
    }
    outputNormTarget=std::sqrt(std::max(1.0f,normSquared));
    if(immediate)outputNorm=outputNormTarget;
}
void SympatheticBank::handleMidi(const juce::MidiMessage& m) noexcept {
    const int ch=std::clamp(m.getChannel()-1,0,15);
    if(m.isNoteOn())held[(size_t)(ch*128+m.getNoteNumber())]=1;
    else if(m.isNoteOff()){auto& h=held[(size_t)(ch*128+m.getNoteNumber())];h=sustain[(size_t)ch]?2:0;}
    else if(m.isController()){int cc=m.getControllerNumber(),value=m.getControllerValue();if(cc==64){sustain[(size_t)ch]=value>=64;if(value<64)for(int n=0;n<128;++n){auto& h=held[(size_t)(ch*128+n)];if(h==2)h=0;}}else if(cc==120||cc==123){if(cc==120)clearEnergy();for(int n=0;n<128;++n)held[(size_t)(ch*128+n)]=0;sustain[(size_t)ch]=false;}}
    auto chord=heldChord();for(int i=0;i<12;++i)publishedHeld[(size_t)i].store(chord[(size_t)i],std::memory_order_relaxed);if(chord[0]>=0)lastHeld=chord;if(p.tuning==7)tune();
}
void SympatheticBank::next(float inputL,float inputR,int /*voices*/,float& left,float& right) noexcept {
    left=right=0;
    wet+=std::clamp((p.enabled?1.0f:0.0f)-wet,-step,step);
    if(wet<1e-7f){clearEnergy();return;}
    freezeAmount+=std::clamp((p.freeze?1.0f:0.0f)-freezeAmount,-step,step);
    const float sourceL=sanitize(inputL),sourceR=sanitize(inputR);
    const float sourcePeak=std::max(std::abs(sourceL),std::abs(sourceR));
    const float gate=sourcePeak>=threshold?1.0f:0.0f;
    const float xL=std::tanh(sourceL*p.send)*gate,xR=std::tanh(sourceR*p.send)*gate;
    // Mid/side enter orthogonal modal coordinates: antiphase input still excites the bank.
    const float mid=.5f*(xL+xR),side=.5f*(xL-xR);
    fastEnvelope+=envelopeAttack*(std::max(std::abs(xL),std::abs(xR))-fastEnvelope);
    const float onset=std::max(0.0f,fastEnvelope-peakEnvelope);
    peakEnvelope=std::max(fastEnvelope,peakEnvelope*envelopeRelease);
    // A source-derived rise deposits a short pulse. Sustained injection is calibrated
    // per second, independently of T60. Neither path generates anything from silence.
    const double inputRe=(60.0/sr*mid+3.0*onset)*(1-freezeAmount);
    const double inputIm=60.0/sr*side*(1-freezeAmount);
    double stored=0,weightsSquared=0;
    for(int i=0;i<12;++i) {
        auto& m=modes[(size_t)i];
        m.weight+=std::clamp((i<p.count?1.0f:0.0f)-m.weight,-step,step);
        if(m.weight<=0){m.re=m.im=0;energy[(size_t)i]=0;continue;}
        const double r=m.radius+(1-m.radius)*freezeAmount;
        const double re=r*(m.re*m.cosine-m.im*m.sine);
        m.im=r*(m.re*m.sine+m.im*m.cosine);m.re=re;
        stored+=m.re*m.re+m.im*m.im;weightsSquared+=m.weight*m.weight;
    }
    // Triangle-inequality budget on the complete complex state vector. Incoming
    // energy softens near capacity; existing tails are never gain-controlled.
    // norm(z + gain*u) <= norm(z) + budget*norm(u)/(budget+norm(u)) < capacity.
    constexpr double capacity=.75;
    const double budget=std::max(0.0,capacity-std::sqrt(stored));
    const double requestedNorm=std::sqrt(weightsSquared*(inputRe*inputRe+inputIm*inputIm));
    const double inputGain=requestedNorm>0?budget/(budget+requestedNorm):1.0;
    controllerGain=(float)inputGain;
    outputNorm+=(outputNormTarget-outputNorm)*step;
    for(int i=0;i<12;++i) {
        auto& m=modes[(size_t)i];if(m.weight<=0)continue;
        m.re+=m.weight*inputGain*inputRe;m.im+=m.weight*inputGain*inputIm;
        double magnitude=std::hypot(m.re,m.im);
        if(!std::isfinite(magnitude)){m.re=m.im=0;magnitude=0;++clipCount;}
        else if(magnitude>1.0){m.re/=magnitude;m.im/=magnitude;magnitude=1;++clipCount;}
        energy[(size_t)i]+=.002f*((float)magnitude-energy[(size_t)i]);
        const float y=(float)m.re*m.gain*m.weight;
        left+=y*m.panL;right+=y*m.panR;
    }
    const float gain=wet*p.returnLevel*.7f/std::max(1.0f,outputNorm);
    left*=gain;right*=gain;
}
}
