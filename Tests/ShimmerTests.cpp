#include "TestFramework.h"
#include "TestHelpers.h"
#include "DSP/Effects/ShimmerReverb.h"
#include "Plugin/PluginEditor.h"
using namespace aeriform;using namespace aeriform::dsp;using namespace aeriform::test;
AERIFORM_TEST(shimmer_pitch_shifter_measured_intervals_and_changes) {
    for(float sr:{44100.0f,48000.0f,96000.0f})for(float interval:{-12.0f,5.0f,7.0f,12.0f}){
        PitchShift shift;shift.prepare(sr);shift.setSemitones(interval);std::vector<float> signal;
        for(int i=0;i<(int)(sr*1.5f);++i){float y=shift.next(.5f*std::sin(kTwoPi*200*i/sr));CHECK(std::isfinite(y)&&std::abs(y)<=.501f);if(i>=(int)sr)signal.push_back(y);}
        const double target=200*std::exp2(interval/12);double measured=estimatePeakFrequency(signal,sr,target);
        std::printf("    shimmer sr=%.0f interval=%.0f expected=%.3f measured=%.3f\n",sr,interval,target,measured);CHECK(std::abs(centsBetween(measured,target))<8);
        float previous=signal.back(),maxStep=0;shift.setSemitones(3.5f);for(int i=0;i<(int)(sr*.1f);++i){float y=shift.next(.5f*std::sin(kTwoPi*200*((int)(sr*1.5f)+i)/sr));maxStep=std::max(maxStep,std::abs(y-previous));previous=y;}CHECK(maxStep<.1f);
    }
}
AERIFORM_TEST(shimmer_feedback_is_bounded_and_bypass_is_exact) {
    for(float sr:{44100.0f,48000.0f,96000.0f}){ShimmerReverb effect;effect.prepare(sr);float l=.3f,r=-.2f;effect.process(&l,&r,1);CHECK_NEAR(l,.3f,0);CHECK_NEAR(r,-.2f,0);
        ShimmerParams p;p.enabled=true;p.feedback=1;p.mix=1;p.size=1;p.damping=0;p.diffusion=1;p.lowCutHz=20;p.highCutHz=20000;effect.setParams(p,(int)sr);
        double energy=0;for(int i=0;i<(int)(sr*2);++i){if(i==(int)sr){p.semitones=-24;p.size=0;effect.setParams(p,256);}l=.8f*std::sin(i*.07f);r=.8f*std::sin(i*.13f);effect.process(&l,&r,1);CHECK(std::isfinite(l)&&std::isfinite(r));CHECK(std::abs(l)<2&&std::abs(r)<2);energy+=l*l+r*r;}CHECK(energy>1e-5);CHECK(effect.safetyClips()==0);
        p.enabled=false;effect.setParams(p);for(int i=0;i<(int)(sr*.05f);++i){l=r=0;effect.process(&l,&r,1);}l=.3f;r=-.2f;effect.process(&l,&r,1);CHECK_NEAR(l,.3f,0);CHECK_NEAR(r,-.2f,0);
    }
}
AERIFORM_TEST(shimmer_shift_is_in_the_reverb_feedback_path) {
    auto render=[](float interval){ShimmerReverb effect;effect.prepare(48000);ShimmerParams p;p.enabled=true;p.feedback=.9f;p.mix=1;p.semitones=interval;p.lowCutHz=20;effect.setParams(p,48000);std::vector<float> x;for(int i=0;i<48000;++i){float l=i<8000?.6f*std::sin(i*kTwoPi*200/48000):0,r=l;effect.process(&l,&r,1);x.push_back(l);}return x;};
    auto a=render(0),b=render(12);double first=0,late=0;for(size_t i=0;i<a.size();++i){double d=std::pow(a[i]-b[i],2);if(i<500)first+=d;if(i>8000)late+=d;}CHECK(first<1e-12);CHECK(late>1e-8);
    std::printf("    shimmer shifted tail difference energy %.9f\n",late);
}

AERIFORM_TEST(shimmer_host_audio_page_and_state) {
    auto render=[](bool on){TestHost h;h.set(ids::shOn,on?1:0);h.set(ids::shMix,.6f);h.noteOn(60);std::vector<float>x;CHECK(h.render(.5,&x).finite);return x;};auto a=render(false),b=render(true);double diff=0;for(size_t i=0;i<a.size();++i)diff+=std::pow(a[i]-b[i],2);CHECK(diff>1e-5);
    TestHost h;h.processor.setEditorPage(4);h.processor.setEditorSection(4,3);h.set(ids::shOn,1);h.set(ids::shInterval,3.5f);auto* interval=h.processor.getAPVTS().getParameter(ids::shInterval);CHECK(interval->getText(interval->getValue(),32)=="+3.50 st");h.set(ids::shInterval,7);std::unique_ptr<juce::AudioProcessorEditor> e(h.processor.createEditor());juce::Image image(juce::Image::ARGB,e->getWidth(),e->getHeight(),true);juce::Graphics g(image);e->paintEntireComponent(g,true);
    auto stream=juce::File::getCurrentWorkingDirectory().getChildFile("docs/experimental/shimmer.png").createOutputStream();CHECK(stream!=nullptr);if(stream){stream->setPosition(0);stream->truncate();CHECK(juce::PNGImageFormat().writeImageToStream(image,*stream));}
    juce::MemoryBlock state;h.processor.getStateInformation(state);TestHost restored;restored.processor.setStateInformation(state.getData(),(int)state.getSize());CHECK_NEAR(restored.get(ids::shInterval),7,.01);CHECK(restored.processor.getEditorSection(4)==3);
}
