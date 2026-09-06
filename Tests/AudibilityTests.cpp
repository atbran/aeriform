#include <juce_audio_formats/juce_audio_formats.h>
#include "TestFramework.h"
#include "TestHelpers.h"
#include "AllocationProbe.h"
#include "DSP/SympatheticBank.h"
#include "DSP/CoupledRoom.h"
#include "DSP/RoomCoupling.h"
#include "Presets/FactoryPresets.h"
using namespace aeriform;using namespace aeriform::dsp;using namespace aeriform::test;
namespace {
using Audition=AeriformProcessor::ReturnAudition;
void patch(TestHost& h,const char* name) {
    for(const auto& p:factoryPresets())if(p.name==name){h.processor.getPresetManager().applyFactoryPreset(p);return;}
    CHECK(false);
}
void writeAudio(const juce::String& name,const juce::AudioBuffer<float>& b,double sr) {
    auto dir=juce::File::getCurrentWorkingDirectory().getChildFile("build/audibility/returns");dir.createDirectory();
    auto file=dir.getChildFile(name+".wav");auto stream=file.createOutputStream();CHECK(stream!=nullptr);
    if(stream){stream->setPosition(0);stream->truncate();juce::WavAudioFormat format;
        std::unique_ptr<juce::AudioFormatWriter> writer(format.createWriterFor(stream.release(),sr,2,24,{},0));
        CHECK(writer!=nullptr);if(writer)CHECK(writer->writeFromAudioSampleBuffer(b,0,b.getNumSamples()));}
}
}
AERIFORM_TEST(audibility_named_patch_return_measurements) {
    for(const char* name:{"Init","Plucked Tube","Metallic Steam"}) {
        double dryEnergy=0;
        for(auto target:{Audition::Off,Audition::Sympathetic,Audition::Room}) {
            TestHost h;patch(h,name);h.set(ids::chorusMix,0);h.set(ids::delayMix,0);h.set(ids::reverbMix,0);h.set(ids::limiterOn,0);
            h.set(ids::roomNetworkReturn,0);h.set(ids::symOn,target==Audition::Sympathetic?1:0);h.set(ids::roomOn,target==Audition::Room?1:0);
            h.processor.setReturnAudition(target);h.render(.5);h.noteOn(60,102);
            juce::AudioBuffer<float> audio(2,48000*3);double energy=0;float peak=0;
            for(int offset=0;offset<audio.getNumSamples();offset+=256) {
                if(offset==72192)h.noteOff(60);
                const auto stats=h.renderBlock();CHECK(stats.finite);peak=std::max(peak,stats.peak);
                const int n=std::min(256,audio.getNumSamples()-offset);
                for(int c=0;c<2;++c){audio.copyFrom(c,offset,h.buffer,c,0,n);for(int i=0;i<n;++i)energy+=std::pow(h.buffer.getSample(c,i),2);}
            }
            if(target==Audition::Off)dryEnergy=energy;
            const double relativeDb=10*std::log10(std::max(1e-30,energy)/std::max(1e-30,dryEnergy));
            const juce::String label=juce::String(name).replaceCharacter(' ','_')+(target==Audition::Off?"-dry":target==Audition::Room?"-room":"-bank");
            std::printf("    %s: return/dry %.2f dB, peak %.5f\n",label.toRawUTF8(),relativeDb,peak);
            CHECK(energy>0);CHECK(peak<2);if(target!=Audition::Off){CHECK(relativeDb>-20);CHECK(relativeDb<-6);}
            writeAudio(label,audio,48000);
        }
    }
}
AERIFORM_TEST(audibility_monitor_preserves_the_running_graph_and_allocates_nothing) {
    for(bool deep:{false,true}) {
        TestHost normal(48000,32),monitored(48000,32);
        for(auto* h:{&normal,&monitored}) {
            h->set(ids::roomOn,1);h->set(ids::symOn,1);h->set(ids::delayMix,.4f);
            if(deep){h->processor.getPatchTools().capture(0);h->processor.getPatchTools().capture(1);h->set(ids::morphOn,1);h->set(ids::morphMode,1);h->set(ids::morphPosition,.5f);}
            h->noteOn(60);h->render(.1);
        }
        for(auto target:{Audition::Sympathetic,Audition::Room,Audition::Off}) {
            monitored.processor.setReturnAudition(target);
            for(int block=0;block<100;++block) {
                normal.renderBlock();AllocationProbe probe;monitored.renderBlock();const auto alloc=probe.finish();CHECK(alloc.allocations==0);CHECK(alloc.deallocations==0);
                const auto& a=normal.processor.getEngine().getReturnTaps();const auto& b=monitored.processor.getEngine().getReturnTaps();
                for(int c=0;c<4;++c)for(int i=0;i<32;++i)CHECK_NEAR(a.getSample(c,i),b.getSample(c,i),0);
            }
        }
        // Output/DC history also needs time to settle after monitoring, but the musical graph is exact.
        normal.render(.5);monitored.render(.5);
        for(int c=0;c<2;++c)for(int i=0;i<32;++i)CHECK_NEAR(normal.buffer.getSample(c,i),monitored.buffer.getSample(c,i),2e-5);
        monitored.processor.setReturnAudition(Audition::Room);juce::MemoryBlock state;monitored.processor.getStateInformation(state);
        monitored.processor.setStateInformation(state.getData(),(int)state.getSize());CHECK(monitored.processor.getReturnAudition()==Audition::Off);
        monitored.processor.setReturnAudition(Audition::Sympathetic);monitored.processor.getPresetManager().loadInit();CHECK(monitored.processor.getReturnAudition()==Audition::Off);
    }
}
AERIFORM_TEST(audibility_bank_antiphase_counts_transients_and_long_term_capacity) {
    for(float sr:{44100.f,48000.f,96000.f})for(int count:{1,3,6,12}) {
        SympatheticBank b;b.prepare(sr);SympatheticParams p;p.enabled=true;p.count=count;p.tuning=5;p.root=60;p.intervals.fill(0);p.send=1;p.returnLevel=2;p.decayMs=20000;p.damping=0;
        b.update(p,(int)sr);float l,r;for(int i=0;i<(int)(sr*.1);++i)b.next(0,0,1,l,r);
        double energy=0,maxState=0;
        AllocationProbe probe;
        for(int i=0;i<(int)(sr*4);++i){float x=.8f*std::sin(kTwoPi*midiNoteToHz(60)*i/sr);b.next(x,-x,i<(int)sr?1:16,l,r);
            CHECK(std::isfinite(l)&&std::isfinite(r));energy+=l*l+r*r;double state=0;for(int mode=0;mode<count;++mode)state+=std::pow(b.modeEnergy(mode),2);maxState=std::max(maxState,state);}
        const auto alloc=probe.finish();CHECK(alloc.allocations==0&&alloc.deallocations==0);CHECK(energy>1);CHECK(maxState<=.75001*.75001);CHECK(b.safetyClips()==0);
    }
    // Exact same audio must not jump simply because the voice bookkeeping changes.
    SympatheticBank a,b;a.prepare(48000);b.prepare(48000);SympatheticParams p;p.enabled=true;a.update(p,48000);b.update(p,48000);
    for(int i=0;i<48000;++i){float l,r,u,v;float x=.2f*std::sin(i*.04f);a.next(x,1,l,r);b.next(x,1+(i/2000)%16,u,v);CHECK_NEAR(l,u,0);CHECK_NEAR(r,v,0);}
    // Compare the initial ringing after a brief broadband strike at very different T60s.
    double initial[2]{};
    for(int setting=0;setting<2;++setting){SympatheticBank bank;bank.prepare(48000);p.decayMs=setting?20000:1000;p.damping=0;p.count=12;bank.update(p,48000);float l,r;
        for(int i=0;i<4800;++i)bank.next(0,1,l,r);
        for(int i=0;i<9600;++i){const float x=i<48?.4f:0;bank.next(x,1,l,r);if(i>=48&&i<2400)initial[setting]+=l*l+r*r;}
        CHECK(bank.safetyClips()==0);}
    CHECK(initial[0]>1e-4);CHECK(initial[1]/initial[0]>.5);CHECK(initial[1]/initial[0]<3);
}

AERIFORM_TEST(audibility_room_coupling_cannot_fund_itself) {
    for(float sr:{44100.f,48000.f,96000.f}) {
        RoomCoupling budget;budget.prepare(sr);double energy=0;
        for(int i=0;i<(int)sr;++i)CHECK_NEAR(budget.next(100,0),0,0);
        AllocationProbe probe;
        for(int i=0;i<(int)(sr*8);++i) {
            const float source=i<(int)sr?.15f*std::sin(i*.17f):0;
            // Hostile return continues for seven seconds after all fresh excitation stops.
            const float y=budget.next(100*std::sin(i*.09f),source*source);
            CHECK(std::isfinite(y)&&std::abs(y)<=.250001f);energy+=y*y/sr;
            CHECK(budget.outputEnergy()<=budget.inputCredit()+1e-9);
            CHECK(budget.storedBudget()>=0&&budget.storedBudget()<=.05);
            if(i>(int)(sr*7))CHECK(std::abs(y)<1e-9f);
        }
        const auto alloc=probe.finish();CHECK(alloc.allocations==0&&alloc.deallocations==0);CHECK(energy>1e-5);
        budget.reset();CHECK_NEAR(budget.next(100,0),0,0);
    }
}
AERIFORM_TEST(audibility_room_return_only_and_output_independence) {
    for(const char* name:{"Init","Plucked Tube","Metallic Steam"}) {
        std::vector<float> baseline;double baseEnergy=0;
        for(float strength:{0.f,.2f,.5f,1.f}) {
            TestHost h;patch(h,name);h.set(ids::limiterOn,0);h.set(ids::roomOn,1);h.set(ids::roomLevel,0);h.set(ids::roomNetworkReturn,strength);
            h.set(ids::chorusMix,0);h.set(ids::delayMix,0);h.set(ids::reverbMix,0);h.render(.3);h.noteOn(60);
            std::vector<float> audio;CHECK(h.render(2,&audio).finite);CHECK(h.processor.getVisualizerModel().roomSafetyClips.load()==0);
            if(strength==0){baseline=audio;for(float x:audio)baseEnergy+=x*x;}
            else {double difference=0;for(size_t i=0;i<audio.size();++i)difference+=std::pow(audio[i]-baseline[i],2);
                const double ratio=10*std::log10(difference/baseEnergy);std::printf("    %s return-only %.1f: difference/dry %.2f dB\n",name,strength,ratio);
                double renderedEnergy=0;for(float x:audio)renderedEnergy+=x*x;
                const double match=std::sqrt(baseEnergy/renderedEnergy);double matchedDifference=0;
                for(size_t i=0;i<audio.size();++i)matchedDifference+=std::pow(match*audio[i]-baseline[i],2);
                std::printf("      level change %.2f dB; matched difference %.2f dB\n",10*std::log10(renderedEnergy/baseEnergy),10*std::log10(matchedDifference/baseEnergy));
                if(strength>=.5f)CHECK(ratio>-35);
                juce::AudioBuffer<float> pair(2,(int)audio.size());
                for(int c=0;c<2;++c)for(int i=0;i<pair.getNumSamples();++i)pair.setSample(c,i,audio[(size_t)i]);
                writeAudio(juce::String(name).replaceCharacter(' ','_')+"-coupling-"+juce::String(strength,1),pair,48000);}
        }
    }
    CoupledRoom a,b;a.prepare(48000);b.prepare(48000);RoomParams p;p.enabled=true;p.networkReturn=1;p.level=0;
    a.update(p,48000);p.level=2;b.update(p,48000);
    for(int i=0;i<48000;++i){float x[1],y[1],l,r;a.makeReturn(x,1,1);b.makeReturn(y,1,1);CHECK_NEAR(x[0],y[0],0);
        a.next(.1f*std::sin(i*.04f),.2f*std::sin(i*.06f),1,l,r);b.next(.1f*std::sin(i*.04f),.2f*std::sin(i*.06f),1,l,r);CHECK_NEAR(a.energy(),b.energy(),0);}
}
AERIFORM_TEST(audibility_room_extreme_instrument_paths_without_master_limiter) {
    for(double sr:{44100.,48000.,96000.})for(int configuration:{0,1,2}) {
        TestHost h(sr,configuration==0?32:256);h.set(ids::limiterOn,0);h.set(ids::roomOn,1);h.set(ids::roomNetworkReturn,1);h.set(ids::roomSend,1);h.set(ids::roomLevel,0);
        h.set(ids::roomFeedback,1);h.set(ids::roomWallDamping,0);h.set(ids::roomAir,0);h.set(ids::resFeedback,1);
        if(configuration>0){patch(h,configuration==1?"Repipe Morph":"Energy Loop Drone");h.set(ids::limiterOn,0);h.set(ids::roomOn,1);h.set(ids::roomNetworkReturn,1);h.set(ids::roomSend,1);h.set(ids::roomLevel,0);h.set(ids::roomFeedback,1);h.set(ids::symOn,1);h.set(ids::symSend,1);h.set(ids::stereoMode,1);h.set(ids::contactOn,1);}
        for(int n=0;n<(configuration==2?16:4);++n)h.noteOn(48+n);
        const auto sustained=h.render(4);CHECK(sustained.finite);CHECK(sustained.peak<4);CHECK(h.activeVoices()>0);CHECK(h.processor.getVisualizerModel().roomSafetyClips.load()==0);
        for(int n=0;n<16;++n)h.noteOff(48+n);CHECK(h.render(2).finite);
    }
}

AERIFORM_TEST(audibility_tone_controls_change_level_matched_audio) {
    auto difference=[](const std::vector<float>& a,const std::vector<float>& b){double ae=0,be=0,error=0;for(float x:a)ae+=x*x;for(float x:b)be+=x*x;const double scale=std::sqrt(ae/be);for(size_t i=0;i<a.size();++i)error+=std::pow(a[i]-scale*b[i],2);return std::sqrt(error/ae);};
    auto bank=[](int setting){SympatheticBank b;b.prepare(48000);SympatheticParams p;p.enabled=true;p.send=1;p.root=60;
        if(setting==1)p.brightness=1;if(setting==2)p.damping=1;if(setting==3)p.damper=.6f;if(setting==4)p.root=67;if(setting==5)p.count=3;
        b.update(p,48000);float l,r;for(int i=0;i<4800;++i)b.next(0,1,l,r);Noise noise;noise.seed(7654321);std::vector<float> out;
        for(int i=0;i<48000;++i){b.next(i<2400?.2f*noise.next():0,1,l,r);out.push_back(l);out.push_back(r);}return out;};
    const auto bankReference=bank(0);
    for(int setting=1;setting<=5;++setting){const double d=difference(bankReference,bank(setting));std::printf("    bank control %d matched residual %.3f\n",setting,d);CHECK(d>.08);}
    auto room=[](int setting){CoupledRoom b;b.prepare(48000);RoomParams p;p.enabled=true;p.send=1;p.networkReturn=0;
        if(setting==1)p.size=1;if(setting==2)p.shape=0;if(setting==3)p.diffusion=0;if(setting==4)p.wallDamping=1;if(setting==5)p.airAbsorption=1;
        b.update(p,48000);float l,r;for(int i=0;i<4800;++i)b.next(0,0,1,l,r);Noise noise;noise.seed(7654321);std::vector<float> out;
        for(int i=0;i<24000;++i){const float x=i<2400?.2f*noise.next():0;b.next(x,x,1,l,r);out.push_back(l);out.push_back(r);}return out;};
    const auto roomReference=room(0);
    for(int setting=1;setting<=5;++setting){const double d=difference(roomReference,room(setting));std::printf("    room control %d matched residual %.3f\n",setting,d);CHECK(d>.08);}
}
AERIFORM_TEST(audibility_mono_oversized_blocks_and_bank_bypass_clear) {
    for(bool deep:{false,true}) {
        TestHost h(48000,32);h.processor.setPlayConfigDetails(0,1,48000,32);h.set(ids::roomOn,1);h.set(ids::symOn,1);
        if(deep){h.processor.getPatchTools().capture(0);h.processor.getPatchTools().capture(1);h.set(ids::morphOn,1);h.set(ids::morphMode,1);h.set(ids::morphPosition,.5f);}
        h.noteOn(60);h.render(.2);h.buffer.setSize(2,1024);h.blockSize=1024;
        for(auto target:{Audition::Sympathetic,Audition::Room,Audition::Off}){h.processor.setReturnAudition(target);CHECK(h.render(.1).finite);CHECK(h.activeVoices()>0);}
        std::unique_ptr<juce::AudioProcessorEditor> editor(h.processor.createEditor());h.processor.setReturnAudition(Audition::Room);editor.reset();CHECK(h.processor.getReturnAudition()==Audition::Off);
    }
    SympatheticBank b;b.prepare(48000);SympatheticParams p;p.enabled=true;b.update(p,48000);float l,r;
    for(int i=0;i<48000;++i)b.next(.2f*std::sin(i*.04f),1,l,r);
    p.enabled=false;b.update(p,48000);for(int i=0;i<4800;++i)b.next(0,1,l,r);
    CHECK(!b.active());for(int i=0;i<12;++i)CHECK_NEAR(b.modeEnergy(i),0,0);
    p.enabled=true;b.update(p,48000);for(int i=0;i<4800;++i){b.next(0,1,l,r);CHECK_NEAR(l,0,0);CHECK_NEAR(r,0,0);}
}
AERIFORM_TEST(audibility_cpu_profile_all_quality_modes) {
    for(int voices:{1,8,16})for(int quality=0;quality<3;++quality) {
        double usage[2]{};
        for(int on=0;on<2;++on){TestHost h;h.set(ids::voiceCount,16);h.set(ids::quality,(float)quality);h.set(ids::symOn,(float)on);h.set(ids::roomOn,(float)on);
            for(int i=0;i<voices;++i)h.noteOn(48+i);h.render(.25);
            const auto start=juce::Time::getHighResolutionTicks();CHECK(h.render(1).finite);
            usage[on]=100*juce::Time::highResolutionTicksToSeconds(juce::Time::getHighResolutionTicks()-start);
        }
        std::printf("    voices %d quality %d: off %.2f%%, bank+room %.2f%% of real time\n",voices,quality,usage[0],usage[1]);
    }
}
