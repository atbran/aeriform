#include "TestFramework.h"
#include "TestHelpers.h"

using namespace aeriform;
using namespace aeriform::test;

namespace
{
    void dryFx (TestHost& h)
    {
        h.set (ids::reverbMix, 0.0f); h.set (ids::delayMix, 0.0f); h.set (ids::chorusMix, 0.0f);
        h.set (ids::excReleaseNoise, 0.0f); h.set (ids::envRelease, 80.0f);
    }
    void enableAll (TestHost& h)
    {
        h.set (ids::resOn, 1.0f); h.set (ids::rbOn, 1.0f); h.set (ids::rcOn, 1.0f);
    }
    bool running (TestHost& h, int i) { return h.processor.getVisualizerModel().resonatorRunning[(size_t) i].load() != 0; }
}

AERIFORM_TEST (routing_modes_render_finite_and_engage_the_right_slots)
{
    for (int mode = 0; mode < (int) NetMode::Count; ++mode)
    {
        TestHost h (48000.0, 256);
        dryFx (h);
        enableAll (h);
        h.set (ids::netMode, (float) mode);
        for (int note : { 48, 55, 64 }) h.noteOn (note);
        const auto s = h.render (0.5);
        const std::string name = choices::netModes()[mode].toStdString();
        CHECK_MSG (s.finite, name + " finite");
        CHECK_MSG (s.peak < 1.3f, name + " bounded " + std::to_string (s.peak));
        CHECK_MSG (s.rms > 1.0e-3, name + " audible " + std::to_string (s.rms));
        CHECK_MSG (running (h, 0), name + " slot A running");
        if (mode == (int) NetMode::Single)
            CHECK_MSG (! running (h, 1) && ! running (h, 2), name + " only A runs");
        else
            CHECK_MSG (running (h, 1) && running (h, 2), name + " B and C run");
        for (int note : { 48, 55, 64 }) h.noteOff (note);
        CHECK (h.render (0.4).finite);
    }
}

AERIFORM_TEST (serial_routing_passes_energy_downstream)
{
    TestHost h (48000.0, 256);
    dryFx (h);
    enableAll (h);
    h.set (ids::netMode, (float) NetMode::Serial);
    h.set (ids::netTap, (float) OutputTap::C);       // listen to the last stage only
    h.set (ids::rcType, (float) ResMode::OpenPipe);
    h.noteOn (60);
    const auto through = h.render (0.5);
    CHECK_MSG (through.rms > 1.0e-3, "C is excited through A -> B -> C: " + std::to_string (through.rms));
    h.noteOff (60);
    h.render (0.5);
    h.set (ids::netSendAB, 0.0f);   // cut the chain: C receives nothing
    h.set (ids::netInjectC, 0.0f);
    h.noteOn (60);
    h.render (0.3);
    const auto cut = h.render (0.3);
    CHECK_MSG (cut.rms < through.rms * 0.05 + 1.0e-5, "cutting the serial send silences C: " + std::to_string (cut.rms));
}

AERIFORM_TEST (parallel_routing_sums_independent_resonators)
{
    TestHost h (48000.0, 256);
    dryFx (h);
    h.set (ids::netMode, (float) NetMode::Parallel);
    h.set (ids::resOn, 0.0f); h.set (ids::rbOn, 1.0f); h.set (ids::rcOn, 0.0f);
    h.set (ids::rbType, (float) ResMode::ModalBank);
    h.set (ids::rbFeedback, 0.4f);
    h.noteOn (60);
    const auto onlyB = h.render (0.5);
    CHECK_MSG (onlyB.rms > 1.0e-3, "resonator B alone in parallel is audible: " + std::to_string (onlyB.rms));
    h.set (ids::rbInput, 0.0f);
    h.render (1.0);
    const auto noInput = h.render (0.3);
    CHECK_MSG (noInput.rms < onlyB.rms * 0.05 + 1.0e-5, "input level 0 -> silent: " + std::to_string (noInput.rms));
}

AERIFORM_TEST (every_cross_feedback_route_is_stable_at_maximum)
{
    const char* routes[] = { ids::netAB, ids::netBA, ids::netBC, ids::netCB, ids::netCA, ids::netAC };
    for (int r = 0; r <= 6; ++r)   // 0..5 = single routes, 6 = all at once
    {
        TestHost h (48000.0, 256);
        dryFx (h);
        enableAll (h);
        h.set (ids::netMode, (float) NetMode::Parallel);
        h.set (ids::netFeedback, 1.0f); h.set (ids::netFbDrive, 1.0f); h.set (ids::netDamping, 0.0f); h.set (ids::netFbDelay, 50.0f);
        h.set (ids::resFeedback, 1.0f); h.set (ids::rbFeedback, 1.0f); h.set (ids::rcFeedback, 1.0f);
        h.set (ids::resSaturation, 1.0f); h.set (ids::rbSaturation, 1.0f); h.set (ids::rcSaturation, 1.0f);
        h.set (ids::resDamping, 0.0f); h.set (ids::rbDamping, 0.0f); h.set (ids::rcDamping, 0.0f);
        h.set (ids::rcType, (float) ResMode::MetallicBar);
        if (r < 6) h.set (routes[r], 1.0f); else for (auto* id : routes) h.set (id, 1.0f);
        for (int note : { 40, 52, 67, 79 }) h.noteOn (note, 127);
        const auto s = h.render (3.0);
        const std::string name = r < 6 ? std::string (routes[r]) : std::string ("all routes");
        CHECK_MSG (s.finite, name + " finite");
        CHECK_MSG (s.peak < 1.3f, name + " bounded, peak " + std::to_string (s.peak));
        for (int note : { 40, 52, 67, 79 }) h.noteOff (note);
        h.set (ids::netPolarity, (float) Polarity::Negative);
        for (int note : { 41, 53 }) h.noteOn (note, 127);
        const auto s2 = h.render (1.5);
        CHECK_MSG (s2.finite && s2.peak < 1.3f, name + " negative polarity bounded");
    }
}

AERIFORM_TEST (topology_changes_during_active_notes_are_click_safe)
{
    TestHost h (48000.0, 256);
    dryFx (h);
    h.set (ids::limiterOn, 0.0f);   // no limiter: clicks would show up as raw sample-to-sample jumps
    for (int note : { 48, 55, 64, 72 }) h.noteOn (note, 90);
    h.render (0.3);
    // steady-state reference: largest sample-to-sample jump of the running sound
    float steadyJump = 0.0f, last = 0.0f;
    for (int b = 0; b < 40; ++b)
    {
        std::vector<float> mono;
        h.renderBlock (&mono);
        for (float v : mono) { steadyJump = std::max (steadyJump, std::fabs (v - last)); last = v; }
    }
    bool finite = true; float peak = 0.0f, jump = 0.0f;
    for (int b = 0; b < 600; ++b)
    {
        switch (b % 12)
        {
            case 0: h.set (ids::netMode, (float) ((b / 12) % 4)); break;
            case 2: h.set (ids::rbOn, (float) ((b / 12) % 2)); break;
            case 4: h.set (ids::rcOn, (float) (((b / 12) + 1) % 2)); break;
            case 6: h.set (ids::rbType, (float) ((b / 12) % (int) ResMode::Count)); break;
            case 8: h.set (ids::resMode, (float) (((b / 12) * 5) % (int) ResMode::Count)); break;
            case 10: h.set (ids::netRepipe, (b / 12) % 2 == 0 ? 1.0f : 0.0f); break;
            default: break;
        }
        std::vector<float> mono;
        const auto s = h.renderBlock (&mono);
        finite = finite && s.finite;
        peak = std::max (peak, s.peak);
        for (float v : mono) { jump = std::max (jump, std::fabs (v - last)); last = v; }
    }
    CHECK (finite);
    CHECK_MSG (peak < 2.0f, "level stays musical while switching (4 voices, no limiter): peak " + std::to_string (peak));
    // a click is a sample-to-sample step of the order of the signal peak; the running sound's largest slope is ~0.45 x peak
    CHECK_MSG (jump < std::max (steadyJump * 3.0f, 0.6f * peak) + 0.05f, "no discontinuities: max jump " + std::to_string (jump) + " vs steady " + std::to_string (steadyJump) + ", peak " + std::to_string (peak));
}

AERIFORM_TEST (energy_loop_is_off_by_default_and_bounded_at_maximum)
{
    TestHost def (48000.0, 256);
    CHECK (def.get (ids::loopOn) < 0.5f);

    for (int src = 0; src < (int) LoopSource::Count; ++src)
        for (int dst = 0; dst < (int) LoopDest::Count; ++dst)
            for (int pol = 0; pol < 2; ++pol)
            {
                TestHost h (48000.0, 256);
                dryFx (h);
                enableAll (h);
                h.set (ids::netMode, (float) NetMode::Serial);
                h.set (ids::netRepipe, 1.0f);
                h.set (ids::loopOn, 1.0f); h.set (ids::loopAmount, 1.0f); h.set (ids::loopSat, pol == 0 ? 0.0f : 1.0f);
                h.set (ids::loopSource, (float) src); h.set (ids::loopDest, (float) dst); h.set (ids::loopPolarity, (float) pol);
                h.set (ids::loopFilter, 12000.0f); h.set (ids::loopDelay, 100.0f);
                h.set (ids::wfOn, 1.0f); h.set (ids::wfFold, 1.0f);
                h.set (ids::resFeedback, 1.0f); h.set (ids::rbFeedback, 1.0f); h.set (ids::rcFeedback, 1.0f);
                for (int note : { 43, 55, 62 }) h.noteOn (note, 127);
                const auto s = h.render (2.0);
                const std::string name = "loop src=" + std::to_string (src) + " dst=" + std::to_string (dst) + " pol=" + std::to_string (pol);
                CHECK_MSG (s.finite, name + " finite");
                CHECK_MSG (s.peak < 1.3f, name + " bounded " + std::to_string (s.peak));
                for (int note : { 43, 55, 62 }) h.noteOff (note);
                CHECK (h.render (0.5).finite);
            }
}

AERIFORM_TEST (repipe_macro_transitions_from_single_to_network)
{
    TestHost h (48000.0, 256);
    dryFx (h);
    h.set (ids::netMode, (float) NetMode::Single);
    h.noteOn (60);
    h.render (0.2);
    std::vector<float> single;
    h.render (0.4, &single);
    CHECK (running (h, 0) && ! running (h, 1) && ! running (h, 2));
    h.set (ids::netRepipe, 1.0f);
    h.render (0.3);
    std::vector<float> piped;
    const auto s = h.render (0.4, &piped);
    CHECK (s.finite && s.peak < 1.3f);
    CHECK_MSG (running (h, 1) && running (h, 2), "repipe engages B and C");
    double diff = 0.0, ref = 0.0;
    for (size_t i = 0; i < std::min (single.size(), piped.size()); ++i) { diff += (double) (single[i] - piped[i]) * (single[i] - piped[i]); ref += (double) single[i] * single[i]; }
    CHECK_MSG (diff > 0.2 * ref, "repipe changes the sound meaningfully");
    // sweeping repipe while playing must stay finite and bounded
    bool finite = true; float peak = 0.0f;
    for (int b = 0; b < 300; ++b)
    {
        h.set (ids::netRepipe, 0.5f + 0.5f * std::sin ((float) b * 0.1f));
        const auto r = h.renderBlock();
        finite = finite && r.finite; peak = std::max (peak, r.peak);
    }
    CHECK (finite && peak < 1.3f);
}

AERIFORM_TEST (voice_stealing_with_complex_routing_is_safe)
{
    TestHost h (48000.0, 128);
    dryFx (h);
    enableAll (h);
    h.set (ids::voiceCount, 4.0f);
    h.set (ids::netMode, (float) NetMode::Serial);
    h.set (ids::netAB, 1.0f); h.set (ids::netBA, 1.0f); h.set (ids::netCA, 1.0f); h.set (ids::netFeedback, 1.0f);
    h.set (ids::loopOn, 1.0f); h.set (ids::loopAmount, 0.8f);
    h.set (ids::wfOn, 1.0f); h.set (ids::wfFold, 0.9f);
    h.set (ids::exbModel, (float) ExciterModel::Complex);
    h.set (ids::mixMode, (float) InteractionMode::FM); h.set (ids::mixB2A, 1.0f);
    bool finite = true; float peak = 0.0f; int maxActive = 0;
    for (int b = 0; b < 400; ++b)
    {
        if (b % 5 == 0) h.noteOn (36 + (b * 7) % 60, 100);
        if (b % 9 == 0) h.noteOff (36 + ((b - 20) * 7) % 60);
        const auto s = h.renderBlock();
        finite = finite && s.finite; peak = std::max (peak, s.peak);
        maxActive = std::max (maxActive, h.activeVoices());
    }
    CHECK (finite);
    CHECK_MSG (peak < 1.3f, "peak " + std::to_string (peak));
    CHECK_MSG (maxActive <= 4, "voice limit respected: " + std::to_string (maxActive));
}

AERIFORM_TEST (modal_resonators_are_tuned_and_distinct)
{
    for (int type : { (int) ResMode::ModalBank, (int) ResMode::MetallicBar, (int) ResMode::Membrane })
    {
        TestHost h (48000.0, 256);
        dryFx (h);
        h.set (ids::exaModel, (float) ExciterModel::Mallet);
        h.set (ids::resMode, (float) type);
        h.set (ids::resDamping, 1.0f);       // high modes die quickly -> fundamental dominates
        h.set (ids::resBrightness, 0.0f);
        h.set (ids::resBodyMix, 0.0f);
        h.set (ids::envRelease, 2000.0f);
        h.noteOn (60);
        h.render (0.05);
        std::vector<float> mono;
        const auto s = h.render (0.4, &mono);
        const double f = estimatePeakFrequency (mono, 48000.0, 261.63);   // spectral peak: the bar / membrane partials are inharmonic
        const std::string name = choices::resModes()[type].toStdString();
        CHECK_MSG (s.finite && s.rms > 1.0e-4, name + " rings: " + std::to_string (s.rms));
        CHECK_MSG (std::fabs (centsBetween (f, 261.63)) < 8.0, name + " fundamental tuned: " + std::to_string (f) + " Hz");
    }
    // the model families must sound different: compare spectra via RMS of the difference
    auto renderType = [] (int type)
    {
        TestHost h (48000.0, 256);
        dryFx (h);
        h.set (ids::exaModel, (float) ExciterModel::Mallet);
        h.set (ids::resMode, (float) type);
        h.noteOn (60);
        std::vector<float> mono;
        h.render (0.4, &mono);
        return mono;
    };
    const auto a = renderType ((int) ResMode::ModalBank), b = renderType ((int) ResMode::MetallicBar), c = renderType ((int) ResMode::FormantBody);
    double dab = 0.0, dac = 0.0, ref = 0.0;
    for (size_t i = 0; i < a.size(); ++i) { dab += (double) (a[i] - b[i]) * (a[i] - b[i]); dac += (double) (a[i] - c[i]) * (a[i] - c[i]); ref += (double) a[i] * a[i]; }
    CHECK_MSG (dab > 0.3 * ref && dac > 0.3 * ref, "modal bank, metallic bar and formant body differ");
}

AERIFORM_TEST (all_resonator_types_are_finite_in_every_slot_at_extremes)
{
    for (int type = 0; type < (int) ResMode::Count; ++type)
    {
        TestHost h (96000.0, 64);
        dryFx (h);
        enableAll (h);
        h.set (ids::netMode, (float) NetMode::Hybrid);
        h.set (ids::resMode, (float) type); h.set (ids::rbType, (float) type); h.set (ids::rcType, (float) type);
        for (auto* id : { ids::resFeedback, ids::rbFeedback, ids::rcFeedback, ids::resSaturation, ids::rbSaturation, ids::rcSaturation,
                          ids::resDispersion, ids::rbDispersion, ids::rcDispersion, ids::resInharm, ids::rbInharm, ids::rcInharm,
                          ids::excReed, ids::rbReed, ids::rcReed, ids::excPressure, ids::netAB, ids::netBA, ids::netCA, ids::netFeedback })
            h.set (id, 1.0f);
        for (auto* id : { ids::resDamping, ids::rbDamping, ids::rcDamping }) h.set (id, 0.0f);
        for (int note : { 0, 60, 127 }) h.noteOn (note, 127);
        const auto s = h.render (1.5);
        const std::string name = choices::resModes()[type].toStdString();
        CHECK_MSG (s.finite, name + " finite");
        CHECK_MSG (s.peak < 1.3f, name + " bounded " + std::to_string (s.peak));
    }
}

