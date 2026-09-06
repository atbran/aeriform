#include "TestFramework.h"
#include "TestHelpers.h"

using namespace aeriform;
using namespace aeriform::test;

// Prints the level of every factory preset (a 4-note chord measured from note-on, effects off) so the
// set stays consistent. Fails if a preset is inaudible or more than 14 dB away from the median.
// Percussive presets are naturally quieter in RMS terms; the window starts at note-on so their
// attack counts.
AERIFORM_TEST (smoke_factory_preset_levels_are_consistent)
{
    TestHost h (48000.0, 256, false);
    h.enableSidechain();
    dsp::Noise scNoise; scNoise.seed (11);
    h.sidechainSource = [&] (long) { return scNoise.next() * 0.5f; };
    auto& pm = h.processor.getPresetManager();
    struct Row { juce::String name; double rms; float peak; };
    std::vector<Row> rows;
    for (int i = 0; i < (int) pm.getEntries().size(); ++i)
    {
        if (! pm.getEntries()[(size_t) i].isFactory) continue;
        CHECK (pm.loadPreset (i));
        h.set (ids::reverbMix, 0.0f); h.set (ids::delayMix, 0.0f); h.set (ids::chorusMix, 0.0f);
        for (int note : { 48, 55, 60, 64 }) h.noteOn (note, 100);
        const auto s = h.render (1.2);
        for (int note : { 48, 55, 60, 64 }) h.noteOff (note);
        h.render (0.3);
        h.processor.getEngine().allNotesOff();
        h.processor.reset();
        rows.push_back ({ pm.getCurrentName(), s.rms, s.peak });
    }
    // loudness proxy: RMS for sustained sounds, a quarter of the attack peak for percussive ones
    auto level = [] (const Row& r) { return std::max (r.rms, (double) r.peak * 0.25); };
    std::vector<double> sorted;
    for (auto& r : rows) sorted.push_back (level (r));
    std::sort (sorted.begin(), sorted.end());
    const double median = sorted[sorted.size() / 2];
    std::printf ("    preset levels (4-note chord from note-on, 1.2 s, fx off), median level %.3f\n", median);
    for (auto& r : rows)
    {
        const double db = 20.0 * std::log10 (std::max (1.0e-6, level (r) / median));
        std::printf ("      %-24s rms %.3f  peak %.2f  (%+.1f dB)\n", r.name.toRawUTF8(), r.rms, r.peak, db);
        CHECK_MSG (r.peak > 0.05f, (r.name + " audible").toStdString());
        CHECK_MSG (std::fabs (db) < 14.0, (r.name + " level within 14 dB of the median").toStdString());   // Percussive Click is a quiet click by design
    }
}
