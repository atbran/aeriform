#include "TestFramework.h"
#include "TestHelpers.h"
#include <chrono>
#include <cstdlib>

using namespace aeriform;
using namespace aeriform::test;

// CPU profile of the v0.2 configurations at every quality mode. Prints a markdown table
// (copied into docs/PERFORMANCE.md). Run with:  AeriformTests --filter=smoke_cpu_profile
namespace
{
    struct Config
    {
        const char* name;
        int voices;                          // notes played
        std::function<void (TestHost&)> setup;
    };

    void playNotes (TestHost& h, int count)
    {
        static const int notes[16] = { 48, 52, 55, 59, 62, 64, 67, 71, 43, 47, 50, 53, 57, 60, 65, 69 };
        for (int i = 0; i < count; ++i) h.noteOn (notes[i], 100);
    }

    double measurePercent (TestHost& h, double seconds)
    {
        const double warm = 0.5;
        h.render (warm);
        const auto t0 = std::chrono::steady_clock::now();
        h.render (seconds);
        const double elapsed = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
        return 100.0 * elapsed / seconds;
    }

    void twoExciters (TestHost& h)
    {
        h.set (ids::exaModel, (float) ExciterModel::Wave);
        h.set (ids::exbModel, (float) ExciterModel::Complex);
        h.set (ids::mixMode, (float) InteractionMode::FM);
        h.set (ids::mixB2A, 0.5f);
    }
}

AERIFORM_TEST (smoke_cpu_profile_configurations)
{
    double seconds = 4.0;
    if (const char* env = std::getenv ("AERIFORM_PROFILE_SECONDS")) seconds = std::max (1.0, std::atof (env));

    const std::vector<Config> configs = {
        { "Default patch (Breath -> single resonator), 8 voices", 8, [] (TestHost&) {} },
        { "2 exciters (Wave FM Complex) + 1 resonator, 8 voices", 8, [] (TestHost& h) { twoExciters (h); } },
        { "2 exciters + 3 resonators parallel, 8 voices", 8, [] (TestHost& h)
          { twoExciters (h); h.set (ids::netMode, (float) NetMode::Parallel); h.set (ids::rbOn, 1.0f); h.set (ids::rcOn, 1.0f); } },
        { "3 resonators serial (Breath), 8 voices", 8, [] (TestHost& h)
          { h.set (ids::netMode, (float) NetMode::Serial); h.set (ids::rbOn, 1.0f); h.set (ids::rcOn, 1.0f); } },
        { "Maximum cross-feedback (all six routes, feedback 100 %), 8 voices", 8, [] (TestHost& h)
          { twoExciters (h); h.set (ids::netMode, (float) NetMode::Hybrid); h.set (ids::rbOn, 1.0f); h.set (ids::rcOn, 1.0f);
            for (auto* id : { ids::netAB, ids::netBA, ids::netBC, ids::netCB, ids::netCA, ids::netAC, ids::netFeedback }) h.set (id, 1.0f); } },
        { "Energy loop on (serial network, loop -> folder in), 8 voices", 8, [] (TestHost& h)
          { h.set (ids::netMode, (float) NetMode::Serial); h.set (ids::rbOn, 1.0f); h.set (ids::rcOn, 1.0f);
            h.set (ids::loopOn, 1.0f); h.set (ids::loopAmount, 0.8f); h.set (ids::wfOn, 1.0f); } },
        { "Wavefolder on (2 exciters, single resonator), 8 voices", 8, [] (TestHost& h)
          { twoExciters (h); h.set (ids::wfOn, 1.0f); h.set (ids::wfFold, 0.8f); h.set (ids::wfStages, 3.0f); } },
        { "Everything: 2 exciters, folder, 3 resonators hybrid, cross-feedback, loop, effects, 8 voices", 8, [] (TestHost& h)
          { twoExciters (h); h.set (ids::wfOn, 1.0f); h.set (ids::netMode, (float) NetMode::Hybrid); h.set (ids::rbOn, 1.0f); h.set (ids::rcOn, 1.0f);
            for (auto* id : { ids::netAB, ids::netBA, ids::netCA }) h.set (id, 0.6f);
            h.set (ids::loopOn, 1.0f); h.set (ids::chorusMix, 0.3f); h.set (ids::delayMix, 0.3f); h.set (ids::reverbMix, 0.3f); } },
        { "Everything, 16 voices", 16, [] (TestHost& h)
          { twoExciters (h); h.set (ids::wfOn, 1.0f); h.set (ids::netMode, (float) NetMode::Hybrid); h.set (ids::rbOn, 1.0f); h.set (ids::rcOn, 1.0f);
            for (auto* id : { ids::netAB, ids::netBA, ids::netCA }) h.set (id, 0.6f);
            h.set (ids::loopOn, 1.0f); h.set (ids::chorusMix, 0.3f); h.set (ids::delayMix, 0.3f); h.set (ids::reverbMix, 0.3f); } },
    };

    std::printf ("\n    | Configuration | Eco | Normal | High |\n    |---|---|---|---|\n");
    for (const auto& c : configs)
    {
        double pct[3] = { 0.0, 0.0, 0.0 };
        for (int q = 0; q < 3; ++q)
        {
            TestHost h (48000.0, 256);
            h.set (ids::voiceCount, 16.0f);
            h.set (ids::envRelease, 4000.0f);
            c.setup (h);
            h.set (ids::quality, (float) q);
            playNotes (h, c.voices);
            pct[q] = measurePercent (h, seconds);
            CHECK (h.render (0.05).finite);
        }
        std::printf ("    | %s | %.1f %% | %.1f %% | %.1f %% |\n", c.name, pct[0], pct[1], pct[2]);
        std::fflush (stdout);
    }
    std::printf ("    (percent of real time, 48 kHz / 256 samples, release build, single core)\n");
    CHECK (true);
}
