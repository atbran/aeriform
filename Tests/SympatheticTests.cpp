#include "TestFramework.h"
#include "TestHelpers.h"
#include "DSP/SympatheticBank.h"
#include "Plugin/PluginEditor.h"
using namespace aeriform;using namespace aeriform::dsp;using namespace aeriform::test;
namespace {
void settle(SympatheticBank& b,const SympatheticParams& p){for(int i=0;i<100;++i)b.update(p,256);}
void run(SympatheticBank& b,int samples,float hz,float sr,float level,int voices=1,std::vector<float>* audio=nullptr){for(int i=0;i<samples;++i){float l,r;b.next(level*std::sin(2*kPi*hz*i/sr),voices,l,r);CHECK(std::isfinite(l)&&std::isfinite(r));CHECK(std::abs(l)<8.01f&&std::abs(r)<8.01f);if(audio)audio->push_back(.5f*(l+r));}}
}
AERIFORM_TEST(sympathetic_tuning_matches_scales_and_measured_audio) {
    for(float sr:{44100.0f,48000.0f,96000.0f}){SympatheticBank b;b.prepare(sr);SympatheticParams p;p.enabled=true;p.count=1;p.root=60;p.tuning=0;p.damping=0;p.decayMs=10000;p.spread=0;p.send=1;p.returnLevel=1;settle(b,p);CHECK_NEAR(b.frequency(0),midiNoteToHz(60),.02);
        run(b,(int)sr,261.6256f,sr,.3f);std::vector<float> tail;run(b,(int)(sr*.4f),0,sr,0,1,&tail);double hz=estimatePeakFrequency(tail,sr,261.6256);CHECK(std::abs(centsBetween(hz,261.6256))<2);
        for(int tuning=0;tuning<7;++tuning){p.tuning=tuning;p.count=12;settle(b,p);float expected=261.6256f;if(tuning==1||tuning==2||tuning==3||tuning==4)expected*=std::exp2(2/12.0f);else if(tuning==5)expected*=std::exp2(4/12.0f);else if(tuning==6)expected*=2;else expected*=std::exp2(1/12.0f);CHECK_NEAR(b.frequency(1),expected,.1);}
    }
}
AERIFORM_TEST(sympathetic_decay_damper_freeze_and_clear) {
    auto decay=[](float damper){SympatheticBank b;b.prepare(48000);SympatheticParams p;p.enabled=true;p.count=1;p.root=60;p.tuning=0;p.decayMs=1000;p.damping=0;p.damper=damper;p.send=1;settle(b,p);run(b,24000,261.6256f,48000,.4f);float before=b.modeEnergy(0);run(b,24000,0,48000,0);return b.modeEnergy(0)/before;};
    const float normal=decay(0),damped=decay(1);CHECK(normal>.02f&&normal<.045f);CHECK(damped<normal*.01f);
    SympatheticBank b;b.prepare(48000);SympatheticParams p;p.enabled=true;p.count=1;p.root=60;p.tuning=0;p.send=1;settle(b,p);run(b,48000,261.6256f,48000,.4f);p.freeze=true;settle(b,p);run(b,4800,0,48000,0);float before=b.modeEnergy(0);run(b,48000,777,48000,1);CHECK_NEAR(b.modeEnergy(0)/before,1,.01);p.clear=true;b.update(p,256);CHECK_NEAR(b.modeEnergy(0),0,0);run(b,4800,0,48000,0);CHECK_NEAR(b.modeEnergy(0),0,0);
}
AERIFORM_TEST(sympathetic_held_notes_capture_pedals_and_preparation) {
    SympatheticBank b;b.prepare(48000);SympatheticParams p;p.enabled=true;p.tuning=7;settle(b,p);for(int note:{60,64,67})b.handleMidi(juce::MidiMessage::noteOn(2,note,(juce::uint8)100));settle(b,p);CHECK_NEAR(b.frequency(1),midiNoteToHz(64),.1);p.capture=true;b.update(p,256);auto chord=b.capturedChord();CHECK(chord[0]==60&&chord[1]==64&&chord[2]==67&&chord[3]==-1);
    b.handleMidi(juce::MidiMessage::controllerEvent(2,64,127));b.handleMidi(juce::MidiMessage::noteOff(2,60));CHECK(b.currentHeldChord()[0]==60);b.handleMidi(juce::MidiMessage::controllerEvent(2,64,0));CHECK(b.currentHeldChord()[0]==64);
    SympatheticBank restored;restored.requestCapturedChord(chord);restored.prepare(96000);CHECK(restored.capturedChord()==chord);p.tuning=8;p.capture=false;settle(restored,p);CHECK(restored.capturedChord()==chord);CHECK_NEAR(restored.frequency(2),midiNoteToHz(67),.1);
}
AERIFORM_TEST(sympathetic_multi_voice_excitation_is_normalized) {
    for(int voices:{1,8,16}){SympatheticBank b;b.prepare(48000);SympatheticParams p;p.enabled=true;p.count=12;p.send=1;p.returnLevel=2;p.brightness=1;p.decayMs=100;p.thresholdDb=-96;p.tuning=5;p.intervals.fill(0);settle(b,p);run(b,48000,130.8128f,48000,(float)voices,voices);CHECK(b.modeEnergy(0)<1.01f/voices);}
}
AERIFORM_TEST(sympathetic_host_audio_and_chord_state) {
    auto render=[](bool on){TestHost h;h.set(ids::symOn,on?1:0);h.set(ids::symTuning,7);h.set(ids::symCount,3);h.set(ids::symSend,1);h.set(ids::symReturn,2);for(int note:{48,52,55})h.noteOn(note);std::vector<float>x;CHECK(h.render(.5,&x).finite);return x;};auto a=render(false),b=render(true);double diff=0;for(size_t i=0;i<a.size();++i)diff+=std::pow(a[i]-b[i],2);CHECK(diff>1e-6);
    TestHost h;for(int note:{60,64,67})h.noteOn(note);h.renderBlock();h.set(ids::symCapture,1);h.renderBlock();auto chord=h.processor.getCapturedChord();CHECK(chord[0]==60&&chord[2]==67);juce::MemoryBlock state;h.processor.getStateInformation(state);AeriformProcessor restored;restored.setStateInformation(state.getData(),(int)state.getSize());CHECK(restored.getCapturedChord()==chord);restored.prepareToPlay(96000,32);CHECK(restored.getCapturedChord()==chord);
    h.processor.getPatchTools().perform("Capture chord",[&]{auto c=chord;c[0]=72;h.processor.setCapturedChord(c);});CHECK(h.processor.getCapturedChord()[0]==72);CHECK(h.processor.getPatchTools().undo.undo());CHECK(h.processor.getCapturedChord()==chord);CHECK(h.processor.getPatchTools().undo.redo());CHECK(h.processor.getCapturedChord()[0]==72);
}

AERIFORM_TEST(sympathetic_page_and_six_tab_navigation_restore) {
    TestHost h;h.processor.setEditorPage(2);h.processor.setEditorSection(2,2);h.set(ids::symOn,1);std::unique_ptr<juce::AudioProcessorEditor> e(h.processor.createEditor());
    auto* editor=dynamic_cast<AeriformEditor*>(e.get());CHECK(editor->getCurrentPage()==2);CHECK(h.processor.getEditorSection(2)==2);
    juce::Image image(juce::Image::ARGB,e->getWidth(),e->getHeight(),true);juce::Graphics g(image);e->paintEntireComponent(g,true);auto file=juce::File::getCurrentWorkingDirectory().getChildFile("docs/experimental/sympathetic.png");auto stream=file.createOutputStream();CHECK(stream!=nullptr);if(stream){stream->setPosition(0);stream->truncate();CHECK(juce::PNGImageFormat().writeImageToStream(image,*stream));}
    editor->showPage(6);CHECK(editor->getCurrentPage()==4);CHECK(h.processor.getEditorSection(4)==1);editor->showPage(7);CHECK(editor->getCurrentPage()==2);CHECK(h.processor.getEditorSection(2)==1);
    juce::MemoryBlock state;h.processor.getStateInformation(state);TestHost b;b.processor.setStateInformation(state.getData(),(int)state.getSize());CHECK(b.processor.getEditorSection(4)==1);CHECK(b.processor.getEditorSection(2)==1);
}
