#pragma once

#include "../DspUtils.h"
#include "../Exciter.h"
#include "WaveOsc.h"
#include "ComplexOsc.h"
#include "NoiseLab.h"
#include "PhysicalExciters.h"
#include "../../Params/ParameterLayout.h"

namespace aeriform::dsp
{
/**
    One exciter slot (A or B). Owns every model's state so switching models
    never allocates; runs at the oversampled rate. Models:
    Off, Breath (the v0.1 exciter), Wave, Complex, 12 noise models, 8 physical
    models and Sidechain. Parameters arrive already modulated from the voice.
*/
class ExciterSlot
{
public:
    struct Params
    {
        ExciterModel model = ExciterModel::Off;
        float level = 1.0f, coarse = 0.0f, fine = 0.0f, keyTrack = 1.0f;
        RetrigMode retrig = RetrigMode::Retrigger;
        float variation = 0.1f, velAmount = 0.5f, pressAmount = 0.3f, drift = 0.05f, phaseDeg = 0.0f, tone = 0.0f;
        float waveShape = 0.0f, wavePw = 0.5f, waveSub = 0.0f, wavePd = 0.0f;
        bool sync = false;
        ComplexOsc::Params cx;
        NoiseLab::Params nz;
        PhysicalExciter::Params ph;
        float scLp = 20000.0f, scHp = 20.0f, scFollow = 0.0f, scTransient = 0.0f;
        ExciterParams breath;          // shared legacy breath parameters
        float pitchModSemis = 0.0f;    // from the matrix (Ex Pitch)
    };

    void prepare (float osRate, int oversampleFactor, int voiceIndex, int slotIndex);
    void reset();

    /** Note-on. noteHz = the tracked note frequency before slot tuning. */
    void noteOn (float velocity, float midiNote, const Params& p);
    void noteOff();

    /** Control-rate update. envelope = breath envelope 0..1, pressureNow = pressure (0..1), aftertouch 0..1. */
    void update (const Params& p, float midiNote, float envelope, float pressureNow, float aftertouch, float velocity, float bpm);

    /** One oversampled sample.
        ext      = sidechain sample (oversampled), shared = shared noise sample,
        breath   = per-sample breath envelope, fmOctaves / pmPhase = audio-rate modulation from the other slot,
        syncPulse = reset phase now (hard sync). */
    inline float next (float ext, float shared, float breath, float fmOctaves, float pmPhase, bool syncPulse) noexcept
    {
        float out = 0.0f;
        wrappedFlag = false;
        switch (params.model)
        {
            case ExciterModel::Off: return 0.0f;
            case ExciterModel::Breath:
                out = breathModel.next (ext, breath) * breathComp;
                break;
            case ExciterModel::Wave:
                if (syncPulse) wave.hardSync();
                out = wave.next (params.waveShape, params.wavePw, params.waveSub, params.wavePd, fmOctaves + driftOct, pmPhase);
                wrappedFlag = wave.wrapped();
                break;
            case ExciterModel::Complex:
                if (syncPulse) complex.hardSync();
                out = complex.next (params.cx, fmOctaves + driftOct, pmPhase);
                wrappedFlag = complex.wrapped();
                break;
            case ExciterModel::Sidechain:
            {
                float x = scHp.highpass (ext);
                x = scLp.lowpass (x);
                if (params.scTransient > 0.0005f)
                {
                    const float slow = scTransLP.process (x);
                    x += params.scTransient * 4.0f * (x - slow);
                }
                if (params.scFollow > 0.0005f)
                {
                    scEnv += (std::fabs (x) > scEnv ? 0.01f : 0.0005f) * (std::fabs (x) - scEnv);
                    x *= lerp (1.0f, std::clamp (scEnv * 3.0f, 0.0f, 1.0f), params.scFollow);
                }
                out = x * 2.0f;
                break;
            }
            default:
                if (isNoise (params.model))
                    out = noise.next (shared);
                else
                    out = physical.next();
                break;
        }

        // physical / noise models follow the breath envelope like the classic exciter unless the model
        // sustains itself (reed, lip, bow, jet already use the envelope as blowing pressure)
        if (! selfSustaining && params.model != ExciterModel::Breath && params.model != ExciterModel::Off)
            out *= envGate + (1.0f - envGate) * breath;

        // tone tilt
        const float tl = toneLP.process (out);
        out = tl + toneHF * (out - tl);

        out *= gain;
        outEnv += (std::fabs (out) > outEnv ? 0.02f : 0.0008f) * (std::fabs (out) - outEnv);
        lastOut = out;
        return out;
    }

    bool wrapped() const noexcept { return wrappedFlag; }
    bool isActive() const noexcept { return params.model != ExciterModel::Off; }
    bool isPitched() const noexcept { return params.model == ExciterModel::Wave || params.model == ExciterModel::Complex; }
    float getEnvelope() const noexcept { return outEnv; }
    float getLastOutput() const noexcept { return lastOut; }
    float chaosX() const noexcept { return complex.chaosStateX(); }
    float chaosY() const noexcept { return complex.chaosStateY(); }
    float getFrequency() const noexcept { return freqHz; }
    ExciterModel getModel() const noexcept { return params.model; }

    static bool isNoise (ExciterModel m) noexcept { return (int) m >= (int) ExciterModel::NoiseWhite && (int) m <= (int) ExciterModel::NoiseMetallic; }
    static bool isPhysical (ExciterModel m) noexcept { return (int) m >= (int) ExciterModel::Reed && (int) m <= (int) ExciterModel::Impact; }

private:
    float sr = 88200.0f;
    int osFactor = 2, voiceIndex = 0, slotIndex = 0;
    Params params;
    ExciterModel lastModel = ExciterModel::Off;

    Exciter breathModel;
    WaveOsc wave;
    ComplexOsc complex;
    NoiseLab noise;
    PhysicalExciter physical;
    SVF scLp, scHp;
    OnePole scTransLP, toneLP;
    SlowRandom driftRnd;
    Noise rng;

    float freqHz = 261.63f, gain = 1.0f, toneHF = 1.0f, driftOct = 0.0f, breathComp = 1.0f, envGate = 0.0f;
    float velocity = 1.0f, varTune = 0.0f, varTone = 0.0f, varParam = 0.0f, randomPhase = 0.0f;
    float scEnv = 0.0f, outEnv = 0.0f, lastOut = 0.0f;
    bool wrappedFlag = false, selfSustaining = false;
};
} // namespace aeriform::dsp
