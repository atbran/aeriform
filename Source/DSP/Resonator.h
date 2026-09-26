#pragma once

#include "DspUtils.h"
#include "FractionalDelay.h"
#include "ModalResonator.h"
#include "NonlinearElement.h"
#include "PipeResonator.h"
#include "ModularFilters.h"
#include "../Params/ParameterLayout.h"

namespace aeriform::dsp
{
struct ResonatorParams
{
    NonlinearParams nonlinear;
    PipeParams pipe;             // used only when type == ResMode::Pipe
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
    float additionalPhaseDelay = 0.0f;
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
    void setNonlinearElement (NonlinearElement* element) noexcept { nonlinear = element; }
    double getLoopMeanSquare() const noexcept { return loopMeanSquare; }

    void setLoopFilter(ModularFilters* f,FilterPosition pos,int lane) noexcept {loopFilter=f;loopPosition=pos;filterLane=lane;}
    void prepare (float sampleRate);
    void reset();

    /** Control-rate update (once per sub-block). */
    void update (const ResonatorParams& p, bool snapLength);

    /** Runs one sample. excitation = input (already filtered); pressureNow = breath pressure 0..1.
        Returns the main output and writes the second pickup tap to tap2. */
    inline float next (float excitation, float pressureNow, float& tap2) noexcept
    {
        delayLen += (targetLen - delayLen) * lenSmooth;

        // Limit only Tension's additional movement; legacy tuning/glide stays
        // unchanged. Fractional state bounds the ratio even if nominal pitch moves.
        const float targetFraction = nonlinear ? 1.0f-nonlinear->delayScale() : 0.0f;
        if (targetFraction > 0.0f || tensionFraction > 0.0f)
        {
            const float step = 0.02f/std::max (2.0f, delayLen);
            tensionFraction += std::clamp (targetFraction-tensionFraction, -step, step);
            tensionFraction = std::clamp (tensionFraction, 0.0f, 1.0f-1.0f/NonlinearElement::maxTensionRatio);
        }
        const float readLength = effectiveDelayLength();
        float d = delay.readLagrange (readLength);
        // Pre acts on the returned junction sample before the loss/filter chain.
        // See docs/NONLINEAR_LOOP_IMPLEMENTATION.md for the actual topology.
        if (nonlinear) d = nonlinear->at (NonlinearPosition::Pre, d, nonlinear->getLoopEnergyRms());
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
        if(loopFilter)d=loopFilter->at(loopPosition,d,filterLane);

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
            // Reed valve: the reflection coefficient r scales the bore wave, and closes (r -> 1) as the
            // pressure difference across the reed grows. (r used to scale the mouth side, which left the
            // junction reflecting only ~30 % of the bore wave: too lossy for the reed ever to speak.)
            const float pMouth = pressureNow * 1.2f + in;
            const float dp = reflected - pMouth;
            const float r = std::clamp (0.7f - 0.3f * dp, -1.0f, 1.0f);
            const float reedOut = pMouth + dp * r;
            x = lerp (reflected + in, reedOut, reedAmount);
        }
        else
        {
            x = reflected + in;
        }

        if (nonlinear) x = nonlinear->at (NonlinearPosition::Post, x, nonlinear->getLoopEnergyRms());
        delay.push (x);
        loopMeanSquare = static_cast<double> (x) * x;
        energy += 0.002f * (std::fabs (x) - energy);

        tap2 = delay.readLinear (pickupDelay) * outputComp;
        const float tap = d;
        lastOut = tap * outputComp;
        if (nonlinear)
        {
            lastOut = nonlinear->at (NonlinearPosition::Pickup, lastOut, nonlinear->getLoopEnergyRms(), 0);
            tap2 = nonlinear->at (NonlinearPosition::Pickup, tap2, nonlinear->getLoopEnergyRms(), 1);
        }
        return lastOut;
    }

    float getEnergy() const noexcept { return energy; }
    float getLastOutput() const noexcept { return lastOut; }
    float getDelayLength() const noexcept { return effectiveDelayLength(); }
    float getNominalDelayLength() const noexcept { return delayLen; }
    float getTensionRatio() const noexcept { return delayLen/effectiveDelayLength(); }
    bool  isFinite() const noexcept { return std::isfinite (lastOut) && std::isfinite (delayLen) && std::isfinite (energy); }

private:
    ModularFilters* loopFilter=nullptr;FilterPosition loopPosition=FilterPosition::ResALoop;int filterLane=0;
    NonlinearElement* nonlinear = nullptr;
    double loopMeanSquare = 0.0;
    float sampleRate = 44100.0f;
    FractionalDelay delay, exciteDelay;
    OnePole dampLP, reflLP, tiltLP;
    Allpass1 dispersion[kMaxDispersionStages];
    DcBlocker dcBlock;

    float effectiveDelayLength() const noexcept
    { return tensionFraction > 0.0f ? std::max (2.0f, delayLen*(1.0f-tensionFraction)) : delayLen; }
    float tensionFraction = 0.0f;
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
    void prepare (float sampleRate, uint32_t noiseSeed = 0x9E3779B9u);
    void reset();
    void update (const ResonatorParams& p, bool snapLength);

    void setLoopFilter(ModularFilters* f,FilterPosition pos,int lane) noexcept {loopFilter=f;loopPosition=pos;filterLane=lane;waveguide.setLoopFilter(f,pos,lane);}
    inline float next (float in, float pressureNow, float& tap2) noexcept
    {
        nonlinear.beginSample();
        if (nonlinear.active()) nonlinear.setLoopEnergyRms (loopEnergy.rms());
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
        if((modal||pipeMode)&&loopFilter)in=loopFilter->at(loopPosition,in,filterLane);
        float y;
        if (pipeMode)
        {
            // PIPE has its own continuous blown excitation, scaled by the shared amp envelope.
            y = pipe.next (in, blowEnv);
            tap2 = y;
            loopEnergy.observe (pipe.getLoopMeanSquare());
        }
        else
        {
            y = modal ? bank.next (in, tap2) : waveguide.next (in, pressureNow, tap2);
            loopEnergy.observe (modal ? bank.getLoopMeanSquare() : waveguide.getLoopMeanSquare());
        }
        nonlinear.endSample();
        if (fadeGain < 1.0f) { y *= fadeGain; tap2 *= fadeGain; }
        return y;
    }

    /** Shared voice amp-envelope level for the PIPE model's blown excitation (per sample). */
    void setBlowEnvelope (float env) noexcept { blowEnv = env; }
    bool isPipe() const noexcept { return pipeMode; }
    const PipeResonator::Telemetry& getPipeTelemetry() const noexcept { return pipe.getTelemetry(); }
    float getTensionRatio() const noexcept { return pipeMode ? 1.0f : (modal ? bank.getTensionRatio() : waveguide.getTensionRatio()); }
    float getLoopEnergyRms() const noexcept { return loopEnergy.rms(); }
    const NonlinearElement& getNonlinearElement() const noexcept { return nonlinear; }
    float getEnergy() const noexcept { return pipeMode ? pipe.getEnergy() : (modal ? bank.getEnergy() : waveguide.getEnergy()); }
    bool  isFinite() const noexcept { return pipeMode ? pipe.isFinite() : (modal ? bank.isFinite() : waveguide.isFinite()); }
    bool  isModal() const noexcept { return modal; }

    static bool isModalType (ResMode t) noexcept
    {
        return t == ResMode::ModalBank || t == ResMode::MetallicBar || t == ResMode::Membrane || t == ResMode::FormantBody;
    }

    /** Linear loop gain used by the network to normalise coupling into this slot. */
    static float couplingLoopGain (const ResonatorParams& r) noexcept
    {
        return r.type == ResMode::Pipe ? PipeResonator::nominalLoopGain (r.pipe, r.freqHz)
                                       : 0.7f + 0.3f * clamp01 (r.feedback);
    }

private:
    void applyPending();
    void applyParams (const ResonatorParams& p, bool snapLength);

    ModularFilters* loopFilter=nullptr;FilterPosition loopPosition=FilterPosition::ResALoop;int filterLane=0;
    NonlinearElement nonlinear;
    LoopEnergyProbe loopEnergy;
    Resonator waveguide;
    ModalBank bank;
    PipeResonator pipe;
    bool modal = false, pipeMode = false;
    float blowEnv = 0.0f;
    ResMode lastType = ResMode::OpenPipe;
    bool pending = false;
    ResonatorParams pendingParams;
    float fadeGain = 1.0f, fadeStep = 0.01f;
};
} // namespace aeriform::dsp
