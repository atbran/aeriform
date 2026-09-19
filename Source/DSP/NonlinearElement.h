#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include "../Params/ParameterLayout.h"

namespace aeriform::dsp
{
struct NonlinearParams
{
    bool on = false, adaa = true;
    NonlinearModel model = NonlinearModel::Saturate;
    NonlinearPosition position = NonlinearPosition::Post;
    float drive = 0.0f, amount = 50.0f, bias = 0.0f;
    float frequencyHz = 220.0f;
};

/** Slot-owned nonlinear processing with independent histories for positions and
    modal/tap lanes. Fixed storage only. */
class NonlinearElement
{
public:
    using Model = NonlinearModel;
    struct Sample { float x = 0.0f, y = 0.0f; };

    void prepare (double sampleRate, int) noexcept
    {
        sr = sampleRate;
        smooth = static_cast<float> (1.0 - std::exp (-1.0 / (0.005 * sr)));
        fadeStep = static_cast<float> (1.0 / (0.002 * sr));
        dcR = std::exp (-6.283185307179586 * 5.0 / sr);
        frictionRecovery = static_cast<float> (1.0 - std::exp (-32.0 / (0.5 * sr)));
        reset();
    }
    void reset() noexcept
    {
        lanes = {}; loopRms = 0.0f; last = {};
        frictionScale = frictionTarget = 1.0f; frictionStep = 0.0f;
        frictionSamples = 0; frictionEnergy = {}; lastFrictionEnergy = {};
        current = desired; transitioning = false;
        fade = current.on ? 1.0f : 0.0f;
        gain = targetGain; amount = targetAmount; bias = targetBias;
    }
    void configure (const NonlinearParams& p, bool snap = false) noexcept
    {
        desired = p;
        desired.drive = std::clamp (p.drive, 0.0f, 100.0f);
        desired.amount = std::clamp (p.amount, 0.0f, 100.0f);
        desired.bias = std::clamp (p.bias, -100.0f, 100.0f);
        // Approximately -3 dB/octave above A3, without attenuating small signals.
        const float keytrack = 1.0f / std::sqrt (std::max (1.0f, p.frequencyHz / 220.0f));
        targetGain = std::clamp (std::exp2 (desired.drive * 0.05f) * keytrack, 1.0f, 32.0f);
        targetAmount = desired.amount * 0.01f;
        targetBias = desired.bias * 0.0002f; // +/- 0.02 sample units
        if (snap) { reset(); return; }
        transitioning = current.on != desired.on || current.model != desired.model
                     || current.position != desired.position
                     || (current.model == Model::Saturate && current.adaa != desired.adaa);
    }
    void setModel (Model m) noexcept { auto p = desired; p.model = m; configure (p); }
    void setParams (float d, float m, float b) noexcept
    { auto p = desired; p.on = true; p.drive = d; p.amount = m; p.bias = b; configure (p); }
    const NonlinearParams& configuration() const noexcept { return desired; }

    // Exactly once per slot sample, never once per modal lane or pickup tap.
    void beginSample() noexcept
    {
        frictionScale = std::clamp (frictionScale + frictionStep, 0.0f, 1.0f);
        if (transitioning)
        {
            fade = std::max (0.0f, fade - fadeStep);
            if (fade <= 0.0f)
            {
                current = desired; transitioning = false;
                // Prime the ADAA interval on re-entry; retain DC/filter state.
                for (auto& position : lanes) for (auto& lane : position) { lane.primed = false; lane.frictionPrimed = false; }
            }
        }
        else fade = std::min (current.on ? 1.0f : 0.0f, fade + fadeStep);
        approach (gain, targetGain); approach (amount, targetAmount); approach (bias, targetBias);
    }
    bool active() const noexcept
    { return (current.model == Model::Saturate || current.model == Model::Hysteresis || current.model == Model::Tension || current.model == Model::Friction) && current.on && fade > 0.0f && amount > 0.0f; }
    bool atPosition (NonlinearPosition p) const noexcept
    { return active() && current.model != Model::Tension && current.position == p; }
    bool tensionActive() const noexcept
    { return active() && current.model == Model::Tension && current.position != NonlinearPosition::Pickup; }
    static constexpr float maxTensionRatio = 1.03f;
    float tensionRatio() const noexcept
    {
        if (!tensionActive()) return 1.0f;
        // RMS is already smoothed once by the slot's 15 ms energy probe.
        return 1.0f + 0.03f*amount*fade*std::clamp (loopRms, 0.0f, 1.0f);
    }

    static double curve (double u) noexcept
    { return std::abs (u) <= 1.0 ? u - u*u*u/3.0 : std::copysign (2.0/3.0, u); }
    static double antiderivative (double u) noexcept
    { const double a = std::abs (u); return a <= 1.0 ? u*u*0.5-u*u*u*u/12.0 : (2.0/3.0)*a-0.25; }

    // Standalone convenience API advances one sample. Slot engines call
    // beginSample once, then at()/modalGain() for their independent lanes.
    float processSample (float x, float energyRms, int lane = 0) noexcept
    {
        beginSample();
        setLoopEnergyRms (energyRms);
        const float y = at (current.position, x, energyRms, lane);
        endSample();
        return y;
    }
    /** Four play operators: projection always lies between the previous state
        and the new input. Positive normalised weights preserve the running-peak
        bound, including during threshold/weight automation. No ADAA or DC stage. */
    float hysteresisValue (float x, int lane, bool modal = false) noexcept
    {
        auto& h = history (current.position, lane);
        auto& state = modal ? h.modalPlay : h.play;
        constexpr double thresholds[] { 0.001, 0.004, 0.016, 0.064 };
        constexpr double light[] { 0.55, 0.25, 0.15, 0.05 };
        constexpr double deep[]  { 0.10, 0.20, 0.30, 0.40 };
        double sum = 0.0, weightSum = 0.0;
        for (size_t i = 0; i < state.size(); ++i)
        {
            // Thresholds are in raw signal units: changing drive must never
            // rescale stored history and thereby manufacture a higher peak.
            const double radius = thresholds[i] * static_cast<double> (gain)/32.0;
            state[i] = std::clamp (state[i], static_cast<double> (x)-radius, static_cast<double> (x)+radius);
            const double weight = light[i] + static_cast<double> (amount)*(deep[i]-light[i]);
            sum += weight*state[i]; weightSum += weight;
        }
        return static_cast<float> (sum/weightSum);
    }
    float processWaveform (float x, int lane) noexcept
    {
        if (current.model == Model::Friction) return frictionValue (x, lane, false);
        if (current.model == Model::Hysteresis)
        {
            const double wet = hysteresisValue (x, lane);
            const double mix = static_cast<double> (amount)*fade;
            return static_cast<float> ((1.0-mix)*x+mix*wet);
        }
        auto& h = history (current.position, lane);
        const double g = gain, b = bias, u = g * (static_cast<double> (x) + b);
        const double offset = curve (g*b);
        double wet;
        if (current.adaa)
        {
            // Re-evaluate the previous raw input in the CURRENT transfer curve
            // during automation; dividing antiderivatives of different curves pops.
            const double previousU = g * (static_cast<double> (h.previousX) + b);
            const double previousF = h.primed && h.gain == gain && h.bias == bias
                                   ? h.previousF : antiderivative (previousU);
            const double f = antiderivative (u);
            const double delta = u - previousU;
            if (h.primed)
                wet = std::abs (delta) < 1.0e-5 ? curve ((u + previousU)*0.5) : (f-previousF)/delta;
            else wet = curve (u);
            h.previousF = f;
        }
        else wet = curve (u);
        wet = (wet-offset)/g;
        h.previousX = x; h.gain = gain; h.bias = bias; h.primed = true;
        // Normalised 5 Hz blocker has at most unity gain. DC is removed from the
        // nonlinear branch, so amount=0 remains a true wire and transitions agree.
        if (current.position != NonlinearPosition::Pickup || std::abs (bias) > 0.0f)
        {
            const double filtered = (1.0+dcR)*0.5*(wet-h.dcX)+dcR*h.dcY;
            h.dcX = wet; h.dcY = filtered; wet = filtered;
        }
        else { h.dcX = wet; h.dcY = wet; }
        const float mix = amount*fade;
        return x + mix * (static_cast<float> (wet)-x);
    }
    float at (NonlinearPosition position, float x, float /*energyRms*/, int lane = 0) noexcept
    {
        const float y = atPosition (position) ? processWaveform (x, lane) : x;
        if (lane == 0 && position == current.position) last = { x, y };
        return y;
    }

    /** Modal recurrences cannot absorb ADAA's extra delay. Apply a nonnegative
        radial loss to BOTH state coordinates, preserving their phase relationship.
        Bias changes the loss curve, never adds an offset to modal state. */
    float modalGain (NonlinearPosition position, float x, int lane) noexcept
    {
        float scale = 1.0f;
        if (atPosition (position))
        {
            if (current.model == Model::Friction)
            {
                const float wet = frictionValue (x, lane, true);
                scale = std::abs (x) > 1.0e-12f ? std::clamp (wet/x, 0.0f, 1.0f) : 1.0f;
            }
            else if (current.model == Model::Hysteresis)
            {
                // Magnitude-driven memory controls radial loss. A retained play
                // value may exceed today's input; it must not amplify modal state.
                const float magnitude = std::abs (x);
                const float wet = hysteresisValue (magnitude, lane, true);
                const float loss = magnitude > 1.0e-12f ? std::clamp (wet/magnitude, 0.0f, 1.0f) : 1.0f;
                scale = 1.0f + amount*fade*(loss-1.0f);
            }
            else if (std::abs (x) > 1.0e-12f)
            {
                const double wet = (curve (gain*(static_cast<double> (x)+bias))-curve (gain*bias))/gain;
                const float loss = std::clamp (static_cast<float> (wet/x), 0.0f, 1.0f);
                scale = 1.0f + amount*fade*(loss-1.0f);
            }
        }
        if (lane == 0 && position == current.position) last = { x, x*scale };
        return scale;
    }

    /** Small-signal phase of the actual blended waveform path. At full wet ADAA
        contributes exactly 0.5 samples; at partial wet compensate the blend,
        including the DC blocker's phase lead. See implementation doc section 8. */
    float loopPhaseDelay (float omega) const noexcept
    {
        if (!active() || current.model != Model::Saturate || current.position == NonlinearPosition::Pickup || omega <= 1.0e-6f) return 0.0f;
        using C = std::complex<double>;
        const C z = std::polar (1.0, -static_cast<double> (omega));
        const C dc = (1.0+dcR)*0.5*(1.0-z)/(1.0-dcR*z);
        const C aa = current.adaa ? (1.0+z)*0.5 : C (1.0, 0.0);
        const double slope = std::max (0.0, 1.0-static_cast<double> (gain*bias)*(gain*bias));
        const double mix = amount*fade;
        return static_cast<float> (-std::arg (C (1.0-mix, 0.0)+mix*slope*dc*aa)/omega);
    }
    Sample lastSample() const noexcept { return last; }
    void setLoopEnergyRms (float rms) noexcept { loopRms = rms; }
    float getLoopEnergyRms() const noexcept { return loopRms; }
    float delayScale() const noexcept { return 1.0f/tensionRatio(); }
    struct FrictionEnergy { double input = 0.0, requested = 0.0, output = 0.0; };
    FrictionEnergy getFrictionEnergy() const noexcept { return lastFrictionEnergy; }
    float getFrictionScale() const noexcept { return frictionScale; }

    // Local 32-sample DSP blocks are independent of host buffer partitioning.
    // The slot calls this after all modal lanes / pickup taps have been processed.
    void endSample() noexcept
    {
        if (++frictionSamples < 32) return;
        frictionSamples = 0;
        lastFrictionEnergy = frictionEnergy;
        blockBoundary (frictionEnergy.input, frictionEnergy.requested);
        frictionEnergy = {};
    }
    void blockBoundary (double inputEnergy, double requestedEnergy) noexcept
    {
        if (requestedEnergy > 1.02*inputEnergy && requestedEnergy > 1.0e-24)
            frictionTarget = frictionScale * static_cast<float> (0.98*std::sqrt (inputEnergy/requestedEnergy));
        else
            frictionTarget = frictionScale + frictionRecovery*(1.0f-frictionScale);
        frictionTarget = std::clamp (frictionTarget, 0.0f, 1.0f);
        frictionStep = (frictionTarget-frictionScale)/32.0f;
    }

private:
    float frictionValue (float x, int lane, bool modal) noexcept
    {
        auto& h = history (current.position, lane);
        const double velocity = h.frictionPrimed ? static_cast<double> (x)-h.frictionX : 0.0;
        h.frictionX = x; h.frictionPrimed = true;
        const double speed = std::abs (velocity);
        // Smooth sign avoids a discontinuity at rest. Rational Stribeck falloff
        // replaces exp(-speed/vs); constants are deliberately conservative.
        const double z = std::min (100.0, speed/0.01);
        const double mu = 0.2 + 0.8/(1.0+z+0.48*z*z+0.235*z*z*z);
        const double normal = amount*fade*(0.002+0.018*gain/32.0);
        const double force = -velocity/(speed+0.0001)*mu*normal;
        // No stored credit: silence cannot seed a new oscillation. Injection
        // tapers to zero at 0.5 signal units, independent of the network governor.
        const double magnitude = std::abs (x);
        const double contact = magnitude/(magnitude+0.02);
        double delta = frictionScale*contact*force;
        if (delta*x > 0.0) delta *= std::max (0.0, 1.0-magnitude/0.5);
        const double requested = x+delta;
        // Immediate <=2% energy bound also covers the first block and automation.
        // Modal loops permit only radial loss; signed force cannot be inserted
        // into one recurrence coordinate without changing its pole stability.
        const double limit = magnitude*(modal ? 1.0 : 1.0099504938362078);
        double y = std::clamp (requested, -limit, limit);
        if (modal) y = x == 0.0f ? 0.0 : x*std::clamp (y/x, 0.0, 1.0);
        const float output = static_cast<float> (y);
        frictionEnergy.input += static_cast<double> (x)*x;
        frictionEnergy.requested += requested*requested;
        frictionEnergy.output += static_cast<double> (output)*output;
        return output;
    }
    struct Lane
    {
        std::array<double, 4> play {}, modalPlay {};
        double previousF = 0.0, dcX = 0.0, dcY = 0.0;
        float previousX = 0.0f, gain = 1.0f, bias = 0.0f;
        float frictionX = 0.0f;
        bool primed = false, frictionPrimed = false;
    };
    std::array<std::array<Lane, 12>, 3> lanes {};
    Lane& history (NonlinearPosition p, int lane) noexcept
    { return lanes[static_cast<size_t> (p)][static_cast<size_t> (std::clamp (lane, 0, 11))]; }
    void approach (float& value, float target) noexcept
    { const float next = value + smooth*(target-value); value = next == value || std::abs (target-next) < 1.0e-6f ? target : next; }
    double sr = 48000.0, dcR = 0.999;
    float smooth = 0.004f, fadeStep = 0.01f, fade = 0.0f, loopRms = 0.0f;
    float gain = 1.0f, targetGain = 1.0f, amount = 0.5f, targetAmount = 0.5f, bias = 0.0f, targetBias = 0.0f;
    FrictionEnergy frictionEnergy, lastFrictionEnergy;
    float frictionScale = 1.0f, frictionTarget = 1.0f, frictionStep = 0.0f, frictionRecovery = 0.001f;
    int frictionSamples = 0;
    bool transitioning = false;
    NonlinearParams current, desired;
    Sample last;
};

/** Observational 15 ms mean-square follower. It never controls legacy DSP.
    One update per slot sample; RMS is evaluated only when requested. */
class LoopEnergyProbe
{
public:
    void prepare (float sampleRate) noexcept
    { coefficient = 1.0 - std::exp (-1.0 / (0.015 * sampleRate)); reset(); }
    void reset() noexcept { meanSquare = 0.0; }
    void observe (double square) noexcept { meanSquare += coefficient * (square - meanSquare); }
    float rms() const noexcept { return static_cast<float> (std::sqrt (meanSquare)); }
private:
    double meanSquare = 0.0, coefficient = 0.0;
};
} // namespace aeriform::dsp
