#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "../Visualization/VisualizerModel.h"
#include "ModMatrix.h"

namespace aeriform
{
class MidiLearn;

/**
    Top-level synthesis engine: voice management, MIDI/MPE handling, per-voice
    physical-modelling chain, global modulation and the stereo effects chain.

    Real-time contract: after prepare(), process() performs no allocation, takes
    no locks (the MPEInstrument's internal CriticalSection is only ever touched
    from the audio thread, so it is uncontended) and does no I/O.
*/
class SynthEngine
{
public:
    static constexpr int kMaxVoices = 16;

    SynthEngine (juce::AudioProcessorValueTreeState& state, VisualizerModel& visualizer);
    ~SynthEngine();

    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void allNotesOff();

    /** Renders one block.
        mainInput      - the DAW main audio input (Aeriform FX); nullptr / empty for the instrument build.
        sidechainInput - the aux / sidechain input (may be nullptr); feeds the existing sidechain features.
        Either input view may be nullptr or have zero channels. */
    void process (juce::AudioBuffer<float>& output,
                  const juce::AudioBuffer<float>* mainInput,
                  const juce::AudioBuffer<float>* sidechainInput,
                  juce::MidiBuffer& midi,
                  const juce::AudioPlayHead::PositionInfo& position,
                  MidiLearn* midiLearn,
                  bool isNonRealtime);

    int getActiveVoiceCount() const noexcept;

    /** Snapshot of the current routing (read from the parameters). Message-thread helper for the GUI. */
    dsp::ModConfig getModConfig() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
} // namespace aeriform
