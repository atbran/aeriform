#include "TestFramework.h"
#include "TestHelpers.h"
#include "GUI/KnobModulation.h"
#include "GUI/Knob.h"
using namespace aeriform;using namespace aeriform::test;
AERIFORM_TEST(knob_modulation_assignment_is_undoable_and_does_not_replace_routes) {
    TestHost h;auto& p=h.processor;for(int i=1;i<=ids::numModSlots;++i){h.set(ids::id(ids::modP(i,ids::ModField::Src)),0);h.set(ids::id(ids::modP(i,ids::ModField::Dst)),0);}p.getPatchTools().undo.clearUndoHistory();
    int slot=KnobModulation::assign(p,ModDest::Damping,ModSource::LFO1);CHECK(slot==1);CHECK(KnobModulation::find(p,ModDest::Damping,ModSource::LFO1)==slot);CHECK_NEAR(KnobModulation::value(p,slot,ids::ModField::Depth),.25,1e-6);CHECK(KnobModulation::assign(p,ModDest::Damping,ModSource::LFO1)==slot);
    CHECK(p.getPatchTools().undo.undo());CHECK(KnobModulation::find(p,ModDest::Damping)<0);CHECK(p.getPatchTools().undo.redo());CHECK(KnobModulation::find(p,ModDest::Damping)==slot);
    for(int i=2;i<=ids::numModSlots;++i){h.set(ids::id(ids::modP(i,ids::ModField::Src)),(float)ModSource::LFO2);h.set(ids::id(ids::modP(i,ids::ModField::Dst)),(float)ModDest::Pitch);}CHECK(KnobModulation::assign(p,ModDest::Feedback,ModSource::LFO3)<0);CHECK(KnobModulation::find(p,ModDest::Damping)==slot);
    KnobModulation::remove(p,slot);CHECK(KnobModulation::find(p,ModDest::Damping)<0);CHECK(p.getPatchTools().undo.undo());CHECK(KnobModulation::find(p,ModDest::Damping)==slot);
}

AERIFORM_TEST(knob_modulation_depth_drag_is_one_undo_action) {
    TestHost h;Knob knob(h.processor,ids::resDamping,{ModDest::Damping,KnobModMapping::Kind::Additive,1});knob.setBounds(0,0,100,100);
    int slot=KnobModulation::assign(h.processor,ModDest::Damping,ModSource::LFO1);CHECK(slot>0);h.processor.getPatchTools().undo.clearUndoHistory();float base=h.get(ids::resDamping);
    auto event=[&](float y,bool dragged){return juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(),{50,y},juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier|juce::ModifierKeys::altModifier),1,0,0,0,0,&knob,&knob,juce::Time::getCurrentTime(),{50,50},juce::Time::getCurrentTime(),1,dragged);};
    knob.mouseDown(event(50,false));for(int i=1;i<=30;++i)knob.mouseDrag(event(50-(float)i,true));knob.mouseUp(event(20,true));CHECK_NEAR(KnobModulation::value(h.processor,slot,ids::ModField::Depth),.45,1e-5);CHECK_NEAR(h.get(ids::resDamping),base,0);CHECK(h.processor.getPatchTools().undo.undo());CHECK_NEAR(KnobModulation::value(h.processor,slot,ids::ModField::Depth),.25,1e-5);CHECK(!h.processor.getPatchTools().undo.canUndo());
}
