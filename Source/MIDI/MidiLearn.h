#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <array>

namespace aeriform
{
/**
    MIDI-CC to parameter mapping with a "learn" workflow.

    - GUI arms learning for a parameter (armLearn). The next CC received on the
      audio thread is captured and the mapping is completed by the GUI poll.
    - Mappings are stored as atomics (audio thread reads, GUI writes).
    - Applying a CC calls setValueNotifyingHost on the mapped parameter; this is
      the conventional JUCE approach and is host-safe for the plug-in formats we build.
*/
class MidiLearn
{
public:
    static constexpr int kNumCCs = 128;
    static constexpr int kUnmapped = -1;

    explicit MidiLearn (juce::AudioProcessorValueTreeState& state);

    // ---- GUI thread -----------------------------------------------------
    void armLearn (const juce::String& paramID);
    void cancelLearn();
    bool isLearning() const noexcept { return learnArmed.load() != 0; }
    juce::String getLearningParam() const;
    /** Call periodically from the GUI. Completes an armed learn if a CC arrived. Returns true when a mapping was made. */
    bool pollLearn();
    void clearMapping (const juce::String& paramID);
    void clearAll();
    int  getMappedCC (const juce::String& paramID) const;   // kUnmapped if none
    juce::String getMappedParam (int cc) const;

    // ---- audio thread ---------------------------------------------------
    /** Handles a controller message. Returns true if it was consumed by a mapping. */
    bool handleController (int cc, int value) noexcept;

    // ---- state ----------------------------------------------------------
    std::unique_ptr<juce::XmlElement> toXml() const;
    void fromXml (const juce::XmlElement* xml);

private:
    juce::AudioProcessorValueTreeState& apvts;
    std::vector<juce::RangedAudioParameter*> params;          // index -> parameter
    std::array<std::atomic<int>, kNumCCs> ccToParam;          // cc -> param index or kUnmapped
    std::atomic<int> learnArmed { 0 };
    std::atomic<int> learnParamIndex { kUnmapped };
    std::atomic<int> learnCapturedCC { kUnmapped };

    int indexOf (const juce::String& paramID) const;
};
} // namespace aeriform
