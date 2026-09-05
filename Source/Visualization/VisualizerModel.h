#pragma once

#include <juce_core/juce_core.h>
#include <atomic>
#include <array>
#include "../Params/ParameterLayout.h"

namespace aeriform
{
/** Single-producer lock-free ring buffer of decimated samples for a GUI scope. */
class ScopeBuffer
{
public:
    static constexpr int kSize = 512;

    void push (float v, int decimation) noexcept
    {
        if (++counter < decimation) return;
        counter = 0;
        const int w = writePos.load (std::memory_order_relaxed);
        data[(size_t) w].store (v, std::memory_order_relaxed);
        writePos.store ((w + 1) % kSize, std::memory_order_release);
    }

    /** Copies the most recent `count` samples (oldest first). */
    void read (float* dest, int count) const noexcept
    {
        const int w = writePos.load (std::memory_order_acquire);
        for (int i = 0; i < count; ++i)
        {
            const int idx = ((w - count + i) % kSize + kSize) % kSize;
            dest[i] = data[(size_t) idx].load (std::memory_order_relaxed);
        }
    }

private:
    std::array<std::atomic<float>, kSize> data {};
    std::atomic<int> writePos { 0 };
    int counter = 0;
};

/**
    Lock-free bridge between the audio engine and the GUI visualiser.

    The audio thread only writes relaxed atomics and single-producer ring
    buffers; the GUI thread polls at frame rate. No allocation, no locks.
*/
class VisualizerModel
{
public:
    static constexpr int kMaxVoices   = 16;
    static constexpr int kScopeSize   = ScopeBuffer::kSize;
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
    std::atomic<float> preLimiterPeak{0}, preLimiterRms{0}, preLimiterMean{0};
    std::atomic<float> postLimiterRms{0}, postLimiterMean{0}, limiterFraction{0}, ceilingFraction{0};
    std::atomic<float> limiterGain     { 1.0f };
    std::atomic<int>   activeVoices    { 0 };
    std::atomic<int>   midiActivity    { 0 };

    // v2.1: per-resonator energies, network feedback energy, governor, exciter levels (newest voice)
    std::array<std::atomic<float>, 3> resonatorEnergy {};
    std::atomic<float> networkEnergy { 0.0f };
    std::atomic<float> governorGain  { 1.0f };
    std::atomic<float> exciterAEnv   { 0.0f };
    std::atomic<float> exciterBEnv   { 0.0f };
    std::atomic<float> sidechainEnv  { 0.0f };
    std::array<std::atomic<int>, 3> resonatorRunning {};

    /** Live modulation amounts (per destination) of the most recently started voice, for the GUI mod rings. */
    std::array<std::atomic<float>, (size_t) ModDest::Count> liveMod {};

    void readLiveMod (std::array<float, (size_t) ModDest::Count>& dest) const noexcept
    {
        for (size_t i = 0; i < dest.size(); ++i) dest[i] = liveMod[i].load (std::memory_order_relaxed);
    }

    // ---- scopes (audio thread writes, GUI reads) ---------------------------
    ScopeBuffer outputScope;     // stereo mix
    ScopeBuffer exciterAScope;   // newest voice, exciter A output
    ScopeBuffer exciterBScope;   // newest voice, exciter B output
    ScopeBuffer foldScope;       // newest voice, after the wavefolder (network input)

    void pushScopeSample (float mono) noexcept { outputScope.push (mono, kDecimation); }
    void readScope (float* dest, int count) const noexcept { outputScope.read (dest, count); }
};
} // namespace aeriform
