#include "TestFramework.h"
#include "TestHelpers.h"
#include "DSP/Wavefolder.h"

using namespace aeriform;
using namespace aeriform::test;

namespace
{
    // exciter-only patch: resonator network bypassed (dry mix), effects off
    void exciterOnly (TestHost& h)
    {
        h.set (ids::reverbMix, 0.0f); h.set (ids::delayMix, 0.0f); h.set (ids::chorusMix, 0.0f);
        h.set (ids::netMix, 0.0f);
        h.set (ids::excReleaseNoise, 0.0f); h.set (ids::excAttackClick, 0.0f); h.set (ids::excPluck, 0.0f);
        h.set (ids::envAttack, 1.0f); h.set (ids::envRelease, 30.0f);
        h.set (ids::artInstability, 0.0f); h.set (ids::artVariation, 0.0f); h.set (ids::artFlowPitch, 0.0f);
        h.set (ids::exaDrift, 0.0f); h.set (ids::exaVariation, 0.0f);
        h.set (ids::exbDrift, 0.0f); h.set (ids::exbVariation, 0.0f);
    }

    double goertzelMagnitude (const std::vector<float>& x, double sampleRate, double hz)
    {
        const double w = 2.0 * 3.14159265358979 * hz / sampleRate;
        const double c = 2.0 * std::cos (w);
        double s0 = 0.0, s1 = 0.0, s2 = 0.0;
        for (float v : x) { s0 = (double) v + c * s1 - s2; s2 = s1; s1 = s0; }
        return std::sqrt (std::max (0.0, s1 * s1 + s2 * s2 - c * s1 * s2)) / (double) x.size();
    }
}

AERIFORM_TEST (every_exciter_model_produces_finite_bounded_output)
{
    for (int slot = 0; slot < 2; ++slot)
        for (int model = 0; model < (int) ExciterModel::Count; ++model)
        {
            TestHost h (48000.0, 256);
            exciterOnly (h);
            h.set (slot == 0 ? ids::exaModel : ids::exbModel, (float) model);
            h.set (slot == 0 ? ids::exbModel : ids::exaModel, (float) ExciterModel::Off);
            h.set (ids::exaPhSpeed, 0.9f); h.set (ids::exbPhSpeed, 0.9f);
            h.noteOn (60, 110);
            const auto s = h.render (0.4);
            h.noteOff (60);
            const auto tail = h.render (0.3);
            const std::string name = choices::exciterModels()[model].toStdString() + (slot == 0 ? " (A)" : " (B)");
            CHECK_MSG (s.finite && tail.finite, name + " finite");
            CHECK_MSG (s.peak < 1.3f, name + " bounded, peak " + std::to_string (s.peak));
            const auto m = (ExciterModel) model;
            if (m != ExciterModel::Off && m != ExciterModel::Sidechain)
                CHECK_MSG (s.rms > 1.0e-5, name + " produces sound, rms " + std::to_string (s.rms));
            else
                CHECK_MSG (s.rms < 1.0e-5, name + " silent, rms " + std::to_string (s.rms));
        }
}

AERIFORM_TEST (seeded_noise_is_deterministic)
{
    auto renderNoise = [] (int seed, int model)
    {
        TestHost h (48000.0, 128);
        exciterOnly (h);
        h.set (ids::exaModel, (float) model);
        h.set (ids::exaNzSeed, (float) seed);
        h.set (ids::exaNzCorrelation, 0.0f);
        h.noteOn (57, 100);
        std::vector<float> mono;
        h.render (0.25, &mono);
        return mono;
    };
    for (int model : { (int) ExciterModel::NoiseWhite, (int) ExciterModel::NoiseCrackle, (int) ExciterModel::NoiseAerosol, (int) ExciterModel::NoiseMetallic })
    {
        const auto a = renderNoise (7, model), b = renderNoise (7, model), c = renderNoise (8, model);
        double maxDiff = 0.0, diffOther = 0.0;
        for (size_t i = 0; i < a.size(); ++i)
        {
            maxDiff = std::max (maxDiff, (double) std::fabs (a[i] - b[i]));
            diffOther = std::max (diffOther, (double) std::fabs (a[i] - c[i]));
        }
        CHECK_MSG (maxDiff < 1.0e-6, "same seed -> identical output (model " + std::to_string (model) + ", diff " + std::to_string (maxDiff) + ")");
        CHECK_MSG (diffOther > 1.0e-3, "different seed -> different output (model " + std::to_string (model) + ")");
    }
}

AERIFORM_TEST (wave_oscillator_is_in_tune)
{
    for (double sr : { 44100.0, 96000.0 })
        for (float shape : { 0.0f, 0.5f, 1.0f })
            for (int note : { 36, 60, 84 })
            {
                TestHost h (sr, 256);
                exciterOnly (h);
                h.set (ids::exaModel, (float) ExciterModel::Wave);
                h.set (ids::exaWaveShape, shape);
                h.set (ids::excLowpass, 20000.0f);
                h.noteOn (note, 100);
                h.render (0.05);
                std::vector<float> mono;
                h.render (0.3, &mono);
                const double expected = dsp::midiNoteToHz ((float) note);
                const double f = estimateFrequency (mono, sr, expected);
                CHECK_MSG (std::fabs (centsBetween (f, expected)) < 3.0, "wave sr=" + std::to_string ((int) sr) + " shape=" + std::to_string (shape)
                                                                        + " note=" + std::to_string (note) + " -> " + std::to_string (f) + " Hz");
            }
}

AERIFORM_TEST (wave_oscillator_is_band_limited)
{
    // A saw at 3 kHz: its 9th harmonic (27 kHz) would alias to 17.1 kHz at 44.1 kHz. Compare that alias
    // bin with the fundamental: a naive saw shows ~ -19 dB, the band-limited one must be far lower.
    TestHost h (44100.0, 256);
    exciterOnly (h);
    h.set (ids::exaModel, (float) ExciterModel::Wave);
    h.set (ids::exaWaveShape, 2.0f / 3.0f);            // saw
    h.set (ids::exaKeytrack, 0.0f);                      // fixed pitch: middle C ...
    h.set (ids::exaCoarse, 24.0f);                       // ... + 24 st = 1046.5 Hz
    h.set (ids::exaFine, 0.0f);
    h.set (ids::excLowpass, 20000.0f);
    // 1046.5 Hz * 2^(fine) -> use coarse only; harmonics at multiples of 1046.5: the 25th (26162 Hz) aliases to 17938 Hz
    h.noteOn (60, 100);
    h.render (0.05);
    std::vector<float> mono;
    h.render (0.5, &mono);
    const double f0 = 1046.5;
    const double fundamental = goertzelMagnitude (mono, 44100.0, f0);
    const double alias = goertzelMagnitude (mono, 44100.0, 44100.0 - 25.0 * f0);
    const double ratioDb = 20.0 * std::log10 (std::max (alias, 1.0e-12) / std::max (fundamental, 1.0e-12));
    std::printf ("      saw alias (25th harmonic folded): %.1f dB below fundamental\n", -ratioDb);
    CHECK_MSG (ratioDb < -40.0, "alias suppression " + std::to_string (ratioDb) + " dB");
}

AERIFORM_TEST (interaction_modes_are_audible_and_stable)
{
    std::vector<double> rmsByMode;
    for (int mode = 0; mode < (int) InteractionMode::Count; ++mode)
    {
        TestHost h (48000.0, 256);
        exciterOnly (h);
        h.set (ids::exaModel, (float) ExciterModel::Wave);
        h.set (ids::exbModel, (float) ExciterModel::Wave);
        h.set (ids::exaWaveShape, 0.66f);
        h.set (ids::exbWaveShape, 0.0f);
        h.set (ids::exbCoarse, 7.0f);
        h.set (ids::mixMode, (float) mode);
        h.set (ids::mixInteraction, 0.7f);
        h.set (ids::mixDepth, 1.0f);
        h.set (ids::mixB2A, 0.8f);
        h.set (ids::mixA2B, 0.5f);
        h.noteOn (52, 100);
        std::vector<float> mono;
        const auto s = h.render (0.5, &mono);
        const std::string name = choices::interactionModes()[mode].toStdString();
        CHECK_MSG (s.finite, name + " finite");
        CHECK_MSG (s.peak < 1.3f, name + " bounded " + std::to_string (s.peak));
        CHECK_MSG (s.rms > 1.0e-3, name + " audible " + std::to_string (s.rms));
        // DC after the block: mean of the last 100 ms must stay small
        double mean = 0.0; const size_t tail = std::min<size_t> (mono.size(), 4800);
        for (size_t i = mono.size() - tail; i < mono.size(); ++i) mean += mono[i];
        mean /= (double) tail;
        CHECK_MSG (std::fabs (mean) < 0.05, name + " no DC (" + std::to_string (mean) + ")");
        rmsByMode.push_back (s.rms);
    }
    // level normalisation keeps the modes within a sane loudness window of each other
    const double lo = *std::min_element (rmsByMode.begin(), rmsByMode.end());
    const double hi = *std::max_element (rmsByMode.begin(), rmsByMode.end());
    CHECK_MSG (hi / lo < 12.0, "interaction modes within ~21 dB of each other: " + std::to_string (20.0 * std::log10 (hi / lo)) + " dB");
}

AERIFORM_TEST (physical_exciters_respond_to_speed_and_pressure)
{
    for (int model : { (int) ExciterModel::Reed, (int) ExciterModel::Lip, (int) ExciterModel::Bow, (int) ExciterModel::Jet })
    {
        TestHost blown (48000.0, 256), silent (48000.0, 256);
        for (auto* h : { &blown, &silent })
        {
            exciterOnly (*h);
            h->set (ids::exaModel, (float) model);
            h->set (ids::envAttack, 20.0f);
        }
        blown.set (ids::exaPhSpeed, 0.9f);
        silent.set (ids::exaPhSpeed, 0.0f);
        blown.noteOn (55, 110); silent.noteOn (55, 110);
        blown.render (0.3); silent.render (0.3);
        const auto a = blown.render (0.3), b = silent.render (0.3);
        const std::string name = choices::exciterModels()[model].toStdString();
        CHECK_MSG (a.finite && b.finite, name + " finite");
        CHECK_MSG (a.rms > 5.0f * b.rms + 1.0e-4, name + " speaks with speed: blown " + std::to_string (a.rms) + " vs unblown " + std::to_string (b.rms));
    }
}

AERIFORM_TEST (sidechain_exciter_model_passes_input_through_the_chain)
{
    TestHost h (48000.0, 256);
    h.prepare (48000.0, 256, false);
    h.enableSidechain();
    exciterOnly (h);
    h.set (ids::exaModel, (float) ExciterModel::Sidechain);
    h.set (ids::excLowpass, 20000.0f);
    h.set (ids::preEnv, 0.0f);
    const double hz = 330.0;
    long counter = 0;
    h.sidechainSource = [&] (long i) { juce::ignoreUnused (i); return 0.5f * std::sin (2.0f * 3.14159265f * (float) hz * (float) (counter++) / 48000.0f); };
    h.noteOn (60, 100);
    h.render (0.1);
    std::vector<float> mono;
    const auto s = h.render (0.3, &mono);
    CHECK (s.finite);
    CHECK_MSG (s.rms > 1.0e-2, "sidechain model audible: " + std::to_string (s.rms));
    const double f = estimateFrequency (mono, 48000.0, hz);
    CHECK_MSG (std::fabs (centsBetween (f, hz)) < 10.0, "sidechain passes the input frequency: " + std::to_string (f));
    h.noteOff (60);
    h.render (0.3);
    const auto gated = h.render (0.2);
    CHECK_MSG (gated.rms < 1.0e-4, "note-gated: " + std::to_string (gated.rms));
}

AERIFORM_TEST (exciter_modulation_destinations_reach_the_dsp)
{
    TestHost h (48000.0, 256);
    exciterOnly (h);
    h.set (ids::exaModel, (float) ExciterModel::Wave);
    h.set (ids::modParam (9, ids::modSrcSuffix), (float) ModSource::ModWheel);
    h.set (ids::modParam (9, ids::modDstSuffix), (float) ModDest::ExALevel);
    h.set (ids::modParam (9, ids::modDepthSuffix), -1.0f);
    h.noteOn (60, 100);
    const auto loud = h.render (0.3);
    h.cc (1, 127);
    h.render (0.1);
    const auto quiet = h.render (0.3);
    CHECK_MSG (quiet.rms < loud.rms * 0.15, "matrix slot 9 -> Ex A Level: " + std::to_string (loud.rms) + " -> " + std::to_string (quiet.rms));
    h.cc (1, 0);
    h.set (ids::modParam (9, ids::modDstSuffix), (float) ModDest::ExAPitch);
    h.set (ids::modParam (9, ids::modDepthSuffix), 0.5f);   // +12 semitones at full
    h.render (0.2);
    std::vector<float> base;
    h.render (0.3, &base);
    h.cc (1, 127);
    h.render (0.2);
    std::vector<float> up;
    h.render (0.3, &up);
    const double f1 = estimateFrequency (base, 48000.0, 261.63), f2 = estimateFrequency (up, 48000.0, 523.25);
    CHECK_MSG (std::fabs (centsBetween (f2, f1 * 2.0)) < 20.0, "Ex A Pitch +12 st: " + std::to_string (f1) + " -> " + std::to_string (f2));
}
