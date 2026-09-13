#pragma once

#include "../DspUtils.h"
#include <vector>
#include <cmath>

namespace aeriform::dsp
{

class JunoChorus
{
public:
    enum class Mode { Off = 0, JunoI, JunoII, JunoDual, Dimension };

    struct ModeSpec
    {
        float rateHz;
        float minDelayMs;
        float maxDelayMs;
        bool  sineShape;
    };

    static constexpr ModeSpec kSpecI         { 0.513f, 1.66f, 5.35f, false };
    static constexpr ModeSpec kSpecII        { 0.863f, 1.66f, 5.35f, false };
    static constexpr ModeSpec kSpecDual      { 9.750f, 3.30f, 3.70f, true  };
    static constexpr ModeSpec kSpecDimension { 0.250f, 4.40f, 5.60f, true  };

    void prepare (double sampleRate);
    void reset();
    void setMode (Mode m) noexcept { pendingMode = m; }
    void setRateScale (float s) noexcept { rateScale = std::clamp (s, 0.05f, 10.0f); }
    void setDepth (float d) noexcept { depth = std::clamp (d, 0.0f, 2.0f); }
    void setMix (float m) noexcept { mixTarget = std::clamp (m, 0.0f, 1.0f); }
    void setWidth (float w) noexcept { width = std::clamp (w, 0.0f, 1.0f); }
    void setToneHz (float hz) noexcept { toneHz = std::clamp (hz, 2000.0f, 18000.0f); }
    void setCurve (float c) noexcept { curve = std::clamp (c, 0.0f, 1.0f); }
    void setDrive (float d) noexcept { drive = std::clamp (d, 0.5f, 4.0f); }

    void process (float* left, float* right, int numSamples) noexcept;

private:
    struct DelayLine
    {
        void prepare (int maxSamples)
        {
            buffer.assign ((size_t) juce::nextPowerOfTwo (maxSamples), 0.0f);
            mask = (int) buffer.size() - 1;
            write = 0;
        }

        void clear() noexcept
        {
            std::fill (buffer.begin(), buffer.end(), 0.0f);
            write = 0;
        }

        inline void push (float x) noexcept
        {
            write = (write + 1) & mask;
            buffer[(size_t) write] = x;
        }

        inline float read (float delaySamples) const noexcept
        {
            const float rp = (float) write - delaySamples;
            const int i1 = (int) std::floor (rp);
            const float frac = rp - (float) i1;

            const float y0 = buffer[(size_t) ((i1 - 1) & mask)];
            const float y1 = buffer[(size_t) (i1 & mask)];
            const float y2 = buffer[(size_t) ((i1 + 1) & mask)];
            const float y3 = buffer[(size_t) ((i1 + 2) & mask)];

            const float c0 = y1;
            const float c1 = 0.5f * (y2 - y0);
            const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
            const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
            return ((c3 * frac + c2) * frac + c1) * frac + c0;
        }

        std::vector<float> buffer;
        int mask = 0;
        int write = 0;
    };

    static inline float triangle (float phase) noexcept
    {
        return 1.0f - 4.0f * std::abs (phase - 0.5f);
    }

    static const ModeSpec& specFor (Mode m) noexcept
    {
        switch (m)
        {
            case Mode::JunoII:    return kSpecII;
            case Mode::JunoDual:  return kSpecDual;
            case Mode::Dimension: return kSpecDimension;
            case Mode::JunoI:
            default:              return kSpecI;
        }
    }

    float mapDelay (float u, float minS, float maxS) const noexcept;

    double sampleRate = 44100.0;

    DelayLine lineL, lineR;
    SVF preL, preR, postL, postR;
    OnePole htiL, htiR;
    DcBlocker dcL, dcR;

    float lfoPhase = 0.0f;
    float lfoInc   = 0.0f;

    Mode mode = Mode::Off, pendingMode = Mode::Off;
    float rateScale = 1.0f, depth = 1.0f, curve = 1.0f;
    float drive = 1.2f, toneHz = 8000.0f, width = 1.0f;
    float mixTarget = 0.0f, mix = 0.0f;

    float curMinS = 0.0f, curMaxS = 0.0f;
    float tgtMinS = 0.0f, tgtMaxS = 0.0f;
    float rampCoeff = 0.005f;
};

} // namespace aeriform::dsp
