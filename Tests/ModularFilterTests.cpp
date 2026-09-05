#include "TestFramework.h"
#include "TestHelpers.h"
#include "DSP/ModularFilters.h"
#include "DSP/Resonator.h"
#include "Plugin/PluginEditor.h"
using namespace aeriform;using namespace aeriform::dsp;using namespace aeriform::test;
AERIFORM_TEST(modular_filters_bypass_and_all_models_are_bounded) {
    ModularFilters rack;rack.prepare(48000);CHECK_NEAR(rack.at(FilterPosition::NetworkInput,.37f),.37f,0);
    for(int model=0;model<(int)FilterModel::Count;++model){rack.reset();MovableFilterParams p;p.on=true;p.model=(FilterModel)model;p.cutoff=1200;p.resonance=.8f;p.drive=.5f;rack.set(0,p);
        float energy=0;for(int i=0;i<12000;++i){rack.advance();float y=rack.at(FilterPosition::NetworkInput,.1f*std::sin(i*.1f));CHECK(std::isfinite(y));CHECK(std::abs(y)<8);energy+=y*y;}CHECK(energy>1e-7f);}
}
AERIFORM_TEST(modular_filter_position_and_type_changes_crossfade) {
    ModularFilters rack;rack.prepare(48000);MovableFilterParams p;p.on=true;p.cutoff=700;p.resonance=.02f;rack.set(0,p);
    float previous=0,delta=0;for(int i=0;i<12000;++i){if(i==3000){p.position=FilterPosition::PostBody;rack.set(0,p);}if(i==6000){p.model=FilterModel::Highpass;rack.set(0,p);}if(i==9000){p.on=false;rack.set(0,p);}
        rack.advance();float x=.1f*std::sin(i*.04f);float y=rack.at(FilterPosition::PostBody,rack.at(FilterPosition::NetworkInput,x));delta=std::max(delta,std::abs(y-previous));previous=y;}
    CHECK(delta<.025f);
}
AERIFORM_TEST(modular_filter_every_placement_reaches_audio) {
    auto render=[](int position,bool on){TestHost h;h.set(ids::netMode,(float)NetMode::Serial);h.set(ids::rbOn,1);h.set(ids::rcOn,1);h.set(ids::exbModel,(float)ExciterModel::Wave);h.set(ids::netAB,.3f);h.set(ids::netBA,.3f);h.set(ids::loopOn,1);h.set(ids::loopAmount,.3f);
        h.set(ids::filter1On,on?1:0);h.set(ids::filter1Position,(float)position);h.set(ids::filter1Cutoff,500);h.set(ids::filter1Resonance,.02f);h.noteOn(60);
        std::vector<float> audio;auto stats=h.render(.3,&audio);CHECK(stats.finite);CHECK(stats.peak<1);return audio;};
    const auto baseline=render(0,false);
    for(int position=0;position<(int)FilterPosition::Count;++position){auto bypass=render(position,false);CHECK(bypass==baseline);auto wet=render(position,true);double diff=0;for(size_t i=0;i<wet.size();++i)diff+=std::pow(wet[i]-baseline[i],2);CHECK_MSG(diff>1e-9,"position "+std::to_string(position)+" difference "+std::to_string(diff));}
}
AERIFORM_TEST(modular_in_loop_lowpass_tuning_is_measured) {
    for(double sr:{44100.0,48000.0,96000.0})for(int note:{48,60,72})for(float cutoff:{1000.0f,4000.0f})for(float resonance:{.018f}) {
        ModularFilters rack;rack.prepare((float)sr);MovableFilterParams f;f.on=true;f.position=FilterPosition::ResALoop;f.cutoff=cutoff;f.resonance=resonance;rack.set(0,f);
        Resonator res;res.prepare((float)sr);res.setLoopFilter(&rack,FilterPosition::ResALoop,0);ResonatorParams p;p.freqHz=midiNoteToHz((float)note);p.feedback=.99f;p.damping=.05f;p.reflection=0;p.shape=.5f;p.saturation=0;p.brightness=.5f;
        for(int i=0;i<(int)(sr*.05);++i)rack.advance();p.additionalPhaseDelay=rack.phaseDelay(FilterPosition::ResALoop,p.freqHz);res.update(p,true);
        Noise rng;rng.seed(51);std::vector<float> signal;signal.reserve((size_t)(sr*.4));float tap=0;
        for(int i=0;i<(int)(sr*.5);++i){rack.advance();float x=res.next((i<64?rng.next()*.005f:0),0,tap);if(i>=(int)(sr*.1))signal.push_back(x);}
        double measured=estimatePeakFrequency(signal,sr,p.freqHz);double cents=centsBetween(measured,p.freqHz);std::printf("    movable loop LP sr=%.0f note=%d cutoff=%.0f res=%.3f phase=%.3f measured=%.3f cents=%+.2f\n",sr,note,cutoff,resonance,p.additionalPhaseDelay,measured,cents);CHECK(std::abs(cents)<5);
    }
}
AERIFORM_TEST(modular_filter_page_renders) {
    TestHost h;std::unique_ptr<juce::AudioProcessorEditor> base(h.processor.createEditor());auto* e=dynamic_cast<AeriformEditor*>(base.get());CHECK(e!=nullptr);if(!e)return;e->showPage(6);
    juce::Image image(juce::Image::ARGB,e->getWidth(),e->getHeight(),true);juce::Graphics g(image);e->paintEntireComponent(g,true);auto f=juce::File::getCurrentWorkingDirectory().getChildFile("docs/experimental/filters.png");f.getParentDirectory().createDirectory();auto stream=f.createOutputStream();if(stream){stream->setPosition(0);stream->truncate();CHECK(juce::PNGImageFormat().writeImageToStream(image,*stream));}
}

AERIFORM_TEST(modular_filter_reset_is_deterministic_after_transition) {
    ModularFilters a,b;a.prepare(48000);b.prepare(48000);MovableFilterParams p;p.on=true;a.set(0,p);for(int i=0;i<1000;++i){a.advance();a.at(p.position,.1f);}p.model=FilterModel::Comb;a.set(0,p);for(int i=0;i<1000;++i){a.advance();a.at(p.position,.1f);}a.reset();a.set(0,p);b.set(0,p);
    for(int i=0;i<2000;++i){a.advance();b.advance();float x=std::sin(i*.03f)*.1f;CHECK_NEAR(a.at(p.position,x),b.at(p.position,x),0);}
}

AERIFORM_TEST(modular_filter_phase_compensation_matches_measured_response) {
    for(float sr:{44100.0f,48000.0f,96000.0f})for(float cutoff:{1000.0f,4000.0f})for(float resonance:{.018f,.12f,.6f})for(int model:{0,1,2,3,4,6,10}) {
        ModularFilters rack;rack.prepare(sr);MovableFilterParams p;p.on=true;p.position=FilterPosition::ResALoop;p.cutoff=cutoff;p.resonance=resonance;p.model=(FilterModel)model;rack.set(0,p);
        // Exact integer cycles remove finite-window phase bias. Measure the filter itself
        // because unity-peak normalization can deliberately extinguish the fundamental.
        const float hz=sr*11/4096;const double omega=2*3.141592653589793*hz/sr;
        std::complex<double> xsum{},ysum{};
        for(int i=0;i<12288;++i){rack.advance();float x=.01f*std::sin((float)(omega*i));float y=rack.at(p.position,x);if(i>=8192){auto z=std::exp(std::complex<double>(0,-omega*i));xsum+=(double)x*z;ysum+=(double)y*z;}}
        const double measured=-std::arg(ysum/xsum)/omega;CHECK_MSG(std::abs(measured-rack.phaseDelay(p.position,hz))<.03,"model "+std::to_string(model)+" sample rate "+std::to_string(sr));
    }
}
