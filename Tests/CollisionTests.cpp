#include "TestFramework.h"
#include "TestHelpers.h"
#include "DSP/CollisionRoute.h"
#include "DSP/ResonatorNetwork.h"
#include "Plugin/PluginEditor.h"
#include <complex>
using namespace aeriform;using namespace aeriform::dsp;using namespace aeriform::test;

AERIFORM_TEST(collision_instantaneous_scattering_does_not_create_energy) {
    Noise rng;rng.seed(9384);
    for(int i=0;i<20000;++i){ContactParams p;p.gap=(rng.next()+1)*.5f;p.stiffness=(rng.next()+1)*.5f;p.hardness=1+(rng.next()+1)*1.5f;p.damping=(rng.next()+1)*.5f;p.friction=(rng.next()+1)*.5f;p.asymmetry=rng.next();p.amount=(rng.next()+1)*.5f;p.polarity=i%2?1:-1;
        const float a=rng.next()*4,b=rng.next()*4,f=CollisionRoute::force(a,b,p);
        CHECK(std::isfinite(f));CHECK(std::abs(f)<=.45f);CHECK(std::pow(a-f,2)+std::pow(b+p.polarity*f,2)<=a*a+b*b+1e-5f);
        if(std::abs(a)<p.gap*(1-.9f*std::abs(p.asymmetry)))CHECK_NEAR(f,0,0);
    }
}
AERIFORM_TEST(collision_oversampling_suppresses_folded_harmonics) {
    constexpr int n=8192,bin=1500,alias=n-3*bin;double magnitude[3]{};
    for(int q=0;q<3;++q){CollisionRoute route;route.prepare(48000);ContactParams p;p.enabled=true;p.amount=1;p.gap=.05f;p.stiffness=1;p.hardness=1;p.damping=0;p.quality=q;route.update(p);
        std::complex<double> sum{};float input[3]{},output[3]{};
        for(int i=0;i<2*n;++i){input[0]=.7f*(float)std::sin(2*3.141592653589793*bin*i/n);route.next(input,output);CHECK(std::isfinite(output[1]));if(i>=n)sum+=(double)output[1]*std::exp(std::complex<double>(0,-2*3.141592653589793*alias*i/n));}
        magnitude[q]=std::abs(sum);}
    const double suppression=20*std::log10(magnitude[0]/std::max(1e-15,magnitude[2]));std::printf("    contact third-harmonic alias suppression 1x -> 4x: %.2f dB\n",suppression);CHECK(suppression>12);
}
AERIFORM_TEST(collision_routes_switch_smoothly_and_remain_bounded_without_output_limiter) {
    for(float sr:{44100.0f,48000.0f,96000.0f}){ResonatorNetwork network;network.prepare(sr);NetworkParams p;p.mode=NetMode::Parallel;p.on[1]=p.on[2]=true;p.contact.enabled=true;p.contact.amount=1;p.contact.gap=0;p.contact.friction=1;p.contact.stiffness=1;p.contact.hardness=1;
        for(auto& res:p.res){res.freqHz=220;res.feedback=1;res.saturation=0;res.damping=.1f;}network.update(p,true);
        float peak=0,l=0,r=0;for(int i=0;i<(int)(sr*2);++i){if(i%1024==0){p.contact.source=(i/1024)%3;p.contact.destination=(p.contact.source+1)%3;p.contact.quality=(i/1024)%3;p.contact.polarity=(i/1024)%2?1:-1;network.update(p,false);}network.next(i<(int)sr?.1f*std::sin(i*.04f):0,0,0,l,r);CHECK(network.isFinite());peak=std::max(peak,std::max(std::abs(l),std::abs(r)));}CHECK(peak<4);}
    CollisionRoute route;route.prepare(48000);ContactParams p;p.enabled=true;p.amount=1;p.gap=0;p.stiffness=1;p.quality=2;route.update(p);float last[3]{},input[3]{},out[3]{};float delta=0;
    for(int i=0;i<18000;++i){if(i==5000){p.source=2;p.destination=0;route.update(p);}if(i==10000){p.quality=0;route.update(p);}if(i==14000){p.enabled=false;route.update(p);}input[0]=.2f*std::sin(i*.03f);input[1]=.1f*std::sin(i*.02f);input[2]=-.3f*std::sin(i*.015f);route.next(input,out);for(int k=0;k<3;++k){delta=std::max(delta,std::abs(out[k]-last[k]));last[k]=out[k];}}
    CHECK(delta<.02f);
}
AERIFORM_TEST(collision_host_controls_change_audio_and_restore_state) {
    auto render=[](bool on){TestHost h;h.set(ids::netMode,(float)NetMode::Parallel);h.set(ids::rbOn,1);h.set(ids::contactOn,on?1:0);h.set(ids::contactGap,0);h.set(ids::contactAmount,1);h.set(ids::contactStiffness,1);h.set(ids::rbCoarse,7);h.noteOn(60);std::vector<float> x;h.render(.4,&x);return x;};
    auto a=render(false),b=render(true);double difference=0;for(size_t i=0;i<a.size();++i)difference+=std::pow(a[i]-b[i],2);double reference=0;for(float v:a)reference+=v*v;std::printf("    contact audio difference relative to bypass: %.2f dB\n",10*std::log10(difference/reference));CHECK(difference/reference>.001);
    TestHost h;h.set(ids::contactOn,1);h.set(ids::contactFriction,.73f);juce::MemoryBlock state;h.processor.getStateInformation(state);TestHost restored;restored.processor.setStateInformation(state.getData(),(int)state.getSize());CHECK_NEAR(restored.get(ids::contactFriction),.73f,1e-5);CHECK(restored.get(ids::contactOn)>.5f);
}
AERIFORM_TEST(collision_page_renders) {
    TestHost h;h.set(ids::contactOn,1);std::unique_ptr<juce::AudioProcessorEditor> base(h.processor.createEditor());auto* e=dynamic_cast<AeriformEditor*>(base.get());CHECK(e!=nullptr);if(!e)return;e->showPage(7);
    juce::Image image(juce::Image::ARGB,e->getWidth(),e->getHeight(),true);juce::Graphics g(image);e->paintEntireComponent(g,true);auto f=juce::File::getCurrentWorkingDirectory().getChildFile("docs/experimental/contact.png");auto stream=f.createOutputStream();CHECK(stream!=nullptr);if(stream){stream->setPosition(0);stream->truncate();CHECK(juce::PNGImageFormat().writeImageToStream(image,*stream));}
}
