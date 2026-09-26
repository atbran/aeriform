#include "TestFramework.h"
#include "TestHelpers.h"
#include "DSP/Voice.h"
#include "DSP/Resonator.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>

using namespace aeriform;
using namespace aeriform::dsp;
using namespace aeriform::test;

// PIPE model (ResMode index 9) integration checks. These follow the bisection order of
// docs/PIPE_MODEL_DIAGNOSTIC.md: the PIPE path must run (0A), sustain while held (0B),
// differ from the existing models (0C), and every PIPE parameter must change the output (Stage 1).
namespace
{
constexpr double kRate = 48000.0;

VoiceParams pipeVoiceParams()
{
    VoiceParams p = defaultVoiceParams();
    p.v[(size_t) P::resMode] = (float) ResMode::Pipe;
    // PIPE's own blown excitation only: the legacy exciters are switched off
    p.v[(size_t) P::exaModel] = (float) ExciterModel::Off;
    p.v[(size_t) P::exbModel] = (float) ExciterModel::Off;
    // these checks document the original additive (Linear) PIPE; Jet and Reed: PipeMouthpieceTests.cpp
    p.v[(size_t) P::resPipeBlow] = (float) PipeBlow::Linear;
    return p;
}

std::vector<float> renderNote (const VoiceParams& p, double seconds, int note = 57, float velocity = 0.8f)
{
    Voice v;
    v.prepare (kRate, 0);
    v.startNote (note, velocity, (float) note, false, 0, 1, 1, p);
    ModSources gs {};
    std::vector<float> out, l (256), r (256);
    const int blocks = (int) std::ceil (seconds * kRate / 256.0);
    for (int b = 0; b < blocks; ++b)
    {
        std::fill (l.begin(), l.end(), 0.0f); std::fill (r.begin(), r.end(), 0.0f);
        v.render (l.data(), r.data(), 256, p, gs, nullptr, nullptr, 0.0f);
        for (int i = 0; i < 256; ++i) out.push_back (0.5f * (l[(size_t) i] + r[(size_t) i]));
    }
    return out;
}

double rms (const std::vector<float>& x, double from, double to)
{
    const size_t a = (size_t) (from * kRate), b = std::min (x.size(), (size_t) (to * kRate));
    double s = 0.0;
    for (size_t i = a; i < b; ++i) s += (double) x[i] * x[i];
    return b > a ? std::sqrt (s / (double) (b - a)) : 0.0;
}

// 24 log-spaced RBJ bandpass bands, 60 Hz .. 12 kHz, level in dB after loudness (RMS) matching.
std::array<double, 24> bandLevels (const std::vector<float>& x, double from, double to)
{
    std::array<double, 24> out {};
    const double total = std::max (1.0e-12, rms (x, from, to));
    const size_t a = (size_t) (from * kRate), e = std::min (x.size(), (size_t) (to * kRate));
    for (int band = 0; band < 24; ++band)
    {
        const double fc = 60.0 * std::pow (200.0, band / 23.0), q = 4.0;
        const double w = 2.0 * 3.14159265358979 * fc / kRate, alpha = std::sin (w) / (2.0 * q), a0 = 1.0 + alpha;
        const double b0 = alpha / a0, b2 = -alpha / a0, a1 = -2.0 * std::cos (w) / a0, a2 = (1.0 - alpha) / a0;
        double z1 = 0.0, z2 = 0.0, s = 0.0;
        for (size_t i = 0; i < e; ++i)
        {
            const double in = x[i] / total, y = b0 * in + z1;
            z1 = -a1 * y + z2; z2 = b2 * in - a2 * y;
            if (i >= a) s += y * y;
        }
        out[(size_t) band] = 10.0 * std::log10 (s / (double) std::max<size_t> (1, e - a) + 1.0e-20);
    }
    return out;
}

int bandsDiffering (const std::vector<float>& x, const std::vector<float>& y, double from, double to, double* maxDiff = nullptr)
{
    const auto bx = bandLevels (x, from, to), by = bandLevels (y, from, to);
    int count = 0; double m = 0.0;
    for (size_t i = 0; i < bx.size(); ++i) { const double d = std::abs (bx[i] - by[i]); m = std::max (m, d); if (d >= 1.5) ++count; }
    if (maxDiff != nullptr) *maxDiff = m;
    return count;
}

// Normalised autocorrelation at the note's period: how periodic (tonal) vs noisy the sound is.
// Decay controls change the sharpness of the harmonic peaks, which band levels cannot see.
double periodicity (const std::vector<float>& x, double from, double to, int note)
{
    const int lag = (int) std::lround (kRate / (440.0 * std::exp2 ((note - 69) / 12.0)));
    const size_t a = (size_t) (from * kRate), e = std::min (x.size(), (size_t) (to * kRate));
    double best = 0.0, r0 = 0.0;
    for (size_t i = a; i < e; ++i) r0 += (double) x[i] * x[i];
    for (int l = lag - 3; l <= lag + 3; ++l)
    {
        double r = 0.0;
        for (size_t i = a; i + (size_t) l < e; ++i) r += (double) x[i] * x[i + (size_t) l];
        best = std::max (best, r / std::max (1.0e-20, r0));
    }
    return best;
}

void maybeWriteWav (const char* name, const std::vector<float>& x)
{
    const char* dir = std::getenv ("AERIFORM_PIPE_RENDER_DIR");
    if (dir == nullptr) return;
    std::ofstream f (std::string (dir) + "/" + name + ".wav", std::ios::binary);
    auto u32 = [&] (uint32_t v) { f.write ((const char*) &v, 4); };
    auto u16 = [&] (uint16_t v) { f.write ((const char*) &v, 2); };
    const uint32_t bytes = (uint32_t) x.size() * 2;
    f.write ("RIFF", 4); u32 (36 + bytes); f.write ("WAVEfmt ", 8); u32 (16); u16 (1); u16 (1);
    u32 ((uint32_t) kRate); u32 ((uint32_t) kRate * 2); u16 (2); u16 (16); f.write ("data", 4); u32 (bytes);
    for (float s : x) { const int16_t v = (int16_t) std::lround (std::clamp (s, -1.0f, 1.0f) * 32767.0f); f.write ((const char*) &v, 2); }
}
}

AERIFORM_TEST (pipe_model_runs_its_own_engine)
{
    // 0A: with every legacy exciter off, only PIPE's continuous excitation can make sound.
    auto pipe = pipeVoiceParams();
    auto open = pipe; open.v[(size_t) P::resMode] = (float) ResMode::OpenPipe;
    const auto yPipe = renderNote (pipe, 1.5), yOpen = renderNote (open, 1.5);
    const double pipeLevel = rms (yPipe, 0.5, 1.5), openLevel = rms (yOpen, 0.5, 1.5);
    std::printf ("    PIPE rms %.5f, Open Pipe rms %.5f (exciters off)\n", pipeLevel, openLevel);
    CHECK (pipeLevel > 0.01);
    CHECK (pipeLevel > 20.0 * openLevel);
    maybeWriteWav ("pipe-default-A3", yPipe);
}

AERIFORM_TEST (pipe_model_differs_from_every_existing_model)
{
    // 0C: same note and the default Breath exciter on the legacy models; PIPE must not match any of them.
    auto pipe = pipeVoiceParams();
    const auto yPipe = renderNote (pipe, 1.5);
    auto legacy = defaultVoiceParams();
    for (int m = 0; m < (int) ResMode::Pipe; ++m)
    {
        legacy.v[(size_t) P::resMode] = (float) m;
        double maxDiff = 0.0;
        const auto yLegacy = renderNote (legacy, 1.5);
        const int bands = bandsDiffering (yPipe, yLegacy, 0.5, 1.5, &maxDiff);
        const double levelDb = 20.0 * std::log10 (rms (yPipe, 0.5, 1.5) / std::max (1.0e-9, rms (yLegacy, 0.5, 1.5)));
        std::printf ("    vs %-15s %2d/24 bands >= 1.5 dB, max %.1f dB, PIPE level %+.1f dB\n", choices::resModes()[m].toRawUTF8(), bands, maxDiff, levelDb);
        CHECK (bands >= 6);
    }
}

AERIFORM_TEST (pipe_model_sustains_while_held)
{
    // 0B: sustain ratio = RMS over 2.0..2.5 s / peak 100 ms-window RMS in the first 0.5 s.
    for (float rt : { 0.5f, 20.0f })
    {
        auto p = pipeVoiceParams();
        p.v[(size_t) P::resPipeRt] = rt;
        const auto y = renderNote (p, 3.0);
        double peak = 0.0;
        for (double t = 0.0; t < 0.5; t += 0.01) peak = std::max (peak, rms (y, t, t + 0.1));
        const double ratio = rms (y, 2.0, 2.5) / std::max (1.0e-12, peak);
        std::printf ("    nominal decay %.1f s: sustain ratio %.3f\n", rt, ratio);
        CHECK (ratio > 0.5);
        CHECK (std::all_of (y.begin(), y.end(), [] (float s) { return std::isfinite (s) && std::abs (s) < 4.0f; }));
        maybeWriteWav (rt < 1.0f ? "pipe-held-rt0.5" : "pipe-held-rt20", y);
    }
}

AERIFORM_TEST (pipe_model_every_parameter_changes_the_output)
{
    // Stage 1 liveness: each PIPE control rendered at two settings, loudness-matched,
    // must move at least one of 24 bands by 1.5 dB. Pressure is checked on level instead,
    // since in the linear regime it is (by design) mostly a volume control.
    struct Case { P id; float a, b; const char* name; };
    const Case cases[] {
        { P::resPipePressure, 10.0f, 100.0f, "pressure" },  { P::resPipeDcNoise, 0.0f, 100.0f, "dcnoise" },
        { P::resPipeExcCut, 200.0f, 12000.0f, "exc_cut" },  { P::resPipeExcRes, 0.5f, 8.0f, "exc_res" },
        { P::resPipeExcKt, 0.0f, 150.0f, "exc_kt" },        { P::resPipeExcVt, 0.0f, 100.0f, "exc_vt" },
        { P::resPipeRt, 0.05f, 10.0f, "rt" },               { P::resPipeRtKt, 0.0f, 150.0f, "rt_kt" }, // base decay set below
        { P::resPipeDamp, 0.0f, 90.0f, "damp" },            { P::resPipeLp, 800.0f, 16000.0f, "lp" },
        { P::resPipeHp, 20.0f, 1500.0f, "hp" },             { P::resPipeFiltKt, 0.0f, 150.0f, "filt_kt" },
        { P::resPipeSatDrive, 1.0f, 32.0f, "sat_drive" },   { P::resPipeSatKnee, 0.0f, 100.0f, "sat_knee" },
        { P::resPipeSatSym, -100.0f, 100.0f, "sat_sym" },   { P::resPipeBore, 0.0f, 1.0f, "bore" },
    };
    for (const auto& c : cases)
    {
        auto pa = pipeVoiceParams(), pb = pa;
        pa.v[(size_t) c.id] = c.a; pb.v[(size_t) c.id] = c.b;
        // decay key tracking: at default pressure the saturator, not the nominal gain, sets most of the
        // loss (P1 found rt_kt the weakest control); check it in the small-signal regime
        if (c.id == P::resPipeRtKt) { pa.v[(size_t) P::resPipeRt] = pb.v[(size_t) P::resPipeRt] = 2.0f; pa.v[(size_t) P::resPipePressure] = pb.v[(size_t) P::resPipePressure] = 5.0f; }
        const int note = (c.id == P::resPipeExcKt || c.id == P::resPipeRtKt || c.id == P::resPipeFiltKt) ? 81 : 57;
        const auto ya = renderNote (pa, 1.2, note), yb = renderNote (pb, 1.2, note);
        double maxDiff = 0.0;
        const int bands = bandsDiffering (ya, yb, 0.4, 1.2, &maxDiff);
        const double levelDb = 20.0 * std::log10 (std::max (1.0e-9, rms (yb, 0.4, 1.2)) / std::max (1.0e-9, rms (ya, 0.4, 1.2)));
        const double tonal = std::abs (periodicity (ya, 0.4, 1.2, note) - periodicity (yb, 0.4, 1.2, note));
        std::printf ("    %-10s %2d/24 bands >= 1.5 dB, max %5.1f dB, level %+6.1f dB, periodicity change %.3f\n", c.name, bands, maxDiff, levelDb, tonal);
        CHECK (bands >= 1 || std::abs (levelDb) >= 3.0 || tonal >= 0.05);
    }
}

AERIFORM_TEST (pipe_model_is_in_tune)
{
    // Contract C4: the fundamental's spectral peak within 3 cents. The third harmonic is reported,
    // not asserted: the key-tracked loop highpass leads the fundamental far more than the overtones,
    // so the partials are flat relative to an in-tune fundamental (see the port report).
    auto peakNear = [] (const std::vector<float>& y, double f)
    {
        double best = -1.0, bestF = f;
        for (double t = f * 0.97; t <= f * 1.03; t += f * 2.0e-5)
        {
            double re = 0.0, im = 0.0; const double w = 2.0 * 3.14159265358979 * t / kRate;
            for (size_t i = 0; i < y.size(); ++i) { re += y[i] * std::cos (w * (double) i); im -= y[i] * std::sin (w * (double) i); }
            if (re * re + im * im > best) { best = re * re + im * im; bestF = t; }
        }
        return bestF;
    };
    for (int note : { 45, 57, 69 })
        for (float bore : { 0.0f, 1.0f })
        {
            ResonatorSlot slot; slot.prepare ((float) kRate);
            ResonatorParams r; r.type = ResMode::Pipe; r.freqHz = 440.0f * std::exp2 ((note - 69) / 12.0f);
            r.pipe.pressure = 0.0f; r.pipe.rtSec = 2.0f; r.pipe.cylinder = bore > 0.5f;
            slot.update (r, true);
            std::vector<float> y;
            for (int i = 0; i < 96000; ++i) { float t; y.push_back (slot.next (i == 0 ? 0.01f : 0.0f, 0.0f, t)); }
            const double c1 = centsBetween (peakNear (y, r.freqHz), r.freqHz);
            const double c3 = centsBetween (peakNear (y, 3.0 * r.freqHz), 3.0 * r.freqHz);
            std::printf ("    note %d %-8s fundamental %+.2f cents, 3rd harmonic %+.1f cents\n", note, bore > 0.5f ? "cylinder" : "cone", c1, c3);
            CHECK (std::abs (c1) < 3.0);
        }
}

AERIFORM_TEST (pipe_slot_reports_telemetry_and_legacy_models_are_untouched)
{
    ResonatorSlot slot; slot.prepare (48000.0f);
    ResonatorParams r; r.type = ResMode::Pipe; r.freqHz = 220.0f; r.pipe.rtSec = 2.0f;
    slot.update (r, true);
    CHECK (slot.isPipe());
    const auto& t = slot.getPipeTelemetry();
    CHECK (t.loopGainDb < 0.0f && t.loopGainDb > -3.0f);
    CHECK (t.effectiveDecaySec > 0.0f && t.effectiveDecaySec <= 2.0f);
    CHECK (std::isfinite (t.phaseCompSamples) && t.peakFound);
    // coupling normalisation for the legacy models is the unchanged 0.7 + 0.3 * feedback law
    ResonatorParams legacy; legacy.feedback = 0.4f;
    CHECK_NEAR (ResonatorSlot::couplingLoopGain (legacy), 0.82f, 1.0e-6);
}

