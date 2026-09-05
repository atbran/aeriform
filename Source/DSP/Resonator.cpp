#include "Resonator.h"

namespace aeriform::dsp
{
namespace
{
    constexpr float kMinFreq = 16.0f;
    constexpr float kMinLoopGain = 0.70f;
    // Never above unity: the linear loop is passive by construction (no growth, no limit cycles).
    // Blown self-oscillation comes from the reed junction driven by pressure, not from loop gain.
    constexpr float kMaxLoopGain = 1.0f;
}

void Resonator::prepare (float sr)
{
    sampleRate = sr;
    const int maxDelay = (int) (sr / kMinFreq * 1.05f) + 64;
    delay.prepare (maxDelay);
    exciteDelay.prepare (maxDelay);
    // Very low cutoff: an in-loop high-pass adds phase lead that de-tunes the upper partials
    // relative to the (compensated) fundamental, so keep its effect negligible above ~30 Hz.
    dcBlock.setCutoff (1.5f, sr);
    lenSmooth = 1.0f - std::exp (-1.0f / (0.0025f * sr));
    reset();
}

void Resonator::reset()
{
    delay.clear();
    exciteDelay.clear();
    dampLP.reset();
    reflLP.reset();
    tiltLP.reset();
    for (auto& ap : dispersion) ap.reset();
    dcBlock.reset();
    ksPrev = 0.0f;
    energy = 0.0f;
    lastOut = 0.0f;
    delayLen = targetLen;
}

float Resonator::loopPhaseDelay (float omega) const noexcept
{
    float tau = dampLP.phaseDelay (omega);
    tau += dcBlock.phaseDelay (omega);

    if (! combMode)
    {
        // end reflection blend: hf + (1 - hf) * H_lp2
        const float a = reflLP.getCoefficient();
        const float b = 1.0f - a;
        const float dre = 1.0f - b * std::cos (omega), dim = b * std::sin (omega);
        const float mag2 = dre * dre + dim * dim;
        const float hre = a * dre / mag2, him = -a * dim / mag2;
        const float re = reflHF + (1.0f - reflHF) * hre;
        const float im = (1.0f - reflHF) * him;
        const float phase = std::atan2 (im, re);
        tau += omega > 1.0e-6f ? -phase / omega : 0.0f;
    }

    if (ksBlend > 0.0f)
    {
        const float re = (1.0f - ksBlend) + ksBlend * 0.5f * (1.0f + std::cos (omega));
        const float im = -ksBlend * 0.5f * std::sin (omega);
        const float phase = std::atan2 (im, re);
        tau += omega > 1.0e-6f ? -phase / omega : 0.0f;
    }

    if (activeDispersion > 0)
        tau += (float) activeDispersion * dispersion[0].phaseDelay (omega);

    return tau;
}

void Resonator::update (const ResonatorParams& p, bool snapLength)
{
    const float f0 = std::clamp (p.freqHz, kMinFreq, sampleRate * 0.125f);
    const float keyRatio = f0 / 261.63f;
    const ResMode type = p.type;

    combMode = type == ResMode::Comb;
    stringMode = type == ResMode::String;
    const bool dispersiveTube = type == ResMode::DispersiveTube;

    // ---- damping / reflection / tilt filters --------------------------------
    const float damping = clamp01 (p.damping + p.variationDamping);
    const float dampTrack = std::clamp (std::pow (keyRatio, 0.45f), 0.3f, 3.0f);
    const float dampCutoff = 20000.0f * std::pow (600.0f / 20000.0f, damping) * dampTrack * (combMode ? 1.4f : 1.0f);
    dampLP.setCutoff (dampCutoff, sampleRate);

    const float reflCutoff = 2200.0f * std::clamp (std::pow (keyRatio, 0.3f), 0.5f, 2.0f);
    reflLP.setCutoff (reflCutoff, sampleRate);
    reflHF = 1.0f - 0.9f * clamp01 (p.reflection);

    const float bright = clamp01 (p.brightness + p.variationBright);
    tiltLP.setCutoff (std::clamp (1000.0f * std::pow (keyRatio, 0.5f), 200.0f, 8000.0f), sampleRate);
    tiltHF = std::exp2 ((bright - 0.5f) * 3.0f);   // +/- 9 dB

    // ---- topology ----------------------------------------------------------------
    ksBlend = stringMode ? 0.6f : 0.0f;
    // comb: reflection parameter selects the polarity (odd harmonics above 50 %)
    const bool inverted = type == ResMode::ClosedPipe || (combMode && p.reflection > 0.5f);
    const float polarity = inverted ? -1.0f : 1.0f;
    const float periodSamples = sampleRate / f0 * (inverted ? 0.5f : 1.0f);

    // ---- dispersion ------------------------------------------------------------------
    const float disp = clamp01 (p.dispersion) * (stringMode ? 1.0f : (dispersiveTube ? 1.0f : 0.7f));
    activeDispersion = combMode ? 0 : (disp > 0.002f ? (dispersiveTube ? kMaxDispersionStages : 4) : 0);
    const float c = -(dispersiveTube ? 0.75f : 0.6f) * disp;
    for (auto& ap : dispersion) ap.setCoefficient (c);

    // ---- saturation ------------------------------------------------------------------
    drive = 0.5f + 3.5f * clamp01 (p.saturation);
    invDrive = 1.0f / drive;
    satBias = 0.35f * clamp01 (p.pressure);
    satBiasOut = fastTanh (satBias);
    outputComp = std::sqrt (drive) * 0.9f;

    loopGain = (kMinLoopGain + (kMaxLoopGain - kMinLoopGain) * clamp01 (p.feedback)) * polarity;
    reedAmount = combMode ? 0.0f : clamp01 (p.reed);

    // ---- tuning: compensate the loop filters' phase delay ----------------------------
    const float omega = kTwoPi * f0 / sampleRate;
    const float tau = loopPhaseDelay (omega)+p.additionalPhaseDelay;
    targetLen = std::clamp (periodSamples - tau, 2.0f, (float) delay.getMaxDelay());
    if (snapLength) delayLen = targetLen;

    combDelay = std::clamp ((0.03f + 0.47f * clamp01 (p.shape)) * targetLen, 1.0f, (float) exciteDelay.getMaxDelay());
    pickupDelay = std::clamp ((0.05f + 0.9f * clamp01 (p.pickup)) * targetLen, 1.0f, (float) delay.getMaxDelay());
}

// ---------------------------------------------------------------------------
void ResonatorSlot::prepare (float sampleRate)
{
    waveguide.prepare (sampleRate);
    bank.prepare (sampleRate);
    modal = false;
    lastType = ResMode::OpenPipe;
    pending = false;
    fadeGain = 1.0f;
    fadeStep = 1.0f / (0.002f * sampleRate);
}

void ResonatorSlot::reset()
{
    waveguide.reset();
    bank.reset();
    if (pending) applyPending();
    fadeGain = 1.0f;
}

void ResonatorSlot::update (const ResonatorParams& p, bool snapLength)
{
    if (p.type != lastType || pending)
    {
        // the switch lands in next() once the output has faded out; a note start (snapLength) switches at once
        lastType = p.type;
        pendingParams = p;
        pending = true;
        if (snapLength) { applyPending(); fadeGain = 1.0f; }
        return;
    }
    applyParams (p, snapLength);
}

void ResonatorSlot::applyPending()
{
    pending = false;
    const bool wantModal = isModalType (pendingParams.type);
    if (wantModal != modal)
    {
        // clear the engine we are switching to; the old one is simply abandoned
        if (wantModal) bank.reset(); else waveguide.reset();
        modal = wantModal;
    }
    applyParams (pendingParams, true);
}

void ResonatorSlot::applyParams (const ResonatorParams& p, bool snapLength)
{
    if (modal)
    {
        ModalBank::Params m;
        m.type = p.type; m.freqHz = p.freqHz; m.feedback = p.feedback; m.damping = clamp01 (p.damping + p.variationDamping);
        m.brightness = clamp01 (p.brightness + p.variationBright); m.inharm = p.inharm; m.size = p.size;
        m.saturation = p.saturation; m.pickup = p.pickup;
        bank.update (m);
    }
    else
    {
        waveguide.update (p, snapLength);
    }
}
} // namespace aeriform::dsp
