#include "TestFramework.h"
#include "TestHelpers.h"
#include <juce_events/juce_events.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <cstring>

static int dumpParameterReference()
{
    aeriform::test::TestHost host;
    const char* sectionNames[] = { "BREATH", "RESONATOR", "MOTION", "SPACE", "MASTER" };
    int lastSection = -1;
    std::printf ("# AERIFORM parameter reference\n\nGenerated from the parameter layout (`AeriformTests --params`).\n");
    for (const auto& info : aeriform::parameterInfos())
    {
        if ((int) info.section != lastSection)
        {
            lastSection = (int) info.section;
            std::printf ("\n## %s\n\n| ID | Name | Range | Default | Description |\n|---|---|---|---|---|\n", sectionNames[lastSection]);
        }
        auto* p = host.processor.getAPVTS().getParameter (info.id);
        juce::String range, def;
        if (info.isChoice)
        {
            if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (p)) { range = c->choices.joinIntoString (" / "); def = c->choices[(int) info.defaultValue]; }
        }
        else if (info.isBool) { range = "off / on"; def = info.defaultValue > 0.5f ? "on" : "off"; }
        else
        {
            range = p->getText (p->convertTo0to1 (info.minValue), 32) + " .. " + p->getText (p->convertTo0to1 (info.maxValue), 32);
            def = p->getText (p->convertTo0to1 (info.defaultValue), 32);
        }
        std::printf ("| `%s` | %s | %s | %s | %s |\n", info.id.toRawUTF8(), info.name.toRawUTF8(), range.toRawUTF8(), def.toRawUTF8(), info.tooltip.toRawUTF8());
    }
    return 0;
}

// Smoke tests are registered with names starting with "smoke_" and are only run
// when --smoke is passed (they render several seconds of audio at three sample rates).
int main (int argc, char** argv)
{
    bool runSmoke = false, runUnit = true;
    const char* filter = nullptr;
    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp (argv[i], "--params") == 0) { juce::ScopedJuceInitialiser_GUI init; return dumpParameterReference(); }
        if (std::strcmp (argv[i], "--smoke") == 0) { runSmoke = true; runUnit = false; }
        else if (std::strcmp (argv[i], "--all") == 0) { runSmoke = true; runUnit = true; }
        else if (std::strncmp (argv[i], "--filter=", 9) == 0) { filter = argv[i] + 9; runSmoke = true; runUnit = true; }
    }

    juce::ScopedJuceInitialiser_GUI juceInit;

    using namespace aeriform::test;
    int run = 0;
    for (auto& t : registry())
    {
        const bool isSmoke = std::strncmp (t.name, "smoke_", 6) == 0;
        if (filter != nullptr && std::strstr (t.name, filter) == nullptr) continue;
        if (isSmoke && ! runSmoke) continue;
        if (! isSmoke && ! runUnit) continue;

        Context::current() = t.name;
        const int failuresBefore = Context::failures();
        std::printf ("[ RUN  ] %s\n", t.name);
        std::fflush (stdout);
        t.fn();
        ++run;
        std::printf ("[ %s ] %s\n", Context::failures() == failuresBefore ? " OK " : "FAIL", t.name);
        std::fflush (stdout);
    }

    std::printf ("\n%d test(s) run, %d check(s), %d failure(s)\n", run, Context::checks(), Context::failures());
    return Context::failures() == 0 ? 0 : 1;
}
