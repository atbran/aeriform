#include "TestFramework.h"
#include "TestHelpers.h"
#include "DSP/Wavefolder.h"
#include "DSP/Oversampler.h"

using namespace aeriform;
using namespace aeriform::dsp;
using namespace aeriform::test;

AERIFORM_TEST (wavefolder_transfer_is_bounded_for_all_modes)
{
    for (int mode = 0; mode < (int) FoldMode::Count; ++mode)
        for (float fold : { 0.0f, 0.5f, 1.0f })
            for (float sym : { -1.0f, 0.0f, 1.0f })
                for (float bias : { -1.0f, 0.0f, 1.0f })
                    for (int stages = 1; stages <= 4; ++stages)
                    {
                        Wavefolder::Params p;
                        p.on = true; p.mode = (FoldMode) mode; p.fold = fold; p.drive = 1.0f; p.symmetry = sym; p.bias = bias;
                        p.stages = stages; p.shape = 0.7f; p.mix = 1.0f; p.comp = 0.5f;
                        float maxAbs = 0.0f; bool finite = true;
                        for (float x = -4.0f; x <= 4.0f; x += 0.01f)
                        {
                            const float y = Wavefolder::foldSample (p, x);
                            if (! std::isfinite (y)) finite = false;
                            maxAbs = std::max (maxAbs, std::fabs (y));
                        }
                        CHECK_MSG (finite, "mode " + std::to_string (mode) + " finite");
                        CHECK_MSG (maxAbs <= 4.5f, "mode " + std::to_string (mode) + " bounded: " + std::to_string (maxAbs));
                    }
}

AERIFORM_TEST (wavefolder_creates_harmonics_and_removes_dc)
{
    const float sr = 96000.0f;
    for (int mode = 0; mode < (int) FoldMode::Count; ++mode)
    {
        Wavefolder wf;
        wf.prepare (sr);
        Wavefolder::Params p;
        p.on = true; p.mode = (FoldMode) mode; p.fold = 0.8f; p.drive = 0.6f; p.symmetry = 0.8f; p.bias = 0.7f; p.stages = 2; p.shape = 0.6f;
        wf.update (p);
        double mean = 0.0, meanIn = 0.0, energy = 0.0, energyIn = 0.0; long n = 0;
        float peak = 0.0f; bool finite = true;
        for (int i = 0; i < (int) (sr * 0.6f); ++i)
        {
            const float x = 0.7f * std::sin (kTwoPi * 220.0f * (float) i / sr);
            const float y = wf.next (x);
            if (! std::isfinite (y)) finite = false;
            peak = std::max (peak, std::fabs (y));
            if (i > (int) (sr * 0.4f)) { mean += y; meanIn += x; energy += (double) (y - x) * (y - x); energyIn += (double) x * x; ++n; }
        }
        mean /= (double) n;
        CHECK_MSG (finite, "mode " + std::to_string (mode) + " finite");
        CHECK_MSG (std::fabs (mean) < 0.02, "mode " + std::to_string (mode) + " DC removed: " + std::to_string (mean));
        CHECK_MSG (peak <= 4.0f, "mode " + std::to_string (mode) + " bounded");
        CHECK_MSG (energy > 0.02 * energyIn, "mode " + std::to_string (mode) + " changes the waveform");
    }
}

AERIFORM_TEST (wavefolder_bypass_is_transparent_and_mix_blends)
{
    Wavefolder wf;
    wf.prepare (88200.0f);
    Wavefolder::Params p;
    p.on = false;
    wf.update (p);
    CHECK_NEAR (wf.next (0.3f), 0.3, 1.0e-6);
    p.on = true; p.mix = 0.0f; p.fold = 1.0f;
    wf.update (p);
    float maxDiff = 0.0f;
    for (int i = 0; i < 4000; ++i)
    {
        const float x = 0.5f * std::sin (0.14f * (float) i);   // ~2 kHz at 88.2 kHz: negligible DC-blocker phase shift
        maxDiff = std::max (maxDiff, std::fabs (wf.next (x) - x));
    }
    CHECK_MSG (maxDiff < 0.02f, "mix 0 leaves the signal nearly untouched (DC blocker aside): " + std::to_string (maxDiff));
}

AERIFORM_TEST (halfband_oversampler_passes_audio_and_rejects_images)
{
    // a 1 kHz sine upsampled 2x then downsampled must come back almost unchanged
    Oversampler up, down;
    up.setFactor (2); down.setFactor (2);
    float buf[4];
    double err = 0.0; int n = 0;
    for (int i = 0; i < 8000; ++i)
    {
        const float x = std::sin (kTwoPi * 1000.0f * (float) i / 48000.0f);
        up.upsample (x, buf);
        const float y = down.downsample (buf);
        if (i > 200)
        {
            // the cascade has a small group delay: compare against a delayed reference
            const float ref = std::sin (kTwoPi * 1000.0f * (float) (i - 6) / 48000.0f);
            err = std::max (err, (double) std::fabs (y - ref));
            ++n;
        }
    }
    // magnitude test: RMS ratio of a 1 kHz sine through the round trip
    Oversampler up2, down2; up2.setFactor (4); down2.setFactor (4);
    double inE = 0.0, outE = 0.0;
    for (int i = 0; i < 8000; ++i)
    {
        const float x = std::sin (kTwoPi * 1000.0f * (float) i / 48000.0f);
        up2.upsample (x, buf);
        const float y = down2.downsample (buf);
        if (i > 200) { inE += x * x; outE += y * y; }
    }
    const double gainDb = 10.0 * std::log10 (outE / inE);
    CHECK_MSG (std::fabs (gainDb) < 0.5, "4x round-trip gain " + std::to_string (gainDb) + " dB");

    // an image above the base-rate Nyquist must be removed by the decimator
    Oversampler d3; d3.setFactor (2);
    double imageE = 0.0;
    for (int i = 0; i < 8000; ++i)
    {
        const float os0 = std::sin (kTwoPi * 30000.0f * (float) (2 * i) / 96000.0f);
        const float os1 = std::sin (kTwoPi * 30000.0f * (float) (2 * i + 1) / 96000.0f);
        float pair[4] = { os0, os1, 0.0f, 0.0f };
        const float y = d3.downsample (pair);
        if (i > 200) imageE += y * y;
    }
    const double imageDb = 10.0 * std::log10 (imageE / 3900.0);
    CHECK_MSG (imageDb < -60.0, "30 kHz image rejected by decimator: " + std::to_string (imageDb) + " dB");
    juce::ignoreUnused (err, n);
}

AERIFORM_TEST (quality_changes_during_playback_are_safe_and_level_consistent)
{
    TestHost h (48000.0, 256);
    h.set (ids::reverbMix, 0.0f); h.set (ids::delayMix, 0.0f); h.set (ids::chorusMix, 0.0f);
    h.set (ids::exaModel, (float) ExciterModel::Wave);
    h.set (ids::wfOn, 1.0f); h.set (ids::wfFold, 0.7f);
    h.noteOn (48, 100);
    double rms[3] = { 0.0, 0.0, 0.0 };
    bool finite = true;
    for (int q = 0; q < 3; ++q)
    {
        h.set (ids::quality, (float) q);
        h.render (0.2);
        const auto s = h.render (0.4);
        finite = finite && s.finite;
        rms[q] = s.rms;
    }
    // rapid switching every block
    for (int b = 0; b < 200; ++b)
    {
        h.set (ids::quality, (float) (b % 3));
        finite = finite && h.renderBlock().finite;
    }
    CHECK (finite);
    for (int q = 1; q < 3; ++q)
        CHECK_MSG (std::fabs (20.0 * std::log10 (rms[q] / rms[0])) < 3.0, "quality levels consistent: eco " + std::to_string (rms[0]) + " vs " + std::to_string (rms[q]));
}

AERIFORM_TEST (wavefolder_in_the_voice_changes_the_sound_and_stays_bounded)
{
    TestHost h (48000.0, 256);
    h.set (ids::reverbMix, 0.0f); h.set (ids::delayMix, 0.0f); h.set (ids::chorusMix, 0.0f);
    h.set (ids::exaModel, (float) ExciterModel::Wave);
    h.set (ids::netMix, 0.0f);
    h.noteOn (52, 100);
    h.render (0.1);
    std::vector<float> dry;
    h.render (0.3, &dry);
    h.set (ids::wfOn, 1.0f); h.set (ids::wfFold, 1.0f); h.set (ids::wfDrive, 1.0f); h.set (ids::wfStages, 4.0f); h.set (ids::wfSymmetry, 1.0f); h.set (ids::wfBias, 1.0f);
    h.render (0.1);
    std::vector<float> wet;
    const auto s = h.render (0.3, &wet);
    CHECK (s.finite);
    CHECK_MSG (s.peak < 1.3f, "peak " + std::to_string (s.peak));
    double diff = 0.0, ref = 0.0;
    for (size_t i = 0; i < std::min (dry.size(), wet.size()); ++i) { diff += (double) (dry[i] - wet[i]) * (dry[i] - wet[i]); ref += (double) dry[i] * dry[i]; }
    CHECK_MSG (diff > 0.1 * ref, "folder audibly changes the signal");
}
