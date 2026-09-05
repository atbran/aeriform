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

    void process (juce::AudioBuffer<float>& output,
                  const juce::AudioBuffer<float>* externalInput,
                  juce::MidiBuffer& midi,
                  const juce::AudioPlayHead::PositionInfo& position,
                  MidiLearn* midiLearn,
                  bool isNonRealtime);

    using EffectiveValues=std::array<float,(size_t)kNumParams>;
    void setEffectiveValues(const EffectiveValues* values, bool skipOutputStage=false) noexcept;
    void processRange(juce::AudioBuffer<float>&,const juce::AudioBuffer<float>*,juce::MidiBuffer&,
                      const juce::AudioPlayHead::PositionInfo&,MidiLearn*,bool,int midiOffset);
    int getActiveVoiceCount() const noexcept;

    /** Snapshot of the current routing (read from the parameters). Message-thread helper for the GUI. */
    dsp::ModConfig getModConfig() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
} // namespace aeriform
