// Offline DSP smoke / stress tests. Run with:  AeriformTests --smoke
#include "TestFramework.h"
#include "TestHelpers.h"
#include "Presets/FactoryPresets.h"
#include <chrono>

using namespace aeriform;
using namespace aeriform::test;

namespace
{
    void playChord (TestHost& h, int velocity = 100)
    {
        for (int note : { 48, 55, 60, 64, 67, 72, 76, 79 }) h.noteOn (note, velocity);
    }
    void releaseChord (TestHost& h)
    {
        for (int note : { 48, 55, 60, 64, 67, 72, 76, 79 }) h.noteOff (note);
    }
}

AERIFORM_TEST (smoke_render_at_three_sample_rates_and_block_sizes)
{
    for (double sr : { 44100.0, 48000.0, 96000.0 })
        for (int bs : { 32, 256, 1024 })
        {
            TestHost h (sr, bs);
            const auto silence = h.render (0.25);
            CHECK (silence.finite && silence.peak < 1.0e-4f);

            playChord (h);
            const auto sustain = h.render (2.0);
            releaseChord (h);
            const auto tail = h.render (3.0);
            const auto after = h.render (1.0);

            char msg[200];
            std::snprintf (msg, sizeof (msg), "sr=%.0f bs=%d finite=%d peak=%.3f rms=%.4f tail=%.4f after=%.6f voices=%d",
                           sr, bs, (int) (sustain.finite && tail.finite && after.finite), sustain.peak, sustain.rms, tail.rms, after.peak, h.activeVoices());
            std::printf ("    %s\n", msg);
            CHECK_MSG (sustain.finite && tail.finite && after.finite, msg);
            CHECK_MSG (sustain.peak < 1.25f, msg);                 // limiter bound
            CHECK_MSG (sustain.rms > 1.0e-3, msg);                 // non-trivial audio
            CHECK_MSG (after.peak < 2.0e-3f, msg);                 // decays (reverb tail may linger quietly)
        }
}

AERIFORM_TEST (smoke_extreme_parameter_combinations_stay_bounded)
{
    TestHost h (96000.0, 128);
    const std::pair<const char*, float> extremes[] = {
        { ids::resFeedback, 1.0f }, { ids::resSaturation, 1.0f }, { ids::resDispersion, 1.0f }, { ids::resDamping, 0.0f },
        { ids::resBrightness, 1.0f }, { ids::resShape, 1.0f }, { ids::resReflection, 0.0f }, { ids::resBodyRes, 1.0f },
        { ids::resBodyMix, 1.0f }, { ids::resBodyFreq, 8000.0f }, { ids::excReed, 1.0f }, { ids::excPressure, 1.0f },
        { ids::excNoise, 1.0f }, { ids::excPluck, 1.0f }, { ids::excTurbulence, 1.0f }, { ids::excLowpass, 20000.0f },
        { ids::excAttackClick, 1.0f }, { ids::excReleaseNoise, 1.0f }, { ids::artCoupling, 1.0f }, { ids::artInstability, 1.0f },
        { ids::artVariation, 1.0f }, { ids::artFlowPitch, 1.0f }, { ids::unisonVoices, 4.0f }, { ids::unisonDetune, 100.0f },
        { ids::voiceCount, 16.0f }, { ids::delayMix, 1.0f }, { ids::delayFeedback, 0.95f }, { ids::reverbMix, 1.0f },
        { ids::reverbDecay, 1.0f }, { ids::reverbSize, 1.0f }, { ids::chorusMix, 1.0f }, { ids::outGain, 12.0f },
        { ids::envAttack, 0.5f }, { ids::envRelease, 12000.0f }, { ids::resCoarse, 24.0f }, { ids::resLength, 0.5f },
    };
    for (const auto& [id, v] : extremes) h.set (id, v);
    for (int i = 1; i <= ids::numModSlots; ++i)
    {
        h.set (ids::modParam (i, ids::modSrcSuffix), (float) ((i % ((int) ModSource::Count - 1)) + 1));
        h.set (ids::modParam (i, ids::modDstSuffix), (float) ((i * 3) % ((int) ModDest::Count - 1) + 1));
        h.set (ids::modParam (i, ids::modDepthSuffix), i % 2 == 0 ? 1.0f : -1.0f);
    }
    for (int note : { 0, 12, 60, 108, 120, 127, 36, 84 }) h.noteOn (note, 127);
    h.cc (1, 127);
    h.aftertouch (127);
    auto s = h.render (3.0);
    CHECK_MSG (s.finite, "extreme render finite");
    CHECK_MSG (s.peak < 1.3f, "extreme peak " + std::to_string (s.peak));
    for (int note : { 0, 12, 60, 108, 120, 127, 36, 84 }) h.noteOff (note);
    s = h.render (2.0);
    CHECK (s.finite);

    // the opposite corner: everything at minimum
    for (auto* p : h.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p)) rp->setValueNotifyingHost (0.0f);
    h.noteOn (60);
    s = h.render (1.0);
    CHECK (s.finite);
    for (auto* p : h.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p)) rp->setValueNotifyingHost (1.0f);
    h.noteOn (64);
    s = h.render (1.0);
    CHECK (s.finite);
    CHECK_MSG (s.peak < 1.3f, "all-max peak " + std::to_string (s.peak));
}

AERIFORM_TEST (smoke_rapid_preset_changes_and_automation_during_playback)
{
    TestHost h (48000.0, 256);
    auto& pm = h.processor.getPresetManager();
    playChord (h);
    const int n = (int) pm.getEntries().size();
    bool finite = true;
    float peak = 0.0f;
    for (int b = 0; b < 400; ++b)
    {
        if (b % 3 == 0) pm.loadPreset (b / 3 % n);
        // sweep a few sensitive parameters every block
        const float t = (float) b / 400.0f;
        h.set (ids::resFeedback, 0.5f + 0.5f * std::sin (t * 40.0f));
        h.set (ids::resCoarse, -24.0f + 48.0f * t);
        h.set (ids::excLowpass, 200.0f + 19000.0f * t);
        h.set (ids::resMode, (float) (b % 3));
        h.set (ids::voiceMode, (float) ((b / 50) % 3));
        h.set (ids::unisonVoices, (float) (1 + (b / 25) % 4));
        h.set (ids::reverbSize, t);
        h.set (ids::delayTime, 10.0f + 1990.0f * t);
        const auto s = h.renderBlock();
        finite = finite && s.finite;
        peak = std::max (peak, s.peak);
        if (b % 40 == 20) { releaseChord (h); }
        if (b % 40 == 30) { playChord (h); }
    }
    CHECK (finite);
    CHECK_MSG (peak < 1.3f, "peak " + std::to_string (peak));
    releaseChord (h);
    CHECK (h.render (1.0).finite);
}

AERIFORM_TEST (smoke_repeated_prepare_release_cycles)
{
    TestHost h;
    for (int i = 0; i < 12; ++i)
    {
        const double sr = (i % 3 == 0) ? 44100.0 : (i % 3 == 1) ? 48000.0 : 96000.0;
        const int bs = 32 << (i % 5);
        h.processor.releaseResources();
        h.prepare (sr, bs);
        h.noteOn (60 + i);
        const auto s = h.render (0.1);
        CHECK (s.finite);
        h.noteOff (60 + i);
        CHECK (h.render (0.1).finite);
    }
}

AERIFORM_TEST (smoke_cpu_measurement_eight_voices_with_effects)
{
    TestHost h (48000.0, 256);
    h.set (ids::reverbMix, 0.3f);
    h.set (ids::delayMix, 0.3f);
    h.set (ids::chorusMix, 0.3f);
    h.set (ids::resDispersion, 0.4f);
    h.set (ids::envRelease, 4000.0f);
    playChord (h);
    h.render (0.5);   // warm-up
    const double seconds = 10.0;
    const auto t0 = std::chrono::steady_clock::now();
    const auto s = h.render (seconds);
    const auto t1 = std::chrono::steady_clock::now();
    const double elapsed = std::chrono::duration<double> (t1 - t0).count();
    const double ratio = elapsed / seconds;
    std::printf ("    8 voices + effects @ 48 kHz / 256: rendered %.1f s of audio in %.3f s  ->  %.1f %% of real time (%d voices active)\n",
                 seconds, elapsed, ratio * 100.0, h.activeVoices());
    CHECK (s.finite);
    CHECK (h.activeVoices() == 8);
    CHECK_MSG (ratio < 0.5, "real-time ratio " + std::to_string (ratio));

    h.set (ids::voiceCount, 16.0f);
    h.set (ids::unisonVoices, 2.0f);
    for (int note : { 50, 53, 57, 62, 65, 69, 74, 77 }) h.noteOn (note);
    h.render (0.5);
    const auto t2 = std::chrono::steady_clock::now();
    h.render (5.0);
    const auto t3 = std::chrono::steady_clock::now();
    const double ratio16 = std::chrono::duration<double> (t3 - t2).count() / 5.0;
    std::printf ("    16 voices (unison 2) + effects @ 48 kHz / 256: %.1f %% of real time\n", ratio16 * 100.0);
    CHECK_MSG (ratio16 < 1.0, "16-voice real-time ratio " + std::to_string (ratio16));
}
