#include "TestFramework.h"
#include "TestHelpers.h"
#include "DSP/Voice.h"
#include "DSP/Resonator.h"
#include <cstdio>

using namespace aeriform;
using namespace aeriform::dsp;
using namespace aeriform::test;

// PIPE Jet and Reed mouthpieces: steady pressure must make the pipe speak by itself, in tune,
// with bounded energy for any setting. Linear (the original PIPE) is covered by PipeTests.cpp.
namespace
{
constexpr double kRate = 48000.0;

VoiceParams mouthpieceParams (PipeBlow blow, bool cylinder = false)
{
    VoiceParams p = defaultVoiceParams();
    p.v[(size_t) P::resMode] = (float) ResMode::Pipe;
    p.v[(size_t) P::exaModel] = (float) ExciterModel::Off;
    p.v[(size_t) P::exbModel] = (float) ExciterModel::Off;
    p.v[(size_t) P::resPipeBlow] = (float) blow;
    p.v[(size_t) P::resPipeBore] = cylinder ? 1.0f : 0.0f;
    // no pitch movement from the articulation section, so tuning can be measured exactly
    p.v[(size_t) P::artInstability] = 0.0f;
    p.v[(size_t) P::artVariation] = 0.0f;
    p.v[(size_t) P::artFlowPitch] = 0.0f;
    p.v[(size_t) P::envSustain] = 1.0f;
    return p;
}

std::vector<float> renderNote (const VoiceParams& p, double seconds, int note)
{
    Voice v;
    v.prepare (kRate, 0);
    v.startNote (note, 0.8f, (float) note, false, 0, 1, 1, p);
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

double magnitudeAt (const std::vector<float>& y, double from, double to, double f)
{
    const size_t a = (size_t) (from * kRate), e = std::min (y.size(), (size_t) (to * kRate));
    double re = 0.0, im = 0.0;
    const double w = 2.0 * 3.14159265358979 * f / kRate;
    for (size_t i = a; i < e; ++i)
    {
        const double win = 0.5 - 0.5 * std::cos (2.0 * 3.14159265358979 * (double) (i - a) / (double) (e - a));
        re += win * y[i] * std::cos (w * (double) i); im -= win * y[i] * std::sin (w * (double) i);
    }
    return std::sqrt (re * re + im * im) / (double) std::max<size_t> (1, e - a);
}

// Strongest spectral peak within +/-60 cents of f (coarse scan, then a fine one).
double peakNear (const std::vector<float>& y, double from, double to, double f)
{
    double bestF = f, best = -1.0;
    for (double c = -60.0; c <= 60.0; c += 2.0)
    {
        const double t = f * std::exp2 (c / 1200.0), m = magnitudeAt (y, from, to, t);
        if (m > best) { best = m; bestF = t; }
    }
    const double centre = bestF;
    for (double c = -2.0; c <= 2.0; c += 0.1)
    {
        const double t = centre * std::exp2 (c / 1200.0), m = magnitudeAt (y, from, to, t);
        if (m > best) { best = m; bestF = t; }
    }
    return bestF;
}

double periodicity (const std::vector<float>& x, double from, double to, double hz)
{
    const int lag = (int) std::lround (kRate / hz);
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

double noteHz (int note) { return 440.0 * std::exp2 ((note - 69) / 12.0); }
}

AERIFORM_TEST (pipe_jet_speaks_from_steady_pressure)
{
    // Pure DC blowing (no noise): the jet turns steady pressure into a sustained periodic tone,
    // which the additive Linear excitation cannot do (its loop highpass removes the DC).
    auto jet = mouthpieceParams (PipeBlow::Jet), linear = mouthpieceParams (PipeBlow::Linear);
    for (auto* p : { &jet, &linear }) p->v[(size_t) P::resPipeDcNoise] = 0.0f;
    const auto yJet = renderNote (jet, 2.0, 60), yLinear = renderNote (linear, 2.0, 60);
    const double jetLevel = rms (yJet, 1.0, 2.0), linearLevel = rms (yLinear, 1.0, 2.0);
    const double tonal = periodicity (yJet, 1.0, 2.0, noteHz (60));
    std::printf ("    jet rms %.4f periodicity %.3f, linear rms %.6f\n", jetLevel, tonal, linearLevel);
    CHECK (jetLevel > 0.03);
    CHECK (tonal > 0.9);
    CHECK (jetLevel > 30.0 * linearLevel);
}

AERIFORM_TEST (pipe_mouthpieces_are_in_tune)
{
    struct Config { PipeBlow blow; bool cylinder; float pressure; const char* name; };
    const Config configs[] { { PipeBlow::Jet, false, 50.0f, "jet cone" }, { PipeBlow::Jet, true, 50.0f, "jet cylinder" },
                             { PipeBlow::Reed, true, 70.0f, "reed cylinder" }, { PipeBlow::Reed, false, 70.0f, "reed cone" } };
    for (const auto& c : configs)
        for (int note : { 36, 48, 60, 72, 84, 96 })
        {
            auto p = mouthpieceParams (c.blow, c.cylinder);
            p.v[(size_t) P::resPipePressure] = c.pressure;
            p.v[(size_t) P::resPipeDcNoise] = 5.0f;
            const auto y = renderNote (p, 1.6, note);
            const double f = peakNear (y, 0.8, 1.6, noteHz (note));
            const double cents = 1200.0 * std::log2 (f / noteHz (note));
            const double tonal = periodicity (y, 0.8, 1.6, noteHz (note));
            std::printf ("    %-14s note %3d  %+6.1f cents  periodicity %.3f  rms %.4f\n", c.name, note, cents, tonal, rms (y, 0.8, 1.6));
            // Jet: whole range. Reed: C2..C5; above that (altissimo) the harmonic pulling is reported only.
            if (c.blow == PipeBlow::Jet || note <= 72) CHECK (std::abs (cents) < 5.0);
            CHECK (tonal > 0.9);
            CHECK (rms (y, 0.8, 1.6) > 0.02 && rms (y, 0.8, 1.6) < 0.3);
        }
}

AERIFORM_TEST (pipe_jet_overblows_and_tone_follows_jet_length)
{
    // A very long jet at gentle pressure locks to the octave (overblowing); the default does not.
    auto base = mouthpieceParams (PipeBlow::Jet);
    auto longJet = base; longJet.v[(size_t) P::resPipeJet] = 100.0f; longJet.v[(size_t) P::resPipePressure] = 25.0f;
    const auto yDefault = renderNote (base, 1.5, 60), yLong = renderNote (longJet, 1.5, 60);
    const double f = noteHz (60);
    const double defRatio = magnitudeAt (yDefault, 0.7, 1.5, 2.0 * f) / magnitudeAt (yDefault, 0.7, 1.5, f);
    const double longRatio = magnitudeAt (yLong, 0.7, 1.5, 2.0 * f) / magnitudeAt (yLong, 0.7, 1.5, f);
    std::printf ("    octave / fundamental: default %.3f, long jet %.3f\n", defRatio, longRatio);
    CHECK (defRatio < 0.3);
    CHECK (longRatio > 3.0);
}

AERIFORM_TEST (pipe_mouthpiece_loudness_does_not_follow_decay_time)
{
    // Output is normalised by the round-trip loss: decay time colours the tone but does not set loudness.
    for (auto blow : { PipeBlow::Jet, PipeBlow::Reed })
    {
        auto shortRt = mouthpieceParams (blow, blow == PipeBlow::Reed), longRt = shortRt;
        shortRt.v[(size_t) P::resPipeRt] = 0.15f; longRt.v[(size_t) P::resPipeRt] = 8.0f;
        const double a = rms (renderNote (shortRt, 1.5, 60), 0.7, 1.5), b = rms (renderNote (longRt, 1.5, 60), 0.7, 1.5);
        const double db = 20.0 * std::log10 (b / std::max (1.0e-9, a));
        std::printf ("    %s: rt 0.15 s rms %.4f, rt 8 s rms %.4f (%+.1f dB)\n", blow == PipeBlow::Jet ? "jet " : "reed", a, b, db);
        CHECK (std::abs (db) < 9.0);
    }
}

AERIFORM_TEST (pipe_mouthpieces_are_bounded_for_extreme_settings)
{
    // Every corner of the controls that matter to the mouthpiece, across the keyboard: finite and bounded.
    int runs = 0; float worst = 0.0f;
    for (auto blow : { PipeBlow::Jet, PipeBlow::Reed })
        for (bool cyl : { false, true })
            for (float drive : { 1.0f, 32.0f })
                for (float jet : { 0.0f, 100.0f })
                    for (float sym : { -100.0f, 100.0f })
                        for (int note : { 21, 108 })
                        {
                            auto p = mouthpieceParams (blow, cyl);
                            p.v[(size_t) P::resPipePressure] = 100.0f; p.v[(size_t) P::resPipeRt] = 30.0f;
                            p.v[(size_t) P::resPipeSatDrive] = drive; p.v[(size_t) P::resPipeJet] = jet;
                            p.v[(size_t) P::resPipeSatSym] = sym; p.v[(size_t) P::resPipeSatKnee] = jet;
                            p.v[(size_t) P::resPipeDcNoise] = 100.0f - jet; p.v[(size_t) P::resPipeLp] = 20000.0f;
                            const auto y = renderNote (p, 0.6, note);
                            bool finite = true;
                            for (float v : y) { finite = finite && std::isfinite (v); worst = std::max (worst, std::abs (v)); }
                            CHECK (finite);
                            ++runs;
                        }
    std::printf ("    %d extreme configurations, peak %.3f\n", runs, worst);
    CHECK (worst < 4.0f);
}

AERIFORM_TEST (pipe_jet_every_control_changes_the_output)
{
    // Liveness in Jet mode: each control at two settings changes level (3 dB), the spectrum
    // (a harmonic moves by 2 dB relative to the fundamental) or tonality.
    struct Case { P id; float a, b; const char* name; };
    const Case cases[] {
        { P::resPipePressure, 30.0f, 100.0f, "pressure" }, { P::resPipeDcNoise, 0.0f, 100.0f, "dcnoise" },
        { P::resPipeExcCut, 300.0f, 12000.0f, "exc_cut" }, { P::resPipeRt, 0.1f, 10.0f, "rt" },
        { P::resPipeDamp, 0.0f, 90.0f, "damp" },           { P::resPipeLp, 800.0f, 16000.0f, "lp" },
        { P::resPipeHp, 20.0f, 1500.0f, "hp" },            { P::resPipeSatDrive, 1.0f, 16.0f, "drive" },
        { P::resPipeSatKnee, 0.0f, 100.0f, "hardness" },   { P::resPipeSatSym, -60.0f, 60.0f, "asymmetry" },
        { P::resPipeBore, 0.0f, 1.0f, "bore" },            { P::resPipeJet, 20.0f, 60.0f, "jet" },
    };
    for (const auto& c : cases)
    {
        auto pa = mouthpieceParams (PipeBlow::Jet), pb = pa;
        pa.v[(size_t) c.id] = c.a; pb.v[(size_t) c.id] = c.b;
        const auto ya = renderNote (pa, 1.2, 60), yb = renderNote (pb, 1.2, 60);
        const double f = noteHz (60);
        const double levelDb = 20.0 * std::log10 (std::max (1.0e-9, rms (yb, 0.4, 1.2)) / std::max (1.0e-9, rms (ya, 0.4, 1.2)));
        double maxHarm = 0.0;
        for (int h = 2; h <= 6; ++h)
        {
            const double ra = magnitudeAt (ya, 0.4, 1.2, h * f) / std::max (1.0e-12, magnitudeAt (ya, 0.4, 1.2, f));
            const double rb = magnitudeAt (yb, 0.4, 1.2, h * f) / std::max (1.0e-12, magnitudeAt (yb, 0.4, 1.2, f));
            maxHarm = std::max (maxHarm, std::abs (20.0 * std::log10 ((rb + 1.0e-9) / (ra + 1.0e-9))));
        }
        const double tonal = std::abs (periodicity (ya, 0.4, 1.2, f) - periodicity (yb, 0.4, 1.2, f));
        // brightness: level of the first difference relative to the signal (sees the breath-noise colour)
        auto brightness = [] (const std::vector<float>& y)
        {
            std::vector<float> d (y.size(), 0.0f);
            for (size_t i = 1; i < y.size(); ++i) d[i] = y[i] - y[i - 1];
            return 20.0 * std::log10 (std::max (1.0e-9, rms (d, 0.4, 1.2)) / std::max (1.0e-9, rms (y, 0.4, 1.2)));
        };
        const double bright = std::abs (brightness (yb) - brightness (ya));
        std::printf ("    %-10s level %+6.1f dB, harmonic change %5.1f dB, brightness %4.1f dB, periodicity change %.3f\n", c.name, levelDb, maxHarm, bright, tonal);
        CHECK (std::abs (levelDb) >= 3.0 || maxHarm >= 2.0 || bright >= 1.5 || tonal >= 0.02);
    }
}

AERIFORM_TEST (pipe_mouthpieces_speak_in_tune_at_other_sample_rates)
{
    for (double sr : { 44100.0, 96000.0 })
        for (auto blow : { PipeBlow::Jet, PipeBlow::Reed })
            for (int note : { 48, 72 })
            {
                auto p = mouthpieceParams (blow, blow == PipeBlow::Reed);
                p.v[(size_t) P::resPipePressure] = blow == PipeBlow::Reed ? 70.0f : 50.0f;
                p.v[(size_t) P::resPipeDcNoise] = 5.0f;
                Voice v; v.prepare (sr, 0);
                v.startNote (note, 0.8f, (float) note, false, 0, 1, 1, p);
                ModSources gs {};
                std::vector<float> y, l (256), r (256);
                for (int b = 0; b < (int) (1.6 * sr / 256.0); ++b)
                {
                    std::fill (l.begin(), l.end(), 0.0f); std::fill (r.begin(), r.end(), 0.0f);
                    v.render (l.data(), r.data(), 256, p, gs, nullptr, nullptr, 0.0f);
                    for (int i = 0; i < 256; ++i) y.push_back (0.5f * (l[(size_t) i] + r[(size_t) i]));
                }
                // the 48 kHz helpers assume kRate, so scan the fundamental at this sample rate directly
                const size_t a = (size_t) (0.8 * sr), e = y.size();
                double best = -1.0, bestC = 0.0;
                for (double c = -40.0; c <= 40.0; c += 0.25)
                {
                    const double f = noteHz (note) * std::exp2 (c / 1200.0), w = 2.0 * 3.14159265358979 * f / sr;
                    double re = 0.0, im = 0.0;
                    for (size_t i = a; i < e; ++i) { re += y[i] * std::cos (w * (double) i); im += y[i] * std::sin (w * (double) i); }
                    if (re * re + im * im > best) { best = re * re + im * im; bestC = c; }
                }
                double s2 = 0.0; for (size_t i = a; i < e; ++i) s2 += (double) y[i] * y[i];
                const double level = std::sqrt (s2 / (double) (e - a));
                std::printf ("    %6.0f Hz %-4s note %d: %+5.2f cents, rms %.4f\n", sr, blow == PipeBlow::Jet ? "jet" : "reed", note, bestC, level);
                CHECK (std::abs (bestC) < 5.0);
                CHECK (level > 0.02 && level < 0.3);
            }
}
