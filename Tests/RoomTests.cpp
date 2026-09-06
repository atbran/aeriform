#include "TestFramework.h"
#include "TestHelpers.h"
#include "DSP/CoupledRoom.h"
#include "Plugin/PluginEditor.h"
using namespace aeriform;using namespace aeriform::dsp;using namespace aeriform::test;
namespace {
void settleRoom(CoupledRoom& r,const RoomParams& p){for(int i=0;i<100;++i)r.update(p,256);}
std::vector<float> renderRoom(int chunk){CoupledRoom room;room.prepare(48000);RoomParams p;p.enabled=true;p.send=1;p.level=1;p.networkReturn=1;p.returnDelayMs=3;settleRoom(room,p);std::vector<float> signal;float returned[32];
    for(int t=0;t<24000;){int n=std::min(chunk,24000-t);room.makeReturn(returned,n,1);for(int i=0;i<n;++i){float l,r;room.next(.1f*std::sin((t+i)*.04f)+returned[i]*.1f,.1f*std::sin((t+i)*.031f),1,l,r);signal.push_back(l);signal.push_back(returned[i]);}t+=n;}return signal;}
}
AERIFORM_TEST(room_return_timing_is_independent_of_chunk_size) {
    auto a=renderRoom(1);for(int chunk:{7,16,32}){auto b=renderRoom(chunk);CHECK(a.size()==b.size());for(size_t i=0;i<a.size();++i)CHECK_NEAR(a[i],b[i],2e-6);}
}
AERIFORM_TEST(room_is_bounded_without_master_limiter_at_multiple_rates_and_voice_counts) {
    for(float sr:{44100.0f,48000.0f,96000.0f})for(int voices:{1,8,16}){CoupledRoom room;room.prepare(sr);RoomParams p;p.enabled=true;p.feedback=1;p.size=1;p.send=1;p.level=2;p.networkReturn=1;p.wallDamping=0;p.airAbsorption=0;p.returnDelayMs=1;settleRoom(room,p);float returned[32];double energy=0;
        for(int t=0;t<(int)sr;t+=32){room.makeReturn(returned,32,voices);for(int i=0;i<32;++i){float l,r;const float x=voices*.8f*std::sin((t+i)*.09f);CHECK(std::abs(returned[i])<=1.00001f);room.next(x+returned[i]*voices,x,voices,l,r);CHECK(std::isfinite(l)&&std::isfinite(r));CHECK(std::abs(l)<4&&std::abs(r)<4);energy+=l*l+r*r;}}
        CHECK(energy>1e-5);CHECK(room.safetyClips()==0);
    }
}
AERIFORM_TEST(room_freeze_and_clear_have_real_stateful_effect) {
    auto tail=[](bool freeze){CoupledRoom room;room.prepare(48000);RoomParams p;p.enabled=true;p.feedback=.7f;p.send=1;p.level=1;settleRoom(room,p);float l,r;for(int i=0;i<24000;++i)room.next(.7f*std::sin(i*.07f),.7f*std::sin(i*.11f),1,l,r);p.freeze=freeze;settleRoom(room,p);double energy=0;for(int i=0;i<96000;++i){room.next(0,0,1,l,r);if(i>72000)energy+=l*l+r*r;}CHECK(room.safetyClips()==0);p.clear=true;room.update(p,256);for(int i=0;i<4000;++i){room.next(0,0,1,l,r);CHECK_NEAR(l,0,0);CHECK_NEAR(r,0,0);}return energy;};
    auto normal=tail(false),held=tail(true);CHECK(held>1e-8);CHECK(held>normal*100);
}
AERIFORM_TEST(room_host_send_and_network_return_reach_audio) {
    auto render=[](float level,float feedback){TestHost h;h.set(ids::roomOn,1);h.set(ids::roomSend,1);h.set(ids::roomLevel,level);h.set(ids::roomNetworkReturn,feedback);for(int n:{48,52,55,59,62,64,67,71})h.noteOn(n);std::vector<float>x;CHECK(h.render(.5,&x).finite);CHECK(h.processor.getVisualizerModel().roomSafetyClips.load()==0);return x;};
    auto base=render(0,0),audible=render(1,0),coupled=render(0,1);double wetDifference=0,returnDifference=0;for(size_t i=0;i<base.size();++i){wetDifference+=std::pow(base[i]-audible[i],2);returnDifference+=std::pow(base[i]-coupled[i],2);}std::printf("    room output difference energy %.8f, network-return difference %.8f\n",wetDifference,returnDifference);CHECK(wetDifference>1e-7);CHECK(returnDifference>1e-10);
}

AERIFORM_TEST(room_page_and_session_restore) {
    TestHost h;h.processor.setEditorPage(2);h.processor.setEditorSection(2,3);h.set(ids::roomOn,1);h.set(ids::roomReturnDelay,47);h.noteOn(60);h.render(.3);
    std::unique_ptr<juce::AudioProcessorEditor> e(h.processor.createEditor());
    juce::Image image(juce::Image::ARGB,e->getWidth(),e->getHeight(),true);juce::Graphics g(image);e->paintEntireComponent(g,true);
    auto stream=juce::File::getCurrentWorkingDirectory().getChildFile("docs/experimental/room.png").createOutputStream();CHECK(stream!=nullptr);if(stream){stream->setPosition(0);stream->truncate();CHECK(juce::PNGImageFormat().writeImageToStream(image,*stream));}
    juce::MemoryBlock state;h.processor.getStateInformation(state);TestHost restored;restored.processor.setStateInformation(state.getData(),(int)state.getSize());
    CHECK(restored.processor.getEditorSection(2)==3);CHECK_NEAR(restored.processor.getAPVTS().getRawParameterValue(ids::roomReturnDelay)->load(),47,.001);
}
