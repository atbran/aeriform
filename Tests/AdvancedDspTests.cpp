#include "TestFramework.h"
#include "TestHelpers.h"
#include "AllocationProbe.h"
#include "DSP/Effects/ModularRack.h"
#include "DSP/Exciter.h"
#include "BaselineBreath.h"
#include "Params/BreathCharacters.h"
#include "Plugin/PluginEditor.h"
#include "GUI/EffectsWorkspace.h"
#include "GUI/WorkspacePage.h"
#include <chrono>
#include <fstream>
using namespace aeriform;using namespace aeriform::dsp;using namespace aeriform::test;
namespace {
VoiceParams defaults(){VoiceParams p;createParameterLayout();for(int i=0;i<kNumParams;++i)p.v[(size_t)i]=parameterInfos()[(size_t)i].defaultValue;return p;}
juce::File artifacts(){const char* path=std::getenv("AERIFORM_DSP_ARTIFACTS");auto f=path?juce::File(path):juce::File::getCurrentWorkingDirectory().getChildFile("artifacts/advanced-dsp");f.createDirectory();return f;}
void wav(const juce::String& name,const std::vector<float>& samples,double sr){auto file=artifacts().getChildFile(name+".wav");auto stream=file.createOutputStream();CHECK(stream!=nullptr);if(!stream)return;stream->setPosition(0);stream->truncate();juce::WavAudioFormat fmt;std::unique_ptr<juce::AudioFormatWriter> writer(fmt.createWriterFor(stream.release(),sr,1,24,{},0));CHECK(writer!=nullptr);if(writer){const float* data[]{samples.data()};writer->writeFromFloatArrays(data,1,(int)samples.size());}}
template<class T>std::vector<T*> descendants(juce::Component& c){std::vector<T*> found;for(auto* child:c.getChildren()){if(auto* p=dynamic_cast<T*>(child))found.push_back(p);auto more=descendants<T>(*child);found.insert(found.end(),more.begin(),more.end());}return found;}
void capture(juce::Component& ed,const juce::String& name){auto image=ed.createComponentSnapshot(ed.getLocalBounds(),true);auto output=artifacts().getChildFile(name+".png").createOutputStream();CHECK(output!=nullptr);if(output){output->setPosition(0);output->truncate();juce::PNGImageFormat png;CHECK(png.writeImageToStream(image,*output));}}
std::vector<float> rackRender(VoiceParams p,int block=128){auto rack=std::make_unique<ModularRack>();rack->prepare(48000);ModValues mod{};std::vector<float> result;float l[1024]{},r[1024]{};for(int n=0;n<8192;n+=block)rack->process(l,r,std::min(block,8192-n),p,mod,120,57);for(int n=0;n<16384;n+=block){int count=std::min(block,16384-n);for(int i=0;i<count;++i)l[i]=r[i]=.2f*std::sin((float)(n+i)*.071f)+.15f*std::sin((float)(n+i)*.129f);rack->process(l,r,count,p,mod,120,57);result.insert(result.end(),l,l+count);}return result;}
}
AERIFORM_TEST(advanced_rack_order_matches_manual_chain_and_is_noncommuting){
 auto p=defaults();p.v[(size_t)P::rack1Type]=1;p.v[(size_t)P::rack2Type]=4;p.v[(size_t)P::rack1RdTime]=17;p.v[(size_t)P::rack1RdMix]=.8f;p.v[(size_t)P::rack2SatLowDrive]=24;p.v[(size_t)P::rack2SatMidDrive]=30;
 auto forward=rackRender(p);p.v[(size_t)P::rackOrder]=(float)rackOrderCode({1,0,2,3});auto reverse=rackRender(p);double difference=0;for(size_t i=0;i<forward.size();++i)difference+=std::abs(forward[i]-reverse[i]);CHECK(difference>1);
 p.v[(size_t)P::rackOrder]=0;auto a=std::make_unique<ModularRack>(),b=std::make_unique<ModularRack>();a->prepare(48000);b->prepare(48000);auto pa=p,pb=p;pa.v[(size_t)P::rack2Type]=0;pb.v[(size_t)P::rack1Type]=0;float l[128]{},r[128]{};ModValues mod{};
 for(int n=0;n<8192;n+=128){a->process(l,r,128,pa,mod,120,57);b->process(l,r,128,pb,mod,120,57);}
 double error=0;for(int n=0;n<16384;n+=128){for(int i=0;i<128;++i)l[i]=r[i]=.2f*std::sin((float)(n+i)*.071f)+.15f*std::sin((float)(n+i)*.129f);a->process(l,r,128,pa,mod,120,57);b->process(l,r,128,pb,mod,120,57);for(int i=0;i<128;++i)error=std::max(error,(double)std::abs(l[i]-forward[(size_t)(n+i)]));}CHECK(error<1e-6);
}
AERIFORM_TEST(advanced_rack_duplicate_instances_unused_bank_and_block_partitions){
 auto p=defaults();p.v[(size_t)P::rack1Type]=1;p.v[(size_t)P::rack2Type]=1;p.v[(size_t)P::rack1RdTime]=11;p.v[(size_t)P::rack2RdTime]=29;
 auto original=rackRender(p,128);auto irregular=rackRender(p,127);double err=0;for(size_t i=0;i<original.size();++i)err=std::max(err,(double)std::abs(original[i]-irregular[i]));CHECK(err<1e-5);
 p.v[(size_t)P::rack2ShMix]=1;auto unused=rackRender(p);CHECK(original==unused);
 p.v[(size_t)P::rack2RdTime]=71;auto changed=rackRender(p);CHECK(original!=changed);CHECK(p.get(P::rack1RdTime)==11);
}
AERIFORM_TEST(advanced_rack_extremes_structural_edits_and_callback_allocations){
 auto p=defaults();ModValues mod{};std::ofstream report(artifacts().getChildFile("rack-timing.csv").getFullPathName().toStdString());report<<"rate,block,type,max_us,budget_us,allocations,deallocations\n";
 for(float sr:{44100.0f,48000.0f,96000.0f})for(int block:{17,64,256,1024})for(int type=1;type<=4;++type){auto rack=std::make_unique<ModularRack>();rack->prepare(sr);for(int i=0;i<4;++i){p.v[(size_t)rackTypes[i]]=(float)type;p.v[(size_t)rackEnables[i]]=1;for(size_t f=0;f<std::size(originalRackFields);++f){auto field=rackFields[i][f];const auto& info=parameterInfos()[(size_t)field];if(!info.isBool&&!info.isChoice)p.v[(size_t)field]=info.maxValue;if(originalRackFields[f]==P::sfFreeze)p.v[(size_t)field]=1;}}
 float l[1024]{},r[1024]{};double worst=0;size_t allocations=0,deallocations=0;
 for(int k=0;k<std::max(70,(int)(sr*.25f/block));++k){for(int i=0;i<block;++i)l[i]=r[i]=k<45?.1f*std::sin((float)(k*block+i)*.13f):0;if(k%9==0)p.v[(size_t)P::rackOrder]=(float)(k%24);if(k==40)p.v[(size_t)P::rack1Enabled]=0;if(k==55)p.v[(size_t)P::rack2Type]=(float)(1+type%4);
 auto start=std::chrono::steady_clock::now();AllocationProbe probe;rack->process(l,r,block,p,mod,120,57);auto counts=probe.finish();worst=std::max(worst,std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count());allocations+=counts.allocations;deallocations+=counts.deallocations;for(int i=0;i<block;++i)CHECK(std::isfinite(l[i])&&std::isfinite(r[i]));}
 CHECK(allocations==0);CHECK(deallocations==0);report<<sr<<','<<block<<','<<type<<','<<worst<<','<<(1e6*block/sr)<<','<<allocations<<','<<deallocations<<'\n';}
}
AERIFORM_TEST(advanced_rack_state_undo_modulation_and_character_transactions){
 TestHost h;auto& tools=h.processor.getPatchTools();tools.perform("Configure rack",[&]{tools.setParameter(ids::rack1Type,2);tools.setParameter(ids::rack1ShMix,.72f);tools.setParameter(ids::rack2Type,2);tools.setParameter(ids::rack2ShMix,.19f);tools.setParameter(ids::rackOrder,6);});
 tools.undo.undo();CHECK(h.get(ids::rack1Type)==0);tools.undo.redo();CHECK_NEAR(h.get(ids::rack1ShMix),.72,1e-5);
 juce::MemoryBlock state;h.processor.getStateInformation(state);tools.setParameter(ids::rack1ShMix,.1f);h.processor.setStateInformation(state.getData(),(int)state.getSize());CHECK_NEAR(h.get(ids::rack1ShMix),.72,1e-5);CHECK_NEAR(h.get(ids::rack2ShMix),.19,1e-5);CHECK(h.get(ids::rackOrder)==6);
 const float envelope=h.get(ids::envAttack),resonator=h.get(ids::resFeedback);tools.perform("Flute Air",[&]{for(size_t f=0;f<std::size(breathCharacterFields);++f)tools.setParameter(ids::id(breathCharacterFields[f]),breathCharacters[4][f]);});CHECK_NEAR(h.get(ids::breathEdge),.85,1e-5);CHECK(h.get(ids::envAttack)==envelope);CHECK(h.get(ids::resFeedback)==resonator);tools.undo.undo();CHECK_NEAR(h.get(ids::breathEdge),0,1e-5);
 auto p=defaults();p.v[(size_t)P::rack1Type]=4;auto dry=rackRender(p);p.v[(size_t)P::rack1SatLowDrive]=30;auto driven=rackRender(p);CHECK(dry!=driven);
 for(auto binding:advancedBindings)CHECK(advancedDestination(binding.parameter)!=ModDest::None);
}
AERIFORM_TEST(advanced_ui_four_complete_slots_no_scroll_and_screenshots){
 TestHost h;std::unique_ptr<juce::AudioProcessorEditor> owned(h.processor.createEditor());auto& editor=*dynamic_cast<AeriformEditor*>(owned.get());editor.showPage(4);for(auto* w:descendants<WorkspacePage>(editor))if(w->isVisible())w->showSection(0);
 auto panels=descendants<RackPanel>(editor);CHECK(panels.size()==4);
 auto verify=[&]{for(auto* panel:panels){panel->refresh();CHECK(panel->getWidth()>=500);CHECK(panel->getHeight()>=300);CHECK(panel->getParentComponent()->getLocalBounds().contains(panel->getBounds()));for(auto* child:panel->getChildren())if(child->isVisible()){CHECK_MSG(panel->getLocalBounds().contains(child->getBounds()),child->getComponentID().toStdString());CHECK(child->getHeight()>=20);}}};
 for(int i=0;i<4;++i)h.set(ids::id(rackTypes[i]),(float)(i+1));verify();capture(editor,"rack-mixed");
 for(int type=1;type<=4;++type){for(int i=0;i<4;++i)h.set(ids::id(rackTypes[i]),(float)type);verify();capture(editor,"rack-four-type-"+juce::String(type));}
 editor.showPage(1);capture(editor,"exciters-breath");editor.showPage(0);capture(editor,"main");CHECK(editor.getConstrainer()->getMinimumWidth()==1280);CHECK(editor.getConstrainer()->getMinimumHeight()==900);
}
AERIFORM_TEST(advanced_breath_listening_matrix_and_baseline){
 std::ofstream metrics(artifacts().getChildFile("breath-levels.csv").getFullPathName().toStdString());metrics<<"name,peak,rms,dc\n";
 auto save=[&](const juce::String& name,const std::vector<float>& samples,float sr){double sum=0,energy=0,peak=0;for(float x:samples){CHECK(std::isfinite(x));sum+=x;energy+=x*x;peak=std::max(peak,(double)std::abs(x));}double rms=std::sqrt(energy/samples.size());metrics<<name<<','<<peak<<','<<rms<<','<<sum/samples.size()<<'\n';CHECK(peak<4);wav(name,samples,sr);auto matched=samples;float gain=(float)std::min(.1/std::max(1e-9,rms),.95/std::max(1e-9,peak));for(auto& x:matched)x*=gain;wav(name+"-matched",matched,sr);};
 for(int c=0;c<5;++c)for(int note:{45,60,81})for(float vel:{.2f,.6f,1.0f}){
  auto h=std::make_unique<TestHost>();for(size_t f=0;f<std::size(breathCharacterFields);++f)h->set(ids::id(breathCharacterFields[f]),breathCharacters[c][f]);h->noteOn(note,(int)(vel*127));std::vector<float> full;h->render(1.5,&full);h->noteOff(note);h->render(.5,&full);auto name=juce::String(breathCharacterNames[c]).replaceCharacter(' ','-')+"-"+juce::String(note)+"-"+juce::String((int)(vel*100));save(name+"-synth",full,48000);
  Exciter source;source.prepare(48000,12345);ExciterParams p;p.noise=breathCharacters[c][0];p.noiseColor=breathCharacters[c][1];p.turbulence=breathCharacters[c][2];p.mouth=breathCharacters[c][3];p.swellMs=breathCharacters[c][4];p.settleMs=breathCharacters[c][5];p.contour=breathCharacters[c][6];p.edge=breathCharacters[c][7];p.attackClick=breathCharacters[c][8];p.releaseNoise=breathCharacters[c][9];p.breathRandom=breathCharacters[c][10];source.update(p,midiNoteToHz((float)note),.5f,0);source.noteOn(vel,midiNoteToHz((float)note));source.update(p,midiNoteToHz((float)note),.5f,0);std::vector<float> isolated(96000);for(int i=0;i<96000;++i){if(i==72000)source.noteOff();isolated[(size_t)i]=source.next(0,i<72000?.5f:0);}save(name+"-source",isolated,48000);
 }
 baseline::Exciter old;old.prepare(48000,12345);baseline::ExciterParams oldp;old.update(oldp,261.63f,.5f,0);old.noteOn(.6f,261.63f);old.update(oldp,261.63f,.5f,0);std::vector<float> before(96000);for(int i=0;i<96000;++i){if(i==72000)old.noteOff();before[(size_t)i]=old.next(0,i<72000?.5f:0);}save("baseline-source-60-60",before,48000);
}


AERIFORM_TEST(advanced_breath_extremes_repeatability_pressure_and_long_holds){
 for(float sr:{44100.0f,48000.0f,96000.0f})for(int extreme=0;extreme<2;++extreme){
  Exciter a,b;a.prepare(sr,12345);b.prepare(sr,12345);ExciterParams p;p.mouth=p.edge=p.turbulence=p.contour=(float)extreme;p.swellMs=extreme?500:5;p.settleMs=extreme?2000:20;p.noise=1;p.attackClick=0;p.releaseNoise=0;
  a.update(p,261.63f,.5f,0);b.update(p,261.63f,.5f,0);a.noteOn(.6f,261.63f);b.noteOn(.6f,261.63f);double sum=0,energy=0,peak=0,error=0;
  for(int i=0;i<(int)(sr*5);++i){if(i%32==0)a.update(p,261.63f,.5f,0);if(i%127==0)b.update(p,261.63f,.5f,0);float x=a.next(0,.5f),y=b.next(0,.5f);error=std::max(error,(double)std::abs(x-y));sum+=x;energy+=x*x;peak=std::max(peak,(double)std::abs(x));}
  CHECK(error<1e-7);CHECK(std::isfinite(energy));CHECK(energy>1e-5);CHECK(peak<2);CHECK(std::abs(sum/(sr*5))<.02);
 }
 auto render=[](ExciterParams p,float pressure){Exciter e;e.prepare(48000,7766);e.update(p,440,pressure,0);e.noteOn(.7f,440);e.update(p,440,pressure,0);std::vector<float> samples(24000);for(auto& x:samples)x=e.next(0,.5f);return samples;};
 ExciterParams p;p.attackClick=p.releaseNoise=0;p.pressureBright=0;p.texturePressure=1;auto reference=render(p,0);CHECK(reference!=render(p,1));p.mouth=.95f;CHECK(reference!=render(p,0));p.mouth=.35f;p.noise=0;auto silent=render(p,0);for(float x:silent)CHECK(x==0);
}
AERIFORM_TEST(advanced_rack_empty_bypass_and_modulation_signal){
 auto rack=std::make_unique<ModularRack>();rack->prepare(48000);auto p=defaults();ModValues mod{};float l[256],r[256],original[256];for(int i=0;i<256;++i)original[i]=l[i]=r[i]=.3f*std::sin(i*.07f);rack->process(l,r,256,p,mod,120,57);for(int i=0;i<256;++i)CHECK(l[i]==original[i]&&r[i]==original[i]);
 p.v[(size_t)P::rack1Type]=1;for(int k=0;k<30;++k)rack->process(l,r,256,p,mod,120,57);p.v[(size_t)P::rack1Enabled]=0;for(int k=0;k<30;++k)rack->process(l,r,256,p,mod,120,57);std::copy_n(original,256,l);std::copy_n(original,256,r);rack->process(l,r,256,p,mod,120,57);for(int i=0;i<256;++i)CHECK(l[i]==original[i]&&r[i]==original[i]);
 auto a=std::make_unique<TestHost>(),b=std::make_unique<TestHost>();for(auto* h:{a.get(),b.get()}){h->set(ids::exaModel,(float)ExciterModel::Wave);h->set(ids::rack1Type,4);h->set(ids::macro1,1);}
 a->set(ids::rack1SatMidDrive,18);b->set(ids::mod1Src,(float)ModSource::Macro1);b->set(ids::mod1Dst,(float)ModDest::rack1SatMidDrive);b->set(ids::mod1Depth,.5f);a->noteOn(60);b->noteOn(60);std::vector<float> x,y;a->render(.2,&x);b->render(.2,&y);CHECK(x.size()==y.size());double error=0;for(size_t i=0;i<x.size();++i)error=std::max(error,(double)std::abs(x[i]-y[i]));CHECK(error<1e-5);
}


AERIFORM_TEST(advanced_spectral_actions_survive_reenable_and_ignore_restored_event_parity){
 auto rack=std::make_unique<ModularRack>();rack->prepare(48000);auto p=defaults();p.v[(size_t)P::rack1Type]=3;p.v[(size_t)P::rack1SfFreeze]=1;p.v[(size_t)P::rack1SfCapture]=1;p.v[(size_t)P::rack1SfRelease]=1;ModValues mod{};float l[256],r[256];
 auto run=[&](int blocks){for(int k=0;k<blocks;++k){for(int i=0;i<256;++i)l[i]=r[i]=.1f*std::sin((k*256+i)*.07f);rack->process(l,r,256,p,mod,120,57);}};
 run(20);CHECK(rack->isFrozen(0));p.v[(size_t)P::rack1SfFreeze]=0;p.v[(size_t)P::rack1SfRelease]=0;run(20);CHECK(!rack->isFrozen(0));p.v[(size_t)P::rack1SfFreeze]=1;p.v[(size_t)P::rack1SfCapture]=0;run(20);CHECK(rack->isFrozen(0));p.v[(size_t)P::rack1Enabled]=0;run(20);p.v[(size_t)P::rack1Enabled]=1;run(20);CHECK(rack->isFrozen(0));rack->reset();run(20);CHECK(rack->isFrozen(0));
 TestHost h;auto& tools=h.processor.getPatchTools();tools.setParameter(ids::rack1SfCapture,1);tools.setParameter(ids::rack1SfRelease,1);tools.capture(0);tools.setParameter(ids::rack1SfCapture,0);tools.setParameter(ids::rack1SfRelease,0);tools.selectEndpoint(1);CHECK(h.get(ids::rack1SfCapture)==0);CHECK(h.get(ids::rack1SfRelease)==0);tools.selectEndpoint(0);CHECK(h.get(ids::rack1SfCapture)==0);CHECK(h.get(ids::rack1SfRelease)==0);tools.randomize(PatchStateManager::Scope::Effects,1,false);CHECK(h.get(ids::rack1SfCapture)==0);CHECK(h.get(ids::rack1SfRelease)==0);
}

AERIFORM_TEST(advanced_spectral_live_session_preset_and_undo_restore){
 TestHost h;h.set(ids::rack1Type,3);h.set(ids::rack1SfFreeze,1);h.set(ids::rack1SfRelease,1);h.noteOn(60);h.render(.15);auto held=[&]{return h.processor.getVisualizerModel().rackFrozen[0].load();};CHECK(held());juce::MemoryBlock state;h.processor.getStateInformation(state);
 auto preset=artifacts().getChildFile("spectral-restore.aeriform");CHECK(h.processor.getPresetManager().saveToFile(preset,"Spectral restore","Test"));
 h.set(ids::rack1SfFreeze,0);h.set(ids::rack1SfRelease,0);h.render(.1);CHECK(!held());h.processor.setStateInformation(state.getData(),(int)state.getSize());h.render(.15);CHECK(held());CHECK(h.get(ids::rack1SfRelease)==0);
 auto& tools=h.processor.getPatchTools();tools.perform("Release spectrum",[&]{tools.setParameter(ids::rack1SfFreeze,0);tools.setParameter(ids::rack1SfRelease,1);});h.render(.1);CHECK(!held());tools.undo.undo();h.render(.15);CHECK(held());CHECK(h.get(ids::rack1SfRelease)==1);
 h.set(ids::rack1SfFreeze,0);h.set(ids::rack1SfRelease,0);h.render(.1);CHECK(!held());CHECK(h.processor.getPresetManager().loadFromFile(preset));h.render(.15);CHECK(held());CHECK(h.get(ids::rack1SfRelease)==0);
}

AERIFORM_TEST(advanced_repair_demonstrations_and_audio_render){
    std::ofstream metrics(artifacts().getChildFile("repair-verification-metrics.csv").getFullPathName().toStdString());
    metrics<<"name,peak,rms,dc,description\n";
    auto saveDemo=[&](const juce::String& name,const std::vector<float>& samples,double sr,const juce::String& noteStr=""){
        double sum=0,energy=0,peak=0;
        for(float x:samples){CHECK(std::isfinite(x));sum+=x;energy+=x*x;peak=std::max(peak,(double)std::abs(x));}
        double rms=std::sqrt(energy/std::max(1ULL,samples.size()));
        metrics<<name<<","<<peak<<","<<rms<<","<<sum/std::max(1ULL,samples.size())<<",\""<<noteStr<<"\"\n";
        wav(name,samples,sr);
        auto matched=samples;
        float gain=(float)std::min(0.1/std::max(1e-9,rms),0.95/std::max(1e-9,peak));
        for(auto& x:matched) x*=gain;
        wav(name+"-matched",matched,sr);
    };

    // 1. Resonant Delay Demonstrations
    {
        TestHost hDry;
        hDry.set(ids::rack1Type,0);
        hDry.noteOn(60,100);std::vector<float> audioDry;
        hDry.render(0.15,&audioDry);hDry.noteOff(60);hDry.render(1.85,&audioDry);
        saveDemo("demo-resdelay-dry",audioDry,48000,"Dry short note");

        TestHost hOrd;
        hOrd.set(ids::rack1Type,1);hOrd.set(ids::rack1RdTime,375);hOrd.set(ids::rack1RdFeedback,0.5f);hOrd.set(ids::rack1RdMix,0.5f);
        hOrd.noteOn(60,100);std::vector<float> audioOrd;
        hOrd.render(0.15,&audioOrd);hOrd.noteOff(60);hOrd.render(1.85,&audioOrd);
        saveDemo("demo-resdelay-ordinary",audioOrd,48000,"375ms delay, 50% mix, 50% feedback");

        TestHost hWet;
        hWet.set(ids::rack1Type,1);hWet.set(ids::rack1RdTime,375);hWet.set(ids::rack1RdFeedback,0.85f);hWet.set(ids::rack1RdMix,1.0f);
        hWet.noteOn(60,100);std::vector<float> audioWet;
        hWet.render(0.15,&audioWet);hWet.noteOff(60);hWet.render(2.35,&audioWet);
        saveDemo("demo-resdelay-wet-highfb",audioWet,48000,"375ms delay, 100% wet, 85% feedback");

        double delayedEnergy=0;
        for(size_t i=18000;i<audioWet.size();++i) delayedEnergy+=audioWet[i]*audioWet[i];
        CHECK(delayedEnergy>0.01);
    }

    // 2. Shimmer Reverb Demonstrations
    {
        TestHost hDry;
        hDry.set(ids::rack1Type,0);
        hDry.noteOn(60,100);std::vector<float> audioDry;
        hDry.render(0.4,&audioDry);hDry.noteOff(60);hDry.render(2.6,&audioDry);
        saveDemo("demo-shimmer-dry",audioDry,48000,"Dry note 60");

        TestHost hOrd;
        hOrd.set(ids::rack1Type,2);hOrd.set(ids::rack1ShMix,0.5f);hOrd.set(ids::rack1ShFeedback,0.6f);hOrd.set(ids::rack1ShInterval,12.0f);
        hOrd.noteOn(60,100);std::vector<float> audioOrd;
        hOrd.render(0.4,&audioOrd);hOrd.noteOff(60);hOrd.render(2.6,&audioOrd);
        saveDemo("demo-shimmer-ordinary",audioOrd,48000,"Shimmer 50% mix, 60% feedback, +12st");

        TestHost hWet;
        hWet.set(ids::rack1Type,2);hWet.set(ids::rack1ShMix,1.0f);hWet.set(ids::rack1ShFeedback,0.85f);hWet.set(ids::rack1ShInterval,12.0f);
        hWet.noteOn(60,100);std::vector<float> audioWet;
        hWet.render(0.4,&audioWet);hWet.noteOff(60);hWet.render(3.1,&audioWet);
        saveDemo("demo-shimmer-wet-highfb",audioWet,48000,"Shimmer 100% wet, 85% feedback, +12st tail");

        double tailEnergy=0;
        for(size_t i=48000;i<audioWet.size();++i) tailEnergy+=audioWet[i]*audioWet[i];
        CHECK(tailEnergy>0.05);
    }

    // 3. Two-Sine Interaction Demonstrations
    {
        auto renderTwoSine=[&](float mode, float interaction, float noteA, float noteB, const juce::String& name, const juce::String& desc){
            TestHost h;
            h.set(ids::exaModel,(float)ExciterModel::Wave);h.set(ids::exaWaveShape,0.0f);
            h.set(ids::exbModel,(float)ExciterModel::Wave);h.set(ids::exbWaveShape,0.0f);
            h.set(ids::exaCoarse,noteA-60.0f);
            h.set(ids::exbCoarse,noteB-60.0f);
            h.set(ids::mixMode,mode);
            h.set(ids::mixInteraction,interaction);
            h.noteOn(60,100);
            std::vector<float> audio;
            h.render(1.0,&audio);
            h.noteOff(60);
            h.render(0.5,&audio);
            saveDemo(name,audio,48000,desc);
            return audio;
        };

        auto mixUnison=renderTwoSine((float)InteractionMode::Crossfade, 0.5f, 60, 60, "demo-twosine-unison-mix", "Unison Crossfade (plain sum)");
        auto fmUnison=renderTwoSine((float)InteractionMode::FM, 0.7f, 60, 60, "demo-twosine-unison-fm-07", "Unison FM (interaction 0.7)");
        auto syncUnison4=renderTwoSine((float)InteractionMode::Sync, 0.4f, 60, 60, "demo-twosine-unison-sync-04", "Unison Hard Sync (interaction 0.4)");
        auto syncUnison8=renderTwoSine((float)InteractionMode::Sync, 0.8f, 60, 60, "demo-twosine-unison-sync-08", "Unison Hard Sync (interaction 0.8)");
        auto minmaxUnison=renderTwoSine((float)InteractionMode::MinMax, 0.5f, 60, 60, "demo-twosine-unison-minmax-05", "Unison Min/Max (interaction 0.5)");

        auto mixFifth=renderTwoSine((float)InteractionMode::Crossfade, 0.5f, 60, 67, "demo-twosine-fifth-mix", "Fifth (C4+G4) Crossfade");
        auto fmFifth=renderTwoSine((float)InteractionMode::FM, 0.7f, 60, 67, "demo-twosine-fifth-fm-07", "Fifth (C4+G4) FM (interaction 0.7)");
        auto syncFifth=renderTwoSine((float)InteractionMode::Sync, 0.5f, 60, 67, "demo-twosine-fifth-sync-05", "Fifth (C4+G4) Sync (interaction 0.5)");
        auto minmaxFifth=renderTwoSine((float)InteractionMode::MinMax, 0.5f, 60, 67, "demo-twosine-fifth-minmax-05", "Fifth (C4+G4) Min/Max (interaction 0.5)");

        double diffFm=0, diffSync=0, diffMinMax=0;
        for(size_t i=0;i<mixFifth.size();++i){
            diffFm+=std::abs(mixFifth[i]-fmFifth[i]);
            diffSync+=std::abs(mixFifth[i]-syncFifth[i]);
            diffMinMax+=std::abs(mixFifth[i]-minmaxFifth[i]);
        }
        CHECK(diffFm>5.0);
        CHECK(diffSync>5.0);
        CHECK(diffMinMax>5.0);
    }

    // 4. Exposed Breath Demonstrations
    {
        auto renderExposed=[&](int charIdx, const juce::String& name, const juce::String& desc){
            Exciter source;source.prepare(48000,12345);ExciterParams p;
            p.noise=breathCharacters[charIdx][0];p.noiseColor=breathCharacters[charIdx][1];p.turbulence=breathCharacters[charIdx][2];
            p.mouth=breathCharacters[charIdx][3];p.swellMs=breathCharacters[charIdx][4];p.settleMs=breathCharacters[charIdx][5];
            p.contour=breathCharacters[charIdx][6];p.edge=breathCharacters[charIdx][7];p.attackClick=breathCharacters[charIdx][8];
            p.releaseNoise=breathCharacters[charIdx][9];p.breathRandom=breathCharacters[charIdx][10];
            source.update(p,261.63f,0.5f,0);source.noteOn(0.7f,261.63f);source.update(p,261.63f,0.5f,0);
            std::vector<float> audio(96000);
            for(int i=0;i<96000;++i){if(i==72000)source.noteOff();audio[(size_t)i]=source.next(0,i<72000?0.5f:0);}
            saveDemo(name,audio,48000,desc);
        };
        renderExposed(0,"demo-breath-soft-exhale-exposed","Soft Exhale exposed noise (vowel formants)");
        renderExposed(4,"demo-breath-flute-air-exposed","Flute Air exposed noise (vocal body + edge)");

        auto renderSynth=[&](int charIdx, const juce::String& name, const juce::String& desc){
            TestHost h;
            for(size_t f=0;f<std::size(breathCharacterFields);++f) h.set(ids::id(breathCharacterFields[f]),breathCharacters[charIdx][f]);
            h.noteOn(60,90);
            std::vector<float> audio;
            h.render(1.5,&audio);
            h.noteOff(60);
            h.render(0.5,&audio);
            saveDemo(name,audio,48000,desc);
        };
        renderSynth(0,"demo-breath-soft-exhale-synth","Soft Exhale initialized synth");
        renderSynth(4,"demo-breath-flute-air-synth","Flute Air initialized synth");
    }
}

