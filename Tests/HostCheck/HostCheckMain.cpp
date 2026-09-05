// AeriformHostCheck: a minimal VST3 host that loads a built .vst3 bundle the way
// a DAW would, then exercises it: scan, instantiate (twice), prepare at several
// sample rates and block sizes, play notes, render, verify finite / bounded /
// non-silent output, round-trip state, and open the editor.
//
//   AeriformHostCheck <path-to-AERIFORM.vst3> [--editor]

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <cstdio>
#include <cmath>

namespace
{
int failures = 0;

void check (bool ok, const char* what)
{
    std::printf ("  [%s] %s\n", ok ? " OK " : "FAIL", what);
    if (! ok) ++failures;
}

struct RenderStats { float peak = 0.0f; double rms = 0.0; bool finite = true; };

RenderStats render (juce::AudioPluginInstance& plugin, double sampleRate, int blockSize, double seconds, bool playNotes)
{
    plugin.setPlayConfigDetails (0, 2, sampleRate, blockSize);
    plugin.prepareToPlay (sampleRate, blockSize);

    juce::AudioBuffer<float> buffer (2, blockSize);
    juce::MidiBuffer midi;
    const int totalBlocks = (int) std::ceil (seconds * sampleRate / blockSize);
    const int noteOffBlock = totalBlocks / 2;
    RenderStats stats;
    double sumSq = 0.0; long n = 0;

    for (int b = 0; b < totalBlocks; ++b)
    {
        midi.clear();
        if (playNotes && b == 0)
            for (int note : { 48, 55, 60, 64, 67, 72, 76, 79 })
                midi.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100), 0);
        if (playNotes && b == noteOffBlock)
            for (int note : { 48, 55, 60, 64, 67, 72, 76, 79 })
                midi.addEvent (juce::MidiMessage::noteOff (1, note), 0);

        buffer.clear();
        plugin.processBlock (buffer, midi);

        for (int ch = 0; ch < 2; ++ch)
        {
            const float* d = buffer.getReadPointer (ch);
            for (int i = 0; i < blockSize; ++i)
            {
                if (! std::isfinite (d[i])) stats.finite = false;
                const float a = std::fabs (d[i]);
                if (a > stats.peak) stats.peak = a;
                sumSq += (double) d[i] * d[i]; ++n;
            }
        }
    }
    plugin.releaseResources();
    stats.rms = n > 0 ? std::sqrt (sumSq / (double) n) : 0.0;
    return stats;
}
} // namespace

int main (int argc, char** argv)
{
    if (argc < 2)
    {
        std::printf ("usage: AeriformHostCheck <path-to-plugin.vst3> [--editor]\n");
        return 2;
    }
    const bool openEditor = argc > 2 && juce::String (argv[2]) == "--editor";

    juce::ScopedJuceInitialiser_GUI juceInit;
    juce::AudioPluginFormatManager formats;
    formats.addFormat (new juce::VST3PluginFormat());

    const juce::File file (juce::File::getCurrentWorkingDirectory().getChildFile (juce::String (argv[1])));
    std::printf ("Scanning %s\n", file.getFullPathName().toRawUTF8());

    juce::OwnedArray<juce::PluginDescription> descriptions;
    for (auto* f : formats.getFormats())
        f->findAllTypesForFile (descriptions, file.getFullPathName());
    check (descriptions.size() == 1, "scan finds exactly one plug-in in the bundle");
    if (descriptions.isEmpty()) return 1;

    const auto& desc = *descriptions[0];
    std::printf ("  name=%s  manufacturer=%s  version=%s  isInstrument=%d\n",
                 desc.name.toRawUTF8(), desc.manufacturerName.toRawUTF8(), desc.version.toRawUTF8(), (int) desc.isInstrument);
    check (desc.isInstrument, "descriptor reports an instrument");

    juce::String error;
    auto instance = formats.createPluginInstance (desc, 48000.0, 512, error);
    check (instance != nullptr, ("instantiate #1 " + error).toRawUTF8());
    if (instance == nullptr) return 1;

    {
        auto second = formats.createPluginInstance (desc, 44100.0, 256, error);
        check (second != nullptr, "instantiate #2 (repeated instantiation)");
        second.reset();
    }

    check (instance->acceptsMidi(), "accepts MIDI");
    check (instance->getParameters().size() > 50, "exposes parameters");

    // Silence with no notes
    auto silent = render (*instance, 48000.0, 512, 0.5, false);
    check (silent.finite, "silent render finite");
    check (silent.peak < 1.0e-3f, "no output without notes");

    for (double sr : { 44100.0, 48000.0, 96000.0 })
        for (int bs : { 32, 256, 1024 })
        {
            auto s = render (*instance, sr, bs, 2.0, true);
            char msg[160];
            std::snprintf (msg, sizeof (msg), "render sr=%.0f bs=%d finite=%d peak=%.3f rms=%.4f", sr, bs, (int) s.finite, s.peak, s.rms);
            check (s.finite && s.peak < 4.0f && s.rms > 1.0e-4, msg);
        }

    // State round trip
    juce::MemoryBlock state;
    instance->getStateInformation (state);
    check (state.getSize() > 100, "state is non-trivial");
    auto* firstParam = instance->getParameters()[0];
    const float original = firstParam->getValue();
    firstParam->setValueNotifyingHost (original < 0.5f ? 0.9f : 0.1f);
    instance->setStateInformation (state.getData(), (int) state.getSize());
    check (std::fabs (firstParam->getValue() - original) < 1.0e-4f, "state restore returns parameter to saved value");

    if (openEditor)
    {
        check (instance->hasEditor(), "has editor");
        for (int round = 0; round < 3; ++round)
        {
            std::unique_ptr<juce::AudioProcessorEditor> editor (instance->createEditorIfNeeded());
            check (editor != nullptr, "editor created (repeated open/close)");
            if (editor == nullptr) break;
            juce::DocumentWindow window ("AeriformHostCheck", juce::Colours::black, juce::DocumentWindow::allButtons);
            window.setContentNonOwned (editor.get(), true);
            window.setVisible (true);
            // render audio while the editor is open, as a host would
            instance->prepareToPlay (48000.0, 256);
            juce::AudioBuffer<float> buf (2, 256);
            juce::MidiBuffer m;
            m.addEvent (juce::MidiMessage::noteOn (1, 60 + round * 4, (juce::uint8) 100), 0);
            for (int b = 0; b < 60; ++b)
            {
                buf.clear();
                instance->processBlock (buf, m);
                m.clear();
                juce::MessageManager::getInstance()->runDispatchLoopUntil (10);
            }
            instance->releaseResources();
            window.clearContentComponent();
            instance->editorBeingDeleted (editor.get());
        }
        check (true, "editor opened and closed 3 times with audio running");
    }

    instance.reset();
    std::printf ("\n%s (%d failure(s))\n", failures == 0 ? "HOST CHECK PASSED" : "HOST CHECK FAILED", failures);
    return failures == 0 ? 0 : 1;
}
