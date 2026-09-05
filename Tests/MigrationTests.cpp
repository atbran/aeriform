#include "TestFramework.h"
#include "TestHelpers.h"

using namespace aeriform;
using namespace aeriform::test;

namespace
{
    // A state blob exactly as v0.1 wrote it: version 1, only the 120 original parameters (subset here).
    std::unique_ptr<juce::XmlElement> makeV01State()
    {
        auto xml = std::make_unique<juce::XmlElement> ("AeriformState");
        xml->setAttribute ("version", 1);
        xml->setAttribute ("pluginVersion", "0.1.0");
        xml->setAttribute ("presetName", "Old Session Sound");
        xml->setAttribute ("presetCategory", "User");
        xml->setAttribute ("presetDirty", false);
        xml->setAttribute ("editorScale", 1.25);
        auto* params = xml->createNewChildElement ("AeriformParams");
        auto add = [params] (const char* id, double v) { auto* e = params->createNewChildElement ("PARAM"); e->setAttribute ("id", id); e->setAttribute ("value", v); };
        add ("exc_noise", 0.42); add ("exc_noise_color", 0.7); add ("exc_pressure", 0.66); add ("exc_pluck", 0.5);
        add ("exc_lp", 3210.0); add ("exc_hp", 120.0); add ("exc_reed", 0.8);
        add ("res_feedback", 0.97); add ("res_mode", 1.0); add ("res_body_freq", 1500.0);
        add ("mod1_src", 7.0); add ("mod1_dst", 8.0); add ("mod1_depth", 0.33);
        add ("lfo2_shape", 4.0); add ("delay_sync", 0.0); add ("voice_count", 6.0); add ("out_gain", -3.0);
        auto* learn = xml->createNewChildElement ("MidiLearn");
        auto* map = learn->createNewChildElement ("Map"); map->setAttribute ("cc", 74); map->setAttribute ("param", "res_damping");
        return xml;
    }
}

AERIFORM_TEST (v01_session_state_loads_and_maps_into_the_new_architecture)
{
    TestHost h;
    // dirty the new parameters first so we can see the load reset them to their v0.1-equivalent defaults
    h.set (ids::exaModel, (float) ExciterModel::Complex);
    h.set (ids::exbModel, (float) ExciterModel::Wave);
    h.set (ids::wfOn, 1.0f);
    h.set (ids::netMode, (float) NetMode::Serial);
    h.set (ids::loopOn, 1.0f);
    h.set (ids::modParam (12, ids::modDepthSuffix), 0.9f);

    auto xml = makeV01State();
    h.processor.applyStateXml (*xml);

    // old values restored
    CHECK_NEAR (h.get (ids::excNoise), 0.42, 1.0e-4);
    CHECK_NEAR (h.get (ids::excNoiseColor), 0.7, 1.0e-4);
    CHECK_NEAR (h.get (ids::excPressure), 0.66, 1.0e-4);
    CHECK_NEAR (h.get (ids::excPluck), 0.5, 1.0e-4);
    CHECK_NEAR (h.get (ids::excLowpass), 3210.0, 1.0);
    CHECK_NEAR (h.get (ids::excHighpass), 120.0, 0.5);
    CHECK_NEAR (h.get (ids::excReed), 0.8, 1.0e-4);
    CHECK_NEAR (h.get (ids::resFeedback), 0.97, 1.0e-4);
    CHECK_NEAR (h.get (ids::resMode), 1.0, 1.0e-6);
    CHECK_NEAR (h.get (ids::resBodyFreq), 1500.0, 1.0);
    CHECK_NEAR (h.get (ids::modParam (1, ids::modSrcSuffix)), 7.0, 1.0e-6);
    CHECK_NEAR (h.get (ids::modParam (1, ids::modDepthSuffix)), 0.33, 1.0e-4);
    CHECK_NEAR (h.get (ids::lfoParam (2, ids::lfoShapeSuffix)), 4.0, 1.0e-6);
    CHECK_NEAR (h.get (ids::delaySync), 0.0, 1.0e-6);
    CHECK_NEAR (h.get (ids::voiceCount), 6.0, 1.0e-6);
    CHECK_NEAR (h.get (ids::outGain), -3.0, 1.0e-4);
    // new parameters at their v0.1-equivalent defaults
    CHECK_NEAR (h.get (ids::exaModel), (double) ExciterModel::Breath, 1.0e-6);
    CHECK_NEAR (h.get (ids::exbModel), (double) ExciterModel::Off, 1.0e-6);
    CHECK (h.get (ids::wfOn) < 0.5f);
    CHECK_NEAR (h.get (ids::netMode), (double) NetMode::Single, 1.0e-6);
    CHECK (h.get (ids::loopOn) < 0.5f);
    CHECK_NEAR (h.get (ids::modParam (12, ids::modDepthSuffix)), 0.0, 1.0e-6);
    CHECK_NEAR (h.get (ids::preEnv), 1.0, 1.0e-6);
    // extras
    CHECK (h.processor.getPresetManager().getCurrentName() == "Old Session Sound");
    CHECK_NEAR (h.processor.getEditorScale(), 1.25, 1.0e-6);
    CHECK (h.processor.getMidiLearn().getMappedCC (ids::resDamping) == 74);
    // and it still makes sound like a v0.1 patch
    h.set (ids::reverbMix, 0.0f);
    h.noteOn (60);
    const auto s = h.render (0.5);
    CHECK (s.finite);
    CHECK_MSG (s.rms > 1.0e-3, "old session sounds: " + std::to_string (s.rms));
}

AERIFORM_TEST (v01_preset_file_loads_and_unknown_ids_are_ignored)
{
    TestHost h;
    auto& pm = h.processor.getPresetManager();
    juce::XmlElement xml ("AeriformPreset");
    xml.setAttribute ("version", 1);
    xml.setAttribute ("name", "Old Reed");
    xml.setAttribute ("category", "Reeds");
    auto* params = xml.createNewChildElement ("Parameters");
    auto add = [params] (const char* id, double v) { auto* e = params->createNewChildElement ("Param"); e->setAttribute ("id", id); e->setAttribute ("value", v); };
    add ("exc_reed", 0.85); add ("exc_pressure", 0.75); add ("res_mode", 1.0); add ("res_feedback", 0.96);
    add ("some_future_parameter", 0.5);
    add ("mod3_dst", 5.0);
    h.set (ids::wfOn, 1.0f);
    h.set (ids::exbModel, (float) ExciterModel::NoiseSteam);
    CHECK (pm.applyPresetXml (xml));
    CHECK_NEAR (h.get (ids::excReed), 0.85, 1.0e-4);
    CHECK_NEAR (h.get (ids::excPressure), 0.75, 1.0e-4);
    CHECK_NEAR (h.get (ids::resMode), 1.0, 1.0e-6);
    CHECK_NEAR (h.get (ids::modParam (3, ids::modDstSuffix)), 5.0, 1.0e-6);
    CHECK (h.get (ids::wfOn) < 0.5f);                                   // reset to default by the preset load
    CHECK_NEAR (h.get (ids::exbModel), (double) ExciterModel::Off, 1.0e-6);
    h.noteOn (57);
    CHECK (h.render (0.3).rms > 1.0e-3);
}

AERIFORM_TEST (v02_state_round_trips_every_new_parameter)
{
    TestHost a;
    a.set (ids::exaModel, (float) ExciterModel::Complex);
    a.set (ids::exbModel, (float) ExciterModel::NoiseAerosol);
    a.set (ids::exaCxChaos, 0.77f);
    a.set (ids::exbNzSeed, 123.0f);
    a.set (ids::exbSync, 1.0f);
    a.set (ids::mixMode, (float) InteractionMode::Xor);
    a.set (ids::wfOn, 1.0f); a.set (ids::wfMode, (float) FoldMode::Chebyshev); a.set (ids::wfStages, 3.0f);
    a.set (ids::preOrder, (float) ShaperOrder::FoldThenShape);
    a.set (ids::netMode, (float) NetMode::Hybrid); a.set (ids::rbType, (float) ResMode::Membrane); a.set (ids::rcOn, 1.0f);
    a.set (ids::netCA, 0.4f); a.set (ids::netRepipe, 0.6f);
    a.set (ids::loopOn, 1.0f); a.set (ids::loopDest, (float) LoopDest::NetworkIn);
    a.set (ids::modParam (16, ids::modSrcSuffix), (float) ModSource::ChaosX);
    a.set (ids::modParam (16, ids::modDstSuffix), (float) ModDest::Fold);
    a.set (ids::modParam (16, ids::modDepthSuffix), -0.25f);
    a.set (ids::quality, (float) QualityMode::High);

    juce::MemoryBlock state;
    a.processor.getStateInformation (state);
    TestHost b;
    b.processor.setStateInformation (state.getData(), (int) state.getSize());
    for (int i = 0; i < kNumParams; ++i)
    {
        const char* id = ids::all[i];
        CHECK_MSG (std::fabs (a.get (id) - b.get (id)) < 1.0e-4f * std::max (1.0f, std::fabs (a.get (id))), std::string ("round trip ") + id);
    }
    auto xml = a.processor.createStateXml();
    CHECK (xml->getIntAttribute ("version") == kStateVersion);
    CHECK (kStateVersion == 2);
}

AERIFORM_TEST (existing_factory_presets_still_load_and_sound_with_the_new_engine)
{
    TestHost h (48000.0, 256, true);   // with sidechain input so Sidechain presets sound too
    dsp::Noise scNoise; scNoise.seed (3);
    h.inputSource = [&] (long) { return scNoise.next() * 0.5f; };
    auto& pm = h.processor.getPresetManager();
    int checked = 0;
    for (int i = 0; i < (int) pm.getEntries().size(); ++i)
    {
        if (! pm.getEntries()[(size_t) i].isFactory) continue;
        CHECK (pm.loadPreset (i));
        h.set (ids::reverbMix, 0.0f); h.set (ids::delayMix, 0.0f);
        h.noteOn (57, 100);
        const auto s = h.render (0.4);
        h.noteOff (57);
        h.render (0.2);
        h.processor.getEngine().allNotesOff();
        h.processor.reset();
        CHECK_MSG (s.finite, ("preset '" + pm.getCurrentName() + "' finite").toStdString());
        CHECK_MSG (s.rms > 1.0e-4, ("preset '" + pm.getCurrentName() + "' audible: " + juce::String (s.rms)).toStdString());
        ++checked;
    }
    CHECK (checked >= 40);
}
