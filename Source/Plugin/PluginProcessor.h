#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "../Params/ParameterLayout.h"
#include "../DSP/SynthEngine.h"
#include "../Presets/PresetManager.h"
#include "../MIDI/MidiLearn.h"
#include "../Visualization/VisualizerModel.h"

#ifndef AERIFORM_FX
 #define AERIFORM_FX 0
#endif

class AeriformProcessor : public juce::AudioProcessor
{
public:
    AeriformProcessor();
    ~AeriformProcessor() override;

    // ---- AudioProcessor ---------------------------------------------------
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void reset() override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    // MIDI is accepted for optional modulation / MIDI-learn but is never required for audio to pass.
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    /** VST3 bus semantics: the instrument's only input is an aux sidechain; Aeriform FX
        has a true main input (getPluginHasMainInput() -> true) plus the aux sidechain. */
    juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &vst3Extensions; }

    // ---- AERIFORM API -----------------------------------------------------
    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }
    aeriform::SynthEngine& getEngine() noexcept { return engine; }
    aeriform::PresetManager& getPresetManager() noexcept { return presetManager; }
    aeriform::MidiLearn& getMidiLearn() noexcept { return midiLearn; }
    aeriform::VisualizerModel& getVisualizerModel() noexcept { return visualizer; }

    float getEditorScale() const noexcept { return editorScale.load(); }
    void setEditorScale (float s) noexcept { editorScale.store (juce::jlimit (0.5f, 3.0f, s)); }
    int  getEditorPage() const noexcept { return editorPage.load(); }
    void setEditorPage (int p) noexcept { editorPage.store (juce::jlimit (0, 4, p)); }

    /** CPU load of the last blocks as a fraction of real time (0..1+). Audio thread writes, GUI reads. */
    float getCpuLoad() const noexcept { return cpuLoad.load(); }

    /** Builds the complete state XML (parameters, preset name, MIDI learn, editor scale). */
    std::unique_ptr<juce::XmlElement> createStateXml();
    /** Restores from state XML; tolerant of missing / unknown / malformed content. */
    void applyStateXml (const juce::XmlElement& xml);

private:
    struct SidechainExtensions : public juce::VST3ClientExtensions
    {
        bool getPluginHasMainInput() const override { return AERIFORM_FX != 0; }
    };
    SidechainExtensions vst3Extensions;

    /** Bus layout for this build (Aeriform FX: main input + output + aux sidechain). */
    static BusesProperties makeBusesProperties();

    juce::AudioProcessorValueTreeState apvts;
    aeriform::VisualizerModel visualizer;
    aeriform::SynthEngine engine;
    aeriform::PresetManager presetManager;
    aeriform::MidiLearn midiLearn;
    std::atomic<float> editorScale { 1.0f };
    std::atomic<int> editorPage { 0 };
    std::atomic<float> cpuLoad { 0.0f };
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AeriformProcessor)
};
