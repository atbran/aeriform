#pragma once

#include "../DspUtils.h"
#include "../FractionalDelay.h"
#include <array>
#include <cmath>
#include <algorithm>

namespace aeriform::dsp
{

/**
    Dattorro figure-8 plate reverberator.
    High echo density, rapid diffusion, and characteristic metallic sheen
    modeled on the classic 1997 Dattorro plate topology.
*/
class DattorroPlate
{
public:
    void prepare (double sr)
    {
        sampleRate = (float) sr;
        const float srScale = sampleRate / 29761.0f;

        preDelayL.prepare ((int) (0.260f * sampleRate) + 64);
        preDelayR.prepare ((int) (0.260f * sampleRate) + 64);

        // Input diffusers
        constexpr int kDiffMax[4] = { 200, 160, 500, 400 };
        for (int i = 0; i < 4; ++i)
            inDiffusers[(size_t) i].prepare ((int) (kDiffMax[i] * srScale * 2.0f) + 32);

        // Tank allpass and delays
        tankModAp1.prepare ((int) (1200 * srScale * 2.0f) + 64);
        tankModAp2.prepare ((int) (1500 * srScale * 2.0f) + 64);
        tankAp1.prepare    ((int) (2500 * srScale * 2.0f) + 64);
        tankAp2.prepare    ((int) (3500 * srScale * 2.0f) + 64);

        tankDelay1.prepare ((int) (6000 * srScale * 2.0f) + 64);
        tankDelay2.prepare ((int) (5000 * srScale * 2.0f) + 64);
        tankDelay3.prepare ((int) (6000 * srScale * 2.0f) + 64);
        tankDelay4.prepare ((int) (5000 * srScale * 2.0f) + 64);

        dampL.setCutoff (10000.0f, sampleRate);
        dampR.setCutoff (10000.0f, sampleRate);
        dcL.setCutoff (10.0f, sampleRate);
        dcR.setCutoff (10.0f, sampleRate);
        mixSmooth.setCutoff (10.0f, sampleRate);

        reset();
    }

    void reset()
    {
        preDelayL.clear(); preDelayR.clear();
        for (auto& d : inDiffusers) d.clear();
        tankModAp1.clear(); tankModAp2.clear();
        tankAp1.clear();    tankAp2.clear();
        tankDelay1.clear(); tankDelay2.clear();
        tankDelay3.clear(); tankDelay4.clear();
        dampL.reset();      dampR.reset();
        dcL.reset();        dcR.reset();
        mixSmooth.reset (mixTarget);
        lfoPhase1 = 0.0f;
        lfoPhase2 = 0.0f;
    }

    void setParams (float mix, float size, float decay, float damping, float preDelayMs, float w, float modulation) noexcept
    {
        mixTarget = clamp01 (mix);
        sizeScale = 0.5f + 1.1f * clamp01 (size);
        decayGain = std::clamp (0.40f + 0.55f * clamp01 (decay), 0.0f, 0.96f);
        dampingFactor = clamp01 (damping);
        preDelaySamples = std::clamp (preDelayMs * 0.001f * sampleRate, 0.0f, (float) preDelayL.getMaxDelay() - 8.0f);
        width = clamp01 (w);
        modExcursion = 1.0f + 12.0f * clamp01 (modulation);

        const float dampHz = std::lerp (18000.0f, 1200.0f, dampingFactor);
        dampL.setCutoff (dampHz, sampleRate);
        dampR.setCutoff (dampHz, sampleRate);

        lfoInc1 = 1.0f / sampleRate;
        lfoInc2 = 0.88f / sampleRate;
    }

    void process (float* left, float* right, int numSamples) noexcept
    {
        if (mixTarget <= 0.0005f && mixSmooth.getState() <= 0.0005f)
            return;

        const float srScale = (sampleRate / 29761.0f) * sizeScale;

        // Input diffuser delays
        const float dIn[4] = { 142.0f * srScale, 107.0f * srScale, 379.0f * srScale, 277.0f * srScale };
        constexpr float cIn[4] = { 0.75f, 0.75f, 0.625f, 0.625f };

        // Tank base lengths
        const float dModAp1 = 672.0f * srScale;
        const float dDelay1 = 4453.0f * srScale;
        const float dAp1    = 1800.0f * srScale;
        const float dDelay2 = 3720.0f * srScale;

        const float dModAp2 = 908.0f * srScale;
        const float dDelay3 = 4217.0f * srScale;
        const float dAp2    = 2656.0f * srScale;
        const float dDelay4 = 3163.0f * srScale;

        // Multi-tap pickup delays
        const float t1_1 = 266.0f * srScale,  t1_2 = 2974.0f * srScale;
        const float t_ap1 = 1913.0f * srScale;
        const float t2_1 = 1996.0f * srScale;
        const float t3_1 = 1990.0f * srScale;
        const float t_ap2 = 187.0f * srScale;
        const float t4_1 = 1066.0f * srScale;

        const float t3_2 = 353.0f * srScale,  t3_3 = 3627.0f * srScale;
        const float t_ap4 = 1228.0f * srScale;
        const float t4_2 = 2673.0f * srScale;
        const float t1_3 = 2111.0f * srScale;
        const float t_ap3 = 335.0f * srScale;
        const float t2_2 = 121.0f * srScale;

        for (int i = 0; i < numSamples; ++i)
        {
            const float mix = mixSmooth.process (mixTarget);
            const float inL = left[i], inR = right[i];

            preDelayL.push (inL);
            preDelayR.push (inR);
            const float pdL = preDelayL.readLinear (preDelaySamples);
            const float pdR = preDelayR.readLinear (preDelaySamples);
            float monoIn = 0.5f * (pdL + pdR);

            // 4 series input allpasses
            for (int d = 0; d < 4; ++d)
            {
                const float delayed = inDiffusers[(size_t) d].readLinear (dIn[d]);
                const float c = cIn[d];
                const float v = monoIn - c * delayed;
                inDiffusers[(size_t) d].push (v);
                monoIn = delayed + c * v;
            }

            // LFOs for tank modulation
            lfoPhase1 += lfoInc1; if (lfoPhase1 >= 1.0f) lfoPhase1 -= 1.0f;
            lfoPhase2 += lfoInc2; if (lfoPhase2 >= 1.0f) lfoPhase2 -= 1.0f;
            const float mod1 = std::sin (kTwoPi * lfoPhase1) * modExcursion;
            const float mod2 = std::cos (kTwoPi * lfoPhase2) * modExcursion;

            // Tank Left Half:
            // input + cross-feedback from Delay4
            const float nodeL = monoIn + tankDelay4.readLinear (dDelay4) * decayGain;

            // Modulated Allpass 1
            const float ap1Del = tankModAp1.readLinear (std::max (2.0f, dModAp1 + mod1));
            constexpr float cTankMod = 0.70f;
            const float vMod1 = nodeL + cTankMod * ap1Del;
            tankModAp1.push (vMod1);
            const float outModAp1 = ap1Del - cTankMod * vMod1;

            // Delay 1
            tankDelay1.push (outModAp1);
            const float outDelay1 = tankDelay1.readLinear (dDelay1);

            // Damping 1
            const float dampOutL = dampL.process (outDelay1);

            // Allpass 1
            const float apDel1 = tankAp1.readLinear (dAp1);
            constexpr float cTankAp = 0.50f;
            const float vAp1 = dampOutL - cTankAp * apDel1;
            tankAp1.push (vAp1);
            const float outAp1 = apDel1 + cTankAp * vAp1;

            // Delay 2
            tankDelay2.push (outAp1);
            const float outDelay2 = tankDelay2.readLinear (dDelay2);

            // Tank Right Half:
            // input + cross-feedback from Delay2
            const float nodeR = monoIn + outDelay2 * decayGain;

            // Modulated Allpass 2
            const float ap2Del = tankModAp2.readLinear (std::max (2.0f, dModAp2 + mod2));
            const float vMod2 = nodeR + cTankMod * ap2Del;
            tankModAp2.push (vMod2);
            const float outModAp2 = ap2Del - cTankMod * vMod2;

            // Delay 3
            tankDelay3.push (outModAp2);
            const float outDelay3 = tankDelay3.readLinear (dDelay3);

            // Damping 2
            const float dampOutR = dampR.process (outDelay3);

            // Allpass 2
            const float apDel2 = tankAp2.readLinear (dAp2);
            const float vAp2 = dampOutR - cTankAp * apDel2;
            tankAp2.push (vAp2);
            const float outAp2 = apDel2 + cTankAp * vAp2;

            // Delay 4
            tankDelay4.push (outAp2);

            // Multi-tap pickup
            float wetL = 0.28f * (tankDelay1.readLinear (t1_1)
                                + tankDelay1.readLinear (t1_2)
                                - tankAp1.readLinear    (t_ap1)
                                + tankDelay2.readLinear (t2_1)
                                - tankDelay3.readLinear (t3_1)
                                - tankAp2.readLinear    (t_ap2)
                                - tankDelay4.readLinear (t4_1));

            float wetR = 0.28f * (tankDelay3.readLinear (t3_2)
                                + tankDelay3.readLinear (t3_3)
                                - tankAp2.readLinear    (t_ap4)
                                + tankDelay4.readLinear (t4_2)
                                - tankDelay1.readLinear (t1_3)
                                - tankAp1.readLinear    (t_ap3)
                                - tankDelay2.readLinear (t2_2));

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

    FractionalDelay preDelayL, preDelayR;
    std::array<FractionalDelay, 4> inDiffusers;

    FractionalDelay tankModAp1, tankModAp2;
    FractionalDelay tankAp1, tankAp2;
    FractionalDelay tankDelay1, tankDelay2, tankDelay3, tankDelay4;

    OnePole dampL, dampR;
    DcBlocker dcL, dcR;
    OnePole mixSmooth;

    float mixTarget = 0.0f, width = 1.0f;
    float preDelaySamples = 0.0f, sizeScale = 1.0f;
    float decayGain = 0.75f, dampingFactor = 0.4f;
    float modExcursion = 4.0f;
    float lfoPhase1 = 0.0f, lfoInc1 = 0.0f;
    float lfoPhase2 = 0.0f, lfoInc2 = 0.0f;
};

} // namespace aeriform::dsp
