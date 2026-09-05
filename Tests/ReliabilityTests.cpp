#include "TestFramework.h"
#include "TestHelpers.h"
#include "GUI/Knob.h"
#include "DSP/Effects/OutputStage.h"
using namespace aeriform;
using namespace aeriform::test;
AERIFORM_TEST(metadata_survives_multiple_instances_and_display_names) {
    TestHost a; const auto* info=findParamInfo(ids::excNoise); const auto name=info->name;
    Knob knob(a.processor,ids::excNoise); knob.setDisplayName("Local name");
    TestHost b; CHECK(findParamInfo(ids::excNoise)==info); CHECK(info->name==name);
}
AERIFORM_TEST(output_meter_distinguishes_gain_reduction_from_ceiling_occupancy) {
    dsp::OutputStage o; o.prepare(48000); o.setParams(0,20,true);
    std::array<float,256> l{},r{};
    for(int b=0;b<20;++b) {for(int i=0;i<256;++i) l[(size_t)i]=r[(size_t)i]=4*std::sin((b*256+i)*0.1f);o.process(l.data(),r.data(),256);}
    const auto m=o.getMeter(); CHECK(m.prePeak>3); CHECK(m.postPeak<0.8f);
    CHECK(m.limitedFraction>0.9f); CHECK(m.ceilingFraction<m.limitedFraction); CHECK(m.preRms>m.postRms);
}
AERIFORM_TEST(oversized_blocks_preserve_input_and_midi_offsets) {
    TestHost large(48000,256,true),small(48000,256,true);
    for(auto* h:{&large,&small}) {h->set(ids::exaModel,(float)ExciterModel::Sidechain);h->set(ids::excNoise,0);h->set(ids::envAttack,1);}
    juce::AudioBuffer<float> whole(2,2048); juce::MidiBuffer events;
    events.addEvent(juce::MidiMessage::noteOn(1,60,(juce::uint8)100),310);
    events.addEvent(juce::MidiMessage::noteOff(1,60),1700);
    for(int i=0;i<2048;++i) for(int c=0;c<2;++c) whole.setSample(c,i,std::sin(i*0.032f)*0.2f);
    large.processor.processBlock(whole,events);
    float difference=0;
    for(int offset=0;offset<2048;offset+=256) {
        juce::AudioBuffer<float> part(2,256); juce::MidiBuffer midi;
        for(int i=0;i<256;++i) for(int c=0;c<2;++c) part.setSample(c,i,std::sin((offset+i)*0.032f)*0.2f);
        for(const auto e:events) if(e.samplePosition>=offset&&e.samplePosition<offset+256) midi.addEvent(e.getMessage(),e.samplePosition-offset);
        small.processor.processBlock(part,midi);
        for(int i=0;i<256;++i) difference=std::max(difference,std::abs(part.getSample(0,i)-whole.getSample(0,offset+i)));
    }
    CHECK(difference<1e-6f); CHECK(whole.getMagnitude(0,2048)>1e-5f);
}
