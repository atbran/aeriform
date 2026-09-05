#include "TestFramework.h"
#include "TestHelpers.h"
using namespace aeriform;
using namespace aeriform::test;
namespace {
const char* presetClass(const juce::String& name,const juce::String& category) {
    if(name.containsIgnoreCase("sidechain")) return "Sidechain-dependent";
    if(name.containsIgnoreCase("drone")) return "Drone";
    if(name.containsIgnoreCase("click")||name.containsIgnoreCase("mallet")) return "Percussive";
    if(name.containsIgnoreCase("pluck")||name.containsIgnoreCase("bell")||category.containsIgnoreCase("pluck")) return "Plucked/decaying";
    return "Sustained";
}
}
AERIFORM_TEST(smoke_factory_preset_classified_levels) {
    const int count=(int)factoryPresets().size();
    std::printf("    name | class | attack peak | pre RMS | post RMS | limited %% | ceiling %% | release s | DC | CPU %%\n");
    for(int preset=0;preset<count;++preset) {
        TestHost h(48000,256,true);
        dsp::Noise input; input.seed(11);
        h.inputSource=[&](long){return input.next()*0.5f;};
        auto& pm=h.processor.getPresetManager();
        CHECK(pm.loadPreset(preset));
        const auto name=pm.getCurrentName(); const auto category=pm.getCurrentCategory();
        const char* kind=presetClass(name,category);
        for(int note:{48,55,60,64}) h.noteOn(note,100);
        auto& v=h.processor.getVisualizerModel();
        const int blocks=(int)std::ceil(2.0*48000/256);
        double pre=0,post=0,limited=0,ceiling=0,dc=0; float attack=0,prePeak=0;
        const auto start=juce::Time::getHighResolutionTicks();
        for(int b=0;b<blocks;++b) {
            const auto s=h.renderBlock(); CHECK(s.finite);
            if(b*256<4800) attack=std::max(attack,s.peak);
            prePeak=std::max(prePeak,v.preLimiterPeak.load());
            pre+=std::pow(v.preLimiterRms.load(),2); post+=s.rms*s.rms;
            limited+=v.limiterFraction.load(); ceiling+=v.ceilingFraction.load(); dc+=v.postLimiterMean.load();
        }
        const double cpu=100*juce::Time::highResolutionTicksToSeconds(juce::Time::getHighResolutionTicks()-start)/(blocks*256.0/48000);
        for(int note:{48,55,60,64}) h.noteOff(note);
        double release=0; int quiet=0;
        for(int b=0;b<1125;++b) {
            auto s=h.renderBlock(); CHECK(s.finite); release=(b+1)*256.0/48000;
            quiet=s.rms<1e-4?quiet+1:0; if(quiet>=8) break;
        }
        std::printf("    %-23s | %-20s | %.3f | %.4f | %.4f | %.1f | %.2f | %.2f | %+.5f | %.1f (pre peak %.3f)\n",name.toRawUTF8(),kind,attack,std::sqrt(pre/blocks),std::sqrt(post/blocks),100*limited/blocks,100*ceiling/blocks,release,dc/blocks,cpu,prePeak);
        CHECK_MSG(post>1e-10,(name+" has integrated audible energy").toStdString());
        CHECK_MSG(std::abs(dc/blocks)<0.03,(name+" persistent DC").toStdString());
        CHECK_MSG(prePeak<100,(name+" internal signal remains bounded").toStdString());
    }
}
