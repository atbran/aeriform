#pragma once

#include "../DspUtils.h"
#include "../FractionalDelay.h"
#include <array>
#include <cmath>
#include <algorithm>

namespace aeriform::dsp
{

/**
    Room / Chamber Reverb algorithm:
    Combines an asymmetric multi-tap early reflection cluster FIR with a
    tight, 4-line high-density recirculating diffuse tank for intimate, natural
    room acoustic coloration and punchy transient response.
*/
class RoomReverb
{
public:
    void prepare (double sr)
    {
        sampleRate = (float) sr;
        const int maxTank = (int) (0.100f * sampleRate) + 64;

        for (auto& line : lines)
            line.prepare (maxTank);

        diffusers[0].prepare ((int) (0.010f * sampleRate) + 32);
        diffusers[1].prepare ((int) (0.010f * sampleRate) + 32);

        preDelayL.prepare ((int) (0.260f * sampleRate) + 64);
        preDelayR.prepare ((int) (0.260f * sampleRate) + 64);

        erDelayL.prepare ((int) (0.100f * sampleRate) + 64);
        erDelayR.prepare ((int) (0.100f * sampleRate) + 64);

        mixSmooth.setCutoff (10.0f, sampleRate);
        dcL.setCutoff (10.0f, sampleRate);
        dcR.setCutoff (10.0f, sampleRate);

        reset();
    }

    void reset()
    {
        for (auto& line : lines) line.clear();
        for (auto& diff : diffusers) diff.clear();
        preDelayL.clear(); preDelayR.clear();
        erDelayL.clear();  erDelayR.clear();
        dcL.reset();       dcR.reset();
        mixSmooth.reset (mixTarget);
        lfoPhase = 0.0f;
    }

    void setParams (float mix, float size, float decay, float damping, float preDelayMs, float w, float modulation) noexcept
    {
        mixTarget = clamp01 (mix);
        sizeScale = 0.4f + 1.2f * clamp01 (size);
        t60Seconds = 0.15f + 3.0f * clamp01 (decay);
        dampingFactor = clamp01 (damping);
        preDelaySamples = std::clamp (preDelayMs * 0.001f * sampleRate, 0.0f, (float) preDelayL.getMaxDelay() - 8.0f);
        width = clamp01 (w);
        modDepth = clamp01 (modulation) * 2.5f;
        lfoInc = 0.65f / sampleRate;

        // Recompute per-line target lengths and gains
        constexpr float kBaseLengths[4] = { 281.0f, 379.0f, 499.0f, 631.0f };
        const float srScale = sampleRate / 44100.0f;

        for (int i = 0; i < 4; ++i)
        {
            lineLength[(size_t) i] = kBaseLengths[i] * srScale * sizeScale;
            // Loop gain for T60 decay
            const float delaySec = lineLength[(size_t) i] / sampleRate;
            lineGain[(size_t) i] = std::pow (10.0f, -3.0f * delaySec / t60Seconds);
            // Damping low-pass cutoff
            const float dampHz = std::lerp (18000.0f, 1000.0f, dampingFactor);
            damp[(size_t) i].setCutoff (dampHz, sampleRate);
        }

        diffLength[0] = std::max (4, (int) (89.0f * srScale * sizeScale));
        diffLength[1] = std::max (4, (int) (131.0f * srScale * sizeScale));
    }

    void process (float* left, float* right, int numSamples) noexcept
    {
        if (mixTarget <= 0.0005f && mixSmooth.getState() <= 0.0005f)
            return;

        // 8 early reflection taps (4 left, 4 right)
        const float erScale = sampleRate * 0.001f * sizeScale;
        const float erTimesL[4] = { 4.5f * erScale, 11.2f * erScale, 19.8f * erScale, 28.4f * erScale };
        const float erGainsL[4] = { 0.75f, -0.55f, 0.40f, -0.28f };
        const float erTimesR[4] = { 5.8f * erScale, 13.7f * erScale, 22.1f * erScale, 31.6f * erScale };
        const float erGainsR[4] = { 0.70f, 0.52f, -0.42f, 0.25f };

        for (int i = 0; i < numSamples; ++i)
        {
            const float mix = mixSmooth.process (mixTarget);
            const float inL = left[i], inR = right[i];

            preDelayL.push (inL);
            preDelayR.push (inR);
            const float pdL = preDelayL.readLinear (preDelaySamples);
            const float pdR = preDelayR.readLinear (preDelaySamples);

            erDelayL.push (pdL);
            erDelayR.push (pdR);

            // Compute early reflections
            float erL = 0.0f, erR = 0.0f;
            for (int t = 0; t < 4; ++t)
            {
                erL += erDelayL.readLinear (erTimesL[t]) * erGainsL[t];
                erR += erDelayR.readLinear (erTimesR[t]) * erGainsR[t];
            }

            // Input diffusion for tank
            float monoIn = 0.5f * (pdL + pdR);
            for (int d = 0; d < 2; ++d)
            {
                const float del = (float) diffLength[(size_t) d];
                const float delayed = diffusers[(size_t) d].readLinear (del);
                constexpr float c = 0.62f;
                const float v = monoIn - c * delayed;
                diffusers[(size_t) d].push (v);
                monoIn = delayed + c * v;
            }

            // LFO modulation
            lfoPhase += lfoInc;
            if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
            const float mod0 = std::sin (kTwoPi * lfoPhase) * modDepth;
            const float mod1 = std::sin (kTwoPi * (lfoPhase + 0.25f)) * modDepth;
            const float mod2 = std::sin (kTwoPi * (lfoPhase + 0.50f)) * modDepth;
            const float mod3 = std::sin (kTwoPi * (lfoPhase + 0.75f)) * modDepth;

            // Read tank delay lines
            const float l0 = lines[0].readLinear (std::max (2.0f, lineLength[0] + mod0));
            const float l1 = lines[1].readLinear (std::max (2.0f, lineLength[1] + mod1));
            const float l2 = lines[2].readLinear (std::max (2.0f, lineLength[2] + mod2));
            const float l3 = lines[3].readLinear (std::max (2.0f, lineLength[3] + mod3));

            // 4x4 Hadamard feedback matrix (lossless unitary mixing)
            const float s0 = l0 + l1;
            const float d0 = l0 - l1;
            const float s1 = l2 + l3;
            const float d1 = l2 - l3;

            const float fb0 = (s0 + s1) * 0.5f * lineGain[0];
            const float fb1 = (d0 + d1) * 0.5f * lineGain[1];
            const float fb2 = (s0 - s1) * 0.5f * lineGain[2];
            const float fb3 = (d0 - d1) * 0.5f * lineGain[3];

            // Feedback damping and injection
            lines[0].push (damp[0].process (monoIn + fb0));
            lines[1].push (damp[1].process (monoIn + fb1));
            lines[2].push (damp[2].process (monoIn + fb2));
            lines[3].push (damp[3].process (monoIn + fb3));

            // Output mixing: combine early reflections and diffuse tank
            float tankL = (l0 + l2) * 0.5f;
            float tankR = (l1 + l3) * 0.5f;

            float wetL = 0.5f * erL + 0.6f * tankL;
            float wetR = 0.5f * erR + 0.6f * tankR;

            wetL = dcL.process (wetL);
            wetR = dcR.process (wetR);

            // Stereo width
            const float mid  = 0.5f * (wetL + wetR);
            const float side = 0.5f * (wetL - wetR) * width;
            wetL = mid + side;
            wetR = mid - side;

            const float dryGain = 1.0f - 0.5f * mix;
            left[i]  = inL * dryGain + wetL * mix;
            right[i] = inR * dryGain + wetR * mix;
        }
    }

private:
    float sampleRate = 44100.0f;
    std::array<FractionalDelay, 4> lines;
    std::array<OnePole, 4> damp;
    std::array<float, 4> lineLength {}, lineGain {};
    std::array<FractionalDelay, 2> diffusers;
    std::array<int, 2> diffLength {};

    FractionalDelay preDelayL, preDelayR;
    FractionalDelay erDelayL, erDelayR;
    DcBlocker dcL, dcR;
    OnePole mixSmooth;

    float mixTarget = 0.0f, width = 1.0f, modDepth = 0.0f;
    float preDelaySamples = 0.0f, sizeScale = 1.0f, t60Seconds = 1.5f;
    float dampingFactor = 0.4f;
    float lfoPhase = 0.0f, lfoInc = 0.0f;
};

} // namespace aeriform::dsp
