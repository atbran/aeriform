#pragma once

#include <juce_core/juce_core.h>
#include <atomic>
#include <array>
#include "../Params/ParameterLayout.h"

namespace aeriform
{
/**
    Lock-free bridge between the audio engine and the GUI visualiser.

    The audio thread only writes relaxed atomics and a single-producer ring
    buffer; the GUI thread polls at frame rate. No allocation, no locks.
*/
class VisualizerModel
{
public:
    static constexpr int kMaxVoices   = 16;
    static constexpr int kScopeSize   = 512;   // decimated output samples kept for the ribbon scope
    static constexpr int kDecimation  = 8;

    struct VoiceSnapshot
    {
        std::atomic<float> energy   { 0.0f };   // resonator loop energy 0..1+
        std::atomic<float> pressure { 0.0f };   // breath pressure 0..1
        std::atomic<float> pitchHz  { 0.0f };   // 0 when idle
        std::atomic<int>   active   { 0 };
    };

    std::array<VoiceSnapshot, kMaxVoices> voices;
    std::atomic<float> masterPeak      { 0.0f };
    std::atomic<float> masterPressure  { 0.0f };
    std::atomic<float> masterEnergy    { 0.0f };
    std::atomic<float> limiterGain     { 1.0f };
    std::atomic<int>   activeVoices    { 0 };
    std::atomic<int>   midiActivity    { 0 };

    /** Live modulation amounts (per destination) of the most recently started voice, for the GUI mod rings. */
    std::array<std::atomic<float>, (size_t) ModDest::Count> liveMod {};

    void readLiveMod (std::array<float, (size_t) ModDest::Count>& dest) const noexcept
    {
        for (size_t i = 0; i < dest.size(); ++i) dest[i] = liveMod[i].load (std::memory_order_relaxed);
    }

    // ---- output scope ring buffer (audio thread writes, GUI reads) --------
    void pushScopeSample (float mono) noexcept
    {
        if (++decimCounter < kDecimation) return;
        decimCounter = 0;
        const int w = writePos.load (std::memory_order_relaxed);
        scope[(size_t) w].store (mono, std::memory_order_relaxed);
        writePos.store ((w + 1) % kScopeSize, std::memory_order_release);
    }

    /** Copies the most recent samples (oldest first) into dest. */
    void readScope (float* dest, int count) const noexcept
    {
        const int w = writePos.load (std::memory_order_acquire);
        for (int i = 0; i < count; ++i)
        {
            const int idx = ((w - count + i) % kScopeSize + kScopeSize) % kScopeSize;
            dest[i] = scope[(size_t) idx].load (std::memory_order_relaxed);
        }
    }

private:
    std::array<std::atomic<float>, kScopeSize> scope {};
    std::atomic<int> writePos { 0 };
    int decimCounter = 0;
};
} // namespace aeriform
