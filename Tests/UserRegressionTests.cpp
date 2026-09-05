#include "TestFramework.h"
#include "TestHelpers.h"
#include "DSP/ResonatorNetwork.h"
using namespace aeriform;using namespace aeriform::dsp;using namespace aeriform::test;
AERIFORM_TEST(user_resonator_bypass_routes_excitation_directly) {
    for(bool explicitBypass:{false,true}){ResonatorNetwork n;n.prepare(48000);NetworkParams p;p.mode=NetMode::Parallel;p.repipe=1;p.loopOn=true;
        if(explicitBypass){p.bypass=true;p.on[1]=p.on[2]=true;}else p.on[0]=p.on[1]=p.on[2]=false;
        n.update(p,true);for(int i=0;i<5000;++i){float l,r,x=.1f*std::sin(i*.05f);n.next(x,0,0,l,r);CHECK_NEAR(l,x*.707f,1e-6);CHECK_NEAR(r,x*.707f,1e-6);}n.update(p,false);for(int i=0;i<3;++i)CHECK(!n.slotRunning(i));
    }
    TestHost h;h.set(ids::resOn,0);h.set(ids::rbOn,0);h.set(ids::rcOn,0);h.noteOn(60);auto stats=h.render(.2);CHECK(stats.finite&&stats.rms>1e-3);
}
AERIFORM_TEST(user_resonator_pitch_edit_persists_with_repipe) {
    TestHost h;h.set(ids::netRepipe,1);h.set(ids::artVariation,0);h.set(ids::artInstability,0);h.set(ids::artFlowPitch,0);h.set(ids::excBreathRandom,0);h.noteOn(60);h.render(.2);
    h.set(ids::resCoarse,7);h.set(ids::rbCoarse,-12);h.set(ids::rcCoarse,12);
    for(int t=0;t<20;++t){CHECK(h.render(.1).finite);CHECK_NEAR(h.get(ids::resCoarse),7,.001);CHECK_NEAR(h.get(ids::rbCoarse),-12,.001);CHECK_NEAR(h.get(ids::rcCoarse),12,.001);const auto& v=h.processor.getVisualizerModel();CHECK_NEAR(v.resonatorTargetHz[0].load(),midiNoteToHz(67),.1);CHECK_NEAR(v.resonatorTargetHz[1].load(),midiNoteToHz(48),.1);CHECK_NEAR(v.resonatorTargetHz[2].load(),midiNoteToHz(72),.1);}
}
