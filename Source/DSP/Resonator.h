#pragma once

#include "DspUtils.h"
#include "FractionalDelay.h"
#include "ModalResonator.h"
#include "../Params/ParameterLayout.h"

namespace aeriform::dsp
{
struct ResonatorParams
{
    ResMode type = ResMode::OpenPipe;
    float freqHz = 440.0f;       // target fundamental (already includes tuning, bend, glide, modulation)
    float feedback = 0.9f;       // 0..1
    float damping = 0.35f;       // 0..1
    float brightness = 0.5f;     // 0..1
    float dispersion = 0.0f;     // 0..1
    float inharm = 0.0f;         // 0..1 (modal models)
    float shape = 0.5f;          // 0..1 excitation position / bore shape
    float reflection = 0.3f;     // 0..1 end reflection (hard -> open)
    float saturation = 0.25f;    // 0..1
    float reed = 0.0f;           // 0..1 reed / jet non-linearity at the junction
    float pressure = 0.0f;       // mouth pressure driving the reed (0..1)
    float size = 0.5f;           // 0..1 body size (modal models)
    float pickup = 0.5f;         // 0..1 second pickup position (Width)
    float variationDamping = 0.0f;   // per-voice offsets (added by the voice)
    float variationBright = 0.0f;
};

/**
    Tuned digital waveguide: fractional delay loop with end-reflection loss,
    damping, dispersion allpasses, DC blocking, saturation and an optional reed
    non-linearity at the excitation junction. The loop length is compensated
    for the phase delay of every in-loop filter so the tube stays in tune.
    Types: Open Pipe, Closed Pipe, String, Comb, Dispersive Tube.

    Stability: the linear loop gain never exceeds 1.0 and every in-loop filter is
    passive, the loop signal passes through a bounded saturator and the injected
    excitation is soft-limited, so energy is bounded for any parameter combination.
*/
class Resonator
{
public:
    static constexpr int kMaxDispersionStages = 8;

    void prepare (float sampleRate);
    void reset();

    /** Control-rate update (once per sub-block). */
    void update (const ResonatorParams& p, bool snapLength);

    /** Runs one sample. excitation = input (already filtered); pressureNow = breath pressure 0..1.
        Returns the main output and writes the second pickup tap to tap2. */
    inline float next (float excitation, float pressureNow, float& tap2) noexcept
    {
        delayLen += (targetLen - delayLen) * lenSmooth;

        float d = delay.readLagrange (delayLen);
        if (! combMode)
        {
            const float lp2 = reflLP.process (d);
            d = lp2 + reflHF * (d - lp2);
        }
        d = dampLP.process (d);
        if (ksBlend > 0.0f)
        {
            const float avg = 0.5f * (d + ksPrev);
            ksPrev = d;
            d = lerp (d, avg, ksBlend);
        }
        for (int i = 0; i < activeDispersion; ++i) d = dispersion[i].process (d);
        d = dcBlock.process (d);

        const float sat = (fastTanh (d * drive + satBias) - satBiasOut) * invDrive;
        const float reflected = sat * loopGain;

        const float tl = tiltLP.process (excitation);
        float in = tl + tiltHF * (excitation - tl);
        if (! combMode)
        {
            exciteDelay.push (in);
            in -= 0.85f * exciteDelay.readLinear (combDelay);
        }
        in = fastTanh (in * 0.5f) * 2.0f;

        float x;
        if (reedAmount > 0.0f)
        {
            const float pMouth = pressureNow * 1.2f + in;
            const float dp = pMouth - reflected;
            const float r = std::clamp (0.7f - 0.3f * dp, -1.0f, 1.0f);
            const float reedOut = reflected + dp * r;
            x = lerp (reflected + in, reedOut, reedAmount);
        }
        else
        {
            x = reflected + in;
        }

        delay.push (x);
        energy += 0.002f * (std::fabs (x) - energy);

        tap2 = delay.readLinear (pickupDelay) * outputComp;
        const float tap = stringMode ? d : x;
        lastOut = tap * outputComp;
        return lastOut;
    }

    float getEnergy() const noexcept { return energy; }
    float getLastOutput() const noexcept { return lastOut; }
    float getDelayLength() const noexcept { return delayLen; }
    bool  isFinite() const noexcept { return std::isfinite (lastOut) && std::isfinite (delayLen) && std::isfinite (energy); }

private:
    float sampleRate = 44100.0f;
    FractionalDelay delay, exciteDelay;
    OnePole dampLP, reflLP, tiltLP;
    Allpass1 dispersion[kMaxDispersionStages];
    DcBlocker dcBlock;

    float delayLen = 100.0f, targetLen = 100.0f, lenSmooth = 0.01f;
    float reflHF = 1.0f, tiltHF = 1.0f, ksBlend = 0.0f, ksPrev = 0.0f;
    int activeDispersion = 0;
    bool stringMode = false, combMode = false;
    float drive = 1.0f, invDrive = 1.0f, satBias = 0.0f, satBiasOut = 0.0f;
    float loopGain = 0.9f, reedAmount = 0.0f;
    float combDelay = 20.0f, pickupDelay = 20.0f;
    float outputComp = 1.0f;
    float energy = 0.0f, lastOut = 0.0f;

    float loopPhaseDelay (float omega) const noexcept;
};

/**
    A resonator slot: selects the waveguide or the modal engine by type and
    presents one interface to the network. Switching engines resets the state of
    the newly selected engine (never the running one) so it is click-free apart
    from the natural onset of the new model.
*/
class ResonatorSlot
{
public:
    void prepare (float sampleRate);
    void reset();
    void update (const ResonatorParams& p, bool snapLength);

    inline float next (float in, float pressureNow, float& tap2) noexcept
    {
        // a model change is applied at zero gain: fade out (~2 ms), switch, fade back in
        if (pending)
        {
            fadeGain -= fadeStep;
            if (fadeGain <= 0.0f) { fadeGain = 0.0f; applyPending(); }
        }
        else if (fadeGain < 1.0f)
        {
            fadeGain = std::min (1.0f, fadeGain + fadeStep);
        }
        float y = modal ? bank.next (in, tap2) : waveguide.next (in, pressureNow, tap2);
        if (fadeGain < 1.0f) { y *= fadeGain; tap2 *= fadeGain; }
        return y;
    }

    float getEnergy() const noexcept { return modal ? bank.getEnergy() : waveguide.getEnergy(); }
    bool  isFinite() const noexcept { return modal ? bank.isFinite() : waveguide.isFinite(); }
    bool  isModal() const noexcept { return modal; }

    static bool isModalType (ResMode t) noexcept
    {
        return t == ResMode::ModalBank || t == ResMode::MetallicBar || t == ResMode::Membrane || t == ResMode::FormantBody;
    }

private:
    void applyPending();
    void applyParams (const ResonatorParams& p, bool snapLength);

    Resonator waveguide;
    ModalBank bank;
    bool modal = false;
    ResMode lastType = ResMode::OpenPipe;
    bool pending = false;
    ResonatorParams pendingParams;
    float fadeGain = 1.0f, fadeStep = 0.01f;
};
} // namespace aeriform::dsp
