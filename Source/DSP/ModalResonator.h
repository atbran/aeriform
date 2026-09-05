#pragma once

#include "DspUtils.h"
#include "../Params/ParameterLayout.h"
#include <array>

namespace aeriform::dsp
{
/**
    Modal resonator bank: up to 12 two-pole resonators at ratios of the
    fundamental. Ratio sets: harmonic (Modal Bank, stretched by inharmonicity),
    free-free bar (Metallic Bar), circular membrane (Membrane) and fixed vocal
    formants scaled by Size (Formant Body). Mode frequencies are set directly,
    so tuning is exact by construction; every mode has |pole| < 1.
*/
class ModalBank
{
public:
    static constexpr int kMaxModes = 12;
    static constexpr float kStrikeNorm = 0.06f;

    struct Params
    {
        ResMode type = ResMode::ModalBank;
        float freqHz = 261.63f, feedback = 0.9f, damping = 0.35f, brightness = 0.5f, inharm = 0.0f, size = 0.5f;
        float saturation = 0.25f, pickup = 0.5f;
    };

    void prepare (float sampleRate) noexcept { sr = sampleRate; reset(); }
    void reset() noexcept
    {
        for (auto& m : modes) m.y1 = m.y2 = 0.0f;
        energy = 0.0f;
    }

    void update (const Params& p) noexcept
    {
        params = p;
        numModes = p.type == ResMode::FormantBody ? 5 : (p.type == ResMode::ModalBank ? 12 : 10);
        // decay: feedback 0..1 -> T60 0.03 .. 14 s (log-ish), damping shortens the high modes
        const float t60 = 0.02f * std::pow (600.0f, clamp01 (p.feedback));   // 20 ms .. 12 s
        const float sizeScale = std::exp2 ((p.size - 0.5f) * 1.6f);
        drive = 1.0f + 6.0f * clamp01 (p.saturation);
        invDrive = 1.0f / drive;

        for (int i = 0; i < numModes; ++i)
        {
            auto& m = modes[(size_t) i];
            float ratio = 1.0f;
            float f;
            switch (p.type)
            {
                case ResMode::MetallicBar: ratio = kBarRatios[i]; break;
                case ResMode::Membrane:    ratio = kMembraneRatios[i]; break;
                case ResMode::FormantBody: break;
                case ResMode::ModalBank:
                default:                   ratio = (float) (i + 1); break;
            }
            if (p.type == ResMode::FormantBody)
                f = kFormantHz[i] * sizeScale;
            else
            {
                // inharmonic stretch: r_k' = r_k * sqrt(1 + B r_k^2)
                const float B = p.inharm * p.inharm * 0.05f;
                ratio *= std::sqrt (1.0f + B * ratio * ratio);
                f = p.freqHz * ratio / (p.type == ResMode::ModalBank ? 1.0f : sizeScale);
            }
            f = std::clamp (f, 20.0f, sr * 0.45f);
            const float k = (float) i / (float) std::max (1, numModes - 1);
            const float modeT60 = t60 * std::exp (-3.5f * p.damping * k) * (p.type == ResMode::Membrane ? 0.6f : 1.0f);
            const float R = std::exp (-6.9078f / (std::max (0.005f, modeT60) * sr));   // T60 -> pole radius
            const float theta = kTwoPi * f / sr;
            m.a1 = 2.0f * R * std::cos (theta);
            m.a2 = -R * R;
            // impulse normalisation: a strike of ~15 sample-units of area rings at unit amplitude in every mode
            // (the 2-pole impulse response peaks at gain / sin(theta)); sustained input is bounded by the AGC + tanh
            m.gain = kStrikeNorm * std::max (0.02f, std::sin (theta));
            // brightness: amplitude tilt across modes; pickup: alternate weighting for the second tap
            const float tilt = std::pow (0.35f + 0.65f * (1.0f - k), (1.0f - p.brightness) * 3.0f);
            m.amp = tilt * (p.type == ResMode::FormantBody ? kFormantAmp[i] : 1.0f);
            m.amp2 = m.amp * std::cos (kPi * (float) i * (0.2f + 0.8f * p.pickup));
        }
        norm = 1.2f / std::sqrt ((float) numModes);
    }

    /** Returns the main output; the second (pickup) tap is written to `tap2`. */
    inline float next (float x, float& tap2) noexcept
    {
        // sustained excitation would pump a high-Q bank without limit: attenuate the input as the bank
        // fills up (energy AGC) and soft-bound the sum, mirroring the waveguide's in-loop saturator
        const float agc = 1.0f / (1.0f + 10.0f * std::max (0.0f, energy - 0.6f));
        const float in = fastTanh (x * drive) * invDrive * agc;
        float sum = 0.0f, sum2 = 0.0f;
        for (int i = 0; i < numModes; ++i)
        {
            auto& m = modes[(size_t) i];
            const float y = m.a1 * m.y1 + m.a2 * m.y2 + m.gain * in;
            m.y2 = m.y1; m.y1 = y;
            sum += y * m.amp;
            sum2 += y * m.amp2;
        }
        sum *= norm; sum2 *= norm;
        sum = fastTanh (sum * 0.5f) * 2.0f;
        sum2 = fastTanh (sum2 * 0.5f) * 2.0f;
        energy += 0.002f * (std::fabs (sum) - energy);
        tap2 = sum2;
        return sum;
    }

    float getEnergy() const noexcept { return energy; }
    bool isFinite() const noexcept { return std::isfinite (energy) && std::isfinite (modes[0].y1); }

private:
    struct Mode { float a1 = 0.0f, a2 = 0.0f, gain = 0.0f, amp = 0.0f, amp2 = 0.0f, y1 = 0.0f, y2 = 0.0f; };
    static constexpr float kBarRatios[kMaxModes]      = { 1.0f, 2.756f, 5.404f, 8.933f, 13.34f, 18.64f, 24.8f, 31.9f, 39.9f, 48.8f, 58.6f, 69.3f };
    static constexpr float kMembraneRatios[kMaxModes] = { 1.0f, 1.594f, 2.136f, 2.296f, 2.653f, 2.918f, 3.156f, 3.501f, 3.600f, 3.652f, 4.060f, 4.154f };
    static constexpr float kFormantHz[kMaxModes]      = { 650.0f, 1080.0f, 2650.0f, 2900.0f, 3250.0f, 0, 0, 0, 0, 0, 0, 0 };
    static constexpr float kFormantAmp[kMaxModes]     = { 1.0f, 0.6f, 0.35f, 0.3f, 0.2f, 0, 0, 0, 0, 0, 0, 0 };

    float sr = 44100.0f, drive = 1.0f, invDrive = 1.0f, norm = 0.3f, energy = 0.0f;
    int numModes = 12;
    Params params;
    std::array<Mode, kMaxModes> modes {};
};
} // namespace aeriform::dsp
