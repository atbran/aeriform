#include "TestFramework.h"
#include "TestHelpers.h"

using namespace aeriform;
using namespace aeriform::test;

namespace
{
    void quietPatch (TestHost& h)
    {
        h.set (ids::reverbMix, 0.0f);
        h.set (ids::delayMix, 0.0f);
        h.set (ids::chorusMix, 0.0f);
        h.set (ids::excReleaseNoise, 0.0f);
        h.set (ids::envRelease, 60.0f);
    }
}

AERIFORM_TEST (polyphony_allocates_eight_voices_and_steals_beyond)
{
    TestHost h;
    quietPatch (h);
    h.set (ids::voiceCount, 8.0f);
    for (int n = 0; n < 8; ++n) h.noteOn (48 + n * 3);
    h.renderBlock();
    CHECK (h.activeVoices() == 8);

    h.noteOn (90);   // ninth note steals
    auto s = h.render (0.05);
    CHECK (s.finite);
    CHECK (h.activeVoices() <= 8);
    bool found = false;
    for (int i = 0; i < 16; ++i)
        if (h.processor.getVisualizerModel().voices[(size_t) i].active.load()
            && std::fabs (h.processor.getVisualizerModel().voices[(size_t) i].pitchHz.load() - dsp::midiNoteToHz (90.0f)) < 20.0f)
            found = true;
    CHECK_MSG (found, "stolen voice plays the new note");

    for (int n = 0; n < 8; ++n) h.noteOff (48 + n * 3);
    h.noteOff (90);
    h.render (0.6);
    CHECK (h.activeVoices() == 0);
    const auto tail = h.render (0.2);
    CHECK (tail.peak < 1.0e-4f);
}

AERIFORM_TEST (voice_count_parameter_limits_polyphony)
{
    TestHost h;
    quietPatch (h);
    h.set (ids::voiceCount, 3.0f);
    for (int n = 0; n < 6; ++n) h.noteOn (50 + n * 2);
    h.render (0.05);
    CHECK (h.activeVoices() <= 3);
    h.set (ids::voiceCount, 16.0f);
    for (int n = 0; n < 6; ++n) h.noteOff (50 + n * 2);
    h.render (0.3);
    for (int n = 0; n < 12; ++n) h.noteOn (40 + n * 2);
    h.renderBlock();
    CHECK (h.activeVoices() == 12);
}

AERIFORM_TEST (mono_and_legato_modes_use_one_voice_and_return_to_held_note)
{
    TestHost h;
    quietPatch (h);
    h.set (ids::voiceMode, (float) VoiceMode::Legato);
    h.noteOn (60);
    h.renderBlock();
    h.noteOn (67);
    h.render (0.05);
    CHECK (h.activeVoices() == 1);
    auto& vis = h.processor.getVisualizerModel();
    auto currentPitch = [&]
    {
        for (int i = 0; i < 16; ++i)
            if (vis.voices[(size_t) i].active.load()) return vis.voices[(size_t) i].pitchHz.load();
        return 0.0f;
    };
    CHECK_NEAR (currentPitch(), dsp::midiNoteToHz (67.0f), 3.0);
    h.noteOff (67);   // second note released: the held first note comes back
    h.render (0.05);
    CHECK (h.activeVoices() == 1);
    CHECK_NEAR (currentPitch(), dsp::midiNoteToHz (60.0f), 3.0);
    h.noteOff (60);
    h.render (0.5);
    CHECK (h.activeVoices() == 0);

    h.set (ids::voiceMode, (float) VoiceMode::Mono);
    h.noteOn (48); h.noteOn (52); h.noteOn (55);
    h.render (0.05);
    CHECK (h.activeVoices() == 1);
    h.noteOff (48); h.noteOff (52); h.noteOff (55);
    h.render (0.5);
    CHECK (h.activeVoices() == 0);
}

AERIFORM_TEST (glide_moves_pitch_smoothly)
{
    TestHost h;
    quietPatch (h);
    h.set (ids::voiceMode, (float) VoiceMode::Legato);
    h.set (ids::glideTime, 400.0f);
    h.noteOn (48);
    h.render (0.1);
    h.noteOn (60);
    h.render (0.1);   // 100 ms into a 400 ms glide: pitch must be between the notes
    auto& vis = h.processor.getVisualizerModel();
    float pitch = 0.0f;
    for (int i = 0; i < 16; ++i) if (vis.voices[(size_t) i].active.load()) pitch = vis.voices[(size_t) i].pitchHz.load();
    CHECK (pitch > dsp::midiNoteToHz (49.0f) && pitch < dsp::midiNoteToHz (59.0f));
    h.render (0.5);
    for (int i = 0; i < 16; ++i) if (vis.voices[(size_t) i].active.load()) pitch = vis.voices[(size_t) i].pitchHz.load();
    CHECK_NEAR (pitch, dsp::midiNoteToHz (60.0f), 2.0);
}

AERIFORM_TEST (unison_stacks_voices_per_note)
{
    TestHost h;
    quietPatch (h);
    h.set (ids::unisonVoices, 3.0f);
    h.noteOn (60);
    h.renderBlock();
    CHECK (h.activeVoices() == 3);
    h.noteOn (64);
    h.renderBlock();
    CHECK (h.activeVoices() == 6);
    h.set (ids::unisonVoices, 1.0f);
    h.noteOff (60); h.noteOff (64);
    h.render (0.5);
    CHECK (h.activeVoices() == 0);
}

AERIFORM_TEST (sustain_pedal_holds_notes)
{
    TestHost h;
    quietPatch (h);
    h.cc (64, 127);
    h.noteOn (60);
    h.renderBlock();
    h.noteOff (60);
    h.render (0.4);
    CHECK (h.activeVoices() == 1);   // still sounding: pedal down
    h.cc (64, 0);
    h.render (0.5);
    CHECK (h.activeVoices() == 0);
}

AERIFORM_TEST (pitch_bend_and_bend_range_change_pitch)
{
    TestHost h;
    quietPatch (h);
    h.set (ids::bendRange, 2.0f);
    auto& vis = h.processor.getVisualizerModel();
    auto pitch = [&]
    {
        for (int i = 0; i < 16; ++i) if (vis.voices[(size_t) i].active.load()) return vis.voices[(size_t) i].pitchHz.load();
        return 0.0f;
    };
    h.noteOn (60);
    h.render (0.05);
    CHECK_NEAR (pitch(), dsp::midiNoteToHz (60.0f), 2.0);
    h.pitchBend (16383);
    h.render (0.05);
    CHECK_NEAR (pitch(), dsp::midiNoteToHz (62.0f), 3.0);
    h.pitchBend (8192);
    h.set (ids::bendRange, 12.0f);
    h.render (0.05);
    h.pitchBend (0);
    h.render (0.05);
    CHECK_NEAR (pitch(), dsp::midiNoteToHz (48.0f), 2.0);
}

AERIFORM_TEST (velocity_and_aftertouch_reach_the_voice)
{
    TestHost h;
    quietPatch (h);
    h.set (ids::excVelocity, 1.0f);
    h.set (ids::envVelToPressure, 1.0f);
    h.noteOn (60, 20);
    const auto soft = h.render (0.4);
    h.noteOff (60);
    h.render (0.5);
    h.noteOn (60, 127);
    const auto hard = h.render (0.4);
    h.noteOff (60);
    h.render (0.5);
    CHECK_MSG (hard.rms > soft.rms * 1.5, "velocity scales level: soft=" + std::to_string (soft.rms) + " hard=" + std::to_string (hard.rms));

    // aftertouch routed to amp via the matrix (depth -1 => pressure silences the note)
    h.set (ids::modParam (1, ids::modSrcSuffix), (float) ModSource::Aftertouch);
    h.set (ids::modParam (1, ids::modDstSuffix), (float) ModDest::Amp);
    h.set (ids::modParam (1, ids::modDepthSuffix), -1.0f);
    h.noteOn (60, 100);
    const auto before = h.render (0.4);
    h.aftertouch (127);
    h.render (0.1);
    const auto after = h.render (0.4);
    CHECK_MSG (after.rms < before.rms * 0.1, "aftertouch->amp: before=" + std::to_string (before.rms) + " after=" + std::to_string (after.rms));
}

AERIFORM_TEST (mod_matrix_routes_mod_wheel_and_lfo)
{
    TestHost h;
    quietPatch (h);
    h.set (ids::modParam (2, ids::modSrcSuffix), (float) ModSource::ModWheel);
    h.set (ids::modParam (2, ids::modDstSuffix), (float) ModDest::Amp);
    h.set (ids::modParam (2, ids::modDepthSuffix), -1.0f);
    h.noteOn (60);
    const auto loud = h.render (0.4);
    h.cc (1, 127);
    h.render (0.1);
    const auto quiet = h.render (0.4);
    CHECK (quiet.rms < loud.rms * 0.1);
    h.cc (1, 0);

    // LFO -> pitch: pitch must move over time
    h.set (ids::modParam (2, ids::modSrcSuffix), (float) ModSource::LFO1);
    h.set (ids::modParam (2, ids::modDstSuffix), (float) ModDest::Pitch);
    h.set (ids::modParam (2, ids::modDepthSuffix), 0.5f);
    h.set (ids::lfoParam (1, ids::lfoRateSuffix), 2.0f);
    auto& vis = h.processor.getVisualizerModel();
    float minP = 1.0e9f, maxP = 0.0f;
    for (int b = 0; b < 100; ++b)
    {
        h.renderBlock();
        for (int i = 0; i < 16; ++i)
            if (vis.voices[(size_t) i].active.load())
            {
                const float p = vis.voices[(size_t) i].pitchHz.load();
                minP = std::min (minP, p); maxP = std::max (maxP, p);
            }
    }
    CHECK_MSG (maxP / minP > 1.5f, "LFO pitch range " + std::to_string (minP) + ".." + std::to_string (maxP));
}

AERIFORM_TEST (midi_learn_maps_controller_to_parameter)
{
    TestHost h;
    auto& learn = h.processor.getMidiLearn();
    learn.armLearn (ids::resBrightness);
    CHECK (learn.isLearning());
    h.cc (74, 100);
    h.renderBlock();
    CHECK (learn.pollLearn());
    CHECK (! learn.isLearning());
    CHECK (learn.getMappedCC (ids::resBrightness) == 74);
    h.cc (74, 0);
    h.renderBlock();
    CHECK_NEAR (h.get (ids::resBrightness), 0.0, 1.0e-4);
    h.cc (74, 127);
    h.renderBlock();
    CHECK_NEAR (h.get (ids::resBrightness), 1.0, 1.0e-4);
    learn.clearMapping (ids::resBrightness);
    CHECK (learn.getMappedCC (ids::resBrightness) == MidiLearn::kUnmapped);
    h.cc (74, 0);
    h.renderBlock();
    CHECK_NEAR (h.get (ids::resBrightness), 1.0, 1.0e-4);   // no longer mapped
}

AERIFORM_TEST (mpe_mode_handles_per_note_expression)
{
    TestHost h;
    quietPatch (h);
    h.set (ids::mpeEnabled, 1.0f);
    h.renderBlock();
    // MPE lower zone: notes on member channels 2 and 3 with different per-note bends
    h.midi.addEvent (juce::MidiMessage::noteOn (2, 60, (juce::uint8) 100), 0);
    h.midi.addEvent (juce::MidiMessage::noteOn (3, 60, (juce::uint8) 100), 0);
    h.renderBlock();
    CHECK (h.activeVoices() == 2);
    h.midi.addEvent (juce::MidiMessage::pitchWheel (2, 16383), 0);   // +48 semitones range -> full up
    h.midi.addEvent (juce::MidiMessage::channelPressureChange (3, 127), 0);
    h.render (0.05);
    auto& vis = h.processor.getVisualizerModel();
    float hi = 0.0f, lo = 1.0e9f, pressure = 0.0f;
    for (int i = 0; i < 16; ++i)
        if (vis.voices[(size_t) i].active.load())
        {
            hi = std::max (hi, vis.voices[(size_t) i].pitchHz.load());
            lo = std::min (lo, vis.voices[(size_t) i].pitchHz.load());
        }
    CHECK_MSG (hi / lo > 4.0f, "per-note bend applies to one note only: " + std::to_string (lo) + " / " + std::to_string (hi));
    juce::ignoreUnused (pressure);
    h.midi.addEvent (juce::MidiMessage::noteOff (2, 60), 0);
    h.midi.addEvent (juce::MidiMessage::noteOff (3, 60), 0);
    h.render (0.5);
    CHECK (h.activeVoices() == 0);
}

AERIFORM_TEST (sample_rate_and_block_size_changes_are_safe)
{
    TestHost h (44100.0, 64);
    quietPatch (h);
    h.noteOn (60);
    CHECK (h.render (0.2).finite);
    h.prepare (96000.0, 1024);
    h.noteOn (64);
    const auto s = h.render (0.3);
    CHECK (s.finite);
    CHECK (s.rms > 1.0e-4);
    h.prepare (48000.0, 32);
    h.noteOn (67);
    CHECK (h.render (0.2).finite);
    h.processor.releaseResources();
    h.prepare (48000.0, 256);
    h.noteOn (60);
    CHECK (h.render (0.2).rms > 1.0e-4);
}

AERIFORM_TEST (bypass_releases_notes_and_outputs_silence)
{
    TestHost h;
    quietPatch (h);
    h.noteOn (60);
    h.render (0.1);
    juce::MidiBuffer empty;
    h.buffer.clear();
    h.processor.processBlockBypassed (h.buffer, empty);
    float peak = 0.0f;
    for (int i = 0; i < h.blockSize; ++i) peak = std::max (peak, std::fabs (h.buffer.getSample (0, i)));
    CHECK (peak == 0.0f);
    h.render (0.5);
    CHECK (h.activeVoices() == 0);
}

AERIFORM_TEST (polyphonic_and_channel_aftertouch_and_controller_reset)
{
    TestHost h;
    quietPatch (h);

    // Route aftertouch to amp with -1.0 depth: full pressure silences the voice
    h.set (ids::modParam (1, ids::modSrcSuffix), (float) ModSource::Aftertouch);
    h.set (ids::modParam (1, ids::modDstSuffix), (float) ModDest::Amp);
    h.set (ids::modParam (1, ids::modDepthSuffix), -1.0f);

    h.noteOn (60, 100);
    h.render (0.2);
    CHECK (h.activeVoices() == 1);
    const auto before60 = h.render (0.2);
    CHECK (before60.rms > 1.0e-4);

    // Polyphonic aftertouch for note 60: silences note 60
    h.midi.addEvent (juce::MidiMessage::aftertouchChange (1, 60, 127), 0);
    h.render (0.05);
    const auto afterPoly = h.render (0.2);
    CHECK_MSG (afterPoly.rms < before60.rms * 0.15, "poly aftertouch for note 60 silences note 60");

    // Release note 60 pressure
    h.midi.addEvent (juce::MidiMessage::aftertouchChange (1, 60, 0), 0);
    h.render (0.1);
    const auto restored60 = h.render (0.2);
    CHECK_MSG (restored60.rms > 1.0e-4, "releasing pressure restores note 60");

    // Channel pressure -> silences note 60
    h.midi.addEvent (juce::MidiMessage::channelPressureChange (1, 127), 0);
    h.render (0.05);
    const auto afterChan = h.render (0.2);
    CHECK_MSG (afterChan.rms < restored60.rms * 0.15, "channel pressure silences voice");

    // Performance MIDI must not generate undo transactions
    CHECK (! h.processor.getPatchTools().undo.canUndo());

    // Send CC 123 (All Notes Off)
    h.midi.addEvent (juce::MidiMessage::controllerEvent (1, 123, 0), 0);
    h.render (0.4);
    CHECK (h.activeVoices() == 0);

    // Send CC 121 (Reset All Controllers)
    h.midi.addEvent (juce::MidiMessage::controllerEvent (1, 121, 0), 0);
    h.render (0.05);
    CHECK (! h.processor.getPatchTools().undo.canUndo());
}

AERIFORM_TEST (voice_vibrato_controls_modulate_speed_and_depth)
{
    TestHost h;
    quietPatch (h);
    h.set (ids::artFlowPitch, 0.0f);
    h.set (ids::artInstability, 0.0f);
    h.set (ids::artVariation, 0.0f);
    h.set (ids::glideTime, 0.0f);
    h.set (ids::vibratoDepth, 0.0f);
    h.set (ids::vibratoRate, 5.0f);

    h.noteOn (60);
    h.render (0.05);
    const float staticPitch = h.processor.getVisualizerModel().voices[0].pitchHz.load();
    CHECK_NEAR (staticPitch, dsp::midiNoteToHz (60.0f), 0.5f);

    // Turn vibrato on with full depth (50 cents = 1/2 semitone up/down)
    h.set (ids::vibratoDepth, 1.0f);
    h.set (ids::vibratoRate, 5.0f); // 5 Hz cycle = 200 ms period

    float minHz = 10000.0f, maxHz = 0.0f;
    for (int step = 0; step < 25; ++step)
    {
        h.render (0.01); // 10 ms steps across 250 ms
        const float f = h.processor.getVisualizerModel().voices[0].pitchHz.load();
        if (f > 0.0f)
        {
            minHz = std::min (minHz, f);
            maxHz = std::max (maxHz, f);
        }
    }

    const float expectedCenter = dsp::midiNoteToHz (60.0f);
    // At 261.63 Hz, 50 cents down is ~254.2 Hz (-7.4 Hz), 50 cents up is ~269.3 Hz (+7.6 Hz)
    CHECK (minHz < expectedCenter - 3.0f);
    CHECK (maxHz > expectedCenter + 3.0f);
    CHECK (maxHz - minHz >= 10.0f);

    h.noteOff (60);
    h.render (0.1);
}
