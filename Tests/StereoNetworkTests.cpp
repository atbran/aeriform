#include "TestFramework.h"
#include "TestHelpers.h"
#include "DSP/StereoResonatorNetwork.h"
#include "GUI/ContactPage.h"
#include "Plugin/PluginEditor.h"
using namespace aeriform;using namespace aeriform::dsp;using namespace aeriform::test;
AERIFORM_TEST(stereo_economy_preserves_original_network_exactly) {
    ResonatorNetwork original;StereoResonatorNetwork stereo;original.prepare(48000);stereo.prepare(48000);NetworkParams p;p.mode=NetMode::Parallel;p.on[1]=p.on[2]=true;p.contact.enabled=true;p.contact.gap=.01f;
    original.update(p,true);stereo.setStereo({});stereo.update(p,true);Noise rng;rng.seed(41);
    for(int i=0;i<12000;++i){float a,b,c,d,x=rng.next()*.02f;original.next(x,0,0,a,b);stereo.next(x,0,0,c,d);CHECK_NEAR(a,c,0);CHECK_NEAR(b,d,0);}CHECK(!stereo.stereoActive());
}
AERIFORM_TEST(stereo_independent_networks_are_mono_compatible_and_bounded) {
    for(float sr:{44100.0f,48000.0f,96000.0f}){StereoResonatorNetwork stereo;stereo.prepare(sr);NetworkParams p;p.mode=NetMode::Parallel;p.on[1]=p.on[2]=true;for(auto& res:p.res){res.freqHz=220;res.feedback=.98f;}StereoNetworkParams s;s.enabled=true;s.divergence=15;s.coupling=1;s.exciterSpread=.5f;s.pickupSpread=.3f;stereo.setStereo(s);stereo.update(p,true);
        double mid=0,side=0;float peak=0;for(int i=0;i<(int)sr;++i){float l,r;float x=.03f*std::sin(i*.03f);stereo.setExciterSide(.01f*std::sin(i*.02f));stereo.next(x,0,0,l,r);CHECK(stereo.isFinite());mid+=std::pow(.5f*(l+r),2);side+=std::pow(.5f*(l-r),2);peak=std::max(peak,std::max(std::abs(l),std::abs(r)));}CHECK(mid>1e-4);CHECK(side>1e-5);CHECK(mid>side*.05);CHECK(peak<2);
        s.width=0;stereo.setStereo(s);stereo.update(p,true);for(int i=0;i<2000;++i){float l,r;stereo.next(.03f*std::sin(i*.04f),0,0,l,r);CHECK_NEAR(l,r,1e-7);}
        s.enabled=false;stereo.setStereo(s);stereo.update(p,false);for(int i=0;i<(int)sr/20;++i){float l,r;stereo.next(0,0,0,l,r);}stereo.update(p,false);CHECK(!stereo.stereoActive());
    }
}
AERIFORM_TEST(stereo_mono_bass_attenuates_low_frequency_side) {
    auto measure=[](float hz){StereoResonatorNetwork n;n.prepare(48000);NetworkParams p;p.res[0].freqHz=80;p.res[0].feedback=.5f;StereoNetworkParams s;s.enabled=true;s.divergence=0;s.pickupSpread=0;s.rotation=.7f;s.monoBass=hz;n.setStereo(s);n.update(p,true);double energy=0;
        for(int i=0;i<48000;++i){float l,r;n.next(.03f*std::sin(2*kPi*80*i/48000),0,0,l,r);if(i>24000)energy+=std::pow(l-r,2);}return energy;};
    const double full=measure(20),bass=measure(800);CHECK(full>1e-6);CHECK(bass<full*.1);
}
AERIFORM_TEST(stereo_host_parameters_change_audio_and_restore_state) {
    auto render=[](bool stereo){TestHost h;h.set(ids::stereoMode,stereo?1:0);h.set(ids::stereoDivergence,20);h.noteOn(60);std::vector<float>x;CHECK(h.render(.3,&x).finite);return x;};auto a=render(false),b=render(true);double difference=0;for(size_t i=0;i<a.size();++i)difference+=std::pow(a[i]-b[i],2);CHECK(difference>1e-7);
    TestHost h;h.set(ids::chorusMix,0);h.set(ids::delayMix,0);h.set(ids::reverbMix,0);h.set(ids::stereoMode,1);h.set(ids::stereoWidth,0);h.noteOn(60);h.render(.05);for(int i=0;i<10;++i){h.renderBlock();for(int j=0;j<h.blockSize;++j)CHECK_NEAR(h.buffer.getSample(0,j),h.buffer.getSample(1,j),1e-6);}
    juce::MemoryBlock state;h.processor.getStateInformation(state);TestHost restored;restored.processor.setStateInformation(state.getData(),(int)state.getSize());CHECK_NEAR(restored.get(ids::stereoMode),1,0);CHECK_NEAR(restored.get(ids::stereoWidth),0,1e-7);
}
AERIFORM_TEST(stereo_page_renders) {
    TestHost h;h.set(ids::stereoMode,1);std::unique_ptr<juce::AudioProcessorEditor> e(h.processor.createEditor());dynamic_cast<AeriformEditor*>(e.get())->showPage(7);
    auto visit=[&](auto&& self,juce::Component& c)->void{if(auto* p=dynamic_cast<ContactPage*>(&c))p->showStereo(true);for(auto* child:c.getChildren())self(self,*child);};visit(visit,*e);
    juce::Image image(juce::Image::ARGB,e->getWidth(),e->getHeight(),true);juce::Graphics g(image);e->paintEntireComponent(g,true);auto f=juce::File::getCurrentWorkingDirectory().getChildFile("docs/experimental/stereo.png");auto stream=f.createOutputStream();CHECK(stream!=nullptr);if(stream){stream->setPosition(0);stream->truncate();CHECK(juce::PNGImageFormat().writeImageToStream(image,*stream));}
}
