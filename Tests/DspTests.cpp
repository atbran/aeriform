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
    bool expectsNominalPitch (const NonlinearParams& p)
    { return !p.on || p.model != NonlinearModel::Tension || p.amount <= 0.0f || p.position == NonlinearPosition::Pickup; }
    struct PitchMeasurement { double hz; bool nominal; };
    PitchMeasurement measureResonatorPitch (float sampleRate, int midiNote, ResMode mode, float dispersion, float damping, NonlinearParams nonlinear = {})
    {
        ResonatorSlot r;
        r.prepare (sampleRate);
        ResonatorParams p;
        p.nonlinear = nonlinear;
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
        return {measured, expectsNominalPitch (p.nonlinear)};
    }
}

AERIFORM_TEST (resonator_tuning_is_accurate_across_range_and_sample_rates)
{
    for (float sr : { 44100.0f, 48000.0f, 96000.0f })
        for (int note : { 36, 48, 60, 72, 84, 96 })
        {
            const double expected = midiNoteToHz ((float) note);
            const auto measured = measureResonatorPitch (sr, note, ResMode::OpenPipe, 0.0f, 0.4f);
            const double cents = centsBetween (measured.hz, expected);
            // The fundamental is compensated exactly; the autocorrelation estimate is biased slightly flat at
            // the bottom of the range by the residual partial stretch of the in-loop filters (< 6 cents at C2).
            const double tolerance = note <= 48 ? 6.0 : 4.0;
            CHECK_MSG (!measured.nominal || std::fabs (cents) < tolerance, "open pipe sr=" + std::to_string ((int) sr) + " note=" + std::to_string (note)
                                                  + " err=" + std::to_string (cents) + " cents");
        }
    // Active Tension explicitly exempts nominal tuning above and below; no
    // tolerances are widened. Its bounded pitch envelope has its own checks.
    // closed pipe (half-length, inverted feedback) must still land on the note
    for (int note : { 40, 60, 80 })
    {
        const double expected = midiNoteToHz ((float) note);
        const auto measured = measureResonatorPitch (48000.0f, note, ResMode::ClosedPipe, 0.0f, 0.4f);
        CHECK_MSG (!measured.nominal || std::fabs (centsBetween (measured.hz, expected)) < (note <= 48 ? 10.0 : 5.0), "closed pipe note=" + std::to_string (note));
    }
    // with dispersion the fundamental is still tuned (partials spread, fundamental compensated)
    for (int note : { 48, 60 })
    {
        const double expected = midiNoteToHz ((float) note);
        const auto measured = measureResonatorPitch (48000.0f, note, ResMode::String, 0.3f, 0.5f);
        CHECK_MSG (!measured.nominal || std::fabs (centsBetween (measured.hz, expected)) < 12.0, "string+dispersion note=" + std::to_string (note));
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


// Cursory scaffold check only; the separate acceptance agent owns golden audio.
AERIFORM_TEST (nonlinear_bypass_identity_and_probe)
{
    for (int model = 0; model < (int) ResMode::Count; ++model)
        for (int position = 0; position < (int) NonlinearPosition::Count; ++position)
        {
            ResonatorSlot off, on;
            off.prepare (48000.0f); on.prepare (48000.0f);
            ResonatorParams params;
            params.type = (ResMode) model;
            off.update (params, true);
            params.nonlinear.on = true;
            params.nonlinear.position = (NonlinearPosition) position;
            params.nonlinear.model = (NonlinearModel) (model % (int) NonlinearModel::Count);
            params.nonlinear.amount = 0.0f; // all models must bypass at zero amount
            params.nonlinear.drive = 100.0f;
            params.nonlinear.bias = -100.0f;
            on.update (params, true);
            for (int sample = 0; sample < 256; ++sample)
            {
                const float input = sample < 16 ? 0.5f : 0.0f;
                float tapOff, tapOn;
                const float a = off.next (input, 0.0f, tapOff);
                const float b = on.next (input, 0.0f, tapOn);
                CHECK (a == b && tapOff == tapOn);
            }
            CHECK (std::isfinite (on.getLoopEnergyRms()));
            CHECK (on.getLoopEnergyRms() > 0.0f);
            on.reset();
            CHECK (on.getLoopEnergyRms() == 0.0f);
        }
}

AERIFORM_TEST (nonlinear_phase1_saturation_and_transitions)
{
    NonlinearElement element;
    element.prepare (48000.0, 64);
    NonlinearParams p; p.on = true; p.adaa = false;
    p.position = NonlinearPosition::Pickup; p.drive = 80.0f; p.amount = 100.0f;
    element.configure (p, true);
    CHECK (std::abs (element.processSample (0.5f, 0.0f)) < 0.1f);
    // Static biased curve is 1-Lipschitz; offset removal preserves contraction.
    double excess = 0.0;
    for (int i = -400; i <= 400; ++i)
    {
        const double x = i * 0.01;
        const double y = (NonlinearElement::curve (32.0*(x+0.02))-NonlinearElement::curve (32.0*0.02))/32.0;
        excess = std::max (excess, std::abs (y)-std::abs (x));
    }
    CHECK (excess < 1.0e-12);
    p.adaa = true; p.position = NonlinearPosition::Post;
    element.configure (p, true);
    CHECK (std::isfinite (element.processSample (0.125f, 0.0f)));
    for (int i = 0; i < 1000; ++i) CHECK (std::isfinite (element.processSample (0.125f, 0.0f)));
    const float omega = kTwoPi*440.0f/48000.0f;
    const float withAdaa = element.loopPhaseDelay (omega);
    p.adaa = false; element.configure (p, true);
    CHECK_NEAR (withAdaa-element.loopPhaseDelay (omega), 0.5f, 1.0e-5);
    // All engine families remain finite through live position/on/ADAA changes.
    for (int type = 0; type < (int) ResMode::Count; ++type)
    {
        ResonatorSlot slot; slot.prepare (48000.0f);
        ResonatorParams r; r.type = (ResMode) type; r.feedback = 0.99f;
        r.nonlinear = p; r.nonlinear.bias = 100.0f;
        slot.update (r, true);
        for (int i = 0; i < 4096; ++i)
        {
            if (i % 128 == 0)
            {
                r.nonlinear.position = (NonlinearPosition) ((i/128)%3);
                r.nonlinear.on = (i/128)%4 != 0;
                r.nonlinear.adaa = (i/128)%2 != 0;
                slot.update (r, false);
            }
            float tap;
            const float y = slot.next (i < 512 ? 0.25f*std::sin (i*0.1f) : 0.0f, 0.0f, tap);
            CHECK (std::isfinite (y) && std::isfinite (tap) && std::abs (y) < 8.0f);
        }
    }
}

AERIFORM_TEST (nonlinear_phase2_hysteresis_peak_memory_and_lanes)
{
    NonlinearElement element;
    element.prepare (48000.0, 64);
    NonlinearParams p; p.on = true; p.model = NonlinearModel::Hysteresis;
    p.drive = 100.0f; p.amount = 100.0f; p.position = NonlinearPosition::Pickup;
    element.configure (p, true);
    const float first = element.processSample (1.0f, 0.0f);
    const float retained = element.processSample (0.0f, 0.0f);
    CHECK (first > 0.0f && first < 1.0f);
    CHECK (retained > 0.0f && retained < first); // Memory, not pointwise contraction.
    CHECK (element.processSample (0.0f, 0.0f, 1) == 0.0f); // Separate pickup history.
    CHECK (element.loopPhaseDelay (0.1f) == 0.0f); // Never borrows Saturate's compensation.
    element.reset();
    CHECK (element.processSample (0.0f, 0.0f) == 0.0f);
    float peak = 0.0f;
    for (int i = 0; i < 4096; ++i)
    {
        if (i%64 == 0)
        {
            p.drive = (float) ((i/64*17)%101);
            p.amount = 20.0f + (float) ((i/64*13)%81);
            element.configure (p);
        }
        const float x = 0.6f*std::sin (i*0.07f) + 0.25f*std::sin (i*0.017f);
        peak = std::max (peak, std::abs (x));
        const float y = element.processSample (x, 0.0f);
        CHECK (std::isfinite (y) && std::abs (y) <= peak+1.0e-6f);
    }
    // Hysteresis does not depend on the Saturate-only bias or ADAA controls.
    NonlinearElement a, b; a.prepare (48000.0, 64); b.prepare (48000.0, 64);
    p.amount = 100.0f; p.drive = 90.0f; p.bias = 0.0f; p.adaa = false;
    a.configure (p, true); p.bias = 100.0f; p.adaa = true; b.configure (p, true);
    for (int i = 0; i < 512; ++i)
    {
        const float x = 0.1f*std::sin (i*0.03f);
        CHECK (a.processSample (x, 0.0f) == b.processSample (x, 0.0f));
    }
}

AERIFORM_TEST (nonlinear_phase2_hysteresis_slot_smoke)
{
    for (int type = 0; type < (int) ResMode::Count; ++type)
        for (int position = 0; position < (int) NonlinearPosition::Count; ++position)
        {
            ResonatorSlot slot; slot.prepare (48000.0f);
            ResonatorParams p; p.type = (ResMode) type; p.feedback = 0.98f;
            p.nonlinear.on = true; p.nonlinear.model = NonlinearModel::Hysteresis;
            p.nonlinear.position = (NonlinearPosition) position;
            p.nonlinear.drive = 90.0f; p.nonlinear.amount = 80.0f;
            slot.update (p, true);
            double energy = 0.0;
            for (int i = 0; i < 2048; ++i)
            {
                if (i == 512) { p.nonlinear.model = NonlinearModel::Saturate; slot.update (p, false); }
                if (i == 1024) { p.nonlinear.model = NonlinearModel::Hysteresis; slot.update (p, false); }
                float tap;
                const float y = slot.next (i < 256 ? 0.3f*std::sin (i*0.1f) : 0.0f, 0.0f, tap);
                CHECK (std::isfinite (y) && std::isfinite (tap) && std::abs (y) < 8.0f);
                energy += static_cast<double> (y)*y;
            }
            CHECK (energy > 0.0);
        }
}

AERIFORM_TEST (nonlinear_phase3_tension_envelope_and_delay_slew)
{
    NonlinearElement element; element.prepare (48000.0, 64);
    NonlinearParams p; p.on = true; p.model = NonlinearModel::Tension; p.amount = 100.0f;
    element.configure (p, true);
    CHECK (!expectsNominalPitch (p));
    float previousRatio = 1.0f;
    for (float energy : {0.0f, 0.25f, 0.5f, 1.0f, 4.0f})
    {
        element.setLoopEnergyRms (energy);
        const float ratio = element.tensionRatio();
        CHECK (ratio >= previousRatio && ratio <= 1.030001f);
        previousRatio = ratio;
    }
    CHECK_NEAR (element.tensionRatio(), 1.03, 1.0e-6);
    p.position = NonlinearPosition::Pickup; element.configure (p, true);
    element.setLoopEnergyRms (1.0f);
    CHECK (expectsNominalPitch (p)); CHECK (element.delayScale() == 1.0f);
    p.position = NonlinearPosition::Post; element.configure (p, true);
    element.setLoopEnergyRms (1.0f);
    Resonator wave; wave.prepare (48000.0f); wave.setNonlinearElement (&element);
    ResonatorParams r; r.freqHz = 110.0f; wave.update (r, true);
    float previousLength = wave.getDelayLength();
    for (int i = 0; i < 2048; ++i)
    {
        float tap; wave.next (0.0f, 0.0f, tap);
        CHECK (std::abs (wave.getDelayLength()-previousLength) <= 0.0201f);
        previousLength = wave.getDelayLength();
    }
    CHECK_NEAR (wave.getTensionRatio(), 1.03f, 1.0e-5);
    element.setLoopEnergyRms (0.0f);
    for (int i = 0; i < 2048; ++i) { float tap; wave.next (0.0f, 0.0f, tap); }
    CHECK_NEAR (wave.getTensionRatio(), 1.0f, 1.0e-6);
    // Nominal pitch changes cannot carry an oversized offset from the old delay.
    element.setLoopEnergyRms (1.0f);
    for (int i = 0; i < 512; ++i) { float tap; wave.next (0.0f, 0.0f, tap); }
    r.freqHz = 4000.0f; wave.update (r, true);
    float tap; wave.next (0.0f, 0.0f, tap);
    CHECK (wave.getTensionRatio() <= 1.030001f);
}

AERIFORM_TEST (nonlinear_phase3_tension_modal_and_network_smoke)
{
    // PIPE (index 9) owns its in-loop saturator and does not host the nonlinear loop element.
    for (int type = 0; type < (int) ResMode::Pipe; ++type)
    {
        ResonatorSlot slot; slot.prepare (48000.0f);
        ResonatorParams p; p.type = (ResMode) type; p.freqHz = 220.0f; p.feedback = 0.5f;
        p.nonlinear.on = true; p.nonlinear.model = NonlinearModel::Tension; p.nonlinear.amount = 100.0f;
        slot.update (p, true);
        float peakRatio = 1.0f;
        for (int i = 0; i < 4096; ++i)
        {
            if (i%32 == 0) slot.update (p, false); // real control-rate coefficient updates
            float tap;
            const float y = slot.next (i < 512 ? 0.6f*std::sin (i*0.029f) : 0.0f, 0.0f, tap);
            CHECK (std::isfinite (y) && std::isfinite (tap));
            const float ratio = slot.getTensionRatio();
            CHECK (ratio >= 1.0f && ratio <= 1.030001f);
            peakRatio = std::max (peakRatio, ratio);
        }
        CHECK (peakRatio > 1.00001f);
        p.nonlinear.on = false;
        for (int i = 0; i < 2048; ++i)
        {
            if (i%32 == 0) slot.update (p, false);
            float tap; slot.next (0.0f, 0.0f, tap);
        }
        CHECK_NEAR (slot.getTensionRatio(), 1.0f, 1.0e-6);
    }
    ResonatorNetwork network; network.prepare (48000.0f);
    NetworkParams p; p.repipe = 1.0f; p.loopOn = true;
    for (auto& slot : p.res) { slot.nonlinear.on = true; slot.nonlinear.model = NonlinearModel::Tension; }
    network.update (p, true);
    for (int i = 0; i < 2048; ++i)
    {
        if (i%32 == 0) network.update (p, false);
        float l,r; network.next (i < 64 ? 0.2f : 0.0f, 0.0f, 0.0f, l,r);
        CHECK (network.isFinite() && std::isfinite (l) && std::isfinite (r));
    }
}


AERIFORM_TEST (nonlinear_phase4_friction_budget_and_silence)
{
    NonlinearElement element; element.prepare (48000.0, 256);
    NonlinearParams p; p.on = true; p.model = NonlinearModel::Friction;
    p.drive = p.amount = 100.0f; element.configure (p, true);
    bool injects = false, changes = false;
    for (int i = 0; i < 4096; ++i)
    {
        const float x = 0.08f*std::sin (i*0.031f);
        const float y = element.processSample (x, 0.08f);
        CHECK (std::isfinite (y));
        CHECK (double(y)*y <= 1.020001*double(x)*x);
        injects = injects || std::abs (y) > std::abs (x)+1.0e-7f;
        changes = changes || std::abs (y-x) > 1.0e-5f;
        if (i%32 == 31)
        {
            const auto energy = element.getFrictionEnergy();
            CHECK (energy.output <= 1.020001*energy.input);
        }
    }
    CHECK (injects && changes);
    for (int i = 0; i < 64; ++i) CHECK (element.processSample (0.0f, 1.0f) == 0.0f);
    element.reset();
    CHECK (element.getFrictionScale() == 1.0f);
    element.blockBoundary (1.0, 4.0);
    for (int i = 0; i < 32; ++i) element.beginSample();
    CHECK_NEAR (element.getFrictionScale(), 0.49f, 1.0e-5);
    const float reduced = element.getFrictionScale();
    element.blockBoundary (1.0, 1.0);
    for (int i = 0; i < 32; ++i) element.beginSample();
    CHECK (element.getFrictionScale() > reduced && element.getFrictionScale() < reduced+0.01f);
}

AERIFORM_TEST (nonlinear_phase4_friction_slot_smoke)
{
    for (int type = 0; type < (int) ResMode::Count; ++type)
        for (int position = 0; position < (int) NonlinearPosition::Count; ++position)
        {
            ResonatorSlot slot; slot.prepare (48000.0f);
            ResonatorParams p; p.type = (ResMode) type; p.feedback = 0.98f;
            p.nonlinear.on = true; p.nonlinear.model = NonlinearModel::Friction;
            p.nonlinear.position = (NonlinearPosition) position;
            p.nonlinear.drive = p.nonlinear.amount = 100.0f;
            slot.update (p, true);
            for (int i = 0; i < 2048; ++i)
            {
                float tap;
                const float y = slot.next (i < 256 ? 0.3f*std::sin (i*0.1f) : 0.0f, 0.0f, tap);
                CHECK (slot.isFinite() && std::isfinite (y) && std::isfinite (tap) && std::abs (y) < 8.0f);
                if (i%32 == 31)
                {
                    const auto energy = slot.getNonlinearElement().getFrictionEnergy();
                    CHECK (energy.output <= 1.020001*energy.input);
                }
            }
        }
}
