#include "TestFramework.h"
#include "TestHelpers.h"
#include "DSP/Effects/ResonantDelay.h"
#include "Plugin/PluginEditor.h"
using namespace aeriform;using namespace aeriform::dsp;using namespace aeriform::test;
namespace {
void warm(ResonantDelay& d,int samples=24000){for(int i=0;i<samples;++i){float l=0,r=0;d.process(&l,&r,1);}}
}
AERIFORM_TEST(resdelay_bypass_and_repeat_timing) {
    ResonantDelay d;ResonantDelayParams p;p.enabled=false;p.timeMs=10;p.stereoOffsetMs=0;p.saturation=0;p.amount=0;p.feedback=.5f;p.mix=1;d.setParams(p);d.prepare(48000);
    for(int i=0;i<1000;++i){float l=std::sin(i*.1f),r=-l,original=l;d.process(&l,&r,1);CHECK_NEAR(l,original,0);CHECK_NEAR(r,-original,0);}
    p.enabled=true;d.setParams(p);warm(d);std::vector<float> audio;
    for(int i=0;i<1500;++i){float l=i==0?1.0f:0,r=l;d.process(&l,&r,1);audio.push_back(l);CHECK_NEAR(l,r,0);}
    CHECK_NEAR(audio[480],.5,1e-5);CHECK_NEAR(audio[960],.25,1e-5);CHECK_NEAR(audio[1440],.125,1e-5);CHECK_NEAR(audio[479],0,1e-6);
}
AERIFORM_TEST(resdelay_extremes_are_bounded_without_output_limiter) {
    for(float sr:{44100.0f,48000.0f,96000.0f})for(int type=0;type<4;++type){ResonantDelay d;ResonantDelayParams p;p.enabled=true;p.feedback=.98f;p.type=type;p.timeMs=1;p.stereoOffsetMs=50;p.saturation=0;p.amount=1;p.damping=0;p.dispersion=1;p.mix=1;d.setParams(p);d.prepare(sr);
        for(int i=0;i<(int)sr;++i){if(i==(int)sr/2){p.tuningHz=5000;p.timeMs=40;p.stereoOffsetMs=-50;p.saturation=1;d.setParams(p);}float l=.8f*std::sin(i*.27f),r=.8f*std::sin(i*.19f);d.process(&l,&r,1);CHECK(std::isfinite(l)&&std::isfinite(r));CHECK(std::abs(l)<=.801f&&std::abs(r)<=.801f);}
        CHECK(d.safetyClips()==0);
    }
}
AERIFORM_TEST(resdelay_modal_feedback_changes_later_repeats) {
    auto render=[](float amount){ResonantDelay d;ResonantDelayParams p;p.enabled=true;p.timeMs=40;p.stereoOffsetMs=0;p.feedback=.8f;p.saturation=0;p.mix=1;p.amount=amount;p.tuningHz=220;p.damping=.15f;d.setParams(p);d.prepare(48000);warm(d);std::vector<float>x;
        for(int i=0;i<24000;++i){float l=i<256?.5f*std::sin(i*.17f):0,r=l;d.process(&l,&r,1);x.push_back(l);}return x;};
    auto neutral=render(0),modal=render(1);double first=0,later=0;for(size_t i=0;i<neutral.size();++i){double diff=std::pow(neutral[i]-modal[i],2);if(i<3840)first+=diff;else later+=diff;}CHECK(first<1e-12);CHECK(later>1e-5);
    ResonantDelay d;ResonantDelayParams p;p.enabled=true;p.tuningHz=440;p.type=2;d.setParams(p);d.prepare(48000);warm(d,48000);CHECK_NEAR(d.modeFrequency(0),440,.1);CHECK_NEAR(d.modeFrequency(1),440*1.594,.2);
}
AERIFORM_TEST(resdelay_chunk_and_bypass_transitions_are_consistent) {
    auto render=[](int block){ResonantDelay d;ResonantDelayParams p;p.enabled=true;p.mix=.6f;d.setParams(p);d.prepare(48000);std::vector<float> l(36000),rr(36000);for(int i=0;i<36000;++i)l[(size_t)i]=rr[(size_t)i]=.2f*std::sin(i*.09f);
        for(int pos=0;pos<36000;pos+=block){d.process(l.data()+pos,rr.data()+pos,std::min(block,36000-pos));}return l;};auto a=render(1);for(int n:{32,256,1024}){auto b=render(n);for(size_t i=0;i<a.size();++i)CHECK_NEAR(a[i],b[i],0);}
    ResonantDelay d;ResonantDelayParams p;p.enabled=true;d.setParams(p);d.prepare(48000);warm(d);p.enabled=false;d.setParams(p);warm(d,2000);float l=.3f,r=-.3f;d.process(&l,&r,1);CHECK_NEAR(l,.3f,0);CHECK_NEAR(r,-.3f,0);
}
AERIFORM_TEST(resdelay_host_audio_and_state) {
    auto render=[](bool enabled){TestHost h;h.set(ids::rdOn,enabled?1:0);h.set(ids::rdTime,80);h.set(ids::rdMix,.6f);h.noteOn(60);std::vector<float>x;CHECK(h.render(.5,&x).finite);return x;};auto a=render(false),b=render(true);double diff=0;for(size_t i=0;i<a.size();++i)diff+=std::pow(a[i]-b[i],2);CHECK(diff>1e-4);
    TestHost h;h.set(ids::rdType,2);h.set(ids::rdTrack,1);juce::MemoryBlock state;h.processor.getStateInformation(state);TestHost bhost;bhost.processor.setStateInformation(state.getData(),(int)state.getSize());CHECK_NEAR(bhost.get(ids::rdType),2,0);CHECK_NEAR(bhost.get(ids::rdTrack),1,0);
}

AERIFORM_TEST(resdelay_page_renders_and_restores) {
    TestHost h;h.processor.setEditorPage(4);h.processor.setEditorSection(4,2);h.set(ids::rdOn,1);
    std::unique_ptr<juce::AudioProcessorEditor> e(h.processor.createEditor());juce::Image image(juce::Image::ARGB,e->getWidth(),e->getHeight(),true);juce::Graphics g(image);e->paintEntireComponent(g,true);
    auto stream=juce::File::getCurrentWorkingDirectory().getChildFile("docs/experimental/resonant-delay.png").createOutputStream();CHECK(stream!=nullptr);if(stream){stream->setPosition(0);stream->truncate();CHECK(juce::PNGImageFormat().writeImageToStream(image,*stream));}
    juce::MemoryBlock state;h.processor.getStateInformation(state);TestHost restored;restored.processor.setStateInformation(state.getData(),(int)state.getSize());CHECK(restored.processor.getEditorSection(4)==2);
}
