#include <juce_audio_formats/juce_audio_formats.h>
#include <cstdlib>
#include "TestFramework.h"
#include "TestHelpers.h"
#include "Presets/FactoryPresets.h"

using namespace aeriform;
using namespace aeriform::test;

AERIFORM_TEST (state_round_trip_restores_parameters_and_extras)
{
    TestHost a;
    a.set (ids::excNoise, 0.123f);
    a.set (ids::resFeedback, 0.777f);
    a.set (ids::resMode, 2.0f);
    a.set (ids::voiceCount, 5.0f);
    a.set (ids::delaySync, 0.0f);
    a.set (ids::modParam (3, ids::modSrcSuffix), (float) ModSource::LFO2);
    a.set (ids::modParam (3, ids::modDstSuffix), (float) ModDest::Feedback);
    a.set (ids::modParam (3, ids::modDepthSuffix), -0.42f);
    a.processor.setEditorScale (1.5f);
    a.processor.getPresetManager().setCurrentName ("Test Patch", "User", true);
    a.processor.getMidiLearn().armLearn (ids::resDamping);
    a.cc (21, 64);
    a.renderBlock();
    CHECK (a.processor.getMidiLearn().pollLearn());
    CHECK (a.processor.getMidiLearn().getMappedCC (ids::resDamping) == 21);

    juce::MemoryBlock state;
    a.processor.getStateInformation (state);
    CHECK (state.getSize() > 0);

    TestHost b;
    b.processor.setStateInformation (state.getData(), (int) state.getSize());
    CHECK_NEAR (b.get (ids::excNoise), 0.123, 1.0e-4);
    CHECK_NEAR (b.get (ids::resFeedback), 0.777, 1.0e-4);
    CHECK_NEAR (b.get (ids::resMode), 2.0, 1.0e-6);
    CHECK_NEAR (b.get (ids::voiceCount), 5.0, 1.0e-6);
    CHECK_NEAR (b.get (ids::delaySync), 0.0, 1.0e-6);
    CHECK_NEAR (b.get (ids::modParam (3, ids::modSrcSuffix)), (double) ModSource::LFO2, 1.0e-6);
    CHECK_NEAR (b.get (ids::modParam (3, ids::modDstSuffix)), (double) ModDest::Feedback, 1.0e-6);
    CHECK_NEAR (b.get (ids::modParam (3, ids::modDepthSuffix)), -0.42, 1.0e-4);
    CHECK_NEAR (b.processor.getEditorScale(), 1.5, 1.0e-6);
    CHECK (b.processor.getPresetManager().getCurrentName() == "Test Patch");
    CHECK (b.processor.getMidiLearn().getMappedCC (ids::resDamping) == 21);
    CHECK (b.processor.getMidiLearn().getMappedParam (21) == ids::resDamping);
}

AERIFORM_TEST (state_is_deterministic_and_tolerates_garbage)
{
    TestHost a;
    a.set (ids::excPluck, 0.6f);
    juce::MemoryBlock s1, s2;
    a.processor.getStateInformation (s1);
    a.processor.getStateInformation (s2);
    CHECK (s1 == s2);

    // garbage input must be ignored without touching parameters
    const char garbage[] = "this is definitely not a valid state blob <<<>>>";
    a.processor.setStateInformation (garbage, (int) sizeof (garbage));
    CHECK_NEAR (a.get (ids::excPluck), 0.6, 1.0e-4);
    a.processor.setStateInformation (nullptr, 0);
    CHECK_NEAR (a.get (ids::excPluck), 0.6, 1.0e-4);

    // an XML state with only some parameters (older / partial) restores the rest to defaults
    juce::XmlElement xml ("AeriformState");
    xml.setAttribute ("version", 1);
    auto* params = xml.createNewChildElement ("AeriformParams");
    auto* p = params->createNewChildElement ("PARAM");
    p->setAttribute ("id", ids::resDamping);
    p->setAttribute ("value", 0.9);
    auto* unknown = params->createNewChildElement ("PARAM");
    unknown->setAttribute ("id", "does_not_exist");
    unknown->setAttribute ("value", 0.5);
    a.processor.applyStateXml (xml);
    CHECK_NEAR (a.get (ids::resDamping), 0.9, 1.0e-4);
    CHECK_NEAR (a.get (ids::excPluck), findParamInfo (ids::excPluck)->defaultValue, 1.0e-4);   // back to default
}

AERIFORM_TEST (preset_xml_round_trip_and_file_io)
{
    TestHost a;
    auto& pm = a.processor.getPresetManager();
    a.set (ids::resDispersion, 0.33f);
    a.set (ids::lfoParam (2, ids::lfoShapeSuffix), (float) LfoShape::Square);
    auto xml = pm.createPresetXml ("Round Trip", "Test", "SoundDesignLab");
    CHECK (xml != nullptr);
    CHECK (xml->getIntAttribute ("version") == PresetManager::kPresetFormatVersion);
    CHECK (xml->getStringAttribute ("author") == "SoundDesignLab");

    a.set (ids::resDispersion, 0.9f);
    a.set (ids::lfoParam (2, ids::lfoShapeSuffix), 0.0f);
    CHECK (pm.applyPresetXml (*xml));
    CHECK (pm.getCurrentAuthor() == "SoundDesignLab");
    CHECK_NEAR (a.get (ids::resDispersion), 0.33, 1.0e-4);
    CHECK_NEAR (a.get (ids::lfoParam (2, ids::lfoShapeSuffix)), (double) LfoShape::Square, 1.0e-6);

    const auto tmp = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("aeriform_test_preset.aerpreset");
    CHECK (pm.saveToFile (tmp, "File Preset", "Test", "AuthorX"));
    a.set (ids::resDispersion, 0.1f);
    CHECK (pm.loadFromFile (tmp));
    CHECK_NEAR (a.get (ids::resDispersion), 0.33, 1.0e-4);
    CHECK (pm.getCurrentName() == "File Preset");
    CHECK (pm.getCurrentAuthor() == "AuthorX");
    CHECK (! pm.isDirty());
    tmp.deleteFile();

    // a non-preset XML is rejected
    juce::XmlElement other ("Something");
    CHECK (! pm.applyPresetXml (other));
}

AERIFORM_TEST (factory_presets_reference_valid_parameters_and_load)
{
    TestHost a;
    auto& pm = a.processor.getPresetManager();
    const auto& presets = factoryPresets();
    CHECK (presets.size() >= 16);
    std::set<juce::String> names;
    for (const auto& preset : presets)
    {
        CHECK_MSG (names.insert (preset.name).second, ("duplicate preset name " + preset.name).toStdString());
        for (const auto& [id, value] : preset.values)
        {
            auto* p = a.processor.getAPVTS().getParameter (id);
            CHECK_MSG (p != nullptr, ("preset '" + preset.name + "' references unknown parameter " + id).toStdString());
            if (p != nullptr)
            {
                const auto& range = p->getNormalisableRange();
                CHECK_MSG (value >= range.start - 1.0e-4f && value <= range.end + 1.0e-4f,
                           ("preset '" + preset.name + "' value out of range for " + id).toStdString());
            }
        }
    }
    for (int i = 0; i < (int) pm.getEntries().size(); ++i)
    {
        if (! pm.getEntries()[(size_t) i].isFactory) continue;
        CHECK (pm.loadPreset (i));
        CHECK (pm.getCurrentName() == pm.getEntries()[(size_t) i].name);
        a.noteOn (60);
        const auto s = a.render (0.25);
        a.noteOff (60);
        a.render (0.1);
        CHECK_MSG (s.finite, ("preset '" + pm.getCurrentName() + "' produced non-finite output").toStdString());
        CHECK_MSG (s.peak < 1.5f, ("preset '" + pm.getCurrentName() + "' too loud: " + juce::String (s.peak)).toStdString());
        a.processor.getEngine().allNotesOff();
        a.render (0.05);
        a.processor.reset();
    }
    CHECK (pm.loadNext());
    CHECK (pm.loadPrevious());
    pm.loadInit();
    CHECK (pm.getCurrentName() == "Init");
}

AERIFORM_TEST (nonlinear_phase0_state_defaults_and_round_trip)
{
    TestHost host;
    host.set (ids::resNlOn, 1.0f);
    host.set (ids::rbNlDrive, 73.0f);
    host.set (ids::rcNlPos, 2.0f);
    auto saved = host.processor.createStateXml();
    CHECK (saved->getIntAttribute ("version") == 4);
    host.set (ids::resNlOn, 0.0f);
    host.processor.applyStateXml (*saved);
    CHECK (host.get (ids::resNlOn) == 1.0f);
    CHECK_NEAR (host.get (ids::rbNlDrive), 73.0, 1.0e-5);
    CHECK (host.get (ids::rcNlPos) == 2.0f);
    // Simulate the actual v3 format: it contains none of the appended IDs.
    saved->setAttribute ("version", 3);
    auto* params = saved->getChildByName (host.processor.getAPVTS().state.getType());
    CHECK (params != nullptr);
    if (params == nullptr) return;
    for (int i = (int) P::resNlOn; i <= (int) P::nlAdaa; ++i)
        for (auto* child : params->getChildIterator())
            if (child->getStringAttribute ("id") == ids::all[i])
            { params->removeChildElement (child, true); break; }
    saved->deleteAllChildElementsWithTagName ("PatchTools");
    host.processor.applyStateXml (*saved);
    for (int i = (int) P::resNlOn; i <= (int) P::nlAdaa; ++i)
        CHECK_NEAR (host.get (ids::all[i]), paramDef ((P) i).defaultValue, 1.0e-6);
}


AERIFORM_TEST (nonlinear_phase5_demonstrator_presets_smoke)
{
    int count = 0;
    for (const auto& preset : factoryPresets())
    {
        if (preset.category != "Nonlinear") continue;
        ++count;
        std::vector<float> rendered[2];
        for (int enabled = 0; enabled < 2; ++enabled)
        {
            TestHost host;
            host.processor.getPresetManager().applyFactoryPreset (preset);
            for (const auto& [id, value] : preset.values)
            {
                auto* parameter = host.processor.getAPVTS().getParameter (id);
                CHECK (parameter != nullptr);
                if (parameter)
                {
                    const auto& range = parameter->getNormalisableRange();
                    CHECK (value >= range.start && value <= range.end);
                }
            }
            host.set (ids::resNlOn, (float) enabled);
            host.render (0.04);
            host.noteOn (57, 110);
            const auto held = host.render (0.8, &rendered[enabled]);
            host.noteOff (57);
            const auto tail = host.render (0.2, &rendered[enabled]);
            CHECK (held.finite && tail.finite && held.peak < 1.5f && tail.peak < 1.5f);
            CHECK (held.rms > 1.0e-5);
            if (const char* folder = std::getenv ("AERIFORM_CAPTURE_DIR"))
            {
                auto dir = juce::File (juce::String::fromUTF8 (folder));
                CHECK (dir.createDirectory().wasOk());
                auto file = dir.getChildFile (preset.name + (enabled ? "-on.wav" : "-off.wav"));
                auto stream = file.createOutputStream();
                CHECK (stream != nullptr);
                if (stream)
                {
                    stream->setPosition (0); stream->truncate();
                    juce::WavAudioFormat format;
                    std::unique_ptr<juce::AudioFormatWriter> writer (format.createWriterFor (stream.release(), host.sampleRate, 1, 24, {}, 0));
                    CHECK (writer != nullptr);
                    const float* data[] { rendered[enabled].data() };
                    if (writer) CHECK (writer->writeFromFloatArrays (data, 1, (int) rendered[enabled].size()));
                }
            }
        }
        CHECK (rendered[0].size() == rendered[1].size());
        double difference = 0.0, reference = 0.0;
        for (size_t i = 0; i < rendered[0].size(); ++i)
        {
            const double delta = double(rendered[1][i])-rendered[0][i];
            difference += delta*delta; reference += double(rendered[0][i])*rendered[0][i];
        }
        const double relative = std::sqrt (difference/std::max (reference, 1.0e-24));
        std::printf ("  %s: relative RMS difference %.6f\n", preset.name.toRawUTF8(), relative);
        CHECK (relative > 0.01); // signal difference, not subjective audibility acceptance
    }
    CHECK (count == 4);
}
