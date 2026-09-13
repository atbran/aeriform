#include "TestFramework.h"
#include "TestHelpers.h"
#include "DSP/Effects/Chorus.h"
#include "DSP/Effects/JunoChorus.h"
#include "DSP/Effects/Reverb.h"
#include "DSP/Effects/RoomReverb.h"
#include "DSP/Effects/DattorroPlate.h"
#include "Plugin/PluginEditor.h"
#include "GUI/Panels/SpacePanel.h"
#include <cmath>
#include <vector>

using namespace aeriform;
using namespace aeriform::dsp;
using namespace aeriform::test;

namespace
{
template <class T>
std::vector<T*> findDescendants (juce::Component& c)
{
    std::vector<T*> found;
    for (auto* child : c.getChildren())
    {
        if (auto* p = dynamic_cast<T*> (child))
            found.push_back (p);
        auto more = findDescendants<T> (*child);
        found.insert (found.end(), more.begin(), more.end());
    }
    return found;
}
} // namespace

AERIFORM_TEST (chorus_all_types_finite_and_distinct)
{
    for (double sr : { 44100.0, 48000.0 })
    {
        Chorus chorus;
        chorus.prepare (sr);

        std::vector<std::vector<float>> outputs;
        const int numSamples = (int) (sr * 0.5);

        for (int typeIdx = 0; typeIdx < (int) ChorusType::Count; ++typeIdx)
        {
            chorus.reset();
            chorus.setType ((ChorusType) typeIdx);
            chorus.setParams (1.0f, 0.4f, 0.4f, 0.8f);

            std::vector<float> l (numSamples), r (numSamples);
            for (int i = 0; i < numSamples; ++i)
            {
                const float s = 0.2f * std::sin ((float) i * 0.05f) + 0.15f * std::sin ((float) i * 0.11f);
                l[i] = s;
                r[i] = s;
            }

            chorus.process (l.data(), r.data(), numSamples);

            for (int i = 0; i < numSamples; ++i)
            {
                CHECK (std::isfinite (l[i]));
                CHECK (std::isfinite (r[i]));
                CHECK (std::abs (l[i]) < 2.5f);
                CHECK (std::abs (r[i]) < 2.5f);
            }

            // Antiphase stereo decorrelation check for Juno and Dimension
            if (typeIdx >= (int) ChorusType::JunoI)
            {
                double stereoDiff = 0.0;
                double monoSumEnergy = 0.0;
                for (int i = 100; i < numSamples; ++i)
                {
                    stereoDiff += std::abs (l[i] - r[i]);
                    const float mono = 0.5f * (l[i] + r[i]);
                    monoSumEnergy += mono * mono;
                }
                // Must have strong stereo spread
                CHECK (stereoDiff > 1.0);
                // Mono sum must remain strong (not cancelled)
                CHECK (monoSumEnergy > 0.5);
            }

            outputs.push_back (l);
        }

        // Verify distinct signatures across all 5 types
        for (size_t i = 0; i < outputs.size(); ++i)
        {
            for (size_t j = i + 1; j < outputs.size(); ++j)
            {
                double diff = 0.0;
                for (size_t k = 200; k < (size_t) numSamples; ++k)
                    diff += std::abs (outputs[i][k] - outputs[j][k]);
                CHECK (diff > 0.5);
            }
        }
    }
}

AERIFORM_TEST (chorus_seamless_mode_switching_under_signal)
{
    Chorus chorus;
    chorus.prepare (48000.0);
    chorus.setParams (1.0f, 0.4f, 0.4f, 0.8f);

    float l[128], r[128];
    for (int block = 0; block < 100; ++block)
    {
        for (int i = 0; i < 128; ++i)
        {
            l[i] = 0.2f * std::sin ((float) (block * 128 + i) * 0.07f);
            r[i] = l[i];
        }

        if (block % 15 == 0)
        {
            const int nextType = (block / 15) % (int) ChorusType::Count;
            chorus.setType ((ChorusType) nextType);
        }

        chorus.process (l, r, 128);

        for (int i = 0; i < 128; ++i)
        {
            CHECK (std::isfinite (l[i]));
            CHECK (std::isfinite (r[i]));
            CHECK (std::abs (l[i]) < 2.0f);
            CHECK (std::abs (r[i]) < 2.0f);
        }
    }
}

AERIFORM_TEST (reverb_all_types_finite_distinct_and_stable)
{
    for (double sr : { 44100.0, 48000.0 })
    {
        FdnReverb reverb;
        reverb.prepare (sr);

        std::vector<std::vector<float>> tails;
        const int numSamples = (int) (sr * 0.6);

        for (int typeIdx = 0; typeIdx < (int) ReverbType::Count; ++typeIdx)
        {
            reverb.reset();
            reverb.setType ((ReverbType) typeIdx);
            reverb.setParams (1.0f, 0.6f, 0.5f, 0.4f, 10.0f, 1.0f, 0.3f);

            std::vector<float> l (numSamples, 0.0f), r (numSamples, 0.0f);
            l[0] = 1.0f; // Unit impulse
            r[0] = 1.0f;

            reverb.process (l.data(), r.data(), numSamples);

            double energy = 0.0;
            for (int i = 0; i < numSamples; ++i)
            {
                CHECK (std::isfinite (l[i]));
                CHECK (std::isfinite (r[i]));
                CHECK (std::abs (l[i]) < 2.0f);
                CHECK (std::abs (r[i]) < 2.0f);
                energy += l[i] * l[i] + r[i] * r[i];
            }

            // Must have sustained reverberant energy
            CHECK (energy > 0.01);
            tails.push_back (l);
        }

        // Verify all 3 reverb types produce distinct acoustics
        for (size_t i = 0; i < tails.size(); ++i)
        {
            for (size_t j = i + 1; j < tails.size(); ++j)
            {
                double diff = 0.0;
                for (size_t k = 100; k < (size_t) numSamples; ++k)
                    diff += std::abs (tails[i][k] - tails[j][k]);
                CHECK (diff > 0.2);
            }
        }
    }
}

AERIFORM_TEST (space_effects_parameters_and_ui_binding)
{
    TestHost h;
    auto& tools = h.processor.getPatchTools();

    // Verify parameter definitions and defaults
    CHECK (h.get (ids::chorusType) == 0.0f);
    CHECK (h.get (ids::reverbType) == 0.0f);

    // Verify parameter edits and undo/redo
    tools.perform ("Set Chorus Juno II", [&] {
        tools.setParameter (ids::chorusType, 2.0f);
        tools.setParameter (ids::reverbType, 1.0f);
    });
    CHECK (h.get (ids::chorusType) == 2.0f);
    CHECK (h.get (ids::reverbType) == 1.0f);

    tools.undo.undo();
    CHECK (h.get (ids::chorusType) == 0.0f);
    CHECK (h.get (ids::reverbType) == 0.0f);

    tools.undo.redo();
    CHECK (h.get (ids::chorusType) == 2.0f);
    CHECK (h.get (ids::reverbType) == 1.0f);

    // Verify UI editor layout and controls
    std::unique_ptr<juce::AudioProcessorEditor> editor (h.processor.createEditor());
    CHECK (editor != nullptr);

    auto spacePanels = findDescendants<SpacePanel> (*editor);
    CHECK (! spacePanels.empty());

    auto buttons = findDescendants<juce::Button> (*editor);
    bool foundJuno = false;
    for (auto* b : buttons)
    {
        if (b->getButtonText().containsIgnoreCase ("JUNO"))
            foundJuno = true;
    }
    CHECK (foundJuno);
}
