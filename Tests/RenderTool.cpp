// Offline audition renderer: `AeriformTests --render=<dir>` writes one stereo WAV per
// scenario (factory wind/pipe presets and PIPE-model configurations) playing the same
// phrase, so DSP changes can be compared by ear and by scripts/audition_analyze.py.
#include "TestHelpers.h"
#include "Presets/FactoryPresets.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <cstdio>

namespace aeriform::test
{
namespace
{
    struct NoteEvent { double start, length; int note, velocity; };

    // phrase: legato-ish line, a repeated fast figure, then a sustained chord
    const std::vector<NoteEvent>& phrase()
    {
        static const std::vector<NoteEvent> events {
            { 0.00, 1.40, 60, 96 }, { 1.50, 0.45, 64, 90 }, { 2.00, 0.45, 67, 100 }, { 2.50, 1.60, 72, 110 },
            { 4.40, 0.18, 67, 80 }, { 4.62, 0.18, 67, 95 }, { 4.84, 0.18, 67, 110 }, { 5.06, 0.60, 69, 100 },
            { 6.00, 2.40, 48, 90 }, { 6.00, 2.40, 55, 90 }, { 6.00, 2.40, 64, 90 },
            { 9.00, 3.00, 45, 100 } };
        return events;
    }
    constexpr double kTotalSeconds = 13.5;

    bool writeWav (const juce::File& file, const juce::AudioBuffer<float>& audio, double sr)
    {
        file.deleteFile();
        std::unique_ptr<juce::FileOutputStream> stream (file.createOutputStream());
        if (stream == nullptr) return false;
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::AudioFormatWriter> writer (wav.createWriterFor (stream.get(), sr, 2, 24, {}, 0));
        if (writer == nullptr) return false;
        stream.release();
        return writer->writeFromAudioSampleBuffer (audio, 0, audio.getNumSamples());
    }

    using Setup = std::function<void (TestHost&)>;

    void loadFactory (TestHost& host, const juce::String& name)
    {
        auto& pm = host.processor.getPresetManager();
        const auto& entries = pm.getEntries();
        for (int i = 0; i < (int) entries.size(); ++i)
            if (entries[(size_t) i].name == name) { pm.loadPreset (i); return; }
        std::printf ("  ! preset not found: %s\n", name.toRawUTF8());
    }

    juce::AudioBuffer<float> renderScenario (const Setup& setup, double sr)
    {
        TestHost host (sr, 256);
        setup (host);
        host.render (0.1);   // let smoothed parameters settle

        const int total = (int) std::ceil (kTotalSeconds * sr);
        juce::AudioBuffer<float> out (2, total);
        out.clear();

        struct Edge { long sample; int note, velocity; bool on; };
        std::vector<Edge> edges;
        for (const auto& e : phrase())
        {
            edges.push_back ({ (long) std::lround (e.start * sr), e.note, e.velocity, true });
            edges.push_back ({ (long) std::lround ((e.start + e.length) * sr), e.note, 0, false });
        }

        for (long pos = 0; pos < total; pos += host.blockSize)
        {
            for (const auto& e : edges)
                if (e.sample >= pos && e.sample < pos + host.blockSize)
                {
                    if (e.on) host.noteOn (e.note, e.velocity, 1, (int) (e.sample - pos));
                    else      host.noteOff (e.note, 1, (int) (e.sample - pos));
                }
            host.renderBlock();
            const int n = (int) std::min<long> (host.blockSize, total - pos);
            for (int ch = 0; ch < 2; ++ch)
                out.copyFrom (ch, (int) pos, host.buffer, ch, 0, n);
        }
        return out;
    }
}

int runRender (const char* outDir)
{
    const juce::File dir = juce::File::getCurrentWorkingDirectory().getChildFile (outDir);
    dir.createDirectory();
    const double sr = 48000.0;

    std::vector<std::pair<juce::String, Setup>> scenarios;
    for (const char* name : { "Airy Flute", "Warm Wooden Pipe", "Reed Song", "Cathedral Organ", "Whistle Lead",
                              "Brass Horn", "Bass Pipe", "Steam Vent", "Industrial Horn", "Soft Breath Pad",
                              "Steam Flute", "Pan Pipes", "Steam Calliope", "Flue Organ 8'", "Steam Whistle",
                              "Shakuhachi Air", "Reed Clarinet", "Reed Sax" })
        scenarios.push_back ({ juce::String ("preset-") + juce::String (name).replaceCharacter (' ', '-').removeCharacters ("'").toLowerCase(),
                               [n = juce::String (name)] (TestHost& h) { loadFactory (h, n); } });

    // PIPE model: resonator A switched to PIPE on the Init patch, breath exciter left as shipped.
    scenarios.push_back ({ "pipe-init", [] (TestHost& h) { loadFactory (h, "Init"); h.set ("res_mode", 9.0f); } });
    // PIPE with a long decay and a mostly-DC blow: the sustained "steam pipe" register.
    scenarios.push_back ({ "pipe-sustain", [] (TestHost& h)
    {
        loadFactory (h, "Init"); h.set ("res_mode", 9.0f);
        h.set ("exa_model", 0.0f);
        h.set ("res_pressure", 70.0f); h.set ("res_dcnoise", 30.0f); h.set ("res_rt", 4.0f);
        h.set ("res_lp", 6000.0f); h.set ("res_sat_drive", 4.0f); h.set ("rev_mix", 0.25f);
    } });
    // PIPE breathy: noisy blow, short decay (pan-pipe / shakuhachi territory).
    scenarios.push_back ({ "pipe-breathy", [] (TestHost& h)
    {
        loadFactory (h, "Init"); h.set ("res_mode", 9.0f);
        h.set ("exa_model", 0.0f);
        h.set ("res_pressure", 60.0f); h.set ("res_dcnoise", 85.0f); h.set ("res_rt", 0.8f);
        h.set ("res_exc_cut", 3500.0f); h.set ("rev_mix", 0.25f);
    } });

    // Diagnostics (AERIFORM_RENDER_DIAG=1): the chain reduced to one exciter and resonator A.
    if (juce::SystemStats::getEnvironmentVariable ("AERIFORM_RENDER_DIAG", {}).isNotEmpty())
    {
        auto dry = [] (TestHost& h)
        {
            loadFactory (h, "Init");
            h.set ("rev_mix", 0.0f); h.set ("res_body_mix", 0.0f);
            h.set ("exc_attack_click", 0.0f); h.set ("exc_release_noise", 0.0f);
            h.set ("art_instability", 0.0f); h.set ("art_variation", 0.0f); h.set ("art_flow_pitch", 0.0f);
            h.set ("exc_breath_random", 0.0f); h.set ("exc_turb", 0.0f);
        };
        scenarios.clear();
        scenarios.push_back ({ "diag-noise-open", [dry] (TestHost& h) { dry (h); } });
        scenarios.push_back ({ "diag-pluck-open", [dry] (TestHost& h)
        { dry (h); h.set ("exc_noise", 0.0f); h.set ("exc_pressure", 0.0f); h.set ("exc_pluck", 1.0f); h.set ("res_feedback", 0.97f); } });
        scenarios.push_back ({ "diag-pluck-open-noshape", [dry] (TestHost& h)
        { dry (h); h.set ("exc_noise", 0.0f); h.set ("exc_pressure", 0.0f); h.set ("exc_pluck", 1.0f); h.set ("res_feedback", 0.97f); h.set ("res_shape", 0.0f); } });
        scenarios.push_back ({ "diag-noise-open-reflect0", [dry] (TestHost& h) { dry (h); h.set ("res_reflect", 0.0f); } });
        scenarios.push_back ({ "diag-noise-open-noexcfilt", [dry] (TestHost& h) { dry (h); h.set ("exc_keytrack", 0.0f); h.set ("exc_hp", 10.0f); } });
        scenarios.push_back ({ "diag-noise-open-shape1", [dry] (TestHost& h) { dry (h); h.set ("res_shape", 1.0f); } });
        scenarios.push_back ({ "diag-noise-open-bright0", [dry] (TestHost& h) { dry (h); h.set ("res_brightness", 0.0f); } });
        scenarios.push_back ({ "diag-noise-closed", [dry] (TestHost& h) { dry (h); h.set ("res_mode", 1.0f); } });
        scenarios.push_back ({ "diag-noise-open-nopre", [dry] (TestHost& h) { dry (h); h.set ("exc_lp", 20000.0f); h.set ("exc_hp", 10.0f); h.set ("exc_keytrack", 0.0f); h.set ("art_press_bright", 0.0f); } });
    }

    for (const auto& [name, setup] : scenarios)
    {
        auto audio = renderScenario (setup, sr);
        const float peak = audio.getMagnitude (0, audio.getNumSamples());
        const auto file = dir.getChildFile (name + ".wav");
        const bool ok = writeWav (file, audio, sr);
        std::printf ("%-28s peak %6.3f  %s\n", name.toRawUTF8(), peak, ok ? "ok" : "WRITE FAILED");
        std::fflush (stdout);
    }
    return 0;
}
} // namespace aeriform::test
