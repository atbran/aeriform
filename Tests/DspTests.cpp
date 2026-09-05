#include "TestFramework.h"
#include "TestHelpers.h"
#include "DSP/DspUtils.h"
#include "DSP/FractionalDelay.h"
#include "DSP/Envelope.h"
#include "DSP/Resonator.h"
#include "DSP/Exciter.h"
#include "DSP/Voice.h"
#include "DSP/Effects/Reverb.h"
#include "DSP/Effects/Delay.h"
#include "DSP/Effects/Chorus.h"
#include "DSP/Effects/OutputStage.h"

using namespace aeriform;
using namespace aeriform::dsp;
using namespace aeriform::test;

AERIFORM_TEST (midi_note_to_frequency)
{
    CHECK_NEAR (midiNoteToHz (69.0f), 440.0, 1.0e-3);
    CHECK_NEAR (midiNoteToHz (60.0f), 261.6256, 1.0e-2);
    CHECK_NEAR (midiNoteToHz (81.0f), 880.0, 1.0e-2);
    CHECK_NEAR (midiNoteToHz (57.0f), 220.0, 1.0e-2);
    CHECK_NEAR (centsToRatio (1200.0f), 2.0, 1.0e-5);
}

AERIFORM_TEST (fast_tanh_is_bounded_and_accurate)
{
    for (float x = -20.0f; x <= 20.0f; x += 0.05f)
    {
        const float y = fastTanh (x);
        CHECK (y >= -1.0f && y <= 1.0f);
        if (std::fabs (x) < 3.0f) CHECK_NEAR (y, std::tanh (x), 3.0e-3);
    }
    CHECK (sanitize (std::numeric_limits<float>::quiet_NaN()) == 0.0f);
    CHECK (sanitize (std::numeric_limits<float>::infinity()) == 0.0f);
}

AERIFORM_TEST (fractional_delay_interpolates_sine_accurately)
{
    FractionalDelay d;
    d.prepare (1024);
    const float sr = 48000.0f, f = 1000.0f, delay = 37.37f;
    double maxErr = 0.0;
    for (int n = 0; n < 2000; ++n)
    {
        const float x = std::sin (kTwoPi * f * (float) n / sr);
        d.push (x);
        if (n > 100)
        {
            // sample written "delay" samples ago corresponds to time index (n - delay) at write time...
            // after push, the latest sample has delay 1 -> delay D corresponds to index n + 1 - D
            const float expected = std::sin (kTwoPi * f * ((float) n + 1.0f - delay) / sr);
            maxErr = std::max (maxErr, (double) std::fabs (d.readLagrange (delay) - expected));
        }
    }
    CHECK_MSG (maxErr < 2.0e-3, "max interpolation error " + std::to_string (maxErr));
}

AERIFORM_TEST (adsr_reaches_full_level_and_returns_to_silence)
{
    ADSR env;
    env.setSampleRate (48000.0f);
    env.setTimes (10.0f, 50.0f, 0.5f, 30.0f);
    env.noteOn();
    float peak = 0.0f;
    for (int i = 0; i < 480 * 3; ++i) peak = std::max (peak, env.next());
    CHECK_NEAR (peak, 1.0, 1.0e-3);
    for (int i = 0; i < 48000 / 4; ++i) env.next();
    CHECK_NEAR (env.getLevel(), 0.5, 2.0e-3);
    env.noteOff();
    for (int i = 0; i < 48000 / 4; ++i) env.next();
    CHECK (! env.isActive());
    CHECK (env.getLevel() == 0.0f);
}

namespace
{
    double measureResonatorPitch (float sampleRate, int midiNote, ResMode mode, float dispersion, float damping)
    {
        Resonator r;
        r.prepare (sampleRate);
        ResonatorParams p;
        p.freqHz = midiNoteToHz ((float) midiNote);
        p.feedback = 0.97f;
        p.damping = damping;
        p.brightness = 0.5f;
        p.dispersion = dispersion;
        p.shape = 0.5f;
        p.reflection = 0.3f;
        p.saturation = 0.1f;
        p.type = mode;
        r.update (p, true);

        const int n = (int) (sampleRate * 0.5f);
        const int start = std::max ((int) (sampleRate * 0.02f), (int) (3.0f * sampleRate / p.freqHz));
        std::vector<float> out;
        out.reserve ((size_t) n);
        int firstBad = -1;
        for (int i = 0; i < n; ++i)
        {
            const float ex = (i < 8) ? 0.5f : 0.0f;   // short impulse
            float tap2 = 0.0f;
            const float y = r.next (ex, 0.0f, tap2);
            if (! std::isfinite (y) && firstBad < 0) firstBad = i;
            if (i > start) out.push_back (y);
        }
        const double measured = estimateFrequency (out, sampleRate, p.freqHz);
        std::printf ("      sr=%.0f note=%d mode=%d disp=%.2f expected=%.2f measured=%.2f (%+.1f cents)%s\n", sampleRate, midiNote, (int) mode,
                     dispersion, p.freqHz, measured, centsBetween (measured, p.freqHz), firstBad >= 0 ? "  NON-FINITE!" : "");
        CHECK_MSG (firstBad < 0, "non-finite resonator output at sample " + std::to_string (firstBad));
        return measured;
    }
}

AERIFORM_TEST (resonator_tuning_is_accurate_across_range_and_sample_rates)
{
    for (float sr : { 44100.0f, 48000.0f, 96000.0f })
        for (int note : { 36, 48, 60, 72, 84, 96 })
        {
            const double expected = midiNoteToHz ((float) note);
            const double measured = measureResonatorPitch (sr, note, ResMode::OpenPipe, 0.0f, 0.4f);
            const double cents = centsBetween (measured, expected);
            // The fundamental is compensated exactly; the autocorrelation estimate is biased slightly flat at
            // the bottom of the range by the residual partial stretch of the in-loop filters (< 6 cents at C2).
            const double tolerance = note <= 48 ? 6.0 : 4.0;
            CHECK_MSG (std::fabs (cents) < tolerance, "open pipe sr=" + std::to_string ((int) sr) + " note=" + std::to_string (note)
                                                  + " err=" + std::to_string (cents) + " cents");
        }
    // closed pipe (half-length, inverted feedback) must still land on the note
    for (int note : { 40, 60, 80 })
    {
        const double expected = midiNoteToHz ((float) note);
        const double measured = measureResonatorPitch (48000.0f, note, ResMode::ClosedPipe, 0.0f, 0.4f);
        CHECK_MSG (std::fabs (centsBetween (measured, expected)) < (note <= 48 ? 10.0 : 5.0), "closed pipe note=" + std::to_string (note));
    }
    // with dispersion the fundamental is still tuned (partials spread, fundamental compensated)
    for (int note : { 48, 60 })
    {
        const double expected = midiNoteToHz ((float) note);
        const double measured = measureResonatorPitch (48000.0f, note, ResMode::String, 0.3f, 0.5f);
        CHECK_MSG (std::fabs (centsBetween (measured, expected)) < 12.0, "string+dispersion note=" + std::to_string (note));
    }
}

AERIFORM_TEST (resonator_stays_finite_and_bounded_under_extreme_settings)
{
    for (float sr : { 44100.0f, 96000.0f })
        for (int mode = 0; mode < (int) ResMode::ModalBank; ++mode)   // waveguide family
        {
            Resonator r;
            r.prepare (sr);
            ResonatorParams p;
            p.freqHz = midiNoteToHz (110.0f);
            p.feedback = 1.0f; p.damping = 0.0f; p.brightness = 1.0f; p.dispersion = 1.0f; p.shape = 1.0f;
            p.reflection = 0.0f; p.saturation = 1.0f; p.type = (ResMode) mode; p.reed = 1.0f; p.pressure = 1.0f; p.pickup = 1.0f;
            r.update (p, true);
            Noise rng; rng.seed (77);
            float peak = 0.0f;
            bool finite = true;
            for (int i = 0; i < (int) (sr * 2.0f); ++i)
            {
                if (i % 4000 == 0) { p.freqHz = midiNoteToHz (20.0f + (float) (i % 100)); r.update (p, false); }
                float tap2 = 0.0f;
                const float y = r.next (rng.next() * 2.0f, 1.0f, tap2);
                if (! std::isfinite (y) || ! std::isfinite (tap2)) { finite = false; break; }
                peak = std::max (peak, std::fabs (y));
            }
            CHECK (finite);
            CHECK_MSG (peak < 12.0f, "peak " + std::to_string (peak));
        }
}

AERIFORM_TEST (voice_is_silent_after_release)
{
    Voice v;
    v.prepare (48000.0, 0);
    VoiceParams p = defaultVoiceParams();
    p.v[(size_t) P::envRelease] = 100.0f;
    p.v[(size_t) P::excReleaseNoise] = 0.0f;
    v.startNote (60, 0.9f, 60.0f, false, 0, 1, 1, p);
    ModSources gs {};
    std::vector<float> l (256), r (256);
    for (int b = 0; b < 40; ++b) { std::fill (l.begin(), l.end(), 0.0f); std::fill (r.begin(), r.end(), 0.0f); v.render (l.data(), r.data(), 256, p, gs, nullptr, nullptr, 0.0f); }
    CHECK (v.isActive());
    v.stopNote (p);
    for (int b = 0; b < 400 && v.isActive(); ++b) { std::fill (l.begin(), l.end(), 0.0f); std::fill (r.begin(), r.end(), 0.0f); v.render (l.data(), r.data(), 256, p, gs, nullptr, nullptr, 0.0f); }
    CHECK (! v.isActive());
    std::fill (l.begin(), l.end(), 0.0f); std::fill (r.begin(), r.end(), 0.0f);
    v.render (l.data(), r.data(), 256, p, gs, nullptr, nullptr, 0.0f);
    float peak = 0.0f;
    for (float s : l) peak = std::max (peak, std::fabs (s));
    CHECK (peak == 0.0f);
}

AERIFORM_TEST (effects_remain_finite_with_extreme_parameters)
{
    const double sr = 48000.0;
    FdnReverb reverb; reverb.prepare (sr); reverb.setParams (1.0f, 1.0f, 1.0f, 0.0f, 200.0f, 1.0f, 1.0f);
    StereoDelay delay; delay.prepare (sr); delay.setParams (1.0f, 2000.0f, 0.95f, 20000.0f, true);
    Chorus chorus; chorus.prepare (sr); chorus.setParams (1.0f, 5.0f, 1.0f, 1.0f);
    OutputStage out; out.prepare (sr); out.setParams (12.0f, 10.0f, true);
    Noise rng; rng.seed (5);
    std::vector<float> l (512), r (512);
    float peak = 0.0f; bool finite = true;
    for (int b = 0; b < 400; ++b)
    {
        for (int i = 0; i < 512; ++i) { l[(size_t) i] = rng.next() * 4.0f; r[(size_t) i] = rng.next() * 4.0f; }
        chorus.process (l.data(), r.data(), 512);
        delay.process (l.data(), r.data(), 512);
        reverb.process (l.data(), r.data(), 512);
        out.process (l.data(), r.data(), 512);
        for (int i = 0; i < 512; ++i)
        {
            if (! std::isfinite (l[(size_t) i]) || ! std::isfinite (r[(size_t) i])) finite = false;
            peak = std::max (peak, std::max (std::fabs (l[(size_t) i]), std::fabs (r[(size_t) i])));
        }
    }
    CHECK (finite);
    CHECK_MSG (peak <= 1.2f, "limited peak " + std::to_string (peak));
}

