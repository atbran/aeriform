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

    DSP buffers are allocated in prepare(). Known realtime audit debt: JUCE
    MPEInstrument still uses an internal lock and dynamically stored notes.
    These must be replaced/verified before claiming allocation-free, lock-free processing.
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
    using CapturedChord=std::array<int,12>;
    CapturedChord getCapturedChord() const noexcept;
    CapturedChord getHeldChord() const noexcept;
    void setCapturedChord(const CapturedChord&) noexcept;
    int getActiveVoiceCount() const noexcept;

    /** Snapshot of the current routing (read from the parameters). Message-thread helper for the GUI. */
    dsp::ModConfig getModConfig() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
} // namespace aeriform
