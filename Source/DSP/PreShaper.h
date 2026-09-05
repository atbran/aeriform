#pragma once

#include "DspUtils.h"
#include "../Params/ParameterLayout.h"

namespace aeriform::dsp
{
/**
    Signal conditioning between the exciter mixer and the resonator network:
    the classic exciter high-pass / low-pass (or a band-pass) with resonance and
    key tracking, drive with bias (asymmetry), slew limiting and transient
    emphasis. Runs at the oversampled rate.
*/
class PreShaper
{
public:
    struct Params
    {
        PreFilterType type = PreFilterType::LowHigh;
        float lowpassHz = 7000.0f, highpassHz = 40.0f, keyTrack = 0.5f, resonance = 0.0f;
        float drive = 0.0f, bias = 0.0f, slew = 0.0f, transient = 0.0f;
        float pressureBright = 0.4f;
    };

    void prepare (float sampleRate) noexcept
    {
        sr = sampleRate;
        lp.setSampleRate (sr); hp.setSampleRate (sr); bp.setSampleRate (sr);
        dc.setCutoff (8.0f, sr);
        reset();
    }
    void reset() noexcept { lp.reset(); hp.reset(); bp.reset(); dc.reset(); slewState = 0.0f; x1 = 0.0f; }

    /** noteHz for key tracking, pressureNow (0..1) for pressure-dependent brightness. */
    void update (const Params& p, float noteHz, float pressureNow) noexcept
    {
        params = p;
        const float track = std::pow (std::max (noteHz, 20.0f) / 261.63f, 0.6f * p.keyTrack);
        const float pressBright = std::exp2 (p.pressureBright * 2.5f * (pressureNow - 0.5f));
        const float q = 0.6f + 9.0f * p.resonance * p.resonance;
        const float lpHz = std::clamp (p.lowpassHz * track * pressBright, 60.0f, sr * 0.45f);
        const float hpHz = std::clamp (p.highpassHz * track, 5.0f, sr * 0.3f);
        lp.set (lpHz, std::max (0.65f, q));
        hp.set (hpHz, std::max (0.6f, q));
        bp.set (std::clamp (std::sqrt (lpHz * hpHz), 30.0f, sr * 0.45f), 0.7f + 12.0f * p.resonance);
        driveGain = 1.0f + 8.0f * p.drive * p.drive;
        biasValue = 0.8f * p.bias;
        biasOut = fastTanh (biasValue * driveGain) / driveGain;
        slewRate = p.slew > 0.001f ? std::exp2 (-9.0f * p.slew) * 2.0f * 44100.0f / sr : 1.0e9f;
        transientGain = p.transient * 8.0f * sr / 44100.0f;
    }

    inline float next (float x) noexcept
    {
        const auto& p = params;
        if (p.type == PreFilterType::BandPass) x = bp.bandpass (x) * 1.5f;
        else { x = hp.highpass (x); x = lp.lowpass (x); }

        if (p.transient > 0.0005f)
        {
            const float d = (x - x1) * transientGain;
            x1 = x;
            x += std::clamp (d, -2.0f, 2.0f);
        }
        if (p.slew > 0.001f)
        {
            const float delta = std::clamp (x - slewState, -slewRate, slewRate);
            slewState += delta;
            x = slewState;
        }
        if (p.drive > 0.0005f || std::fabs (p.bias) > 0.0005f)
        {
            x = fastTanh ((x + biasValue) * driveGain) / driveGain - biasOut;
            x = dc.process (x) * std::sqrt (driveGain);
        }
        return x;
    }

private:
    float sr = 88200.0f;
    Params params;
    SVF lp, hp, bp;
    DcBlocker dc;
    float driveGain = 1.0f, biasValue = 0.0f, biasOut = 0.0f, slewRate = 1.0e9f, slewState = 0.0f, transientGain = 0.0f, x1 = 0.0f;
};
} // namespace aeriform::dsp
