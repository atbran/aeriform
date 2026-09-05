#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "../Params/ParameterLayout.h"
#include "../DSP/SynthEngine.h"
#include "../Presets/PresetManager.h"
#include "../MIDI/MidiLearn.h"
#include "../Visualization/VisualizerModel.h"
#include "../State/PatchStateManager.h"
#include "../DSP/Effects/OutputStage.h"

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

    /** Declares the audio input as a VST3 sidechain (aux) bus so hosts offer it for routing into the instrument. */
    juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &vst3Extensions; }

    // ---- AERIFORM API -----------------------------------------------------
    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }
    aeriform::SynthEngine& getEngine() noexcept { return engine; }
    aeriform::PresetManager& getPresetManager() noexcept { return presetManager; }
    aeriform::MidiLearn& getMidiLearn() noexcept { return midiLearn; }
    aeriform::VisualizerModel& getVisualizerModel() noexcept { return visualizer; }

    aeriform::PatchStateManager& getPatchTools() noexcept { return patchTools; }

    float getEditorScale() const noexcept { return editorScale.load(); }
    void setEditorScale (float s) noexcept { editorScale.store (juce::jlimit (0.5f, 3.0f, s)); }
    int  getEditorPage() const noexcept { return editorPage.load(); }
    void setEditorPage (int p) noexcept { editorPage.store (juce::jlimit (0, 5, p)); }

    /** CPU load of the last blocks as a fraction of real time (0..1+). Audio thread writes, GUI reads. */
    float getCpuLoad() const noexcept { return cpuLoad.load(); }

    /** Builds the complete state XML (parameters, preset name, MIDI learn, editor scale). */
    std::unique_ptr<juce::XmlElement> createStateXml();
    /** Restores from state XML; tolerant of missing / unknown / malformed content. */
    void applyStateXml (const juce::XmlElement& xml);

private:
    struct SidechainExtensions : public juce::VST3ClientExtensions
    {
        bool getPluginHasMainInput() const override { return false; }
    };
    SidechainExtensions vst3Extensions;

    juce::AudioProcessorValueTreeState apvts;
    aeriform::VisualizerModel visualizer;
    aeriform::SynthEngine engine;
    aeriform::PresetManager presetManager;
    aeriform::MidiLearn midiLearn;
    aeriform::PatchStateManager patchTools;
    aeriform::VisualizerModel morphVisualizer;
    aeriform::SynthEngine morphEngine;
    aeriform::dsp::OutputStage morphOutput;
    juce::AudioBuffer<float> morphBuffer, morphInput;
    juce::MidiBuffer primeMidi;
    std::array<unsigned char,2048> heldNotes{};
    aeriform::PatchStateManager::Values effectiveA{},effectiveB{};
    bool wasDeep=false;
    int preparedBlock=512;
    void processMorph(juce::AudioBuffer<float>&,const juce::AudioBuffer<float>*,juce::MidiBuffer&,const juce::AudioPlayHead::PositionInfo&);
    std::atomic<float> editorScale { 1.0f };
    std::atomic<int> editorPage { 5 };
    std::atomic<float> cpuLoad { 0.0f };
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AeriformProcessor)
};
