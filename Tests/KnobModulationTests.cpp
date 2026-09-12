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

AERIFORM_TEST(macro_knob_assignment_and_macro_modulation) {
    TestHost h;
    auto& p = h.processor;
    for (int i = 1; i <= ids::numModSlots; ++i) {
        h.set(ids::id(ids::modP(i, ids::ModField::Src)), 0);
        h.set(ids::id(ids::modP(i, ids::ModField::Dst)), 0);
    }
    // Map Macro 1 as a modulation source to Damping
    int slot1 = KnobModulation::assign(p, ModDest::Damping, ModSource::Macro1);
    CHECK(slot1 == 1);
    CHECK(KnobModulation::find(p, ModDest::Damping, ModSource::Macro1) == slot1);

    // Map LFO 1 to modulate Macro 1 knob
    int slot2 = KnobModulation::assign(p, ModDest::Macro1, ModSource::LFO1);
    CHECK(slot2 == 2);
    CHECK(KnobModulation::find(p, ModDest::Macro1, ModSource::LFO1) == slot2);

    // Verify macro names persist across XML state save/restore
    p.setMacroName(0, "Timbre");
    p.setMacroName(1, "Space");
    CHECK(p.getMacroName(0) == "Timbre");
    CHECK(p.getMacroName(1) == "Space");

    auto xml = p.createStateXml();
    CHECK(xml != nullptr);

    TestHost h2;
    h2.processor.applyStateXml(*xml);
    CHECK(h2.processor.getMacroName(0) == "Timbre");
    CHECK(h2.processor.getMacroName(1) == "Space");
    CHECK(h2.processor.getMacroName(2) == "Macro 3");
}

AERIFORM_TEST(continuous_modulation_destinations_reach_dsp_and_render_finite) {
    TestHost h;
    h.set(ids::resOn, 1.0f); h.set(ids::rbOn, 1.0f); h.set(ids::rcOn, 1.0f);
    h.set(ids::netMode, (float)NetMode::Parallel);

    const ModDest testDests[] = {
        ModDest::ResAWet, ModDest::ResBWet, ModDest::ResCWet,
        ModDest::ResAWidth, ModDest::ResBWidth, ModDest::ResCWidth,
        ModDest::ResAInharm, ModDest::ResBInharm, ModDest::ResCInharm,
        ModDest::ResASize, ModDest::ResBSize, ModDest::ResCSize,
        ModDest::ResASaturation, ModDest::ResBSaturation, ModDest::ResCSaturation,
        ModDest::Pluck, ModDest::PluckLength,
        ModDest::FoldMix, ModDest::FoldShape,
        ModDest::ChorusRate, ModDest::ChorusDepth,
        ModDest::DelayTimeL, ModDest::DelayFeedback, ModDest::DelayFilter,
        ModDest::ReverbDecay, ModDest::ReverbSize, ModDest::ReverbDamp, ModDest::ReverbPredelay
    };

    for (auto dst : testDests) {
        h.set(ids::id(ids::modP(1, ids::ModField::Src)), (float)ModSource::LFO1);
        h.set(ids::id(ids::modP(1, ids::ModField::Dst)), (float)dst);
        h.set(ids::id(ids::modP(1, ids::ModField::Depth)), 0.5f);
        h.noteOn(60);
        auto s = h.render(0.04);
        CHECK(s.finite);
        CHECK(s.peak < 2.0f);
        h.noteOff(60);
        h.render(0.02);
    }
}

AERIFORM_TEST(unipolar_macro_modulation_ring_and_indicator_travel_match) {
    TestHost h;
    auto& p = h.processor;

    // Case 1: Knob at 0.0, Macro depth +1.0 -> Ring should span 0.0 to 1.0 unidirectionally
    h.set (ids::resWet, 0.0f);
    h.set (ids::id (ids::modP (1, ids::ModField::Src)), (float) ModSource::Macro1);
    h.set (ids::id (ids::modP (1, ids::ModField::Dst)), (float) ModDest::ResAWet);
    h.set (ids::id (ids::modP (1, ids::ModField::Depth)), 1.0f);
    h.set (ids::macro1, 0.0f);
    h.render (0.05);

    std::array<float, (size_t) ModDest::Count> liveMod {};
    p.getVisualizerModel().readLiveMod (liveMod);
    auto config = p.getEngine().getModConfig();

    Knob wetKnob (p, ids::resWet, KnobModMapping (ModDest::ResAWet, KnobModMapping::Kind::Additive, 1.0f));
    wetKnob.updateModRing (config, liveMod);

    CHECK (wetKnob.getHasMod());
    CHECK_NEAR (wetKnob.getModMinNorm(), 0.0f, 1e-4f);
    CHECK_NEAR (wetKnob.getModMaxNorm(), 1.0f, 1e-4f);
    CHECK_NEAR (wetKnob.getModLiveNorm(), 0.0f, 1e-4f);

    // Macro at 50% -> live indicator at 0.50
    h.set (ids::macro1, 0.5f);
    h.render (0.05);
    p.getVisualizerModel().readLiveMod (liveMod);
    wetKnob.updateModRing (config, liveMod);
    CHECK_NEAR (wetKnob.getModLiveNorm(), 0.5f, 1e-4f);

    // Macro at 100% -> live indicator at 1.00 (reaches end of ring)
    h.set (ids::macro1, 1.0f);
    h.render (0.05);
    p.getVisualizerModel().readLiveMod (liveMod);
    wetKnob.updateModRing (config, liveMod);
    CHECK_NEAR (wetKnob.getModLiveNorm(), 1.0f, 1e-4f);

    // Case 2: Knob at 0.5 (centered), Macro depth +0.4 -> Unidirectional arc from 0.5 to 0.9 (NOT 0.1 to 0.9)
    h.set (ids::resWet, 0.5f);
    h.set (ids::id (ids::modP (1, ids::ModField::Depth)), 0.4f);
    h.set (ids::macro1, 0.0f);
    h.render (0.05);
    config = p.getEngine().getModConfig();
    p.getVisualizerModel().readLiveMod (liveMod);
    wetKnob.updateModRing (config, liveMod);

    CHECK_NEAR (wetKnob.getModMinNorm(), 0.5f, 1e-4f);
    CHECK_NEAR (wetKnob.getModMaxNorm(), 0.9f, 1e-4f);
    CHECK_NEAR (wetKnob.getModLiveNorm(), 0.5f, 1e-4f);

    h.set (ids::macro1, 1.0f);
    h.render (0.05);
    p.getVisualizerModel().readLiveMod (liveMod);
    wetKnob.updateModRing (config, liveMod);
    // At macro 1.0, indicator dot moves to 0.9 (100% of the drawn arc, not stopping halfway!)
    CHECK_NEAR (wetKnob.getModLiveNorm(), 0.9f, 1e-4f);

    // Case 3: Bipolar LFO modulation on centered knob -> Symmetrical arc from [norm - depth, norm + depth]
    h.set (ids::id (ids::modP (1, ids::ModField::Src)), (float) ModSource::LFO1);
    h.set (ids::id (ids::modP (1, ids::ModField::Depth)), 0.2f);
    h.render (0.05);
    config = p.getEngine().getModConfig();
    p.getVisualizerModel().readLiveMod (liveMod);
    wetKnob.updateModRing (config, liveMod);

    CHECK_NEAR (wetKnob.getModMinNorm(), 0.3f, 1e-4f);
    CHECK_NEAR (wetKnob.getModMaxNorm(), 0.7f, 1e-4f);
}

