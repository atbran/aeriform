#include "TestFramework.h"
#include "TestHelpers.h"
#include "AllocationProbe.h"
#include "DSP/Effects/SpectralFreeze.h"
using namespace aeriform;using namespace aeriform::dsp;using namespace aeriform::test;
AERIFORM_TEST(spectral_fft_matches_known_bins_and_round_trip) {
    FixedFFT<11> fft;fft.prepare();std::array<FixedFFT<11>::Complex,2048> data,original;
    for(int i=0;i<2048;++i)data[(size_t)i]={std::cos(kTwoPi*37*i/2048),0};fft.transform(data);CHECK_NEAR(std::abs(data[37]),1024,.02);CHECK_NEAR(std::abs(data[2048-37]),1024,.02);CHECK(std::abs(data[31])<.01);
    for(int i=0;i<2048;++i)data[(size_t)i]={std::sin(i*.173f),std::cos(i*.257f)};original=data;fft.transform(data);fft.transform(data,true);for(size_t i=0;i<data.size();++i){CHECK_NEAR(data[i].real(),original[i].real(),2e-6);CHECK_NEAR(data[i].imag(),original[i].imag(),2e-6);}
}
namespace {
void feed(SpectralFreeze& f,int samples,float sr,float hz,float level,std::vector<float>* output=nullptr){for(int i=0;i<samples;++i){float l=level*std::sin(kTwoPi*hz*i/sr),r=l;f.process(&l,&r,1);CHECK(std::isfinite(l)&&std::isfinite(r));CHECK(std::abs(l)<=4.001f&&std::abs(r)<=4.001f);if(output)output->push_back(l);}}
}
AERIFORM_TEST(spectral_capture_holds_pitch_and_release_returns_live_audio) {
    for(float sr:{44100.0f,48000.0f,96000.0f}){auto f=std::make_unique<SpectralFreeze>();f->prepare(sr);SpectralParams p;p.enabled=true;f->setParams(p);feed(*f,(int)sr,sr,375,.4f);p.freeze=true;f->setParams(p);feed(*f,2048,sr,375,.4f);CHECK(f->isFrozen());CHECK(f->captures()==1);feed(*f,(int)(sr*.2f),sr,0,0);std::vector<float> frozen;feed(*f,(int)(sr*.4f),sr,0,0,&frozen);double energy=0;for(float v:frozen)energy+=v*v;CHECK(energy>1);const double hz=estimatePeakFrequency(frozen,sr,375);std::printf("    spectral sr=%.0f target=375 measured=%.3f energy=%.3f\n",sr,hz,energy);CHECK(std::abs(centsBetween(hz,375))<15);
        p.freeze=false;p.release=true;f->setParams(p);feed(*f,4096,sr,0,0);float l=.3f,r=-.2f;f->process(&l,&r,1);CHECK_NEAR(l,.3f,0);CHECK_NEAR(r,-.2f,0);CHECK(!f->isFrozen());}
}
AERIFORM_TEST(spectral_capture_has_no_cpp_allocations_or_frees) {
    auto f=std::make_unique<SpectralFreeze>();f->prepare(48000);SpectralParams p;p.enabled=true;f->setParams(p);feed(*f,4096,48000,220,.3f);AllocationProbe probe;
    for(int block=0;block<100;++block){if(block%10==0){p.capture=!p.capture;p.semitones=(float)(block%24-12);p.blur=.8f;p.randomPhase=.7f;f->setParams(p);}float l[64]{},r[64]{};f->process(l,r,64);}auto count=probe.finish();CHECK(count.allocations==0);CHECK(count.deallocations==0);CHECK(f->captures()>1);
}
AERIFORM_TEST(spectral_blur_shift_decay_and_bypass_have_real_effects) {
    auto render=[](float shift,float blur,float decay){auto f=std::make_unique<SpectralFreeze>();f->prepare(48000);SpectralParams p;p.enabled=true;f->setParams(p);feed(*f,48000,48000,375,.4f);p.freeze=true;f->setParams(p);feed(*f,2048,48000,375,.4f);p.semitones=shift;p.blur=blur;p.decayMs=decay;f->setParams(p);feed(*f,12000,48000,0,0);std::vector<float>x;feed(*f,24000,48000,0,0,&x);return x;};
    auto base=render(0,0,0),up=render(12,0,0),blur=render(0,1,0),decay=render(0,0,150);double baseEnergy=0,decayEnergy=0,blurDiff=0;for(size_t i=0;i<base.size();++i){baseEnergy+=base[i]*base[i];decayEnergy+=decay[i]*decay[i];blurDiff+=std::pow(base[i]-blur[i],2);}CHECK(decayEnergy<baseEnergy*.001);CHECK(blurDiff>1e-4);const double hz=estimatePeakFrequency(up,48000,750);std::printf("    spectral octave measured %.3f\n",hz);CHECK(std::abs(centsBetween(hz,750))<15);
    auto f=std::make_unique<SpectralFreeze>();f->prepare(48000);float l=.3f,r=-.2f;f->process(&l,&r,1);CHECK_NEAR(l,.3f,0);CHECK_NEAR(r,-.2f,0);
}

// Short implementation smoke only; release acceptance belongs to the external testing task.
#include "DSP/Effects/MultibandSaturation.h"
#include "Plugin/PluginEditor.h"
AERIFORM_TEST(v3_features_host_smoke_and_effect_pages) {
    TestHost host;host.set(ids::satOn,1);host.set(ids::satLowDrive,12);host.set(ids::satMidDrive,18);host.set(ids::satHighDrive,24);host.set(ids::satMidModel,1);host.set(ids::satHighModel,3);host.noteOn(60);
    auto live=host.render(.2);CHECK(live.finite);CHECK(live.rms>1e-5);CHECK(live.peak<4);
    host.set(ids::sfOn,1);host.render(.1);host.set(ids::sfCapture,1);host.render(.1);CHECK(host.processor.getVisualizerModel().spectralFrozen.load());
    host.set(ids::sfRelease,1);host.render(.1);CHECK(!host.processor.getVisualizerModel().spectralFrozen.load());
    for(int quality=0;quality<3;++quality){host.set(ids::satQuality,(float)quality);CHECK(host.render(.08).finite);}
    juce::MemoryBlock state;host.processor.getStateInformation(state);TestHost restored;restored.processor.setStateInformation(state.getData(),(int)state.getSize());CHECK_NEAR(restored.get(ids::satHighDrive),24,1e-5);CHECK_NEAR(restored.get(ids::satHighModel),3,0);
    for(int section:{4,5}){host.processor.setEditorSection(4,section);std::unique_ptr<juce::AudioProcessorEditor> editor(host.processor.createEditor());auto* typed=dynamic_cast<AeriformEditor*>(editor.get());CHECK(typed!=nullptr);if(!typed)continue;typed->showPage(4);juce::Image image(juce::Image::ARGB,editor->getWidth(),editor->getHeight(),true);juce::Graphics g(image);editor->paintEntireComponent(g,true);auto file=juce::File::getCurrentWorkingDirectory().getChildFile(section==4?"docs/experimental/spectral-v3.png":"docs/experimental/saturation-v3.png");auto stream=file.createOutputStream();CHECK(stream!=nullptr);if(stream){stream->setPosition(0);stream->truncate();CHECK(juce::PNGImageFormat().writeImageToStream(image,*stream));}}
    MultibandSaturation sat;sat.prepare(48000);float l=.31f,r=-.27f;sat.process(&l,&r,1);CHECK_NEAR(l,.31f,0);CHECK_NEAR(r,-.27f,0);
}
